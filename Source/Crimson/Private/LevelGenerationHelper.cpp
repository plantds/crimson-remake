// Copyright (c) 2020 Tension Graphics AB


#include "LevelGenerationHelper.h"
#include "ProceduralSegmentRowData.h"

#include "Engine/LevelStreamingDynamic.h"

void ALevelGenerationHelper::BeginPlay()
{
	Super::BeginPlay();
}

void ALevelGenerationHelper::GenerateStoryLevelSequence()
{
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Yellow, FString::Printf(TEXT("Generation Seed: %i"), randomStream.GetCurrentSeed()));

	if (!segmentDataTable ||
		startSegmentName.IsNone() ||
		endSegmentName.IsNone())
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, FString::Printf(TEXT("Error Spawning Segments: Check Persistent Level Defaults")));
		return;
	}

	// Add Start Segment
	selectedSegments.Add(*reinterpret_cast<FProceduralSegmentRowData*>(segmentDataTable->FindRowUnchecked(startSegmentName)));
	if (printLevelSequence) GEngine->AddOnScreenDebugMessage(-1, 1000.0f, FColor::Magenta, FString::Printf(TEXT("%i   %s"), selectedSegments.Num() - 1, *selectedSegments.Last().levelPath));

	// Middle Segments Loop
	for (const auto& minMaxEnvironmentTypeCombo : minMaxEnvironmentTypesInRow)
	{
		if (minMaxEnvironmentTypeCombo.Key == ESegmentTypes::None) { continue; }

		int randomSegmentAmount = randomStream.RandRange(minMaxEnvironmentTypeCombo.Value.X, minMaxEnvironmentTypeCombo.Value.Y);

		FNameArray copySpecificArray = specificSegments.Contains(minMaxEnvironmentTypeCombo.Key) ? specificSegments[minMaxEnvironmentTypeCombo.Key] : FNameArray();

		for (int i = 0; i < randomSegmentAmount; i++)
		{
			// If Possible: Randomize from specific Tiles
			if (copySpecificArray.levelNames.Num() > 0)
			{
				// I hope no one reads this, or asks me about it - Trust me, it makes sense
				float randomThreshold = 1.0f / (randomSegmentAmount + 1 - copySpecificArray.levelNames.Num() - i);
				if (FMath::FRandRange(0.0f, 1.0f) <= randomSegmentAmount)
				{
					int randomSpecificLevelIndex = FMath::RandRange(0, copySpecificArray.levelNames.Num() - 1);

					selectedSegments.Add(*reinterpret_cast<FProceduralSegmentRowData*>(segmentDataTable->FindRowUnchecked(copySpecificArray.levelNames[randomSpecificLevelIndex])));

					copySpecificArray.levelNames.RemoveAt(randomSpecificLevelIndex);

					continue;
				}
			}

			// Else: Add Random Tile of Type
			auto pickableSegments = GetAllSegmentsWithTypes(minMaxEnvironmentTypeCombo.Key, false);

			float lowerWeight = gameplayRandomWeights.X * ((gameplayTypeThreshold) / maxSameGameplayTypeInRow);
			float upperWeight = 1.0f - gameplayRandomWeights.Y * ((1.0f - gameplayTypeThreshold) / maxSameGameplayTypeInRow);
			float randomGameplayType = randomStream.FRandRange(lowerWeight, upperWeight);

			// Get a random segment from available segments
			while (pickableSegments.Num())
			{
				int randomIndex = randomStream.RandRange(0, pickableSegments.Num() - 1);

				// Make sure it doesnt place same tile twice in a row
				if (selectedSegments.Last().levelPath == pickableSegments[randomIndex].levelPath)
				{
					pickableSegments.RemoveAt(randomIndex);
					continue;
				}

				// Checks Tile Gameplay Type
				if (useGameplayType)
				{
					if ((randomGameplayType <= gameplayTypeThreshold && pickableSegments[randomIndex].gameplayType != EGameplayType::Combat)
						|| (randomGameplayType > gameplayTypeThreshold && pickableSegments[randomIndex].gameplayType != EGameplayType::Platforming))
					{
						pickableSegments.RemoveAt(randomIndex);
						continue;
					}
				}

				// Checks tile direction
				float possibleDirection = currentDirection + pickableSegments[randomIndex].tileEndDirection;
				if (possibleDirection > maxAmountOfTurns || possibleDirection < -maxAmountOfTurns)
				{
					pickableSegments.RemoveAt(randomIndex);
					continue;
				}

				selectedSegments.Add(pickableSegments[randomIndex]);
				if (printLevelSequence)
				{
					switch (minMaxEnvironmentTypeCombo.Key)
					{
						case ESegmentTypes::Nature:
							GEngine->AddOnScreenDebugMessage(-1, 1000.0f, FColor::Turquoise, FString::Printf(TEXT("%i   %s"), selectedSegments.Num() - 1, *selectedSegments.Last().levelPath));
							break;
						case ESegmentTypes::Outskirts:
							GEngine->AddOnScreenDebugMessage(-1, 1000.0f, FColor::Cyan, FString::Printf(TEXT("%i   %s"), selectedSegments.Num() - 1, *selectedSegments.Last().levelPath));
							break;
						case ESegmentTypes::City:
							GEngine->AddOnScreenDebugMessage(-1, 1000.0f, FColor::Orange, FString::Printf(TEXT("%i   %s"), selectedSegments.Num() - 1, *selectedSegments.Last().levelPath));
							break;
					}
				}
				copySpecificArray.levelNames.RemoveSingle(FName(FPaths::GetBaseFilename(selectedSegments.Last().levelPath)));

				// Reset and increment the gameplay types
				switch (selectedSegments.Last().gameplayType)
				{
					case EGameplayType::Combat:
						gameplayRandomWeights.X++;
						gameplayRandomWeights.Y = 0;
						//GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Orange, FString::Printf(TEXT("Combat Level")));
						break;
					case EGameplayType::Platforming:
						gameplayRandomWeights.Y++;
						gameplayRandomWeights.X = 0;
						//GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Purple, FString::Printf(TEXT("Platforming Level")));
						break;
				}

				// Update Tile Direction
				currentDirection += selectedSegments.Last().tileEndDirection;

				break;
			}
		}

		// Spawn Connector Piece
		// MOSTLY DEBUG: Skipping Center connector piece
		if (minMaxEnvironmentTypeCombo.Key != ESegmentTypes::City)
		{
			auto pickableSegments = GetAllSegmentsWithTypes(minMaxEnvironmentTypeCombo.Key, true);
			if (pickableSegments.Num() > 0)
			{
				int randomIndex = randomStream.RandRange(0, pickableSegments.Num() - 1);
				selectedSegments.Add(pickableSegments[randomIndex]);
				if (printLevelSequence) GEngine->AddOnScreenDebugMessage(-1, 1000.0f, FColor::Red, FString::Printf(TEXT("%i   %s"), selectedSegments.Num() - 1, *selectedSegments.Last().levelPath));
				currentDirection += selectedSegments.Last().tileEndDirection;
			}
		}
	}

	selectedSegments.Add(*reinterpret_cast<FProceduralSegmentRowData*>(segmentDataTable->FindRowUnchecked(endSegmentName)));
	if (printLevelSequence) GEngine->AddOnScreenDebugMessage(-1, 1000.0f, FColor::Magenta, FString::Printf(TEXT("%i   %s"), selectedSegments.Num() - 1, *selectedSegments.Last().levelPath));
}


