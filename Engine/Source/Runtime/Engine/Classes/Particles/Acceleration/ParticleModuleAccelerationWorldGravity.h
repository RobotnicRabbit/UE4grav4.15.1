/*==============================================================================
ParticleModuleAccelerationWorldGravity: Particle acceleration based on world gravity.
==============================================================================*/

#pragma once
#include "Particles/Acceleration/ParticleModuleAccelerationBase.h"
#include "ParticleModuleAccelerationWorldGravity.generated.h"

UCLASS(editinlinenew, hidecategories = (Object, Acceleration), meta = (DisplayName = "World Gravity"), MinimalAPI)
class UParticleModuleAccelerationWorldGravity : public UParticleModuleAccelerationBase
{
	GENERATED_UCLASS_BODY()

		/**
		*	How much to scale the world gravity acceleration by
		*/
		UPROPERTY(EditAnywhere, Category = ParticleModuleGravityScaleConstant)
		float GravityScale;

	//Begin UParticleModule Interface
	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

#if WITH_EDITOR
	virtual bool IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString) override;
#endif
	//End UParticleModule Interface
};

