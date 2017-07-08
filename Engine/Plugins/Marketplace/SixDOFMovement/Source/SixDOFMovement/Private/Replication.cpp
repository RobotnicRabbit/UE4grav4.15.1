//Copyright 2016-2017 Mookie

#include "SixDOFMovementPrivatePCH.h"
#include "UnrealNetwork.h"

void USixDOFMovementComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, PitchInput, COND_Custom);
	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, YawInput, COND_Custom);
	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, RollInput, COND_Custom);

	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, ForwardInput, COND_Custom);
	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, RightInput, COND_Custom);
	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, UpInput, COND_Custom);

	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, LookAtVector, COND_Custom);
	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, AutolevelUpVector, COND_Custom);
	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, MoveToVector, COND_Custom);
	DOREPLIFETIME_CONDITION(USixDOFMovementComponent, InputSpaceRotator, COND_Custom);
}

void USixDOFMovementComponent::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) {
	Super::PreReplication(ChangedPropertyTracker);

	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, PitchInput, ReplicateInputs);
	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, YawInput, ReplicateInputs);
	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, RollInput, ReplicateInputs);

	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, ForwardInput, ReplicateInputs);
	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, RightInput, ReplicateInputs);
	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, UpInput, ReplicateInputs);

	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, LookAtVector, ReplicateInputs);
	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, AutolevelUpVector, ReplicateInputs);
	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, MoveToVector, ReplicateInputs);
	DOREPLIFETIME_ACTIVE_OVERRIDE(USixDOFMovementComponent, InputSpaceRotator, ReplicateInputs);
}

void USixDOFMovementComponent::BroadcastMovement(float DeltaTime) {
	if (GetOwner()->Role != ROLE_Authority) return;
	if (!ReplicateMovement) return;
	if (MovementReplicationFrequency <= 0.0f) return; //do not replicate if frequency is 0

	if (TimeSincePositionBroadcast >= 1.0f / MovementReplicationFrequency) {
		TimeSincePositionBroadcast = 0.0f;

		FVector Location = GetOwner()->GetActorLocation();
		Location = UGameplayStatics::RebaseLocalOriginOntoZero(GetWorld(), Location); //change for 413
		MovementRep(Location, GetOwner()->GetActorRotation(), Velocity, AngularVelocity);
	}

	TimeSincePositionBroadcast += DeltaTime;
}

void USixDOFMovementComponent::BroadcastInput(float DeltaTime) {
	if (GetOwner()->Role != ROLE_AutonomousProxy) return;
	if (!ReplicateInputs) return;
	if (InputReplicationFrequency <= 0.0f) return; //do not replicate if frequency is 0

	if (TimeSinceInputBroadcast >= 1.0f / InputReplicationFrequency) {

		if ((ClientForwardInput != ForwardInput) ||
		(ClientRightInput != RightInput) ||
		(ClientUpInput != UpInput) ||
		(ClientPitchInput != PitchInput) ||
		(ClientYawInput != YawInput) ||
		(ClientRollInput != RollInput)) {
			TimeSinceInputBroadcast = 0.0f;
			InputRep(FVector(ClientForwardInput, ClientRightInput, ClientUpInput), FVector(ClientPitchInput, ClientYawInput, ClientRollInput));
		}

		if ((ClientLookAtVector != LookAtVector) ||
		(ClientMoveToVector != MoveToVector)){
			TimeSinceInputBroadcast = 0.0f;

			FVector MoveToVec = ClientMoveToVector;

			if (MoveToMode == EAutopilotMode::LM_Location) {  //change for 413
				MoveToVec = UGameplayStatics::RebaseLocalOriginOntoZero(GetWorld(), MoveToVec);  //change for 413
			}

			GoalRep(ClientLookAtVector, MoveToVec);
		}

		if (ClientAutolevelUpVector != AutolevelUpVector) {
			TimeSinceInputBroadcast = 0.0f;
			UpVecRep(ClientAutolevelUpVector);
		}

		if (ClientInputSpaceRotator != InputSpaceRotator){
			TimeSinceInputBroadcast = 0.0f;
			InputSpaceRep(ClientInputSpaceRotator);
		}

	}

	TimeSinceInputBroadcast += DeltaTime;
}

void USixDOFMovementComponent::MovementRep_Implementation(FVector NewLocation, FRotator NewRotation, FVector NewVelocity, FVector NewAngularVelocity) {
	AActor* Owner = GetOwner();
	if (!IsValid(Owner)) return;

	if (Owner->Role != ROLE_Authority) {
		NewLocation=UGameplayStatics::RebaseZeroOriginOntoLocal(GetWorld(), NewLocation); //change for 413

		if ((NewLocation - Owner->GetActorLocation()).SizeSquared() >= FMath::Pow(MovementReplicationTeleportThreshold, 2.0f)) {
			SetPositionAndVelocity(NewLocation, NewRotation, NewVelocity, NewAngularVelocity);
		}
		else {
			SetPositionAndVelocitySoft(NewLocation, NewRotation, NewVelocity, NewAngularVelocity);
		}
	}
}

