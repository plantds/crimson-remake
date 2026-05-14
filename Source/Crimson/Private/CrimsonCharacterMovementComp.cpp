// Copyright (c) 2020 Tension Graphics AB

#include "CrimsonCharacterMovementComp.h"
#include "CrimsonCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Helper Macros
#if 1
float MacroDuration = 2.f;
#define SLOG(x) GEngine->AddOnScreenDebugMessage(-1, MacroDuration ? MacroDuration : -1.f, FColor::Yellow, x);
#define POINT(x, c) DrawDebugPoint(GetWorld(), x, 10, c, !MacroDuration, MacroDuration);
#define LINE(x1, x2, c) DrawDebugLine(GetWorld(), x1, x2, c, !MacroDuration, MacroDuration);
#define CAPSULE(x, c) DrawDebugCapsule(GetWorld(), x, CapHH(), CapR(), FQuat::Identity, c, !MacroDuration, MacroDuration);
#else
#define SLOG(x)
#define POINT(x, c)
#define LINE(x1, x2, c)
#define CAPSULE(x, c)
#endif

#pragma region Saved Move

UCrimsonCharacterMovementComp::FSavedMove_Crimson::FSavedMove_Crimson() {
	Saved_bWantsToSprint =		0;
	Saved_bPrevWantsToCrouch =	0;
}

bool UCrimsonCharacterMovementComp::FSavedMove_Crimson::CanCombineWith(const FSavedMovePtr& _newMove, ACharacter* _inCharacter, float _maxDelta) const
{
	FSavedMove_Crimson* NewCrimsonMove = static_cast<FSavedMove_Crimson*>(_newMove.Get());

	if (Saved_bWantsToSprint != NewCrimsonMove->Saved_bWantsToSprint)
		return false;

	if (Saved_bWantsToDash != NewCrimsonMove->Saved_bPrevWantsToCrouch)
		return false;

	if (Saved_bWantsToDash != NewCrimsonMove->Saved_bWantsToDash)
		return false;

	if (Saved_bWallRunIsRight != NewCrimsonMove->Saved_bWallRunIsRight)
		return false;

	return FSavedMove_Character::CanCombineWith(_newMove, _inCharacter, _maxDelta);
}

void UCrimsonCharacterMovementComp::FSavedMove_Crimson::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bWantsToSprint		= 0;
	Saved_bWantsToDash			= 0;
	Saved_bPressedCrimsonJump	= 0;

	Saved_bHadAnimRootMotion	= 0;
	Saved_bTransitionFinished	= 0;

	Saved_bPrevWantsToCrouch	= 0;

	Saved_bWallRunIsRight		= 0;
}

uint8 UCrimsonCharacterMovementComp::FSavedMove_Crimson::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (Saved_bWantsToSprint)		Result |= FLAG_Sprint;
	if (Saved_bWantsToDash)			Result |= FLAG_Dash;
	if (Saved_bPressedCrimsonJump)	Result |= FLAG_JumpPressed;

	return Result;
}

void UCrimsonCharacterMovementComp::FSavedMove_Crimson::SetMoveFor(ACharacter* _c, float _inDeltaTime, FVector const& _newAccel, FNetworkPredictionData_Client_Character& _clientData)
{
	FSavedMove_Character::SetMoveFor(_c, _inDeltaTime, _newAccel, _clientData);

	UCrimsonCharacterMovementComp* CharacterMovement = Cast< UCrimsonCharacterMovementComp>(_c->GetCharacterMovement());

	Saved_bWantsToSprint		= CharacterMovement->Safe_bWantsToSprint;
	Saved_bPrevWantsToCrouch	= CharacterMovement->Safe_bPrevWantsToCrouch;
	Saved_bPressedCrimsonJump	= CharacterMovement->CrimsonCharacterOwner->bPressedCrimsonJump;

	Saved_bHadAnimRootMotion	= CharacterMovement->Safe_bHadAnimRootMotion;
	Saved_bTransitionFinished	= CharacterMovement->Safe_bTransitionFinished;

	Saved_bWantsToDash			= CharacterMovement->Safe_bWantsToDash;

	Saved_bWallRunIsRight		= CharacterMovement->Safe_bWallRunIsRight;
}

void UCrimsonCharacterMovementComp::FSavedMove_Crimson::PrepMoveFor(ACharacter* _c)
{
	Super::PrepMoveFor(_c);

	UCrimsonCharacterMovementComp* CharacterMovement = Cast< UCrimsonCharacterMovementComp>(_c->GetCharacterMovement());

	CharacterMovement->Safe_bWantsToSprint							= Saved_bWantsToSprint;
	CharacterMovement->Safe_bPrevWantsToCrouch						= Saved_bPrevWantsToCrouch;
	CharacterMovement->CrimsonCharacterOwner->bPressedCrimsonJump	= Saved_bPressedCrimsonJump;

	CharacterMovement->Safe_bHadAnimRootMotion						= Saved_bHadAnimRootMotion;
	CharacterMovement->Safe_bTransitionFinished						= Saved_bTransitionFinished;

	CharacterMovement->Safe_bWantsToDash							= Saved_bWantsToDash;

	CharacterMovement->Safe_bWallRunIsRight							= Saved_bWallRunIsRight;
}

#pragma endregion

