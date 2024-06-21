// Copyright Epic Games, Inc. All Rights Reserved.

#include "MPCourse001Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"


//////////////////////////////////////////////////////////////////////////
// AMPCourse001Character

AMPCourse001Character::AMPCourse001Character():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this,&ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnJoinSessionComplete))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if(OnlineSubsystem)
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
/*
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				10.0f,
				FColor::Blue,
				FString::Printf(TEXT("Found Subystem %s"),*OnlineSubsystem->GetSubsystemName().ToString()) 
			);
		}
*/
	}


}

void AMPCourse001Character::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AMPCourse001Character::CreateGameSession()
{
	// executed when press 1 key
	if(!OnlineSessionInterface.IsValid())
	{
		return;
	}

	// checking for exixting session
	auto ExixtingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if(ExixtingSession!=nullptr)
	{
		OnlineSessionInterface->DestroySession(NAME_GameSession);
	}

	// Adding our delegate to the deligate list
	OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// calling create session function
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	SessionSettings->bIsLANMatch = false;
	SessionSettings->NumPublicConnections = 4;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;
	SessionSettings->Set(FName("MatchType"),FString("FreeForAll"),EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
}

void AMPCourse001Character::JoinGameSession()
{
	// find game sessions
	if(!OnlineSessionInterface.IsValid())
	{
		return;
	}

	// Adding Our Delegate to the Delegate list
	OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	// Session Search Settings
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 10000;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	// Player Controller for Unique Net id
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	OnlineSessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}

void AMPCourse001Character::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if( bWasSuccessful)
	{
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				10.0f,
				FColor::Green,
				FString::Printf(TEXT("Created Session: %s"),*SessionName.ToString()) 
			);
		}
	}
	else 
	{
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				10.0f,
				FColor::Red,
				FString(TEXT("failed to create session!"))
			);
		}
	}

	UWorld* World = GetWorld();
	if(World)
	{
		World->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"));
	}
}

void AMPCourse001Character::OnFindSessionsComplete(bool bWasSuccessful)
{	
	if(!OnlineSessionInterface.IsValid())
	{
		return;
	}

	for(auto Result:SessionSearch->SearchResults)
	{
		FString Id = Result.GetSessionIdStr();
		FString User = Result.Session.OwningUserName;
		FString MatchType;
		Result.Session.SessionSettings.Get(FName("MatchType"),MatchType);
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Cyan,
				FString::Printf(TEXT("Id: %s, User: %s"),*Id,*User)
			);
		}

		if(MatchType == FString("FreeForAll"))
		{
			if(GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Cyan,
					FString::Printf(TEXT("Joining Match Type: %s"),*MatchType)
				);
			}
			OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
			
			const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
			OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(),NAME_GameSession,Result);
		}
	}

}

void AMPCourse001Character::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if(!OnlineSessionInterface.IsValid())
	{
		return;
	}
	FString Address;
	if(OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession,Address))
	{
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Yellow,
				FString::Printf(TEXT("Connect String: %s"),*Address)
			);
		}
		APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
		if(PlayerController)
		{
			PlayerController->ClientTravel(Address,ETravelType::TRAVEL_Absolute);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMPCourse001Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMPCourse001Character::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMPCourse001Character::Look);
	}
}

void AMPCourse001Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMPCourse001Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}




