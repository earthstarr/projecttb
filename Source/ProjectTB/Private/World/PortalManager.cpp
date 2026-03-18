// Fill out your copyright notice in the Description page of Project Settings.


#include "World/PortalManager.h"

#include "TBGameInstance.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/TBBattleHUD.h"
#include "World/WorldPlayerController.h"
#include "World/DataStruct/PortalSpawnConfig.h"
#include "World/LevelActor/AnchorSet/PortalSpawnAnchorSet.h"
#include "World/LevelActor/Portal/PortalBase.h"


void UPortalManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	PC = Cast<AWorldPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPortalManager 클래스. OnWorldBeginPlay 함수의 PC가 없습니다."));
		return;
	}
	
	// 포탈 정보 캐시화
	UTBGameInstance* GI = Cast<UTBGameInstance>(InWorld.GetGameInstance());
	check(GI);

	CachePortalConfig = GI->PortalSpawnConfig.LoadSynchronous();
	if (ensureAlways(CachePortalConfig) == false)
	{
		return;
	}
	
	
	// 이동 비활성화
	if (PC->GetCharacter() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPortalManager 클래스. OnWorldBeginPlay 함수의 GetCharacter가 없습니다. 배틀 맵에서 실행됐습니다.")); 
		return;
	}
	else
	{
		PC->GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_None);
	}
	
	// Map_World로 이동.
	FString Path = TEXT("/Game/Blueprints2/DataTable/DT_Room.DT_Room");
	UDataTable* LoadedTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *Path));

	if (LoadedTable == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("UPortalManager::OnWorldBeginPlay의 LoadedTable가 비었습니다. 이동할 맵 데이터의 경로가 코드상의 Path와 동일한지 확인해주세요."))
		return;
	}
	
	// 맵이 이동할 수 있는 맵 선별 및 캐시화
	CachePortalRoomCandidates(LoadedTable);
	
	// 적 정보에 대한 데이터 테이블
	Path = TEXT("/Game/Blueprints2/DataTable/DT_EnemySetup.DT_EnemySetup");
	EnemyDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *Path));
	
	// InitRoomHandle 구조체 채우기
	InitRoomHandle.DataTable = LoadedTable;
	InitRoomHandle.RowName = FName("Map_World");
	InitRoomLoad();
}

void UPortalManager::InitRoomLoad()
{
	if (InitRoomHandle.IsNull()) { UE_LOG(LogTemp,Warning,TEXT("UPortalManager 클래스의 InitRoomLoad 함수의 InitRoomHandle이 없습니다.")); return; }
	OnLevelLoadStarted(InitRoomHandle);
}

void UPortalManager::OnLevelLoadStarted(const FDataTableRowHandle& SelectedRoomHandle)
{
	if (SelectedRoomHandle.IsNull()) { UE_LOG(LogTemp, Warning, TEXT("UPortalManager 클래스. OnLevelLoadStarted 함수의 SelectedRoomHandle이 없습니다.")); return;}
	if (PC == nullptr) { UE_LOG(LogTemp, Warning, TEXT("UPortalManager 클래스. OnLevelLoadStarted 함수의 PC가 없습니다.")); return; }
	
	UE_LOG(LogTemp, Log, TEXT("UPortalManager::OnLevelLoadStarted Debug RowName: %s"), *SelectedRoomHandle.RowName.ToString());
	CamManager = PC->PlayerCameraManager;
	
	// 시작 투명도, 끝 투명도, 페이드 시간, 페이드 색상, 오디오 페이드 여부, 페이드 이후 상태 유지 여부
	CamManager->StartCameraFade(0.f, 1.f, 0.5f, FLinearColor::Black, false, true);
	TransitionStartTime = GetWorld()->GetTimeSeconds();
	
	//페이드 인 기능. 이제는 안전하게 페이드 인 이후 방 로딩함.
	//CamManager->StartCameraFade(0.f, 1.f, 0.5f, FLinearColor::Black, false, true);
	//OnFadeInFinished(SelectedRoomHandle);

	CurrentPortalSpawnAnchorSet = nullptr;

	//페이드 인 이후 방 로딩
	PendingRoomHandle = SelectedRoomHandle;
	GetWorld()->GetTimerManager().ClearTimer(FadeInTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(FadeInTimerHandle, this, &UPortalManager::OnFadeInFinished, 0.5f, false);
}

void UPortalManager::OnReturnToWorldLevel(const FDataTableRowHandle& PostBattleRoomData)
{
	// 돌아갈 맵이 없다면 월드 맵으로
	if (PostBattleRoomData.IsNull())
	{
		InitRoomLoad();
		return;
	}
	OnLevelLoadStarted(PostBattleRoomData);
}

void UPortalManager::OnLevelLoadFinished()
{
	//호출 시점의 문제로 인해 기존 이동 로직 Shown으로 변경.
}

void UPortalManager::OnFadeInFinished()
{
	// 방 생성
	RoomData = PendingRoomHandle.GetRow<FRoomData>(TEXT("RoomData"));

	if (RoomData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UPortalManager::OnFadeInFinished 의 RoomData가 nullptr 입니다."));
		return;
	}
	
	bool bSuccess = false;
	ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		GetWorld(), RoomData->NextLevel, RoomData->StartPosition, RoomData->StartRotation, bSuccess);

	if (StreamingLevel && bSuccess)
	{
		// 로딩이 완료되면 아래 OnLevelLoaded 함수가 실행되도록 예약
		//StreamingLevel->OnLevelLoaded.AddDynamic(this, &UPortalManager::OnLevelLoadFinished);	// 데이터상 로딩 완료 말고 객체 생성까지 마친 LevelShown으로 변경
		StreamingLevel->OnLevelShown.AddDynamic(this, &UPortalManager::OnLevelShown);
	}
	
	NextLevelInstance = StreamingLevel;
	
}