#pragma region Client Network Prediction Data

UCrimsonCharacterMovementComp::FNetworkPredictionData_Client_Crimson::FNetworkPredictionData_Client_Crimson(const UCharacterMovementComponent& _clientMovement)
	: Super(_clientMovement) {}

FSavedMovePtr UCrimsonCharacterMovementComp::FNetworkPredictionData_Client_Crimson::AllocateNewMove() { return FSavedMovePtr(new FSavedMove_Crimson); }

#pragma endregion

UCrimsonCharacterMovementComp::UCrimsonCharacterMovementComp()
{
	NavAgentProps.bCanCrouch = true;
	CrimsonServerMoveBitWriter.SetAllowResize(true);
}

void UCrimsonCharacterMovementComp::TickComponent(float _deltaTime, ELevelTick _tickType, FActorComponentTickFunction* _thisTickFunction)
{
	Super::TickComponent(_deltaTime, _tickType, _thisTickFunction);
}

#pragma region CMC

void UCrimsonCharacterMovementComp::InitializeComponent()
{
	Super::InitializeComponent();

	CrimsonCharacterOwner = Cast<ACrimsonCharacter>(GetOwner());
}

void UCrimsonCharacterMovementComp::UpdateFromCompressedFlags(uint8 _flags)
{
	Super::UpdateFromCompressedFlags(_flags);

	Safe_bWantsToSprint = (_flags & FSavedMove_Crimson::FLAG_Sprint)	!= 0;
	Safe_bWantsToDash = (_flags & FSavedMove_Crimson::FLAG_Dash)		!= 0;
}

void UCrimsonCharacterMovementComp::OnClientCorrectionReceived(FNetworkPredictionData_Client_Character& _clientData, float _timeStamp, FVector _newLocation, 
															   FVector _newVelocity, UPrimitiveComponent* _newBase, FName _newBaseBoneName, bool _bHasBase,
															   bool _bBaseRelativePosition, uint8 _serverMovementMode, FVector _serverGravityDirection)
{
	Super::OnClientCorrectionReceived(_clientData, _timeStamp, _newLocation, 
									  _newVelocity, _newBase, _newBaseBoneName, _bHasBase, 
									  _bBaseRelativePosition, _serverMovementMode, _serverGravityDirection);
	
	CorrectionCount++;
}

FNetworkPredictionData_Client* UCrimsonCharacterMovementComp::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

	if (ClientPredictionData == nullptr) {
		UCrimsonCharacterMovementComp* MutableThis = const_cast<UCrimsonCharacterMovementComp*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Crimson(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.0f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.0f;
	}
	return ClientPredictionData;
}

// Getters / Helpers
bool UCrimsonCharacterMovementComp::CanCoyoteJump() const
{
	if (HasJumped)
		return false;

	if (CoyoteJumpTimer > 0.0f)
		return true;
	return false;
}

bool UCrimsonCharacterMovementComp::IsMovingOnGround() const { return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide); }

bool UCrimsonCharacterMovementComp::CanCrouchInCurrentState() const { return Super::CanCrouchInCurrentState() && IsMovingOnGround(); }

bool UCrimsonCharacterMovementComp::CanAttemptJump() const { return Super::CanAttemptJump() || IsWallRunning() || CanCoyoteJump(); }

bool UCrimsonCharacterMovementComp::DoJump(bool _bReplayingMoves, float _deltatime)
{
	bool bWasWallRunning = IsWallRunning();
	if (Super::DoJump(_bReplayingMoves, _deltatime))
	{
		HasJumped = true;
		if (bWasWallRunning)
		{
			FVector V = FVector(0.0f,CrimsonCharacterOwner->GetScaleValue(), 0.0f);
			Velocity += IsGettingInputRight() ? (V * CrimsonCharacterOwner->GetCapsuleComponent()->GetRightVector() * WallJumpInputOutForce) : CrimsonCharacterOwner->GetFollowCamera()->GetForwardVector() * WallJumpOffForce;
			
		}
		return true;
	}
	if (CanCoyoteJump() && !HasJumped && CharacterOwner) {
		HasJumped = true;

		if (!bConstrainToPlane || !FMath::IsNearlyEqual(FMath::Abs(GetGravitySpaceZ(PlaneConstraintNormal)), 1.f))
		{
			const bool bFirstJump = (CharacterOwner->JumpCurrentCountPreJump == 0);

			if (bFirstJump || bDontFallBelowJumpZVelocityDuringJump)
			{
				if (HasCustomGravity())
				{
					SetGravitySpaceZ(Velocity, FMath::Max<FVector::FReal>(GetGravitySpaceZ(Velocity), JumpZVelocity));
				}
				else
				{
					Velocity.Z = FMath::Max<FVector::FReal>(Velocity.Z, JumpZVelocity);
				}
			}

			SetMovementMode(MOVE_Falling);
			return true;
		}
	}
	return false;
}

void UCrimsonCharacterMovementComp::GetMaxSprintSpeed(float& speed)
{
	speed = MaxSprintSpeed;
}

float UCrimsonCharacterMovementComp::GetMaxSpeed() const
{
	if (IsMovementMode(MOVE_Walking) && Safe_bWantsToSprint && !IsCrouching()) return MaxSprintSpeed;

	if (MovementMode != MOVE_Custom) return Super::GetMaxSpeed();

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		return MaxSlideSpeed;
	case CMOVE_WallRun:
		return MaxWallRunSpeed;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
			return -1.f;
	}
}

