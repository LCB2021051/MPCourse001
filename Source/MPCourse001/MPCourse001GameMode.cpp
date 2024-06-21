// Copyright Epic Games, Inc. All Rights Reserved.

#include "MPCourse001GameMode.h"
#include "MPCourse001Character.h"
#include "UObject/ConstructorHelpers.h"

AMPCourse001GameMode::AMPCourse001GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
