// Copyright (c) 2020 Tension Graphics AB


#include "CrimsonCameraManager.h"
#include "CrimsonCharacter.h"
#include "CrimsonCharacterMovementComp.h"
#include "Components/CapsuleComponent.h"

ACrimsonCameraManager::ACrimsonCameraManager()
{

}

void ACrimsonCameraManager::UpdateViewTarget(FTViewTarget& _outVT, float _deltaTime)
{
	Super::UpdateViewTarget(_outVT, _deltaTime);

	if (ACrimsonCharacter* CrimsonCharacter = Cast<ACrimsonCharacter>(GetOwningPlayerController()->GetPawn())) {
		UCrimsonCharacterMovementComp* CCMC = CrimsonCharacter->GetCrimsonCharacterMovement();
		FVector TargetCrouchOffest = FVector(0.0f, 0.0f,
				CCMC->GetCrouchedHalfHeight() - CrimsonCharacter->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 100.0f);
		FVector Offset = FMath::Lerp(FVector::ZeroVector, TargetCrouchOffest, FMath::Clamp(CrouchBlendTime / CrouchBlendDuration, 0.0f, 1.0f));

		if (CCMC->IsCrouching()) {
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime + _deltaTime, 0.0f, CrouchBlendDuration);
			Offset -= TargetCrouchOffest;
		}
		else {
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime - _deltaTime, 0.0f, CrouchBlendTime);
		}

		if (CCMC->IsMovingOnGround())
			_outVT.POV.Location += Offset;
	}
}