float UCrimsonCharacterMovementComp::GetMaxBrakingDeceleration() const
{
	if (MovementMode != MOVE_Custom) return Super::GetMaxBrakingDeceleration();

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		return BrakingDecelerationSliding;
	case CMOVE_WallRun:
		return 1000.0f;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
			return -1.f;
	}
}


// Movement Pipeline
void UCrimsonCharacterMovementComp::UpdateCharacterStateBeforeMovement(float _deltaSeconds)
{
	bHasStartedToMantle = false;

	if(!IsCustomMovementMode(CMOVE_Slide))
		bIsSliding = false;

	if (IsCustomMovementMode(CMOVE_WallRun) || !IsFalling()) {
		hasDashed = false;
		CoyoteJumpTimer = CoyoteJumpTime;
		HasJumped = false;
	}

	if (IsMovingOnGround()) {
		WallHitActor = nullptr;
	}
	
	if (IsFalling())
		CoyoteJumpTimer -= _deltaSeconds;

	// Slide
	if ((MovementMode == MOVE_Walking && bWantsToCrouch && Safe_bPrevWantsToCrouch) || ForceSlide())
	{
		if (CanSlide() || ForceSlide())
		{
			SetMovementMode(MOVE_Custom, CMOVE_Slide);
		}
	}
	else if ((IsCustomMovementMode(CMOVE_Slide) && !bWantsToCrouch) && !ForceSlide())
	{
		SetMovementMode(MOVE_Walking);
	}

	// Dash
	if (Safe_bWantsToDash && CanDash())
	{
		PerformDash();
		Safe_bWantsToDash = false;
	}

	// Try Mantle
	if (CrimsonCharacterOwner->bPressedCrimsonJump)
	{
		//SLOG("Trying Crimsonjump");
		if (TryMantle())
		{
			CrimsonCharacterOwner->StopJumping();
		}
		else
		{
			//SLOG("Failed Mantle, Reverting to jump");
			CrimsonCharacterOwner->bPressedCrimsonJump = false;
			CharacterOwner->bPressedJump = true;
			CharacterOwner->CheckJumpInput(_deltaSeconds);
		}
	}

	// Transition
	if (Safe_bTransitionFinished)
	{
		if (TransitionName == "Mantle")
		{
			if (IsValid(TransitionQueuedMontage))
			{
				SetMovementMode(MOVE_Flying);
				CharacterOwner->PlayAnimMontage(TransitionQueuedMontage, TransitionQueuedMontageSpeed);
				TransitionQueuedMontageSpeed = 0.f;
				TransitionQueuedMontage = nullptr;
			}
			else
			{
				SetMovementMode(MOVE_Walking);
			}
		}
		TransitionName = "";
		Safe_bTransitionFinished = false;
	}

	// Wall Run
	if (IsFalling())
	{
		TryWallRun();
	}

	Super::UpdateCharacterStateBeforeMovement(_deltaSeconds);
}

void UCrimsonCharacterMovementComp::UpdateCharacterStateAfterMovement(float _deltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(_deltaSeconds);

	if (!HasAnimRootMotion() && Safe_bHadAnimRootMotion && IsMovementMode(MOVE_Flying))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Ending Anim Root Motion"))
		SetMovementMode(MOVE_Walking);
	}

	if (GetRootMotionSourceByID(TransitionRMS_ID) && GetRootMotionSourceByID(TransitionRMS_ID)->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		RemoveRootMotionSourceByID(TransitionRMS_ID);
		Safe_bTransitionFinished = true;
	}

	Safe_bHadAnimRootMotion = HasAnimRootMotion();
}

void UCrimsonCharacterMovementComp::PhysCustom(float _deltaTime, int32 _iterations)
{
	Super::PhysCustom(_deltaTime, _iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(_deltaTime, _iterations);
		break;
	case CMOVE_WallRun:
		PhysWallRun(_deltaTime, _iterations);
		break;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
	}
}

void UCrimsonCharacterMovementComp::OnMovementUpdated(float _deltaSeconds, const FVector& _oldLocation, const FVector& _oldVelocity)
{
	Super::OnMovementUpdated(_deltaSeconds, _oldLocation, _oldVelocity);

	if (IsMovementMode(MOVE_Flying) && !HasRootMotionSources()) SetMovementMode(MOVE_Falling);
	
	Safe_bPrevWantsToCrouch = bWantsToCrouch;
}

// Movement Event
void UCrimsonCharacterMovementComp::OnMovementModeChanged(EMovementMode _previousMovementMode, uint8 _previousCustomMode)
{
	Super::OnMovementModeChanged(_previousMovementMode, _previousCustomMode);

	if (_previousMovementMode == MOVE_Custom && _previousCustomMode == CMOVE_Slide) ExitSlide();
	if (IsCustomMovementMode(CMOVE_Slide)) EnterSlide(_previousMovementMode, (ECustomMovementMode)_previousCustomMode);

	if (IsWallRunning() && GetOwnerRole() == ROLE_SimulatedProxy)
	{
		FVector Start = UpdatedComponent->GetComponentLocation();
		FVector End = Start + UpdatedComponent->GetRightVector() * CapR() * 2;
		auto Params = CrimsonCharacterOwner->GetIgnoreCharacterParams();
		FHitResult WallHit;
		Safe_bWallRunIsRight = GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);
	}
}


