//Copyright 2016-2017 Mookie

#include "SixDOFMovementPrivatePCH.h"

FVector USixDOFMovementComponent::AutopilotRotation(const FVector &Location, const FQuat &Rotation) const {
	FVector Input = RotationInput();
	FVector LookAtOutput = FVector(0, 0, 0);
	FVector LevelOutput = FVector(0, 0, 0);
	FVector DampOutput = FVector(0, 0, 0);

	if (LookAt) {
		FVector LookAtGoal;
		switch (LookAtMode) {

			case(EAutopilotMode::LM_Direction): {
				LookAtGoal = LookAtVector.GetSafeNormal();
				break;
			}

			case(EAutopilotMode::LM_Controller): {
				if (PawnOwner) {
					LookAtGoal = PawnOwner->GetControlRotation().Quaternion().GetForwardVector();
				}
				else {
					UE_LOG(LogTemp, Warning, TEXT("using LM_Controller with non-pawn actor"));
					LookAtGoal = FVector(0, 0, 0);
				}
				break;
			}

			case(EAutopilotMode::LM_Location): {
				LookAtGoal = (LookAtVector - Location).GetSafeNormal();
				break;
			}

			default:{
				UE_LOG(LogTemp, Warning, TEXT("invalid autopilot rotation mode"));
			}

		}

		float FwdDot = FVector::DotProduct(Rotation.GetForwardVector(),LookAtGoal);
		float RightDot = FVector::DotProduct(Rotation.GetRightVector(), LookAtGoal);
		float UpDot = FVector::DotProduct(Rotation.GetUpVector(), LookAtGoal);

		LookAtOutput.X = -FMath::Atan2(RightDot, FwdDot)*LookAtSensitivityRoll;
		LookAtOutput.Y = -FMath::Atan2(UpDot, FwdDot)*LookAtSensitivityPitch;
		LookAtOutput.Z = FMath::Atan2(RightDot, FwdDot)*LookAtSensitivityYaw;
	}

	if (Autolevel) {
		float FwdDot = FVector::DotProduct(Rotation.GetForwardVector(), AutolevelUpVector);
		float RightDot = FVector::DotProduct(Rotation.GetRightVector(), AutolevelUpVector);
		float UpDot = FVector::DotProduct(Rotation.GetUpVector(), AutolevelUpVector);

		LevelOutput.X = -FMath::Atan2(RightDot, UpDot)*AutolevelSensitivityRoll;
		LevelOutput.Y = FMath::Atan2(FwdDot, UpDot)*AutolevelSensitivityPitch;
		LevelOutput.Z = -FMath::Atan2(FwdDot, RightDot)*AutolevelSensitivityYaw;
	}

	if (RotationDamping) {
		FVector DampInput = -Rotation.Inverse().RotateVector(AngularVelocity)/180.0f*PI;
		DampOutput = DampInput*FVector(RotationDampingRoll, RotationDampingPitch, RotationDampingYaw);
	}
	
	return BlendInput(LookAtOutput+LevelOutput+DampOutput,Input);
}

FVector USixDOFMovementComponent::AutopilotTranslation(const FVector &Location, const FQuat &Rotation) const {
	FVector Input = TranslationInput();
	FVector MoveToOutput = FVector(0, 0, 0);
	FVector DampOutput = FVector(0, 0, 0);

	if (MoveTo) {
		FVector MoveToGoal;
		switch (MoveToMode) {
			case(EAutopilotMode::LM_Direction): {
				MoveToGoal = MoveToVector;
				break;
			}

			case(EAutopilotMode::LM_Controller): {
				if (PawnOwner) {
					MoveToGoal = PawnOwner->ConsumeMovementInputVector();
				}
				else {
					UE_LOG(LogTemp, Warning, TEXT("using LM_Controller with non-pawn actor"));
					MoveToGoal = FVector(0, 0, 0);
				}
				break;
			}

			case(EAutopilotMode::LM_Location): {
				MoveToGoal = (MoveToVector - Location);
				break;
			}

			default: {
				UE_LOG(LogTemp, Warning, TEXT("invalid autopilot translation mode"));
			}
		}

		float FwdDot = FVector::DotProduct(Rotation.GetForwardVector(), MoveToGoal);
		float RightDot = FVector::DotProduct(Rotation.GetRightVector(), MoveToGoal);
		float UpDot = FVector::DotProduct(Rotation.GetUpVector(), MoveToGoal);

		MoveToOutput = FVector(FwdDot, RightDot, UpDot);
		MoveToOutput = MoveToOutput*FVector(MoveToSensitivityForward, MoveToSensitivityRight, MoveToSensitivityUp);
	}

	if(TranslationDamping) {
		FVector DampInput = -Rotation.Inverse().RotateVector(Velocity);
		DampOutput = DampInput*FVector(TranslationDampingForward, TranslationDampingRight, TranslationDampingUp);
	}

	return BlendInput(MoveToOutput+DampOutput, Input);
}