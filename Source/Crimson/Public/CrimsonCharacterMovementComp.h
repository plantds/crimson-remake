// Copyright (c) 2020 Tension Graphics AB

#pragma once

#include "CoreMinimal.h"
#include "Crimson/Crimson.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "CrimsonCharacterMovementComp.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDashStartDelegate);

UENUM(BlueprintType)
enum ECustomMovementMode {
	CMOVE_None    UMETA(Hidden),
	CMOVE_Slide   UMETA(DisplayName = "Slide"),
	CMOVE_WallRun UMETA(DisplayName = "Wall Run"),
	CMOVE_MAX     UMETA(Hidden),
};

UCLASS()
class CRIMSON_API UCrimsonCharacterMovementComp : public UCharacterMovementComponent
{
	GENERATED_BODY()


	typedef UCharacterMovementComponent Super;

	class FSavedMove_Crimson : public FSavedMove_Character {
		typedef FSavedMove_Character Super;
	public:
		enum CompressedFlags
		{
			FLAG_Sprint = 0x10,
			FLAG_Dash = 0x20,
			FLAG_Custom_2 = 0x40,
			FLAG_Custom_3 = 0x80,
		};

		// Flags
		uint8 Saved_bWantsToSprint : 1;
		uint8 Saved_bWantsToDash : 1;
		uint8 Saved_bPressedCrimsonJump : 1;

		// Other Vars
		uint8 Saved_bHadAnimRootMotion : 1;
		uint8 Saved_bTransitionFinished : 1;
		uint8 Saved_bPrevWantsToCrouch : 1;
		uint8 Saved_bWallRunIsRight : 1;

		FSavedMove_Crimson();

		virtual bool CanCombineWith(const FSavedMovePtr& _newMove, ACharacter* _inCharacter, float _maxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* _c, float _inDeltaTime, FVector const& _newAccel, FNetworkPredictionData_Client_Character& _clientData) override;
		virtual void PrepMoveFor(ACharacter* _c) override;
	};

	class FNetworkPredictionData_Client_Crimson : public FNetworkPredictionData_Client_Character {
	public:
		FNetworkPredictionData_Client_Crimson(const UCharacterMovementComponent& _clientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};

public:
	// Parameters
	UPROPERTY(EditAnywhere) float MaxSprintSpeed = 950.0f;

	//JUMP
	UPROPERTY(EditAnywhere) float CoyoteJumpTime = 0.5f;
	float CoyoteJumpTimer = CoyoteJumpTime;
	bool HasJumped = false;

	// Slide
	UPROPERTY(EditAnywhere) float MinSlideSpeed = 750.0f;
	UPROPERTY(EditAnywhere) float MaxSlideSpeed = 400.0f;
	UPROPERTY(EditAnywhere) float SlideEnterImpulse = 700.0f;
	UPROPERTY(EditAnywhere) float SlideGravityForce = 4000.0f;
	UPROPERTY(EditAnywhere) float SlideFrictionFactor = 0.06f;
	UPROPERTY(EditAnywhere) float BrakingDecelerationSliding = 1000.0f;

	// Dash
	UPROPERTY(EditAnywhere) float DashCooldownDuration = 1.0f;
	UPROPERTY(EditAnywhere) float DashImpulse = 1500.0f;

	// Mantle
	UPROPERTY(EditAnywhere) float MantleMaxDistance = 200.0f;
	UPROPERTY(EditAnywhere) float MantleReachHeight = 50.0f;
	UPROPERTY(EditAnywhere) float MinMantleDepth = 30.0f;
	UPROPERTY(EditAnywhere) float MantleMinWallSteepnessAngle = 75.0f;
	UPROPERTY(EditAnywhere) float MantleMaxSurfaceAngle = 40.0f;
	UPROPERTY(EditAnywhere) float MantleMaxAlignmentAngle = 45.0f;

	// Wall Run
	UPROPERTY(EditAnywhere) float MinWallRunSpeed = 200.0f;
	UPROPERTY(EditAnywhere) float MaxWallRunSpeed = 800.0f;
	UPROPERTY(EditAnywhere) float MaxVerticalWallRunSpeed = 200.0f;
	UPROPERTY(EditAnywhere) float WallRunPullAwayAngle = 75.0f;
	UPROPERTY(EditAnywhere) float WallAttractionForce = 200.0f;
	UPROPERTY(EditAnywhere) float MinWallRunHeight = 50.0f;
	UPROPERTY(EditAnywhere) UCurveFloat* WallRunGravityScaleCurve;
	UPROPERTY(EditAnywhere) float WallJumpOffForce = 300.0f;
	UPROPERTY(EditAnywhere) float WallJumpInputOutForce = 300.0f;
	UPROPERTY(EditAnywhere) float DropSpeedMulti = 3.0f;


	// Toggles 
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool ToggleSprint = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool ToggleCrouch = false;

	UPROPERTY(Transient) ACrimsonCharacter* CrimsonCharacterOwner;