bool UCrimsonCharacterMovementComp::ServerCheckClientError(float ClientTimeStamp, float DeltaTime,
	const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation,
	UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	if (GetCurrentNetworkMoveData()->NetworkMoveType == FCharacterNetworkMoveData::ENetworkMoveType::NewMove)
	{
		float LocationError = FVector::Dist(UpdatedComponent->GetComponentLocation(), ClientWorldLocation);
		GEngine->AddOnScreenDebugMessage(6, 100.f, FColor::Yellow, FString::Printf(TEXT("Loc: %s"), *ClientWorldLocation.ToString()));
		AccumulatedClientLocationError += LocationError * DeltaTime;
	}

	return Super::ServerCheckClientError(ClientTimeStamp, DeltaTime, Accel, ClientWorldLocation, RelativeClientLocation,
		ClientMovementBase,
		ClientBaseBoneName, ClientMovementMode);

}

void UCrimsonCharacterMovementComp::CallServerMovePacked(const FSavedMove_Character* _newMove, const FSavedMove_Character* _pendingMove, const FSavedMove_Character* _oldMove)
{
	// Get storage container we'll be using and fill it with movement data
	FCharacterNetworkMoveDataContainer& MoveDataContainer = GetNetworkMoveDataContainer();
	MoveDataContainer.ClientFillNetworkMoveData(_newMove, _pendingMove, _oldMove);

	// Reset bit writer without affecting allocations
	FBitWriterMark BitWriterReset;
	BitWriterReset.Pop(CrimsonServerMoveBitWriter);

	// 'static' to avoid reallocation each invocation
	static FCharacterServerMovePackedBits PackedBits;
	UNetConnection* NetConnection = CharacterOwner->GetNetConnection();


	{
		// Extract the net package map used for serializing object references.
		CrimsonServerMoveBitWriter.PackageMap = NetConnection ? ToRawPtr(NetConnection->PackageMap) : nullptr;
	}

	if (CrimsonServerMoveBitWriter.PackageMap == nullptr)
	{
		UE_LOG(LogNetPlayerMovement, Error, TEXT("CallServerMovePacked: Failed to find a NetConnection/PackageMap for data serialization!"));
		return;
	}

	// Serialize move struct into a bit stream
	if (!MoveDataContainer.Serialize(*this, CrimsonServerMoveBitWriter, CrimsonServerMoveBitWriter.PackageMap) || CrimsonServerMoveBitWriter.IsError())
	{
		UE_LOG(LogNetPlayerMovement, Error, TEXT("CallServerMovePacked: Failed to serialize out movement data!"));
		return;
	}

	// Copy bits to our struct that we can NetSerialize to the server.
	PackedBits.DataBits.SetNumUninitialized(CrimsonServerMoveBitWriter.GetNumBits());

	check(PackedBits.DataBits.Num() >= CrimsonServerMoveBitWriter.GetNumBits());
	FMemory::Memcpy(PackedBits.DataBits.GetData(), CrimsonServerMoveBitWriter.GetData(), CrimsonServerMoveBitWriter.GetNumBytes());

	TotalBitsSent += PackedBits.DataBits.Num();

	// Send bits to server!
	ServerMovePacked_ClientSend(PackedBits);

	MarkForClientCameraUpdate();
}

float UCrimsonCharacterMovementComp::inverseLerp(float a, float b, float t)
{
	return (t - a) / (b - a);
}

#pragma endregion

#pragma region Slide

void UCrimsonCharacterMovementComp::EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	SLOG("Entered a slide");

	bWantsToCrouch = true;
	Velocity += Velocity.GetSafeNormal2D() * SlideEnterImpulse;

	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true, NULL);
}

void UCrimsonCharacterMovementComp::ExitSlide()
{
	//bWantsToCrouch = false;

}

bool UCrimsonCharacterMovementComp::CanSlide() const
{
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector End = Start + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.5f * FVector::DownVector;
	FName ProfileName = TEXT("BlockAll");
	bool bValidSurface = GetWorld()->LineTraceTestByProfile(Start, End, ProfileName, CrimsonCharacterOwner->GetIgnoreCharacterParams());
	bool bEnoughSpeed = Velocity.SizeSquared() > pow(MinSlideSpeed, 2);

	return bValidSurface && bEnoughSpeed;
}

