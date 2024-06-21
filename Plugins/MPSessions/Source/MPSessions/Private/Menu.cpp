// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MPSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"


void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
    PathToLobby = FString::Printf(TEXT("%s?listen"),*LobbyPath);
    NumPublicConnections = NumberOfPublicConnections;
    MatchType = TypeOfMatch;
    AddToViewport();
    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);

    UWorld* World = GetWorld();
    if(World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if(PlayerController)
        {
            FInputModeUIOnly InputModeData;
            InputModeData.SetWidgetToFocus(TakeWidget());
            InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(true);
        }
    }

    UGameInstance* GameInstance = GetGameInstance();
    if(GameInstance)
    {
        MPSessionSubsystem = GameInstance->GetSubsystem<UMPSessionsSubsystem>();
    }

    if(MPSessionSubsystem)
    {
        MPSessionSubsystem->MPOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
        MPSessionSubsystem->MPOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
        MPSessionSubsystem->MPOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MPSessionSubsystem->MPOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
        MPSessionSubsystem->MPOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
    }
}

bool UMenu::Initialize()
{
    if(!Super::Initialize())
    {
        return false;
    }
    if(HostButton)
    {
        HostButton->OnClicked.AddDynamic(this,&ThisClass::HostButtonClicked);
    }
    if(JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
    }
    return true;
}

void UMenu::NativeDestruct()
{
    MenuTearDown();
 
	Super::NativeDestruct();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
    if(bWasSuccessful)
    {
        if(GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Green,
                FString(TEXT("Creating Session Was Successfully!"))
            );
        }
        UWorld* World = GetWorld();
        if(World)
        {
            World->ServerTravel(FString(PathToLobby));
        }
    }
    else
    {
        if(GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Red,
                FString(TEXT("Creating Session Was UnSuccessfully!"))
            );
        }
        HostButton->SetIsEnabled(true);
    }
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
    if(MPSessionSubsystem == nullptr)
    {
        return;
    }

    for (auto Result: SessionResults)
    {
		FString CurrMatchType;
		Result.Session.SessionSettings.Get(FName("MatchType"),CurrMatchType);

        if(CurrMatchType == MatchType)
		{
			MPSessionSubsystem->JoinSession(Result);
            return;
		}
    }
    if(!bWasSuccessful || SessionResults.Num() == 0)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if(Subsystem)
    {
        IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
        if(SessionInterface.IsValid())
        {
            FString Address;
            SessionInterface->GetResolvedConnectString(NAME_GameSession,Address);
           
            APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
            if(PlayerController)
            {
                PlayerController->ClientTravel(Address,ETravelType::TRAVEL_Absolute);
            }
            
        }
    }
    if(Result != EOnJoinSessionCompleteResult::Success)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
}

void UMenu::HostButtonClicked()
{
    HostButton->SetIsEnabled(false);
    if(MPSessionSubsystem)
    {
        MPSessionSubsystem->CreateSession(NumPublicConnections,MatchType);
        
    }

}

void UMenu::JoinButtonClicked()
{
    JoinButton->SetIsEnabled(false);
    if(MPSessionSubsystem)
    {
        MPSessionSubsystem->FindSessions(10000);
    }
}

void UMenu::MenuTearDown()
{
    RemoveFromParent();
    UWorld* World = GetWorld();
    if(World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if(PlayerController)
        {   
            FInputModeGameOnly InputModeData;
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(false);
        }
    }
}
