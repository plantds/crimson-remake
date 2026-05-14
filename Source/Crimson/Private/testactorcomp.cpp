// Copyright (c) 2020 Tension Graphics AB


#include "testactorcomp.h"

// Sets default values for this component's properties
Utestactorcomp::Utestactorcomp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void Utestactorcomp::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void Utestactorcomp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