void UCrimsonCharacterMovementComp::PhysSlide(float _deltaTime, int32 _iterations)
{
	if (_deltaTime < MIN_TICK_TIME)
		return;

	if (!CanSlide() && !ForceSlide())
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(_deltaTime, _iterations);
		return;
	}

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = _deltaTime;

	bIsSliding = true;

	// Perform the move
	while ((remainingTime >= MIN_TICK_TIME) && (_iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		_iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, _iterations);
		remainingTime -= timeTick;

		// Save current values
		UPrimitiveComponent* const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;

		FVector SlopeForce = CurrentFloor.HitResult.Normal;
		SlopeForce.Z = 0.0f;
		Velocity += SlopeForce * SlideGravityForce * _deltaTime;

		Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector().GetSafeNormal2D());

		// Apply acceleration
		CalcVelocity(timeTick, GroundFriction * SlideFrictionFactor, false, GetMaxBrakingDeceleration());

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;
		bool bFloorWalkable = CurrentFloor.IsWalkableFloor();

		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if (IsFalling())
			{
				// pawn decided to jump up
				const float DesiredDist = Delta.Size();
				if (DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}
				StartNewPhysics(remainingTime, _iterations);
				return;
			}
			else if (IsSwimming()) //just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, _iterations);
				return;
			}
		}
		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}
		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if (bCheckLedges && !CurrentFloor.IsWalkableFloor())
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f, 0.f, -1.f);
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, CurrentFloor);
			if (!NewDelta.IsZero())
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;

				// Try new movement direction
				Velocity = NewDelta / timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// see if it is OK to jump
				bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, _iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				if (ShouldCatchAir(OldFloor, CurrentFloor))
				{
					HandleWalkingOffLedge(OldFloor.HitResult.ImpactNormal, OldFloor.HitResult.Normal, OldLocation, timeTick);
					if (IsMovingOnGround())
					{
						// If still walking, then fall. If not, assume the user set a different mode they want to keep.
						StartFalling(_iterations, remainingTime, timeTick, Delta, OldLocation);
					}
					return;
				}

				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
				bForceNextFloorCheck = true;
			}
			// check if just entered water
			if (IsSwimming())
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, _iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, _iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;
			}
		}
		// Allow overlap events and such to change physics state and velocity
		if (IsMovingOnGround() && bFloorWalkable)
		{
			// Make velocity reflect actual move
			if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
				MaintainHorizontalGroundVelocity();
			}
		}

		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}

	//FHitResult Hit;
	//FQuat NewRotation = FRotationMatrix::MakeFromXZ(Velocity.GetSafeNormal2D(), FVector::UpVector).ToQuat();
	//SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, false, Hit);
}

void UCrimsonCharacterMovementComp::IsSliding(bool& Sliding)
{
	Sliding = bIsSliding;
}

bool UCrimsonCharacterMovementComp::ForceSlide()
{
	if (IsFalling())
		return false;

	FHitResult ground;
	FVector Start = UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH();
	FVector End = FVector::DownVector * 50.0f;
	auto param = CrimsonCharacterOwner->GetIgnoreCharacterParams();

	GetWorld()->LineTraceSingleByProfile(ground, Start, Start + End, "BlockAll", param);
	if (!ground.IsValidBlockingHit()) {
		FrocedSlide = false;
		return false;
	}
	
	bool hasTag = false;
	for (FName tag : ground.GetActor()->Tags) {
		if (tag == "ForceSlide") {
			hasTag = true;
			break;
		}
	}

	if (!hasTag) {
		FrocedSlide = false;
		return false;
	}

	// before returning true
	bWantsToCrouch = true;
	Safe_bPrevWantsToCrouch = true;
	FrocedSlide = true;
	return true;
}

void UCrimsonCharacterMovementComp::IsForcedSlide(bool& Slidind)
{
	Slidind = FrocedSlide;
}

#pragma endregion

#pragma region Dash

void UCrimsonCharacterMovementComp::OnDashCooldownFinished()
{
	Safe_bWantsToDash = true;
}

bool UCrimsonCharacterMovementComp::CanDash() const
{
	return (IsWalking() || IsCrouching() || IsFalling() && !hasDashed);
}

void UCrimsonCharacterMovementComp::PerformDash()
{
	hasDashed = true;

	DashStartTime = GetWorld()->GetTimeSeconds();

	FVector DashDir = (Acceleration.IsNearlyZero() ? UpdatedComponent->GetForwardVector() : Acceleration).GetSafeNormal2D();

	auto Params = CrimsonCharacterOwner->GetIgnoreCharacterParams();

	FHitResult ground;
	FVector Start =	UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH();
	FVector End = FVector::DownVector * 30.0f;

	GetWorld()->LineTraceSingleByProfile(ground, Start, Start+ End, "BlockAll", Params);
	if (ground.IsValidBlockingHit()) {
		LINE(ground.ImpactPoint, ground.ImpactPoint + ground.ImpactNormal*30.0f, FColor::Cyan);
		DashDir = FVector::VectorPlaneProject(DashDir, ground.ImpactNormal);
		LINE(ground.ImpactPoint, ground.ImpactPoint + DashDir * 30.0f, FColor::Red);
	}

	Velocity = DashImpulse * DashDir;

	bWantsToCrouch = false;

	SetMovementMode(MOVE_Flying);

	DashStartDelegate.Broadcast();
}

#pragma endregion

#pragma region Mantle

