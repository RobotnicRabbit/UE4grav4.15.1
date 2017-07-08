//Copyright 2016-2017 Mookie

#include "SixDOFMovementPrivatePCH.h"

FVector USixDOFMovementComponent::GetGravity_Implementation(FVector Location) const{
	if (UseGravity) {
		if (UseGravityVector) {
			return Gravity;
		}
		else {
			return FVector(0, 0, GetWorld()->GetGravityZ());
		}
	}
	else { return FVector(0, 0, 0); }
}

FVector USixDOFMovementComponent::GetWind_Implementation(FVector Location) const{
	return Wind;
}