TArray<FProceduralSegmentRowData> ALevelGenerationHelper::GetAllSegmentsWithTypes(int32 types, bool getConnectorPieces)
{
	TArray<FProceduralSegmentRowData> matchingSegments = {};

	segmentDataTable->ForeachRow<FProceduralSegmentRowData>(TEXT("Context"), [types, getConnectorPieces, &matchingSegments](const FName& Key, const FProceduralSegmentRowData& Value){
		if ((getConnectorPieces && Value.connectorPiece) || (!getConnectorPieces && !Value.connectorPiece))
		{
			if (Value.environmentType & types)
			{
				matchingSegments.Add(Value);
			}
		}
	});

	return matchingSegments;
}

void ALevelGenerationHelper::LoadAndUnloadSegmentsAroundIndex(int levelIndex)
{
	loadedLevelsCenterIndex = levelIndex;

	UnloadSegmentsTooFarAway();

	LoadSegmentsClose();
}

void ALevelGenerationHelper::LoadSegmentsClose()
{
	for (int i = loadedLevelsCenterIndex - 1; i <= loadedLevelsCenterIndex + 1; i++)
	{
		// Possibly keep last level always loaded

		if (i < 0 || i >= selectedSegments.Num() || loadedSegments.Contains(i)) // Index out of bounds
			continue;

		bool outSuccess = false;
		FTransform spawnTransform = GetSegmentSpawnTransform(i);

		auto streamedSegment = ULevelStreamingDynamic::LoadLevelInstance(ULevelStreamingDynamic::FLoadLevelInstanceParams(GetWorld(), selectedSegments[i].levelPath, spawnTransform), outSuccess);
		
		TScriptDelegate delegate = {};
		delegate.BindUFunction(this, "OnStreamingLevelShown");
		streamedSegment->OnLevelShown.Add(delegate);

		if (!outSuccess)
			GEngine->AddOnScreenDebugMessage(-1, 100.0f, FColor::Red, FString(TEXT("WEE WOO WEE WOO")));

		loadedSegments.Add(i, streamedSegment);
		OnNewLevelLoaded.Broadcast(i);
	}
}