bool UCrimsonCharacterMovementComp::TryMantle()
{
	if (!(IsMovementMode(MOVE_Walking) && !IsCrouching()) && !IsMovementMode(MOVE_Falling)) return false;

	//Helper Variables
	FVector BaseLoc = UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH();
	FVector Fwd = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	auto Params = CrimsonCharacterOwner->GetIgnoreCharacterParams();
	float MaxHeight = CapHH() * 2 + MantleReachHeight;
	float CosMMWSA = FMath::Cos(FMath::DegreesToRadians(MantleMinWallSteepnessAngle));
	float CosMMSA = FMath::Cos(FMath::DegreesToRadians(MantleMaxSurfaceAngle));
	float CosMMAA = FMath::Cos(FMath::DegreesToRadians(MantleMaxAlignmentAngle));

	//SLOG("Starting Mantle Attempt")

	// Check Front Face
	FHitResult FrontHit;
	float CheckDistance = FMath::Clamp(Velocity | Fwd, CapR() + 30, MantleMaxDistance);
	FVector FrontStart = BaseLoc + FVector::UpVector * (MaxStepHeight - 1);
	for (int i = 0; i < 6; i++)
	{
		LINE(FrontStart, FrontStart + Fwd * CheckDistance, FColor::Red)
			if (GetWorld()->LineTraceSingleByProfile(FrontHit, FrontStart, FrontStart + Fwd * CheckDistance, "BlockAll", Params)) break;
		FrontStart += FVector::UpVector * (2.f * CapHH() - (MaxStepHeight - 1)) / 5;
	}
	if (!FrontHit.IsValidBlockingHit()) return false;
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	if (FMath::Abs(CosWallSteepnessAngle) > CosMMWSA || (Fwd | -FrontHit.Normal) < CosMMAA) return false;

	POINT(FrontHit.Location, FColor::Red);

	// Check Height
	TArray<FHitResult> HeightHits;
	FHitResult SurfaceHit;
	FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, FrontHit.Normal).GetSafeNormal();
	float WallCos = FVector::UpVector | FrontHit.Normal;
	float WallSin = FMath::Sqrt(1 - WallCos * WallCos);
	FVector TraceStart = FrontHit.Location + Fwd + WallUp * (MaxHeight - (MaxStepHeight - 1)) / WallSin;
	LINE(TraceStart, FrontHit.Location + Fwd, FColor::Orange)
	if (!GetWorld()->LineTraceMultiByProfile(HeightHits, TraceStart, FrontHit.Location + Fwd, "BlockAll", Params)) return false;
	for (const FHitResult& Hit : HeightHits)
	{
		if (Hit.IsValidBlockingHit())
		{
			SurfaceHit = Hit;
			break;
		}
	}
	if (!SurfaceHit.IsValidBlockingHit() || (SurfaceHit.Normal | FVector::UpVector) < CosMMSA) return false;
	float Height = (SurfaceHit.Location - BaseLoc) | FVector::UpVector;

	//SLOG(FString::Printf(TEXT("Height: %f"), Height));
	POINT(SurfaceHit.Location, FColor::Blue);

	if (Height > MaxHeight) return false;

	// Check Clearance
	float SurfaceCos = FVector::UpVector | SurfaceHit.Normal;
	float SurfaceSin = FMath::Sqrt(1 - SurfaceCos * SurfaceCos);
	FVector ClearCapLoc = SurfaceHit.Location + Fwd * CapR() + FVector::UpVector * (CapHH() + 1 + CapR() * 2 * SurfaceSin);
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(CapR(), CapHH());
	if (GetWorld()->OverlapAnyTestByProfile(ClearCapLoc, FQuat::Identity, "BlockAll", CapShape, Params))
	{
		CAPSULE(ClearCapLoc, FColor::Red)
			return false;
	}
	else
	{
		CAPSULE(ClearCapLoc, FColor::Green)
	}
	//SLOG("Can Mantle")

	bHasStartedToMantle = true;

	// Mantle Selection	
	FVector ShortMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, false);
	FVector TallMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, true);

	bool bTallMantle = false;
	if (IsMovementMode(MOVE_Walking) && Height > CapHH() * 2)
		bTallMantle = true;
	else if (IsMovementMode(MOVE_Falling) && (Velocity | FVector::UpVector) < 0)
	{
		if (!GetWorld()->OverlapAnyTestByProfile(TallMantleTarget, FQuat::Identity, "BlockAll", CapShape, Params))
			bTallMantle = true;
	}
	FVector TransitionTarget = ClearCapLoc;/*bTallMantle ? TallMantleTarget : ShortMantleTarget;*/
	CAPSULE(TransitionTarget, FColor::Yellow);

	// Perform Transition to Mantle
	CAPSULE(UpdatedComponent->GetComponentLocation(), FColor::Red);

	float UpSpeed = Velocity | FVector::UpVector;
	float TransDistance = FVector::Dist(TransitionTarget, UpdatedComponent->GetComponentLocation());
	
	TransitionQueuedMontageSpeed = FMath::GetMappedRangeValueClamped(FVector2D(-500, 750), FVector2D(.9f, 1.2f), UpSpeed);
	TransitionRMS.Reset();
	TransitionRMS = MakeShared<FRootMotionSource_MoveToForce>();
	TransitionRMS->AccumulateMode = ERootMotionAccumulateMode::Override;
	
	TransitionRMS->Duration = FMath::Clamp(TransDistance / 500.f, .1f, .25f);
	//SLOG(FString::Printf(TEXT("Duration: %f"), TransitionRMS->Duration));
	TransitionRMS->StartLocation = UpdatedComponent->GetComponentLocation();
	TransitionRMS->TargetLocation = TransitionTarget;

	// Apply Transition Root Motion Source
	Velocity = FVector::ZeroVector;
	SetMovementMode(MOVE_Flying);
	TransitionRMS_ID = ApplyRootMotionSource(TransitionRMS);
	TransitionName = "Mantle";

	return true;
}

