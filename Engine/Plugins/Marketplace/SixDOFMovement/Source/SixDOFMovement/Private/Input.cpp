//Copyright 2016-2017 Mookie

#include "SixDOFMovementPrivatePCH.h"

float USixDOFMovementComponent::SetForwardInput(float Input) {
	ClientForwardInput = Input;
	if(GetOwner()->Role == ROLE_Authority){
		ForwardInput = Input;
	}

	return Input;
}

float USixDOFMovementComponent::SetRightInput(float Input) {
	ClientRightInput = Input;
	if(GetOwner()->Role == ROLE_Authority){
		RightInput = Input;
	}

	return Input;
}

float USixDOFMovementComponent::SetUpInput(float Input) {
	ClientUpInput = Input;
	if (GetOwner()->Role == ROLE_Authority) {
		UpInput = Input;
	}

	return Input;
}

float USixDOFMovementComponent::SetPitchInput(float Input) {
	ClientPitchInput = Input;
	if (GetOwner()->Role == ROLE_Authority) {
		PitchInput = Input;
	}

	return Input;
}

float USixDOFMovementComponent::SetYawInput(float Input) {
	ClientYawInput = Input;
	if (GetOwner()->Role == ROLE_Authority) {
		YawInput = Input;
	}

	return Input;
}

float USixDOFMovementComponent::SetRollInput(float Input) {
	ClientRollInput = Input;
	if (GetOwner()->Role == ROLE_Authority) {
		RollInput = Input;
	}

	return Input;
}

FVector USixDOFMovementComponent::SetLookAtVector(FVector Input) {
	ClientLookAtVector = Input;
	if (GetOwner()->Role == ROLE_Authority) {
		LookAtVector = Input;
	}

	return Input;
}

FVector USixDOFMovementComponent::SetAutolevelUpVector(FVector Input) {
	ClientAutolevelUpVector = Input;
	if (GetOwner()->Role == ROLE_Authority) {
		AutolevelUpVector = Input;
	}

	return Input;
}

FVector USixDOFMovementComponent::SetMoveToVector(FVector Input) {
	ClientMoveToVector = Input;
	if (GetOwner()->Role == ROLE_Authority) {
		MoveToVector = Input;
	}

	return Input;
}

FRotator USixDOFMovementComponent::SetInputSpaceRotator(FRotator Input) {
	ClientInputSpaceRotator = Input;
	if (GetOwner()->Role == ROLE_Authority) {
		InputSpaceRotator = Input;
	}

	return Input;
}

FVector  USixDOFMovementComponent::RotationInput() const{
	switch (RotationInputSpace) {
	case(EInputSpace::IS_World):
		return UpdatedComponent->GetComponentQuat().Inverse().RotateVector(FVector(RollInput, PitchInput, YawInput));
	case(EInputSpace::IS_Custom):
		return (UpdatedComponent->GetComponentQuat().Inverse()*InputSpaceRotator.Quaternion()).RotateVector(FVector(RollInput, PitchInput, YawInput));
	default:
		return FVector(RollInput, PitchInput, YawInput);
	}
}

FVector  USixDOFMovementComponent::TranslationInput() const{
	switch (TranslationInputSpace) {
	case(EInputSpace::IS_World):
		return UpdatedComponent->GetComponentQuat().Inverse().RotateVector(FVector(ForwardInput, RightInput, UpInput));
	case(EInputSpace::IS_Custom):
		return (UpdatedComponent->GetComponentQuat().Inverse()*InputSpaceRotator.Quaternion()).RotateVector(FVector(ForwardInput, RightInput, UpInput));
	default:
		return FVector(ForwardInput, RightInput, UpInput);
	}
}