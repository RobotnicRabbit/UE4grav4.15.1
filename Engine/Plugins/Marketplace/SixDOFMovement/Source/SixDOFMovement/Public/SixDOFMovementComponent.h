//Copyright 2016-2017 Mookie


#pragma once

#include "Engine.h"
#include "GameFramework/MovementComponent.h"
#include "SixDOFMovementComponent.generated.h"

UENUM(BlueprintType)
enum class EAccelerationMode : uint8 {
	AM_Exponential UMETA(DisplayName = "Exponential"),
	AM_Linear UMETA(DisplayName="Linear"),
	AM_Square UMETA(DisplayName = "Square"),
	AM_Frictionless UMETA(DisplayName = "Frictionless")
};

UENUM(BlueprintType)
enum class EAutopilotMode : uint8 {
	LM_Direction UMETA(DisplayName = "Direction Vector"),
	LM_Location UMETA(DisplayName = "Location Vector"),
	LM_Controller UMETA(DisplayName = "Use Controller Input"),
};

UENUM(BlueprintType)
enum class EInputSpace : uint8 {
	IS_Local UMETA(DisplayName = "Local Space"),
	IS_World UMETA(DisplayName = "World Space"),
	IS_Custom UMETA(DisplayName = "Input Rotator Space"),
};


UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent))
class USixDOFMovementComponent : public UPawnMovementComponent
{
	GENERATED_UCLASS_BODY()

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	FCalculateCustomPhysics OnCalculateCustomPhysics;

	UFUNCTION(BlueprintNativeEvent, Category = World) FVector GetWind(FVector Location) const;
	UFUNCTION(BlueprintNativeEvent, Category = World) FVector GetGravity(FVector Location) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = World) FVector Wind;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = World) bool UseGravity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = World) bool UseGravityVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = World) FVector Gravity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NonPhysSubstep, meta = (ClampMin = 0)) float MinFrameRate = 60;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NonPhysSubstep, meta = (ClampMin = 1)) int MaxStepsPerFrame = 12;

	UFUNCTION(BlueprintCallable, Category = Input) float SetForwardInput(float Input);
	UFUNCTION(BlueprintCallable, Category = Input) float SetRightInput(float Input);
	UFUNCTION(BlueprintCallable, Category = Input) float SetUpInput(float Input);

	UFUNCTION(BlueprintCallable, Category = Input) float SetPitchInput(float Input);
	UFUNCTION(BlueprintCallable, Category = Input) float SetYawInput(float Input);
	UFUNCTION(BlueprintCallable, Category = Input) float SetRollInput(float Input);

	UFUNCTION(BlueprintCallable, Category = Input) FVector SetLookAtVector(FVector Input);
	UFUNCTION(BlueprintCallable, Category = Input) FVector SetAutolevelUpVector(FVector Input);
	UFUNCTION(BlueprintCallable, Category = Input) FVector SetMoveToVector(FVector Input);
	UFUNCTION(BlueprintCallable, Category = Input) FRotator SetInputSpaceRotator(FRotator Input);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (ClampMin = 0)) float MaxSpeedForward=1000.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (ClampMin = 0)) float MaxSpeedRight=1000.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (ClampMin = 0)) float MaxSpeedUp=1000.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (ClampMin = 0)) float InertiaForward = 0.20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (ClampMin = 0)) float InertiaRight = 0.20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (ClampMin = 0)) float InertiaUp = 0.20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement) EAccelerationMode MovementAcceleration;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation, meta = (ClampMin = 0)) float MaxSpeedPitch=90.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation, meta = (ClampMin = 0)) float MaxSpeedYaw=90.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation, meta = (ClampMin = 0)) float MaxSpeedRoll=90.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation, meta = (ClampMin = 0)) float InertiaPitch=0.20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation, meta = (ClampMin = 0)) float InertiaYaw=0.20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation, meta = (ClampMin = 0)) float InertiaRoll=0.20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation) EAccelerationMode RotationAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Collision, meta = (ClampMin = 0)) float CollisionMargin = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Collision, meta = (ClampMin = 1)) int CollisionMaxIterations = 4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Collision, meta = (ClampMin = 0)) float CollisionRestitution = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Replication) bool ReplicateMovement = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Replication) bool ReplicateInputs = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Replication, meta = (ClampMin = 0)) float MovementReplicationFrequency=10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Replication, meta = (ClampMin = 0)) float InputReplicationFrequency = 30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Replication, meta = (ClampMin = 0)) float MovementReplicationSmoothingTime=0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Replication, meta = (ClampMin = 0)) float MovementReplicationTeleportThreshold = 1000.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Input) float PitchInput;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Input) float YawInput;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Input) float RollInput;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Input) float ForwardInput;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Input) float RightInput;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Input) float UpInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input) EInputSpace TranslationInputSpace;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input) EInputSpace RotationInputSpace;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = Input) FRotator InputSpaceRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input) bool ClampMovementInput = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input) bool AllowChordBoosting = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input) bool ClampRotationInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation) bool LookAt = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation) EAutopilotMode LookAtMode;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = AutopilotRotation) FVector LookAtVector = FVector(1.0f,0.0f,0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation, meta = (ClampMin = 0)) float LookAtSensitivityPitch = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation, meta = (ClampMin = 0)) float LookAtSensitivityYaw = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation, meta = (ClampMin = 0)) float LookAtSensitivityRoll = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation) bool RotationDamping = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation, meta = (ClampMin = 0)) float RotationDampingPitch = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation, meta = (ClampMin = 0)) float RotationDampingYaw = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation, meta = (ClampMin = 0)) float RotationDampingRoll = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation) bool Autolevel = false;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = AutopilotRotation) FVector AutolevelUpVector = FVector(0.0f, 0.0f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation, meta = (ClampMin = 0)) float AutolevelSensitivityPitch = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation, meta = (ClampMin = 0)) float AutolevelSensitivityYaw = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotRotation, meta = (ClampMin = 0)) float AutolevelSensitivityRoll = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotTranslation) bool MoveTo = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AutopilotTranslation) EAutopilotMode MoveToMode;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = AutopilotTranslation) FVector MoveToVector = FVector(0.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotTranslation, meta = (ClampMin = 0)) float MoveToSensitivityForward = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotTranslation, meta = (ClampMin = 0)) float MoveToSensitivityRight = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotTranslation, meta = (ClampMin = 0)) float MoveToSensitivityUp = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotTranslation) bool TranslationDamping = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotTranslation, meta = (ClampMin = 0)) float TranslationDampingForward = 0.01f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotTranslation, meta = (ClampMin = 0)) float TranslationDampingRight = 0.01f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutopilotTranslation, meta = (ClampMin = 0)) float TranslationDampingUp = 0.01f;

	UPROPERTY(BlueprintReadWrite, Category = Velocity) FVector AngularVelocity;

