//Copyright 2016-2017 Mookie

#include "SixDOFMovementPrivatePCH.h"
#include "Classes/PhysicsEngine/PhysicsSettings.h"

USixDOFMovementComponent::USixDOFMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	OnCalculateCustomPhysics.BindUObject(this, &USixDOFMovementComponent::CustomPhysics);
	SetTickGroup(ETickingGroup::TG_PrePhysics);
	bReplicates = (ReplicateInputs || ReplicateMovement);

	ClientLookAtVector= LookAtVector;
	ClientAutolevelUpVector= AutolevelUpVector;
	ClientMoveToVector= MoveToVector;
	ClientInputSpaceRotator = InputSpaceRotator;

	return;
}

void USixDOFMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {

	// skip if we don't want component updated when not rendered or if updated component can't move
	if (ShouldSkipUpdate(DeltaTime)) return;
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime == 0) return; //nothing needs to be done if this is zero anyway

	if ( (!IsValid(UpdatedComponent) ) || ( !IsValid( GetOwner() ) ) ) {
		UE_LOG(LogTemp, Warning, TEXT("Invalid Actor or Root Component"));
		return;
	}

	bool UsePhysics = false;

	if (IsValid(UpdatedPrimitive)) {
		if (UpdatedPrimitive->IsSimulatingPhysics()) {
			UsePhysics = true;
		}
	}

	if (!UsePhysics) {
		int StepsPerFrame = FMath::Clamp(FMath::CeilToInt(DeltaTime * MinFrameRate), 1, MaxStepsPerFrame);
		for (int i = 0; i < StepsPerFrame; i++) {
			SimulationStep(DeltaTime / float(StepsPerFrame), false);
		}
	}
	else {
		FBodyInstance *BodyInstance = UpdatedPrimitive->GetBodyInstance();
		if (!BodyInstance) {
			UE_LOG(LogTemp, Error, TEXT("Physics enabled but component has no BodyInstance"));
			return;
		}

		//find if subtepping is enabled in project settings
		bool Substep;
		const UPhysicsSettings* Settings = GetDefault<UPhysicsSettings>();
		if (Settings) {
			Substep = Settings->bSubstepping;
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("Project settings inaccessible"));
			Substep = false;
		}

		if (Substep) {
			BodyInstance->AddCustomPhysics(OnCalculateCustomPhysics);
		}
		else {
			SimulationStep(DeltaTime, true, BodyInstance);
		}
	}

	ErrorCatchUp(DeltaTime);
	BroadcastMovement(DeltaTime);
	BroadcastInput(DeltaTime);
}

void USixDOFMovementComponent::CustomPhysics(float DeltaTime, FBodyInstance* BodyInstance)
{
	SimulationStep(DeltaTime, true, BodyInstance);
}