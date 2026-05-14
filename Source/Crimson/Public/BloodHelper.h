// Copyright (c) 2020 Tension Graphics AB

#pragma once

#include "CoreMinimal.h"
#include "BloodHelper.generated.h"

UCLASS()
class CRIMSON_API ABloodHelper : public AActor
{
	GENERATED_BODY()

	int16 BloodMaskSize = 64;

public:

	UFUNCTION( BlueprintCallable, Category = "BloodHelper" )
	void SpawnBloodAtMeshUV( UMaterialInstanceDynamic* Material, FVector2D UVCoordinates );
};