void USixDOFMovementComponent::SetPositionAndVelocity(FVector NewLocation, FRotator NewRotation, FVector NewVelocity, FVector NewAngularVelocity) {
	AActor* Owner = GetOwner();

	Owner->SetActorLocationAndRotation(NewLocation, NewRotation, false, NULL, ETeleportType::TeleportPhysics);
	Velocity = NewVelocity;
	AngularVelocity = NewAngularVelocity;

	if (IsValid(UpdatedPrimitive)) {
		if (UpdatedPrimitive->IsSimulatingPhysics()) {
			UpdatedPrimitive->SetPhysicsLinearVelocity(NewVelocity);
			UpdatedPrimitive->SetPhysicsAngularVelocity(NewAngularVelocity);
		}
	}

	LocationError = FVector(0, 0, 0);
	VelocityError = FVector(0, 0, 0);
	AngularVelocityError = FVector(0, 0, 0);
	RotationError = FQuat::Identity;
}

void USixDOFMovementComponent::SetPositionAndVelocitySoft(FVector NewLocation, FRotator NewRotation, FVector NewVelocity, FVector NewAngularVelocity) {
	AActor* Owner = GetOwner();

	LocationError = NewLocation - Owner->GetActorLocation();
	VelocityError = NewVelocity - Velocity;
	AngularVelocityError = NewAngularVelocity - AngularVelocity;

	RotationError = FQuat(NewRotation.Quaternion()*Owner->GetActorRotation().Quaternion().Inverse());
}

void USixDOFMovementComponent::ErrorCatchUp(float DeltaTime) {
	if (!ReplicateMovement) return;
	AActor* Owner = GetOwner();

	float ErrorDelta = FMath::Min(DeltaTime/MovementReplicationSmoothingTime,1.0f);

	Owner->SetActorLocation(Owner->GetActorLocation() + (LocationError*ErrorDelta),false,NULL,ETeleportType::TeleportPhysics);
	Velocity += VelocityError*ErrorDelta;
	AngularVelocity += AngularVelocityError*ErrorDelta;

	Owner->SetActorRotation(FQuat::Slerp(Owner->GetActorRotation().Quaternion(), RotationError*Owner->GetActorRotation().Quaternion(),ErrorDelta), ETeleportType::TeleportPhysics);

	if (IsValid(UpdatedPrimitive)){
		if(UpdatedPrimitive->IsSimulatingPhysics()) {
			UpdatedPrimitive->SetPhysicsLinearVelocity(VelocityError*ErrorDelta,true);
			UpdatedPrimitive->SetPhysicsAngularVelocity(AngularVelocityError*ErrorDelta, true);
		}
	}

	LocationError *= 1.0f - ErrorDelta;
	VelocityError *= 1.0f - ErrorDelta;
	AngularVelocityError *= 1.0f - ErrorDelta;
	RotationError = FQuat::Slerp(RotationError, FQuat::Identity,ErrorDelta);
}

void USixDOFMovementComponent::InputRep_Implementation(FVector TranslationInput, FVector RotationInput) {
	ForwardInput = TranslationInput.X;
	RightInput = TranslationInput.Y;
	UpInput = TranslationInput.Z;

	PitchInput = RotationInput.X;
	YawInput = RotationInput.Y;
	RollInput = RotationInput.Z;
}
bool USixDOFMovementComponent::InputRep_Validate(FVector TranslationInput, FVector RotationInput) {
	return true;
}

void USixDOFMovementComponent::GoalRep_Implementation(FVector LookAtGoal, FVector MoveToGoal) {
	LookAtVector = LookAtGoal;  
//change for 413, all below to this line:  MoveToVector = MoveToGoal;
	if (MoveToMode == EAutopilotMode::LM_Location) {
		MoveToVector = UGameplayStatics::RebaseZeroOriginOntoLocal(GetWorld(),MoveToGoal);
	}
	else {
		MoveToVector = MoveToGoal;
	}
//end change for 413
}
bool USixDOFMovementComponent::GoalRep_Validate(FVector LookAtGoal, FVector MoveToGoal) {
	return true;
}

void USixDOFMovementComponent::UpVecRep_Implementation(FVector LevelGoal) {
	AutolevelUpVector = LevelGoal;
}
bool USixDOFMovementComponent::UpVecRep_Validate(FVector LevelGoal) {
	return true;
}

void USixDOFMovementComponent::InputSpaceRep_Implementation(FRotator InputRotator) {
	InputSpaceRotator = InputRotator;
}
bool USixDOFMovementComponent::InputSpaceRep_Validate(FRotator InputRotator) {
	return true;
}