// Copyright (c) 2020 Tension Graphics AB

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "LevelGenerationHelper.generated.h"

class ULevelStreamingDynamic;
struct FProceduralSegmentRowData;
enum ESegmentTypes : int32;

USTRUCT(BlueprintType)
struct FNameArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> levelNames;

	FNameArray()
	{
		levelNames = {};
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewLevelLoadedSignature, int, LevelIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelShown);

UCLASS()
class CRIMSON_API ALevelGenerationHelper : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnNewLevelLoadedSignature OnNewLevelLoaded;
	UPROPERTY(BlueprintAssignable)
	FOnLevelShown OnLevelShown;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float maxAmountOfTurns = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<TEnumAsByte<ESegmentTypes>, FNameArray> specificSegments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UDataTable* segmentDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool useGameplayType = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float gameplayTypeThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int maxSameGameplayTypeInRow = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName startSegmentName = "";
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName endSegmentName = "";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<TEnumAsByte<ESegmentTypes>, FIntPoint> minMaxEnvironmentTypesInRow = {};

	UPROPERTY()
	TArray<FProceduralSegmentRowData> selectedSegments = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRandomStream randomStream = {};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DEBUG")
	bool printLevelSequence = false;

	UFUNCTION(BlueprintCallable)
	void GenerateStoryLevelSequence();

	UFUNCTION(BlueprintCallable)
	void LoadAndUnloadSegmentsAroundIndex(int levelIndex);

	UFUNCTION(BlueprintCallable)
	FTransform GetSegmentSpawnTransform(int segmentIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetAmountOfLevelsInSequence();

	// ENDLESS

	UFUNCTION(BlueprintCallable)
	void BeginEndless();

	UFUNCTION(BlueprintCallable)
	void SpawnNextEndlessSegment();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int endlessLevelsSpawned = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform endlessTransform = {};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform previousEndlessTransform = {};

private:
	UPROPERTY()
	float currentDirection = 0.0f;

	UPROPERTY()
	int loadedLevelsCenterIndex = 0;

	UPROPERTY()
	FVector2f gameplayRandomWeights = FVector2f::ZeroVector;

	UPROPERTY()
	TMap<int, ULevelStreamingDynamic*> loadedSegments = {};

	UFUNCTION()
	TArray<FProceduralSegmentRowData> GetAllSegmentsWithTypes(int32 types, bool getConnectorPieces);

	UFUNCTION()
	void LoadSegmentsClose();

	UFUNCTION()
	void UnloadSegmentsTooFarAway();

	UFUNCTION()
	void OnStreamingLevelShown();

	// ENDLESS

	UPROPERTY()
	bool isGoingTowardTown = false;


	UPROPERTY()
	int randomTileOfTypeAmount = 0;

	UPROPERTY()
	TEnumAsByte<ESegmentTypes> currentEndlessTileType = ESegmentTypes(1<<1);

	//UPROPERTY()
	TArray<TTuple<FProceduralSegmentRowData, ULevelStreamingDynamic*>> loadedEndlessLevels = {};

	UFUNCTION()
	bool LoadEndlessLevel(FName levelName);
	UFUNCTION()
	void TryUnloadEndlessLevels();

	UFUNCTION()
	ESegmentTypes GetTileTypeIncremented(ESegmentTypes type, bool incrementUp);
};