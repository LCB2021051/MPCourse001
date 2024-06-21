// Fill out your copyright notice in the Description page of Project Settings.


#include "MPSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

UMPSessionsSubsystem::UMPSessionsSubsystem():
    CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
    FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionComplete)),
    JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
    DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
    StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if(Subsystem)
    {
        SessionInterface = Subsystem->GetSessionInterface();
    }
}

void UMPSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
    if(!SessionInterface.IsValid())
    {
        return;
    }

    auto ExixtingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if(ExixtingSession!=nullptr)
    {
        bCreateSessionOnDestroy = true;
        LastNumPublicConnection = NumPublicConnections;
        LastMatchType = MatchType;
        DestroySession();
    }

    // Store a delegate in a FDelegateHandle so thet we can remove it from the delegate list
    CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

    LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
    LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
    LastSessionSettings->NumPublicConnections = NumPublicConnections; // Number of Connections for a particular sessions
    LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;
    LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    LastSessionSettings->BuildUniqueId = 1;

    const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

    if(!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

        // broadcast our own custom delegate 
        MPOnCreateSessionComplete.Broadcast(false);
    }
}

void UMPSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
    if(!SessionInterface.IsValid())
    {
        return;
    }

    FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

    // Session Search Settings
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = false;
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	// Player Controller for Unique Net id
    const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if(!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
    {
        // clear delegate list
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

        MPOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(),false);
    }
}

void UMPSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult &SessionResult)
{
    if(!SessionInterface.IsValid())
    {
        MPOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
        return;
    }

    JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
			
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    if(!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(),NAME_GameSession,SessionResult))
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        MPOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
    }
}

void UMPSessionsSubsystem::DestroySession()
{
    if(!SessionInterface.IsValid())
    {
        MPOnDestroySessionComplete.Broadcast(false);
        return;
    }

    DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
    if(!SessionInterface->DestroySession(NAME_GameSession))
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        MPOnDestroySessionComplete.Broadcast(false);
    }
}

void UMPSessionsSubsystem::StartSession()
{
    if(!SessionInterface.IsValid())
    {
        MPOnStartSessionComplete.Broadcast(false);
        return;
    }

    StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);
    if(!SessionInterface->StartSession(NAME_GameSession))
    {
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
        MPOnStartSessionComplete.Broadcast(false);
    }
}

void UMPSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if(SessionInterface)
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
    }

    MPOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMPSessionsSubsystem::OnFindSessionComplete(bool bWasSuccessful)
{
    if(SessionInterface)
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
    }
    if(LastSessionSearch->SearchResults.Num()<=0)
    {
        MPOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(),false);
        return;
    }
    MPOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults,bWasSuccessful);
}

void UMPSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if(SessionInterface)
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
    }
    MPOnJoinSessionComplete.Broadcast(Result);
}

void UMPSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    if(SessionInterface)
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
    }
    if(bWasSuccessful && bCreateSessionOnDestroy)
    {
        bCreateSessionOnDestroy = false;
        CreateSession(LastNumPublicConnection,LastMatchType);
    }
    MPOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UMPSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if(SessionInterface)
    {
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
    }
    MPOnStartSessionComplete.Broadcast(bWasSuccessful);
}