	// Flags
	bool Safe_bWantsToSprint;
	bool Safe_bWantsToDash;

	bool Safe_bHadAnimRootMotion;
	bool Safe_bPrevWantsToCrouch;

	float DashStartTime;
	FTimerHandle TimerHandle_DashCooldown;

	bool Safe_bTransitionFinished;
	TSharedPtr<FRootMotionSource_MoveToForce> TransitionRMS;
	FString TransitionName;
	UPROPERTY(Transient) UAnimMontage* TransitionQueuedMontage;
	float TransitionQueuedMontageSpeed;
	int TransitionRMS_ID;

	bool Safe_bWallRunIsRight;

	bool bHasStartedToMantle = false;
	bool bIsSliding = false;

	float AccumulatedClientLocationError = 0.0f;

	int TickCount = 0;
	int CorrectionCount = 0;
	int TotalBitsSent = 0;

	// Replication 
	UPROPERTY(ReplicatedUsing = OnRep_Dash) bool Proxy_bDash;
	UPROPERTY(ReplicatedUsing = OnRep_ShortMantle) bool Proxy_bShortMantle;
	UPROPERTY(ReplicatedUsing = OnRep_TallMantle) bool Proxy_bTallMantle;

public:
	UPROPERTY(BlueprintAssignable) FDashStartDelegate DashStartDelegate;

public:
	UCrimsonCharacterMovementComp();

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//ActorComp
protected:
	virtual void InitializeComponent() override;

	// Character Movement Component
public:
	bool CanCoyoteJump() const;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual bool IsMovingOnGround() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	virtual bool CanAttemptJump() const override;
	virtual bool DoJump(bool bReplayingMoves, float _deltatime) override;

	UFUNCTION(BlueprintCallable, BlueprintPure) void GetMaxSprintSpeed(float& speed);

protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, 
											UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, 
											FVector ServerGravityDirection) override;

public:
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

protected:
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual bool ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, 
										const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, 
										uint8 ClientMovementMode) override;

	FNetBitWriter CrimsonServerMoveBitWriter;

	virtual void CallServerMovePacked(const FSavedMove_Character* NewMove, const FSavedMove_Character* PendingMove, const FSavedMove_Character* OldMove) override;

	private:
		float inverseLerp(float a,float b, float t);

	// Slide
private:
	void EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);
	void ExitSlide();
	bool CanSlide() const;
	void PhysSlide(float deltaTime, int32 Iterations);
	UFUNCTION(BlueprintPure) void IsSliding(bool& Sliding);
	bool ForceSlide();
	UFUNCTION(BlueprintPure) void IsForcedSlide(bool& Slidind);
	bool FrocedSlide = false;

	// Dash
private:
	void OnDashCooldownFinished();

	bool CanDash() const;
	void PerformDash();

	bool hasDashed = false;

	// Mantle
private:
	bool TryMantle();
	FVector GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit, bool bTallMantle) const;
	UFUNCTION(BlueprintPure) void HasStartedToMantle(bool& Mantle);

	// Wall Run
private:
	bool TryWallRun();
	void PhysWallRun(float deltaTime, int32 Iterations);
	bool IsGettingInputRight();
	UFUNCTION(BlueprintCallable, BlueprintPure) void GetWallrunRight(bool& isWallruningRight);

	AActor* WallHitActor = nullptr;
	float wallrunStartTime = 0.0f;

	// Helpers
private:
	bool IsServer() const;
	float CapR() const;
	float CapHH() const;

	// Interface
public:
	UFUNCTION(BlueprintCallable) void SprintPressed();
	UFUNCTION(BlueprintCallable) void SprintReleased();

	UFUNCTION(BlueprintCallable) void CrouchPressed();
	UFUNCTION(BlueprintCallable) void CrouchReleased();

	UFUNCTION(BlueprintCallable) void DashPressed();
	UFUNCTION(BlueprintCallable) void DashReleased();

	UFUNCTION(BlueprintCallable) void AttackImpulse(float Force);

	UFUNCTION(BlueprintPure) bool IsCustomMovementMode(ECustomMovementMode _inCustomMovementMode) const;
	UFUNCTION(BlueprintPure) bool IsMovementMode(EMovementMode _inMovementMode) const;

	UFUNCTION(BlueprintPure) bool IsWallRunning() const { return IsCustomMovementMode(CMOVE_WallRun); }
	UFUNCTION(BlueprintPure) bool WallRunningIsRight() const { return Safe_bWallRunIsRight; }

	// Toggles
	UFUNCTION(BlueprintCallable) void SetToggleSprint(bool _toggle);
	UFUNCTION(BlueprintCallable) void SetToggleCrouch(bool _toggle);

	// Proxy Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
	UFUNCTION() void OnRep_Dash();
	UFUNCTION() void OnRep_ShortMantle();
	UFUNCTION() void OnRep_TallMantle();
};