private:
	float ClientForwardInput;
	float ClientRightInput;
	float ClientUpInput;

	float ClientPitchInput;
	float ClientYawInput;
	float ClientRollInput;

	FVector ClientLookAtVector;
	FVector ClientAutolevelUpVector;
	FVector ClientMoveToVector;
	FRotator ClientInputSpaceRotator;

	void BroadcastMovement(float DeltaTime);
	void BroadcastInput(float DeltaTime);
	void SetPositionAndVelocity(FVector NewLocation, FRotator NewRotation, FVector NewVelocity, FVector NewAngularVelocity);
	void SetPositionAndVelocitySoft(FVector NewLocation, FRotator NewRotation, FVector NewVelocity, FVector NewAngularVelocity);
	void ErrorCatchUp(float DeltaTime);
	float TimeSincePositionBroadcast=0.0f;
	float TimeSinceInputBroadcast = 0.0f;

	FVector LocationError = FVector(0, 0, 0);
	FVector VelocityError = FVector(0, 0, 0);
	FVector AngularVelocityError = FVector(0, 0, 0);
	FQuat RotationError = FQuat::Identity;

	void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	UFUNCTION(NetMulticast, Unreliable) void MovementRep(FVector NewLocation, FRotator NewRotation, FVector NewVelocity, FVector NewAngularVelocity);
	UFUNCTION(Server, WithValidation, Unreliable) void InputRep(FVector TranslationInput, FVector RotationInput);
	UFUNCTION(Server, WithValidation, Unreliable) void GoalRep(FVector LookAtGoal, FVector MoveToGoal);
	UFUNCTION(Server, WithValidation, Unreliable) void UpVecRep(FVector LevelGoal);
	UFUNCTION(Server, WithValidation, Unreliable) void InputSpaceRep(FRotator Rotator);

	FVector AutopilotRotation(const FVector &Location, const FQuat &Rotation) const;
	FVector AutopilotTranslation(const FVector &Location, const FQuat &Rotation) const;
	FVector RotationInput() const;
	FVector TranslationInput() const;
	FVector UpdateAngularVelocity(const FQuat &Rotation,const FVector &OldAngularVelocity,const FVector &Input, float DeltaTime) const;
	FVector UpdateLinearVelocity(const FVector &Location,const FQuat &Rotation,const FVector &OldVelocity,const FVector &Input, float DeltaTime) const;

	float AxisAcceleration(EAccelerationMode Mode, float Velocity, float Goal, float MaxSpeed, float Inertia, float Input, float DeltaTime) const;

	inline float BlendInput(float low, float high) const;
	inline FVector BlendInput(FVector low, FVector high) const;

	void SimulationStep(float DeltaTime, bool UsePhysics, FBodyInstance* BodyInstance=nullptr);

	void CustomPhysics(float DeltaTime, FBodyInstance* BodyInstance);
};