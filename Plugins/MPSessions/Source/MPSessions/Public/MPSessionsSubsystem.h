// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "MPSessionsSubsystem.generated.h"

//
// Declaring some Custom Delegates for the Menu to bind Callbacks to
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMPOnCreateSessionComplete, bool, bWasSuccessful);
// why didn't we use Dynamic Multicast ?
DECLARE_MULTICAST_DELEGATE_TwoParams(FMPOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMPOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMPOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMPOnStartSessionComplete, bool, bWasSuccessful);

UCLASS()
class MPSESSIONS_API UMPSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMPSessionsSubsystem();

	//
	// To handle Session functionallity. the menu class will call these
	//
	void CreateSession(int32 NumPublicConnections,FString MatchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();

	//
	// Custom Delegates to bind Menu callbacks
	//
	FMPOnCreateSessionComplete MPOnCreateSessionComplete;
	FMPOnFindSessionsComplete MPOnFindSessionsComplete;
	FMPOnJoinSessionComplete MPOnJoinSessionComplete;
	FMPOnDestroySessionComplete MPOnDestroySessionComplete;
	FMPOnStartSessionComplete MPOnStartSessionComplete;

protected:

	//
	//	internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	// These don't need to be called outside this class.
	//
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	
	//
	// to add to the online Session Interface delegates list
	// we'll bind our multiplayerSessionSubsystem internal callbacks to these
	//
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;

	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;

	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	bool bCreateSessionOnDestroy{false};
	int32 LastNumPublicConnection;
	FString LastMatchType;
};
