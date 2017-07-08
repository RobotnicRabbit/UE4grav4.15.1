//Copyright 2016-2017 Mookie

#include "SixDOFMovementPrivatePCH.h"


FVector USixDOFMovementComponent::UpdateLinearVelocity(const FVector &Location, const FQuat &Rotation, const FVector &OldVelocity, const FVector &Input, float DeltaTime) const {
	FVector NewInput = Input;

	if (ClampMovementInput) {
		if (AllowChordBoosting) {
			NewInput.X = (FMath::Clamp(NewInput.X, -1.0f, 1.0f));
			NewInput.Y = (FMath::Clamp(NewInput.Y, -1.0f, 1.0f));
			NewInput.Z = (FMath::Clamp(NewInput.Z, -1.0f, 1.0f));
		}
		else {
			NewInput = NewInput.GetClampedToMaxSize(1.0f);
		}
	}

	FVector VelLocal = FVector(
		FVector::DotProduct(OldVelocity, Rotation.GetForwardVector()),
		FVector::DotProduct(OldVelocity, Rotation.GetRightVector()),
		FVector::DotProduct(OldVelocity, Rotation.GetUpVector())
	);

	const FVector WindLocal = FVector(
		FVector::DotProduct(Wind, Rotation.GetForwardVector()),
		FVector::DotProduct(Wind, Rotation.GetRightVector()),
		FVector::DotProduct(Wind, Rotation.GetUpVector())
	);

	const FVector VelGoal = FVector(
		NewInput.X*MaxSpeedForward + WindLocal.X,
		NewInput.Y*MaxSpeedRight + WindLocal.Y,
		NewInput.Z*MaxSpeedUp + WindLocal.Z
	);

	VelLocal = FVector(
		AxisAcceleration(MovementAcceleration, VelLocal.X, VelGoal.X, MaxSpeedForward, InertiaForward, NewInput.X, DeltaTime),
		AxisAcceleration(MovementAcceleration, VelLocal.Y, VelGoal.Y, MaxSpeedRight, InertiaRight, NewInput.Y, DeltaTime),
		AxisAcceleration(MovementAcceleration, VelLocal.Z, VelGoal.Z, MaxSpeedUp, InertiaUp, NewInput.Z, DeltaTime)
	);

	FVector NewVelocity =
		Rotation.GetForwardVector()*VelLocal.X +
		Rotation.GetRightVector()*VelLocal.Y +
		Rotation.GetUpVector()*VelLocal.Z;

	NewVelocity += GetGravity(Location)*DeltaTime;

	return NewVelocity;
}

FVector USixDOFMovementComponent::UpdateAngularVelocity(const FQuat &Rotation,const FVector &OldAngularVelocity,const FVector &Input, float DeltaTime) const {
	FVector NewInput = Input;

	if (ClampRotationInput) {
		NewInput.X = (FMath::Clamp(NewInput.X, -1.0f, 1.0f));
		NewInput.Y = (FMath::Clamp(NewInput.Y, -1.0f, 1.0f));
		NewInput.Z = (FMath::Clamp(NewInput.Z, -1.0f, 1.0f));
	}

	FVector RotLocal = FVector(
		FVector::DotProduct(OldAngularVelocity, Rotation.GetAxisX()),
		FVector::DotProduct(OldAngularVelocity, Rotation.GetAxisY()),
		FVector::DotProduct(OldAngularVelocity, Rotation.GetAxisZ())
	);

	const FVector RotGoal = FVector(
		NewInput.X*MaxSpeedRoll,
		NewInput.Y*MaxSpeedPitch,
		NewInput.Z*MaxSpeedYaw
	);

	RotLocal = FVector(
		AxisAcceleration(RotationAcceleration, RotLocal.X, RotGoal.X, MaxSpeedRoll, InertiaRoll, NewInput.X, DeltaTime),
		AxisAcceleration(RotationAcceleration, RotLocal.Y, RotGoal.Y, MaxSpeedPitch, InertiaPitch, NewInput.Y, DeltaTime),
		AxisAcceleration(RotationAcceleration, RotLocal.Z, RotGoal.Z, MaxSpeedYaw, InertiaYaw, NewInput.Z, DeltaTime)
	);

	FVector NewAngularVelocity =
		Rotation.GetAxisX()*RotLocal.X +
		Rotation.GetAxisY()*RotLocal.Y +
		Rotation.GetAxisZ()*RotLocal.Z;

	return NewAngularVelocity;
}

float USixDOFMovementComponent::AxisAcceleration(EAccelerationMode Mode, float Current, float Goal, float MaxSpeed, float Inertia, float Input, float DeltaTime) const {

	switch (Mode) {
	case(EAccelerationMode::AM_Exponential): {
		if (Inertia > 0.0f) {
			return FMath::Lerp(Current, Goal, FMath::Min(DeltaTime / Inertia, 1.0f));
		}
		else {
			return Goal;
		}
	}

	case(EAccelerationMode::AM_Linear): {
		if (Inertia > 0.0f) {
			return (Current + FMath::Clamp(Goal - Current, -DeltaTime*MaxSpeed / Inertia, DeltaTime*MaxSpeed / Inertia));
		}
		else {
			return Goal;
		}
	}

	case(EAccelerationMode::AM_Square): {
		if (Inertia > 0.0f && MaxSpeed != 0.0f) {
			float Change = (Goal - Current) / MaxSpeed;
			return FMath::Lerp(Current, Goal, FMath::Min(DeltaTime * FMath::Abs(Change) / Inertia, 1.0f));
		}
		else {
			return Goal;
		}
	}

	case(EAccelerationMode::AM_Frictionless): {
		if (Inertia > 0.0f) {
			return (Current + Input * MaxSpeed * DeltaTime / Inertia);
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("Frictionless requires positive inertia"));
			return Current;
		}
	}

	default: { //formality, all possible modes are covered
		UE_LOG(LogTemp, Warning, TEXT("Invalid acceleration mode"));
		return 0;
	}

	}
}