void UPortalManager::OnLevelShown()
{
	UE_LOG(LogTemp, Log, TEXT("UPortalManager::OnLevelLoadFinished Enter"));

	TeleportLevel();
	
	const int32 PortalMoveCount = GetCurrentPortalMoveCount();
	
	// Boss 방은 맵에 수동 배치한 포탈을 사용하므로 자동 생성하지 않는다.
	if (RoomData->RoomType == ERoomType::Boss)
	{
	}
	// 보스룸 앞이 비전투 방이면 보스 대기룸 입구 포탈 하나만 생성
	else if (RoomData->RoomType != ERoomType::Battle && PortalMoveCount == 14)
	{
		bPendingPortalGeneration = true;
		bPortalGeneratedForCurrentRoom = false;
		MakeBossPreparationEntrancePortal();
	}
	// 3. 그 외 일반 월드/이벤트/상점 방이면: 기존 랜덤 포탈 생성
	else if (RoomData->RoomType != ERoomType::Battle)
	{
		bPendingPortalGeneration = true;
		bPortalGeneratedForCurrentRoom = false;
		TryGeneratePortalForCurrentRoom();
	}
	
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this]()
	{
		if (RoomData)
		{
			UE_LOG(LogTemp, Log, TEXT("UPortalManager::OnLevelLoadFinished Debug RoomType: %d"),
			       static_cast<int32>(RoomData->RoomType));
			ActivateHUDMode(RoomData->RoomType);
		}
	}));
}

void UPortalManager::TeleportLevel()
{
	UE_LOG(LogTemp, Log, TEXT("UPortalManager::TeleportLevel Enter"));
		
	//월드 캐릭터가 직접 이동
	if (PC)
	{
		APawn* PlayerPawn = PC->GetPawn();
		
		if (PlayerPawn)
		{
			// 데이터 테이블에 써져 있는 위치 값으로 순간이동
			PlayerPawn->SetActorLocationAndRotation( RoomData->StartPosition, RoomData->StartRotation);
			PC->SetControlRotation(FRotator(0,0,0));
			UE_LOG(LogTemp, Log, TEXT("텔레포트 완료"));
			PC->GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			
			// 이전 맵 언로드
			if (CurrentLevelInstance != nullptr)
			{
				CurrentLevelInstance->SetIsRequestingUnloadAndRemoval(true);
				UE_LOG(LogTemp, Log, TEXT("이전 레벨 언로드 요청됨"));
			}
			CurrentLevelInstance = NextLevelInstance;
			NextLevelInstance = nullptr;
		}
	}
}