FVector UCrimsonCharacterMovementComp::GetMantleStartLocation(FHitResult _frontHit, FHitResult _surfaceHit, bool _bTallMantle) const
{
	float CosWallSteepnessAngle = _frontHit.Normal | FVector::UpVector;
	float DownDistance = _bTallMantle ? CapHH() * 2.f : MaxStepHeight - 1;
	FVector EdgeTangent = FVector::CrossProduct(_surfaceHit.Normal, _frontHit.Normal).GetSafeNormal();

	FVector MantleStart = _surfaceHit.Location;
	MantleStart += _frontHit.Normal.GetSafeNormal2D() * (2.f + CapR());
	MantleStart += UpdatedComponent->GetForwardVector().GetSafeNormal2D().ProjectOnTo(EdgeTangent) * CapR() * .3f;
	MantleStart += FVector::UpVector * CapHH();
	MantleStart += FVector::DownVector * DownDistance;
	MantleStart += _frontHit.Normal.GetSafeNormal2D() * CosWallSteepnessAngle * DownDistance;

	return MantleStart;
}

void UCrimsonCharacterMovementComp::HasStartedToMantle(bool& Mantle)
{
	Mantle = bHasStartedToMantle;
}

#pragma endregion

#pragma region Wall Run

bool UCrimsonCharacterMovementComp::TryWallRun()
{
	if (!IsFalling()) return false;
	if (Velocity.SizeSquared2D() < pow(MinWallRunSpeed, 2)) return false;
	if (Velocity.Z < -MaxVerticalWallRunSpeed) return false;
	FVector Start	 = UpdatedComponent->GetComponentLocation();
	FVector LeftEnd	 = Start - UpdatedComponent->GetRightVector() * CapR() * 2;
	FVector RightEnd = Start + UpdatedComponent->GetRightVector() * CapR() * 2;
	auto Params		 = CrimsonCharacterOwner->GetIgnoreCharacterParams();
	FHitResult FloorHit, WallHit;

	// Check Player Height
	if (GetWorld()->LineTraceSingleByProfile(FloorHit, Start, Start + FVector::DownVector * (CapHH() + MinWallRunHeight), "BlockAll", Params))
	{
		return false;
	}

	// Left Cast
	GetWorld()->LineTraceSingleByProfile(WallHit, Start, LeftEnd, "BlockAll", Params);
	if (WallHit.IsValidBlockingHit() && (Velocity | WallHit.Normal) < 0)
	{
		Safe_bWallRunIsRight = false;
	}

	// Right Cast
	else
	{
		GetWorld()->LineTraceSingleByProfile(WallHit, Start, RightEnd, "BlockAll", Params);
		if (WallHit.IsValidBlockingHit() && (Velocity | WallHit.Normal) < 0)
		{
			Safe_bWallRunIsRight = true;
		}
		else
		{	
			return false;
		}
	}

	if (WallHitActor == WallHit.GetActor())
		return false;
	else
		WallHitActor = WallHit.GetActor();



	UE_LOG(LogTemp, Warning, TEXT("WALL OF RUN"));

	FVector ProjectedVelocity = FVector::VectorPlaneProject(Velocity, WallHit.Normal);
	if (ProjectedVelocity.SizeSquared2D() < pow(MinWallRunSpeed, 2)) return false;

	// Passed all conditions
	Velocity	= ProjectedVelocity;
	Velocity.Z	= FMath::Clamp(Velocity.Z, 0.f, MaxVerticalWallRunSpeed);
	SetMovementMode(MOVE_Custom, CMOVE_WallRun);
	wallrunStartTime = 0.0f;
	SLOG("Starting WallRun");
	return true;
}

