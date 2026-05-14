// Copyright (c) 2020 Tension Graphics AB

#pragma once

#include "CoreMinimal.h"
#include "ProceduralSegmentRowData.generated.h"

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum ESegmentTypes : int32
{
	None		= 0 UMETA(Hidden),
	Nature		= 1 << 1,
	Outskirts	= 1 << 2,
	City		= 1 << 3
};
ENUM_CLASS_FLAGS(ESegmentTypes);

UENUM(BlueprintType)
enum EGameplayType
{
	Combat,
	Platforming
};

USTRUCT(BlueprintType)
struct FProceduralSegmentRowData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString levelPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EGameplayType> gameplayType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ESegmentTypes> environmentType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ESegmentTypes> connectToType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform tileEndTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float tileEndDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool connectorPiece;

	FProceduralSegmentRowData()
	{
		levelPath			= "";
		gameplayType		= {};
		environmentType		= {};
		connectToType		= {};
		tileEndTransform	= {};
		tileEndDirection	= 0.0f;
		connectorPiece		= false;
	}
};