void ALevelGenerationHelper::UnloadSegmentsTooFarAway()
{
	for (auto itr = loadedSegments.CreateIterator(); itr; ++itr)
	{
		// Possibly keep last level always loaded

		if (!(itr.Key() + 1 == loadedLevelsCenterIndex || itr.Key() - 1 == loadedLevelsCenterIndex || itr.Key() == loadedLevelsCenterIndex)) // Level too far away!
		{
			itr.Value()->SetIsRequestingUnloadAndRemoval(true);
			itr.Value()->OnLevelShown.Clear();
			itr.RemoveCurrent();
		}
	}
}

FTransform ALevelGenerationHelper::GetSegmentSpawnTransform(int segmentIndex)
{
	FTransform spawnTransform = FTransform::Identity;

	for (int i = 0; i < segmentIndex; i++)
	{
		spawnTransform.SetLocation(spawnTransform.GetLocation() + spawnTransform.GetRotation() * selectedSegments[i].tileEndTransform.GetLocation());
		spawnTransform.SetRotation(spawnTransform.GetRotation() * selectedSegments[i].tileEndTransform.GetRotation());
	}

	return spawnTransform;
}

int ALevelGenerationHelper::GetAmountOfLevelsInSequence()
{
	return selectedSegments.Num();
}

void ALevelGenerationHelper::OnStreamingLevelShown()
{
	OnLevelShown.Broadcast();
}

// --------------- ENDLESS ---------------

void ALevelGenerationHelper::BeginEndless()
{
	randomTileOfTypeAmount = randomStream.RandRange(minMaxEnvironmentTypesInRow[currentEndlessTileType].X, minMaxEnvironmentTypesInRow[currentEndlessTileType].Y);

	LoadEndlessLevel(startSegmentName);

	SpawnNextEndlessSegment();
}

