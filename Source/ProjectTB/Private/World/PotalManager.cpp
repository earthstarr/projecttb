// Fill out your copyright notice in the Description page of Project Settings.


#include "World/PotalManager.h"

#include "Engine/LevelStreamingDynamic.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/TBBattleHUD.h"
#include "World/WorldPlayerController.h"

void UPotalManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	PC = Cast<AWorldPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPotalManager 클래스. OnWorldBeginPlay 함수의 PC가 없습니다."));
		return;
	}
	
	
	//한틱 다음에 실행
	
	// 이동 비활성화
	if (PC->GetCharacter() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPotalManager 클래스. OnWorldBeginPlay 함수의 GetCharacter가 없습니다. 배틀 맵에서 실행됐습니다.")); 
		return;
	}
	else
	{
		PC->GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_None);
	}

	FString Path = TEXT("/Game/Blueprints2/DataTable/DT_Room.DT_Room");
	UDataTable* LoadedTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *Path));

	if (LoadedTable)
	{
		// 2. InitRoomHandle 구조체 채우기
		InitRoomHandle.DataTable = LoadedTable;
		InitRoomHandle.RowName = FName("Map_World");
		InitRoomLoad();
	}

}

void UPotalManager::InitRoomLoad()
{
	if (InitRoomHandle.IsNull()) { UE_LOG(LogTemp,Warning,TEXT("UPotalManager 클래스의 InitRoomLoad 함수의 InitRoomHandle이 없습니다.")); return; }
	OnLevelLoadStarted(InitRoomHandle);
}

void UPotalManager::OnLevelLoadStarted(const FDataTableRowHandle& SelectedRoomHandle)
{
	if (SelectedRoomHandle.IsNull()) { UE_LOG(LogTemp, Warning, TEXT("UPotalManager 클래스. OnLevelLoadStarted 함수의 SelectedRoomHandle이 없습니다.")); return;}
	if (PC == nullptr) { UE_LOG(LogTemp, Warning, TEXT("UPotalManager 클래스. OnLevelLoadStarted 함수의 PC가 없습니다.")); return; }
	
	UE_LOG(LogTemp, Log, TEXT("UPotalManager::OnLevelLoadStarted Debug RowName: %s"), *SelectedRoomHandle.RowName.ToString());
	CamManager = PC->PlayerCameraManager;
	
	// 시작 투명도, 끝 투명도, 페이드 시간, 페이드 색상, 오디오 페이드 여부, 페이드 이후 상태 유지 여부
	CamManager->StartCameraFade(0.f, 1.f, 0.5f, FLinearColor::Black, false, true);
	TransitionStartTime = GetWorld()->GetTimeSeconds();

	//todo 페이드 인 이후에 방 생성하도록 변경할 것
	
	// 방 생성
	RoomData = SelectedRoomHandle.GetRow<FRoomData>(TEXT("RoomData"));

	bool bSuccess = false;
	ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		GetWorld(), RoomData->NextLevel, RoomData->StartPosition, RoomData->StartRotation, bSuccess);

	if (StreamingLevel && bSuccess)
	{
		// 로딩이 완료되면 아래 OnLevelLoaded 함수가 실행되도록 예약
		//StreamingLevel->OnLevelLoaded.AddDynamic(this, &UPotalManager::OnLevelLoadFinished);	// 데이터상 로딩 완료 말고 객체 생성까지 마친 LevelShown으로 변경
		StreamingLevel->OnLevelShown.AddDynamic(this, &UPotalManager::OnLevelShown);
	}
	
	NextLevelInstance = StreamingLevel;
}

void UPotalManager::OnReturnToWorldLevel(const FDataTableRowHandle& ReturnRoomData)
{
	//지금은 테스트로 지정한 방으로 복귀
	OnLevelLoadStarted(ReturnRoomData);
}

void UPotalManager::OnLevelLoadFinished()
{
	//기존 이동 로직 Shown으로 변경
}

void UPotalManager::OnLevelShown()
{
	UE_LOG(LogTemp, Log, TEXT("UPotalManager::OnLevelLoadFinished Enter"));

	//진행중인 페이드 인 시간을 확인하고 잔여 시간이 있다면 대기
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float DeltaTime = CurrentTime - TransitionStartTime;
	
	if (DeltaTime < 0.5f)
	{
		float Delay = 0.5f - DeltaTime;
		FTimerHandle DelayTeleportTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(DelayTeleportTimerHandle, FTimerDelegate::CreateLambda([this]()
		{
			TeleportLevel();
			GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this]()
			{
				if (RoomData)
				{
					UE_LOG(LogTemp, Log, TEXT("UPotalManager::OnLevelLoadFinished Debug RoomType: %d"), static_cast<int32>(RoomData->RoomType));
					ActivateHUDMode(RoomData->RoomType);
				}
			}));
		}), Delay, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("딜레이가 필요 없습니다. 즉시 텔레포트 합니다."));
		TeleportLevel();
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this]()
		{
			if (RoomData)
			{
				UE_LOG(LogTemp, Log, TEXT("UPotalManager::OnLevelLoadFinished Debug RoomType: %d"), static_cast<int32>(RoomData->RoomType));
				ActivateHUDMode(RoomData->RoomType);
			}
		}));
	}
}

void UPotalManager::TeleportLevel()
{
	UE_LOG(LogTemp, Log, TEXT("UPotalManager::TeleportLevel Enter"));
		
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

void UPotalManager::StartBattleManagerSearch()
{
	if (!GetWorld())
	{
		return;
	}

	//이미 진행된 타이머는 정리
	StopBattleManagerSearch();
	BattleManagerSearchElapsed = 0.0f;

	//짧은 주기동안 배틀 매니저를 찾음
	GetWorld()->GetTimerManager().SetTimer(BattleManagerSearchTimerHandle,this,&UPotalManager::TryBattleManagerSearch,BattleManagerSearchInterval,true);
}

void UPotalManager::StopBattleManagerSearch()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(BattleManagerSearchTimerHandle);
	BattleManagerSearchElapsed = 0.0f;
}

void UPotalManager::TryBattleManagerSearch()
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
			UE_LOG(LogTemp, Log, TEXT("UPotalManager::TryBattleManagerSearch - BattleManager found"));
			// TBBattleHUD에 찾은 배틀 매니저를 넘겨줌.
			TBHUD->SetBattleManager(BattleManager);
			StopBattleManagerSearch();
			return;
		}
	}

	//탐색 가능 시간 초과
	if (BattleManagerSearchElapsed >= BattleManagerSearchTimeout)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPotalManager::TryBattleManagerSearch - Timed out finding BattleManager"));
		TBHUD->ExitBattleMode();
		StopBattleManagerSearch();
	}
}

void UPotalManager::ActivateHUDMode(const ERoomType RoomType)
{
	UE_LOG(LogTemp, Log, TEXT("UPotalManager::ActivateHUDMode Enter"));

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
		TBHUD->ExitBattleMode();
		PC->SetInputModeWorld();
		TBHUD->StartFadeIn();
		break;
	case ERoomType::Event:
		break;
	case ERoomType::Shop:
		break;
	default:
		break;
	}
}
