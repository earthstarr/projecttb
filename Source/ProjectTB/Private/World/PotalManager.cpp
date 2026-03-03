// Fill out your copyright notice in the Description page of Project Settings.


#include "World/PotalManager.h"

#include "Engine/LevelStreamingDynamic.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/DataStruct/RoomDataStruct.h"

void UPotalManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	PC = GetWorld()->GetFirstPlayerController();
	if (PC == nullptr) { UE_LOG(LogTemp, Warning, TEXT("UPotalManager 클래스. OnWorldBeginPlay 함수의 PC가 없습니다.")); return; }
	
	// 이동 비활성화
	PC->GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_None);
	
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
	
	CamManager = PC->PlayerCameraManager;
	
	// 시작 투명도, 끝 투명도, 페이드 시간, 페이드 색상, 오디오 페이드 여부, 페이드 이후 상태 유지 여부
	CamManager->StartCameraFade(0.f, 1.f, 0.5f, FLinearColor::Black, false, true);
	TransitionStartTime = GetWorld()->GetTimeSeconds();

	// 방 생성
	RoomData = SelectedRoomHandle.GetRow<FRoomData>(TEXT("RoomData"));

	bool bSuccess = false;
	ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		GetWorld(), RoomData->NextLevel, RoomData->StartPosition, RoomData->StartRotation, bSuccess);

	if (StreamingLevel && bSuccess)
	{
		// 로딩이 완료되면 아래 OnLevelLoaded 함수가 실행되도록 예약
		StreamingLevel->OnLevelLoaded.AddDynamic(this, &UPotalManager::OnLevelLoadFinished);
	}
	
	NextLevelInstance = StreamingLevel;
}

void UPotalManager::OnLevelLoadFinished()
{
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
		}), Delay, false);
	}
	else
	{
		TeleportLevel();
	}
}

void UPotalManager::TeleportLevel()
{
	//컨트롤러를 어떻게 넘겨줄것인가. 턴제 게임의 컨트롤러는 누구인가. 그 컨트롤러에 빙의를 해야 하는가.
	//전투가 아니라 이벤트나 상점이라면 여전히 내가 이동해야 한다.
		
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
			
			// 페이드 인
			FTimerHandle DelayFadeInTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(DelayFadeInTimerHandle, FTimerDelegate::CreateLambda([this]()
			{
				CamManager->StartCameraFade(1.f, 0.f, 1.f, FLinearColor::Black, false, false);
			}), 1.0f, false);
		}
	}
}
