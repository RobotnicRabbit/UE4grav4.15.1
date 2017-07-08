//Copyright 2016-2017 Mookie

#include "SixDOFMovementPrivatePCH.h"

void USixDOFMovementComponent::SimulationStep(float DeltaTime, bool UsePhysics, FBodyInstance* BodyInstance) {
	if (DeltaTime == 0) return; //nothing needs to be done if this is zero anyway

	FVector OldLocation;
	FQuat OldRotation;
	FVector OldVelocity;
	FVector OldAngularVelocity;

	if (UsePhysics) {
		if (!BodyInstance) {
			UE_LOG(LogTemp, Error, TEXT("Physics enabled but component has no BodyInstance, this shouldn't be happening"));
			return;
		}

		OldLocation = BodyInstance->GetUnrealWorldTransform().GetLocation();
		OldRotation = BodyInstance->GetUnrealWorldTransform().GetRotation();
		OldVelocity = BodyInstance->GetUnrealWorldVelocity();
		OldAngularVelocity = BodyInstance->GetUnrealWorldAngularVelocity();
	}
	else {
		OldRotation = UpdatedComponent->GetComponentQuat();
		OldLocation = UpdatedComponent->GetComponentLocation();
		OldVelocity = Velocity;
		OldAngularVelocity = AngularVelocity;
	}

	FVector APRot = AutopilotRotation(OldLocation, OldRotation);
	FVector APTrans = AutopilotTranslation(OldLocation, OldRotation);
	AngularVelocity = UpdateAngularVelocity(OldRotation, OldAngularVelocity, APRot, DeltaTime);
	Velocity = UpdateLinearVelocity(OldLocation, OldRotation, OldVelocity, APTrans, DeltaTime);

	if (UsePhysics) {
		BodyInstance->AddForce((Velocity - OldVelocity) / DeltaTime, false, true);
		BodyInstance->AddTorque((AngularVelocity - OldAngularVelocity), false, true);
	}
	else {
		FQuat DeltaRotation = FQuat((AngularVelocity + OldAngularVelocity)*PI / 360.0f, DeltaTime);
		FQuat Rotation = DeltaRotation*OldRotation;

		float RemainingDelta = DeltaTime;
		int RemainingSteps = CollisionMaxIterations;
		while ((RemainingDelta > 0.0f) && (RemainingSteps > 0)) {
			FHitResult Result;
			SafeMoveUpdatedComponent((Velocity + OldVelocity) *RemainingDelta / 2.0f, Rotation, true, Result, ETeleportType::None);

			if (Result.bBlockingHit) {
				FVector SlideVec = FVector::CrossProduct(Velocity, Result.Normal).RotateAngleAxis(90.0f, Result.Normal);
				FVector Reflect = Velocity.MirrorByVector(Result.Normal);
				Velocity = FMath::Lerp(SlideVec, Reflect, CollisionRestitution);
				OldVelocity = Velocity; //switch to euler on collision
				RemainingDelta = RemainingDelta*(1.0f - Result.Time);
				RemainingSteps--;

				if (CollisionMargin > 0.0f) {
					SafeMoveUpdatedComponent(Result.Normal*CollisionMargin, Rotation, false, Result, ETeleportType::None);
				}
			}
			else {
				RemainingDelta = 0;
			}
		}
	}
}