void ALevelGenerationHelper::SpawnNextEndlessSegment()
{
	if (randomTileOfTypeAmount > 0) // Get Random Level Pls
	{
		// Get Random Tile of Type
		auto pickableSegments = GetAllSegmentsWithTypes(currentEndlessTileType, false);

		float lowerWeight = gameplayRandomWeights.X * ((gameplayTypeThreshold) / maxSameGameplayTypeInRow);
		float upperWeight = 1.0f - gameplayRandomWeights.Y * ((1.0f - gameplayTypeThreshold) / maxSameGameplayTypeInRow);
		float randomGameplayType = randomStream.FRandRange(lowerWeight, upperWeight);

		// Get a random segment from available segments
		while (pickableSegments.Num())
		{
			int randomIndex = randomStream.RandRange(0, pickableSegments.Num() - 1);

			// Make sure it doesnt place same tile twice in a row
			if (loadedEndlessLevels.Last().Key.levelPath == pickableSegments[randomIndex].levelPath)
			{
				pickableSegments.RemoveAt(randomIndex);
				continue;
			}

			// Checks Tile Gameplay Type
			if (useGameplayType)
			{
				if ((randomGameplayType <= gameplayTypeThreshold && pickableSegments[randomIndex].gameplayType != EGameplayType::Combat)
					|| (randomGameplayType > gameplayTypeThreshold && pickableSegments[randomIndex].gameplayType != EGameplayType::Platforming))
				{
					pickableSegments.RemoveAt(randomIndex);
					continue;
				}
			}

			// Checks tile direction
			float possibleDirection = currentDirection + pickableSegments[randomIndex].tileEndDirection;
			if (possibleDirection > maxAmountOfTurns || possibleDirection < -maxAmountOfTurns)
			{
				pickableSegments.RemoveAt(randomIndex);
				continue;
			}

			// Reset and increment the gameplay types
			switch (pickableSegments[randomIndex].gameplayType)
			{
				case EGameplayType::Combat:
					gameplayRandomWeights.X++;
					gameplayRandomWeights.Y = 0;
					break;
				case EGameplayType::Platforming:
					gameplayRandomWeights.Y++;
					gameplayRandomWeights.X = 0;
					break;
			}

			LoadEndlessLevel(FName(*FPaths::GetBaseFilename(pickableSegments[randomIndex].levelPath)));
			TryUnloadEndlessLevels();

			break;
		}

		randomTileOfTypeAmount--;

		if (pickableSegments.Num() <= 0)
		{
			SpawnNextEndlessSegment();
			return;
		}
	}
	else // Connector
	{
		auto pickableSegments = GetAllSegmentsWithTypes(currentEndlessTileType, true);

		auto nextTileType = GetTileTypeIncremented(currentEndlessTileType, isGoingTowardTown);
		if (nextTileType == ESegmentTypes::None)
		{
			isGoingTowardTown = !isGoingTowardTown;
			nextTileType = GetTileTypeIncremented(currentEndlessTileType, isGoingTowardTown);
		}

		for (auto& segment : pickableSegments)
		{
			if (segment.connectToType != nextTileType) continue;

			LoadEndlessLevel(FName(*FPaths::GetBaseFilename(segment.levelPath)));
			TryUnloadEndlessLevels();

			break;
		}

		currentEndlessTileType = nextTileType;
		randomTileOfTypeAmount = randomStream.RandRange(minMaxEnvironmentTypesInRow[currentEndlessTileType].X, minMaxEnvironmentTypesInRow[currentEndlessTileType].Y);
	}
}

bool ALevelGenerationHelper::LoadEndlessLevel(FName levelName)
{
	FProceduralSegmentRowData levelData = *reinterpret_cast<FProceduralSegmentRowData*>(segmentDataTable->FindRowUnchecked(levelName));


	bool outSuccess = false;
	auto streamedSegment = ULevelStreamingDynamic::LoadLevelInstance(ULevelStreamingDynamic::FLoadLevelInstanceParams(GetWorld(), levelData.levelPath, endlessTransform), outSuccess);

	TScriptDelegate delegate = {};
	delegate.BindUFunction(this, "OnStreamingLevelShown");
	streamedSegment->OnLevelShown.Add(delegate);

	OnNewLevelLoaded.Broadcast(0);
	loadedEndlessLevels.Add({ levelData, streamedSegment });

	// --------------- AdvanceEndlessTransform ---------------

	previousEndlessTransform = endlessTransform;

	FTransform newTransform = levelData.tileEndTransform;

	endlessTransform.SetLocation(endlessTransform.GetLocation() + endlessTransform.GetRotation() * newTransform.GetLocation());
	endlessTransform.SetRotation(endlessTransform.GetRotation() * newTransform.GetRotation());


	// Update Tile Direction
	currentDirection += levelData.tileEndDirection;


	endlessLevelsSpawned++;


	return outSuccess;
}

void ALevelGenerationHelper::TryUnloadEndlessLevels()
{
	while (loadedEndlessLevels.Num() > 3)
	{
		auto& level = loadedEndlessLevels[0];
		level.Value->SetIsRequestingUnloadAndRemoval(true);
		level.Value->OnLevelShown.Clear();
		loadedEndlessLevels.RemoveAt(0);
	}
}

ESegmentTypes ALevelGenerationHelper::GetTileTypeIncremented(ESegmentTypes type, bool incrementUp)
{
	if (incrementUp)
	{
		switch (type)
		{
			case ESegmentTypes::Nature:		return ESegmentTypes::Outskirts;
			case ESegmentTypes::Outskirts:	return ESegmentTypes::City;
			case ESegmentTypes::City:		return ESegmentTypes::None;
		}
	}
	else
	{
		switch (type)
		{
			case ESegmentTypes::City:		return ESegmentTypes::Outskirts;
			case ESegmentTypes::Outskirts:	return ESegmentTypes::Nature;
			case ESegmentTypes::Nature:		return ESegmentTypes::None;
		}
	}

	return ESegmentTypes::None;
}
