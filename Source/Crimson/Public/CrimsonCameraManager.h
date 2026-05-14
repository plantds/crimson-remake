// Copyright (c) 2020 Tension Graphics AB

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"

#include "CrimsonCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class CRIMSON_API ACrimsonCameraManager  : public APlayerCameraManager
{
	GENERATED_BODY()


	UPROPERTY(EditDefaultsOnly) float CrouchBlendDuration = 0.5f;
	float CrouchBlendTime = 0.0f;
public:
	ACrimsonCameraManager();

	virtual void UpdateViewTarget(FTViewTarget& _outVT, float _deltaTime) override;
};