void UPortalManager::StartBattleManagerSearch()
{
	if (!GetWorld())
	{
		return;
	}

	//이미 진행된 타이머는 정리
	StopBattleManagerSearch();
	BattleManagerSearchElapsed = 0.0f;

	//짧은 주기동안 배틀 매니저를 찾음
	GetWorld()->GetTimerManager().SetTimer(BattleManagerSearchTimerHandle,this,&UPortalManager::TryBattleManagerSearch,BattleManagerSearchInterval,true);
}

void UPortalManager::StopBattleManagerSearch()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(BattleManagerSearchTimerHandle);
	BattleManagerSearchElapsed = 0.0f;
}

void UPortalManager::TryBattleManagerSearch()
{
	if (!GetWorld() || PC == nullptr)
	{
		StopBattleManagerSearch();
		return;
	}

	ATBBattleHUD* TBHUD = Cast<ATBBattleHUD>(PC->GetHUD());
	if (TBHUD == nullptr)
	{
		StopBattleManagerSearch();
		return;
	}

	BattleManagerSearchElapsed += BattleManagerSearchInterval;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABattleManager::StaticClass(), FoundActors);

	if (!FoundActors.IsEmpty())
	{
		if (ABattleManager* BattleManager = Cast<ABattleManager>(FoundActors[0]))
		{
			UE_LOG(LogTemp, Log, TEXT("UPortalManager::TryBattleManagerSearch - BattleManager found"));
			// TBBattleHUD에 찾은 배틀 매니저를 넘겨줌.
			TBHUD->SetBattleManager(BattleManager);
			StopBattleManagerSearch();
			return;
		}
	}

	//탐색 가능 시간 초과
	if (BattleManagerSearchElapsed >= BattleManagerSearchTimeout)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPortalManager::TryBattleManagerSearch - Timed out finding BattleManager"));
		TBHUD->ExitBattleMode();
		StopBattleManagerSearch();
	}
}

void UPortalManager::ActivateHUDMode(const ERoomType RoomType)
{
	UE_LOG(LogTemp, Log, TEXT("UPortalManager::ActivateHUDMode Enter"));

	if (PC == nullptr)
	{
		return;
	}
	ATBBattleHUD* TBHUD = Cast<ATBBattleHUD>(PC->GetHUD());
	if (TBHUD == nullptr) return;
	
	
	switch (RoomType)
	{
	case ERoomType::Battle:
		TBHUD->EnterBattleMode();
		PC->SetInputModeBattle();
		StartBattleManagerSearch();
		break;
	case ERoomType::World:
		PC->SetInputModeWorld();
		TBHUD->StartFadeIn();
		break;
	case ERoomType::Event:
		PC->SetInputModeWorld();
		TBHUD->StartFadeIn();
		//이벤트 맵 입장
		//월드랑 동일
		//
		break;
	case ERoomType::Shop:
		PC->SetInputModeWorld();
		TBHUD->StartFadeIn();
		break;
	case ERoomType::Boss:
		PC->SetInputModeWorld();
		TBHUD->StartFadeIn();
	default:
		break;
	}
}

bool UPortalManager::GetRandomEnemyGroup(EBattleType BattleType, FEnemyGroupData& OutData)
{
	OutData = FEnemyGroupData();

	UDataTable* LoadedEnemyTable = EnemyDataTable.LoadSynchronous();
	if (LoadedEnemyTable == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPortalManager::GetRandomEnemyGroup - EnemyDataTable is nullptr"));
		return false;
	}

	TArray<const FEnemyGroupData*> Candidates;
	static const FString ContextString(TEXT("GetRandomEnemyGroup"));

	for (const FName& RowName : LoadedEnemyTable->GetRowNames())
	{
		const FEnemyGroupData* EnemyGroupRow =
			LoadedEnemyTable->FindRow<FEnemyGroupData>(RowName, ContextString);

		if (EnemyGroupRow == nullptr)
		{
			continue;
		}

		if (EnemyGroupRow->BattleType != BattleType)
		{
			continue;
		}

		Candidates.Add(EnemyGroupRow);
	}

	if (Candidates.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPortalManager::GetRandomEnemyGroup - No candidates for BattleType"));
		return false;
	}

	const int32 RandomIndex = FMath::RandRange(0, Candidates.Num() - 1);
	OutData = *Candidates[RandomIndex];

	return true;
}