void UCrimsonCharacterMovementComp::PhysWallRun(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && 
		!CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	bJustTeleported = false;
	float remainingTime = deltaTime;
	// Perform the move
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller ||
		bRunPhysicsWithNoController || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();

		wallrunStartTime += deltaTime * DropSpeedMulti;

		FVector Start = UpdatedComponent->GetComponentLocation();
		FVector CastDelta = UpdatedComponent->GetRightVector() * CapR() * 2;
		FVector End = Safe_bWallRunIsRight ? Start + CastDelta : Start - CastDelta;
		auto Params = CrimsonCharacterOwner->GetIgnoreCharacterParams();
		float SinPullAwayAngle = FMath::Sin(FMath::DegreesToRadians(WallRunPullAwayAngle));
		FHitResult WallHit;
		GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);
		bool bWantsToPullAway = WallHit.IsValidBlockingHit() && !Acceleration.IsNearlyZero() && (Acceleration.GetSafeNormal() | WallHit.Normal) > SinPullAwayAngle;
		if (!WallHit.IsValidBlockingHit() || bWantsToPullAway)
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(remainingTime, Iterations);
			return;
		}

		// Apply acceleration
		CalcVelocity(timeTick, 0.f, false, GetMaxBrakingDeceleration());
		Velocity = FVector::VectorPlaneProject(Velocity, WallHit.Normal);
		bool bVelUp = Velocity.Z > 0.f;
		Velocity.Z += GetGravityZ() * wallrunStartTime /*WallRunGravityScaleCurve->GetFloatValue(inverseLerp(0.0f, 5.0f, wallrunStartTime))*/ * timeTick;
		UE_LOG(LogTemp, Error, TEXT("The float value is: %f"), Velocity.Z);
		if (Velocity.SizeSquared2D() < pow(MinWallRunSpeed, 2) /*|| Velocity.Z < -MaxVerticalWallRunSpeed*/)
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(remainingTime, Iterations);
			return;
		}

		// Compute move parameters
		const FVector Delta = timeTick * Velocity; // dx = v * dt
		const bool bZeroDelta = Delta.IsNearlyZero();
		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			FHitResult Hit;
			SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
			FVector WallAttractionDelta = -WallHit.Normal * WallAttractionForce * timeTick;
			SafeMoveUpdatedComponent(WallAttractionDelta, UpdatedComponent->GetComponentQuat(), true, Hit);
		}
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick; // v = dx / dt
	}

	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector CastDelta = UpdatedComponent->GetRightVector() * CapR() * 2;
	FVector End = Safe_bWallRunIsRight ? Start + CastDelta : Start - CastDelta;
	auto Params = CrimsonCharacterOwner->GetIgnoreCharacterParams();
	FHitResult FloorHit, WallHit;
	GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);
	GetWorld()->LineTraceSingleByProfile(FloorHit, Start, Start + FVector::DownVector * (CapHH() + MinWallRunHeight * .5f), "BlockAll", Params);
	if (FloorHit.IsValidBlockingHit() || !WallHit.IsValidBlockingHit() || Velocity.SizeSquared2D() < pow(MinWallRunSpeed, 2))
	{
		SetMovementMode(MOVE_Falling);
	}
}

bool UCrimsonCharacterMovementComp::IsGettingInputRight()
{
	return CrimsonCharacterOwner->GetScaleValue() != 0.0f ? true : false;
}

void UCrimsonCharacterMovementComp::GetWallrunRight(bool& isWallruningRight)
{
	isWallruningRight = Safe_bWallRunIsRight;
	return;
}

#pragma region Helpers

bool UCrimsonCharacterMovementComp::IsServer()	const { return CharacterOwner->HasAuthority(); }
float UCrimsonCharacterMovementComp::CapR()		const { return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(); }
float UCrimsonCharacterMovementComp::CapHH()	const { return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(); }

#pragma endregion

#pragma region Interface

void UCrimsonCharacterMovementComp::SprintPressed()
{
	if (ToggleSprint)
		Safe_bWantsToSprint = !Safe_bWantsToSprint;
	else
		Safe_bWantsToSprint = true;
}

void UCrimsonCharacterMovementComp::SprintReleased()
{
	if(!ToggleSprint)
		Safe_bWantsToSprint = false;
}

void UCrimsonCharacterMovementComp::CrouchPressed()
{
	if (ToggleCrouch)
		bWantsToCrouch = !bWantsToCrouch;
	else 
		bWantsToCrouch = true;
}

void UCrimsonCharacterMovementComp::CrouchReleased()
{
	if (!ToggleCrouch)
		bWantsToCrouch = false;
}

void UCrimsonCharacterMovementComp::DashPressed()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - DashStartTime >= DashCooldownDuration)
		Safe_bWantsToDash = true;
	else
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_DashCooldown, this, &UCrimsonCharacterMovementComp::OnDashCooldownFinished, DashCooldownDuration - (CurrentTime - DashStartTime));
}

void UCrimsonCharacterMovementComp::DashReleased()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DashCooldown);
	Safe_bWantsToDash = false;
}

void UCrimsonCharacterMovementComp::AttackImpulse(float Force)
{

	Velocity = Force * UpdatedComponent->GetForwardVector().GetSafeNormal2D();
}


void UCrimsonCharacterMovementComp::SetToggleSprint(bool _toggle)
{
	ToggleSprint = _toggle;
}

void UCrimsonCharacterMovementComp::SetToggleCrouch(bool _toggle)
{
	ToggleCrouch = _toggle;
}

bool UCrimsonCharacterMovementComp::IsCustomMovementMode(ECustomMovementMode _inCustomMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == _inCustomMovementMode;
}

bool UCrimsonCharacterMovementComp::IsMovementMode(EMovementMode _inMovementMode) const
{
	return _inMovementMode == MovementMode;
}

#pragma endregion

#pragma region Replication


void UCrimsonCharacterMovementComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UCrimsonCharacterMovementComp, Proxy_bDash, COND_SkipOwner)

	DOREPLIFETIME_CONDITION(UCrimsonCharacterMovementComp, Proxy_bShortMantle, COND_SkipOwner)
	DOREPLIFETIME_CONDITION(UCrimsonCharacterMovementComp, Proxy_bTallMantle, COND_SkipOwner)
}

void UCrimsonCharacterMovementComp::OnRep_Dash()
{
	//CharacterOwner->PlayAnimMontage(DashMontage);
	DashStartDelegate.Broadcast();
}

void UCrimsonCharacterMovementComp::OnRep_ShortMantle()
{
	//CharacterOwner->PlayAnimMontage(ProxyShortMantleMontage);
}

void UCrimsonCharacterMovementComp::OnRep_TallMantle()
{
	//CharacterOwner->PlayAnimMontage(ProxyTallMantleMontage);
}

#pragma endregion