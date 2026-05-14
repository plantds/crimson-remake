// Copyright (c) 2020 Tension Graphics AB

#pragma once

#include "CoreMinimal.h"
#include "Crimson/Crimson.h"
#include "GameFramework/Character.h"
#include "CrimsonCharacterMovementComp.h"

#include "CrimsonCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS(Config = GAME)
class ACrimsonCharacter : public ACharacter
{
	GENERATED_BODY()


	typedef ACharacter Super;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement) UCrimsonCharacterMovementComp* CrimsonCharacterMovementComponent;
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true")) USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true")) UCameraComponent* FollowCamera;
	float InputScaleValue = 0.0f;
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Input) float TurnRateGamepad;

public:
	bool bPressedCrimsonJump;

public:
	ACrimsonCharacter(const FObjectInitializer& _objectInitializer);

	virtual void Jump() override;
	virtual void StopJumping() override;
	virtual void BeginPlay() override;
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce /*=false*/) override;
	// Input
private:
	void MoveForward(float _value);
	void MoveRight(float _value);
	void TurnAtRate(float _rate);
	void LookUpAtRate(float _rate);
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetScaleValue() const { return InputScaleValue; }

	UFUNCTION(BlueprintPure) FORCEINLINE UCrimsonCharacterMovementComp* GetCrimsonCharacterMovement() const { return CrimsonCharacterMovementComponent; }

	FCollisionQueryParams GetIgnoreCharacterParams() const;
};