void UPortalManager::CachePortalRoomCandidates(UDataTable* RoomTable)
{
	BattleRoomCandidates.Reset();
	EventRoomCandidates.Reset();
	ShopRoomCandidates.Reset();
	BossPreparationRoomHandle = FDataTableRowHandle();
	
	if (RoomTable == nullptr)
	{
		return;
	}

	static const FString ContextString(TEXT("CachePortalRoomCandidates"));

	for (const FName& RowName : RoomTable->GetRowNames())
	{
		const FRoomData* RoomRow = RoomTable->FindRow<FRoomData>(RowName, ContextString);
		if (RoomRow == nullptr)
		{
			continue;
		}

		FDataTableRowHandle NewHandle;
		NewHandle.DataTable = RoomTable;
		NewHandle.RowName = RowName;

		switch (RoomRow->RoomType)
		{
		case ERoomType::Battle:
			BattleRoomCandidates.Add(NewHandle);
			break;
		case ERoomType::Event:
			EventRoomCandidates.Add(NewHandle);
			break;
			
			//이벤트 포탈은 상점도 갈 수 있도록 설계
		case ERoomType::Shop:
			ShopRoomCandidates.Add(NewHandle);
			EventRoomCandidates.Add(NewHandle);
			break;
			
		case ERoomType::Boss:
			// Boss 타입은 랜덤 후보에는 넣지 않고, 강제 이동용으로만 보관
			BossPreparationRoomHandle = NewHandle;
		default:
			break;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("BattleRoomCandidates: %d"), BattleRoomCandidates.Num());
	UE_LOG(LogTemp, Log, TEXT("EventRoomCandidates: %d"), EventRoomCandidates.Num());
	UE_LOG(LogTemp, Log, TEXT("ShopRoomCandidates: %d"), ShopRoomCandidates.Num());
}

bool UPortalManager::GetRandomRoomHandleByPortalType(EEventRoomType PortalType, FDataTableRowHandle& OutHandle) const
{
	const TArray<FDataTableRowHandle>* Candidates = nullptr;

	switch (PortalType)
	{
	case EEventRoomType::Battle:
		Candidates = &BattleRoomCandidates;
		break;
	case EEventRoomType::Event:
		Candidates = &EventRoomCandidates;
		break;
	case EEventRoomType::Shop:
		Candidates = &ShopRoomCandidates;
		break;
	default:
		return false;
	}

	if (Candidates == nullptr || Candidates->IsEmpty())
	{
		return false;
	}

	// 현재 층 수에 따라 다른 난이도 가져오기
	return GetRoomHandleBasedOnPortalMovement(Candidates, OutHandle);
}

void UPortalManager::MakeNewPortal()
{
	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal Enter - CurrentLevelInstance=%s CachePortalConfig=%s"),
		CurrentLevelInstance ? TEXT("Valid") : TEXT("Null"),
		CachePortalConfig ? TEXT("Valid") : TEXT("Null"));

	if (CurrentLevelInstance == nullptr || CachePortalConfig == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal EarlyReturn - CurrentLevelInstance or CachePortalConfig is null"));
		return;
	}

	if (!bPendingPortalGeneration || bPortalGeneratedForCurrentRoom)
	{
		return;
	}
	
	ULevel* LoadedLevel = CurrentLevelInstance->GetLoadedLevel();
	if (LoadedLevel == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal EarlyReturn - LoadedLevel is null"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal LoadedLevel=%s Actors.Num=%d"),
		*LoadedLevel->GetName(), LoadedLevel->Actors.Num());

	// 로드된 맵에서 포탈이 스폰 될 수 있는 지점 확보
	TArray<ATargetPoint*> SpawnPoints;
	CollectPortalSpawnPoints(LoadedLevel, SpawnPoints);

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal SpawnPoints.Num=%d"), SpawnPoints.Num());

	if (SpawnPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal EarlyReturn - SpawnPoints is empty"));
		return;
	}

	// 가능한 지점 안에서 최소 1개  ~ n 개 지점에 생성
	const int32 PortalCount = FMath::RandRange(1, SpawnPoints.Num());
	TArray<ATargetPoint*> AvailablePoints = SpawnPoints;

	bool bShopPortalSpawned = false;

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal PortalCount=%d"), PortalCount);

	int32 SpawnedPortalCount = 0;
	
	for (int32 Index = 0; Index < PortalCount && !AvailablePoints.IsEmpty(); ++Index)
	{
		// 가능한 지점에서 한 곳 뽑아서 스폰 포인트로 잡고 제거
		const int32 PointIndex = FMath::RandRange(0, AvailablePoints.Num() - 1);
		ATargetPoint* SpawnPoint = AvailablePoints[PointIndex];
		AvailablePoints.RemoveAtSwap(PointIndex);

		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal Loop=%d SelectedSpawnPoint=%s RemainingPoints=%d"),
			Index, SpawnPoint ? *SpawnPoint->GetName() : TEXT("Null"), AvailablePoints.Num());

		TArray<EEventRoomType> AvailablePortalTypes;

		// 이번에 생성할 수 있는 포탈의 종류. 상점은 최대 1번만 뽑힐 수 있도록 가드 변수에 따라 제어됨.
		if (CanSpawnPortalType(EEventRoomType::Battle))
		{
			AvailablePortalTypes.Add(EEventRoomType::Battle);
		}

		if (CanSpawnPortalType(EEventRoomType::Event))
		{
			AvailablePortalTypes.Add(EEventRoomType::Event);
		}

		if (!bShopPortalSpawned && CanSpawnPortalType(EEventRoomType::Shop))
		{
			AvailablePortalTypes.Add(EEventRoomType::Shop);
		}

		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal AvailablePortalTypes.Num=%d bShopPortalSpawned=%s"),
			AvailablePortalTypes.Num(), bShopPortalSpawned ? TEXT("true") : TEXT("false"));
		
		if (AvailablePortalTypes.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal Break - AvailablePortalTypes is empty"));
			break;
		}
		
		// AvailablePortalTypes
		bool bSpawnedAtThisPoint = false;
		
		// 가능한 타입 넣고 생성 시도. 실패하면 다른 타입 넣고 재시도.
		while (!AvailablePortalTypes.IsEmpty())
		{
			const int32 TypeIndex = FMath::RandRange(0, AvailablePortalTypes.Num() - 1);
			const EEventRoomType ChosenPortalType = AvailablePortalTypes[TypeIndex];
			AvailablePortalTypes.RemoveAtSwap(TypeIndex);

			UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal TrySpawn - SpawnPoint=%s PortalType=%d RemainingTypeOptions=%d"),
				SpawnPoint ? *SpawnPoint->GetName() : TEXT("Null"), static_cast<int32>(ChosenPortalType), AvailablePortalTypes.Num());

			if (SpawnPortalAtPoint(SpawnPoint, LoadedLevel, ChosenPortalType))
			{
				if (ChosenPortalType == EEventRoomType::Shop)
				{
					bShopPortalSpawned = true;
				}
				
				++SpawnedPortalCount;
				bSpawnedAtThisPoint = true;
				UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] MakeNewPortal SpawnSuccess - SpawnPoint=%s PortalType=%d"),
					SpawnPoint ? *SpawnPoint->GetName() : TEXT("Null"), static_cast<int32>(ChosenPortalType));
				break;
			}
		}

		if (!bSpawnedAtThisPoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("UPortalManager::MakeNewPortal - Failed to spawn portal at selected point."));
		}
	}
	
	if (SpawnedPortalCount > 0)
	{
		bPortalGeneratedForCurrentRoom = true;
		bPendingPortalGeneration = false;
	}
}

