// Copyright (c) 2020 Tension Graphics AB


#include "BloodHelper.h"

#include "Kismet/KismetSystemLibrary.h"

void ABloodHelper::SpawnBloodAtMeshUV( UMaterialInstanceDynamic* Material, FVector2D UVCoordinates )
{
    FHashedMaterialParameterInfo params;
	UTexture* material_texture;

    Material->GetTextureParameterValue(params, material_texture );

	UTexture2D*           BloodMaskTexture = UTexture2D::CreateTransient( BloodMaskSize, BloodMaskSize );
	FTexturePlatformData* PlatformData     = BloodMaskTexture->GetPlatformData();
	FTexture2DMipMap      FirstMip         = PlatformData->Mips[ 0 ];
	FByteBulkData*        ImageData        = &FirstMip.BulkData;
	uint8*                RawImageData     = ( uint8* )ImageData->Lock( LOCK_READ_WRITE );

	int ArraySize = BloodMaskSize * BloodMaskSize * 4;
    //FMemory::Memzero( RawImageData, ArraySize );

    int PixelX = FMath::Clamp( ( int )( UVCoordinates.X * BloodMaskSize ), 0, BloodMaskSize - 1 );
    int PixelY = FMath::Clamp( ( int )( UVCoordinates.Y * BloodMaskSize ), 0, BloodMaskSize - 1 );

    int PixelIndex = ( PixelY * BloodMaskSize + PixelX ) * 4;

    RawImageData[ PixelIndex + 3 ] = 255; // A

	//for ( auto i = 0; i < ArraySize; i += 4 )
	//{
	//	RawImageData[ i ] = 255;
	//}

	ImageData->Unlock();
	BloodMaskTexture->UpdateResource();

    //UKismetSystemLibrary::PrintString( GEngine->GetWorld(), FString::Printf( TEXT( "Blood: %f %f" ), UVCoordinates.X, UVCoordinates.Y ) );

	Material->SetTextureParameterValue( "BloodMask", BloodMaskTexture );
}