void UPortalManager::CollectPortalSpawnPoints(ULevel* LoadedLevel, TArray<ATargetPoint*>& OutSpawnPoints) const
{
	OutSpawnPoints.Reset();

	if (LoadedLevel == nullptr)
	{
		return;
	}

	APortalSpawnAnchorSet* AnchorSet = CurrentPortalSpawnAnchorSet.Get();
	if (!IsValid(AnchorSet))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] CollectPortalSpawnPoints - CurrentPortalSpawnAnchorSet is invalid."));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] CollectPortalSpawnPoints - CurrentAnchorSet=%s LoadedLevel=%s"),
		*AnchorSet->GetName(),
		*LoadedLevel->GetName());

	AnchorSet->GetValidPortalSpawnPoints(OutSpawnPoints);

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] CollectPortalSpawnPoints - AnchorSet=%s SpawnPoints=%d"),
		*AnchorSet->GetName(), OutSpawnPoints.Num());
}

bool UPortalManager::SpawnPortalAtPoint(ATargetPoint* SpawnPoint, ULevel* LoadedLevel, EEventRoomType PortalType)
{
	if (SpawnPoint == nullptr || LoadedLevel == nullptr || CachePortalConfig == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] SpawnPortalAtPoint EarlyReturn - SpawnPoint=%s LoadedLevel=%s CachePortalConfig=%s"),
			SpawnPoint ? TEXT("Valid") : TEXT("Null"),
			LoadedLevel ? TEXT("Valid") : TEXT("Null"),
			CachePortalConfig ? TEXT("Valid") : TEXT("Null"));
		return false;
	}
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] SpawnPortalAtPoint EarlyReturn - World is null"));
		return false;
	}

	FDataTableRowHandle SelectedHandle;
	if (!GetRandomRoomHandleByPortalType(PortalType, SelectedHandle))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] SpawnPortalAtPoint EarlyReturn - Failed to get room handle for PortalType=%d"),
			static_cast<int32>(PortalType));
		return false;
	}

	TSoftClassPtr<APortalBase> PortalSoftClass;
	switch (PortalType)
	{
	case EEventRoomType::Battle:
		PortalSoftClass = CachePortalConfig->BattlePortalClass;
		break;
	case EEventRoomType::Event:
		PortalSoftClass = CachePortalConfig->EventPortalClass;
		break;
	case EEventRoomType::Shop:
		PortalSoftClass = CachePortalConfig->ShopPortalClass;
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] SpawnPortalAtPoint EarlyReturn - Unknown PortalType=%d"),
			static_cast<int32>(PortalType));
		return false;
	}

	TSubclassOf<APortalBase> PortalClass = PortalSoftClass.LoadSynchronous();
	if (PortalClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] SpawnPortalAtPoint EarlyReturn - PortalClass load failed for PortalType=%d"),
			static_cast<int32>(PortalType));
		return false;
	}
	
	
	const FTransform SpawnTransform = SpawnPoint->GetActorTransform();
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.OverrideLevel = LoadedLevel;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bDeferConstruction = true;	// begin play 전에 PreInitializeComponents 이전에 값을 넣고 초기화를 시킬 예정

	APortalBase* SpawnedPortal = World->SpawnActor<APortalBase>(PortalClass.Get(), SpawnTransform, SpawnParams);
	

	if (SpawnedPortal == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] SpawnPortalAtPoint EarlyReturn - SpawnActor failed for PortalType=%d SpawnPoint=%s"),
			static_cast<int32>(PortalType), *SpawnPoint->GetName());
		return false;
	}

	SpawnedPortal->InitializePortal(SelectedHandle);
	SpawnedPortal->FinishSpawning(SpawnTransform);
	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] SpawnPortalAtPoint Success - PortalType=%d SpawnPoint=%s RowName=%s"),
		static_cast<int32>(PortalType), *SpawnPoint->GetName(), *SelectedHandle.RowName.ToString());
	return true;
}

bool UPortalManager::GetRoomHandleBasedOnPortalMovement(const TArray<FDataTableRowHandle>* Candidates, FDataTableRowHandle& OutHandle) const
{
	TArray<FDataTableRowHandle> FilteredCandidates;
	FilterCandidatesByPortalMoveCount(Candidates, FilteredCandidates);
	
	if (FilteredCandidates.IsEmpty())
	{
		int32 PortalMoveCount = 0;
		if (const UTBGameInstance* GI = Cast<UTBGameInstance>(GetWorld()->GetGameInstance()))
		{
			PortalMoveCount = GI->GetPortalMoveCount();
		}

		UE_LOG(LogTemp, Warning,
			TEXT("UPortalManager::GetRoomHandleBasedOnPortalMovement - No candidates at PortalMoveCount %d"),
			PortalMoveCount);
		return false;
	}

	const int32 RandomIndex = FMath::RandRange(0, FilteredCandidates.Num() - 1);
	OutHandle = FilteredCandidates[RandomIndex];
	return true;
}

bool UPortalManager::CanSpawnPortalType(EEventRoomType PortalType) const
{
	const TArray<FDataTableRowHandle>* Candidates = nullptr;

	switch (PortalType)
	{
	case EEventRoomType::Battle:
		Candidates = &BattleRoomCandidates;
		break;
	case EEventRoomType::Event:
		Candidates = &EventRoomCandidates;
		break;
	case EEventRoomType::Shop:
		Candidates = &ShopRoomCandidates;
		break;
	default:
		return false;
	}

	TArray<FDataTableRowHandle> FilteredCandidates;
	FilterCandidatesByPortalMoveCount(Candidates, FilteredCandidates);

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] CanSpawnPortalType PortalType=%d Candidates=%d Filtered=%d"),
		static_cast<int32>(PortalType),
		Candidates ? Candidates->Num() : 0,
		FilteredCandidates.Num());

	return !FilteredCandidates.IsEmpty();
}

void UPortalManager::FilterCandidatesByPortalMoveCount(const TArray<FDataTableRowHandle>* Candidates,
	TArray<FDataTableRowHandle>& OutFilteredCandidates) const
{
	OutFilteredCandidates.Reset();

	if (Candidates == nullptr || Candidates->IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] FilterCandidatesByPortalMoveCount EarlyReturn - Candidates is null or empty"));
		return;
	}

	// 현재 층 수 (이동 하기 전에 포탈에 닿아서 1 증가된 상태)
	int32 PortalMoveCount = 0;
	if (const UTBGameInstance* GI = Cast<UTBGameInstance>(GetWorld()->GetGameInstance()))
	{
		PortalMoveCount = GI->GetPortalMoveCount();
	}

	OutFilteredCandidates.Reserve(Candidates->Num());	// 크기 미리 확보

	static const FString ContextString(TEXT("FilterCandidatesByPortalMoveCount"));

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] FilterCandidatesByPortalMoveCount Start - PortalMoveCount=%d Candidates=%d"),
		PortalMoveCount, Candidates->Num());

	for (const FDataTableRowHandle& Candidate : *Candidates)
	{
		const FRoomData* RoomRow = Candidate.GetRow<FRoomData>(ContextString);
		if (RoomRow == nullptr)
		{
			continue;
		}

		if (PortalMoveCount < RoomRow->MinPortalMoveCount)
		{
			continue;
		}

		if (RoomRow->MaxPortalMoveCount >= 0 && PortalMoveCount > RoomRow->MaxPortalMoveCount)
		{
			continue;
		}

		OutFilteredCandidates.Add(Candidate);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] FilterCandidatesByPortalMoveCount Result - Filtered=%d"),
		OutFilteredCandidates.Num());
}

void UPortalManager::RegisterPortalSpawnAnchorSet(APortalSpawnAnchorSet* AnchorSet)
{
	if (AnchorSet == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] RegisterPortalSpawnAnchorSet - AnchorSet is null"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] RegisterPortalSpawnAnchorSet - Name=%s Level=%s Pending=%s Generated=%s BeforeCount=%d"),
		*AnchorSet->GetName(),
		AnchorSet->GetLevel() ? *AnchorSet->GetLevel()->GetName() : TEXT("Null"),
		bPendingPortalGeneration ? TEXT("true") : TEXT("false"),
		bPortalGeneratedForCurrentRoom ? TEXT("true") : TEXT("false"),
		RegisteredPortalSpawnAnchorSets.Num());

	RegisteredPortalSpawnAnchorSets.AddUnique(AnchorSet);
	CurrentPortalSpawnAnchorSet = AnchorSet;

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] RegisterPortalSpawnAnchorSet - AfterCount=%d"),
		RegisteredPortalSpawnAnchorSets.Num());

	if (bPendingPortalGeneration && !bPortalGeneratedForCurrentRoom)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] RegisterPortalSpawnAnchorSet - Retry MakeNewPortal"));
		TryGeneratePortalForCurrentRoom();
	}
}

void UPortalManager::UnregisterPortalSpawnAnchorSet(APortalSpawnAnchorSet* AnchorSet)
{
	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] UnregisterPortalSpawnAnchorSet - Name=%s BeforeCount=%d"),
		AnchorSet ? *AnchorSet->GetName() : TEXT("Null"),
		RegisteredPortalSpawnAnchorSets.Num());
	
	RegisteredPortalSpawnAnchorSets.Remove(AnchorSet);
	
	if (CurrentPortalSpawnAnchorSet.Get() == AnchorSet)
	{
		CurrentPortalSpawnAnchorSet = nullptr;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] UnregisterPortalSpawnAnchorSet - AfterCount=%d"),
	RegisteredPortalSpawnAnchorSets.Num());
	
}

void UPortalManager::TryGeneratePortalForCurrentRoom()
{
	if (RoomData == nullptr)
	{
		return;
	}

	const int32 PortalMoveCount = GetCurrentPortalMoveCount();

	// Boss 방은 수동 배치한 포탈만 사용
	if (RoomData->RoomType == ERoomType::Boss)
	{
		return;
	}

	// 14층 비전투 방이면 보스 대기룸 입구 포탈 하나만 생성
	if (RoomData->RoomType != ERoomType::Battle && PortalMoveCount == 14)
	{
		MakeBossPreparationEntrancePortal();
		return;
	}

	// 그 외 일반 월드/이벤트/상점 방은 기존 랜덤 포탈 생성
	if (RoomData->RoomType != ERoomType::Battle)
	{
		MakeNewPortal();
	}
}

bool UPortalManager::SpawnFixedPortalAtPoint(ATargetPoint* SpawnPoint, ULevel* LoadedLevel, 
                                             const TSoftClassPtr<APortalBase>& PortalSoftClass, const FDataTableRowHandle& FixedHandle)
{
	// 목적지 handle, 포탈 클래스, 스폰 위치 확인
	if (SpawnPoint == nullptr || LoadedLevel == nullptr || FixedHandle.IsNull() || PortalSoftClass.IsNull())
	{
		return false;
	}

	TSubclassOf<APortalBase> PortalClass = PortalSoftClass.LoadSynchronous();
	if (PortalClass == nullptr)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.OverrideLevel = LoadedLevel;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bDeferConstruction = true;

	const FTransform SpawnTransform = SpawnPoint->GetActorTransform();

	APortalBase* SpawnedPortal = GetWorld()->SpawnActor<APortalBase>(PortalClass.Get(), SpawnTransform, SpawnParams);
	if (SpawnedPortal == nullptr)
	{
		return false;
	}

	SpawnedPortal->InitializePortal(FixedHandle);
	SpawnedPortal->FinishSpawning(SpawnTransform);
	return true;
}


void UPortalManager::MakeBossPreparationEntrancePortal()
{
	// 보스 대기룸 row와 전용 포탈 클래스가 있어야 한다.
	if (BossPreparationRoomHandle.IsNull() || CachePortalConfig == nullptr)
	{
		return;
	}

	ULevel* LoadedLevel = CurrentLevelInstance ? CurrentLevelInstance->GetLoadedLevel() : nullptr;
	if (LoadedLevel == nullptr)
	{
		return;
	}

	TArray<ATargetPoint*> SpawnPoints;
	CollectPortalSpawnPoints(LoadedLevel, SpawnPoints);
	if (SpawnPoints.IsEmpty())
	{
		return;
	}

	const int32 PointIndex = FMath::RandRange(0, SpawnPoints.Num() - 1);

	// 14층 비전투 방에서는 BossPreparationPortal을 하나만 생성한다.
	// PortalMoveCount를 유지한 채 보스 대기룸으로 이동시키는 용도. 전투를 치룬 방이랑의 격차를 만들지 않기 위한 장치
	const bool bSpawned = SpawnFixedPortalAtPoint(
		SpawnPoints[PointIndex],
		LoadedLevel,
		CachePortalConfig->BossPreparationPortalClass,
		BossPreparationRoomHandle);

	// 실제 생성에 성공했을 때만 완료 처리
	if (bSpawned)
	{
		bPortalGeneratedForCurrentRoom = true;
		bPendingPortalGeneration = false;
	}
}

int32 UPortalManager::GetCurrentPortalMoveCount() const
{
	if (const UTBGameInstance* GI = Cast<UTBGameInstance>(GetWorld()->GetGameInstance()))
	{
		return GI->GetPortalMoveCount();
	}
	return 0;
}

void UPortalManager::SetBattleTransitionData(FBattleTransitionData& InBattleTransitionData)
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetWorld()->GetGameInstance());
	if (GI == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UPortalManager::SetBattleTransitionData GI is Nullptr"));
		return;
	}
	
	GI->BattleTransitionData = InBattleTransitionData;
}
