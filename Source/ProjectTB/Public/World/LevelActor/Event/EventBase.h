// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EventBase.generated.h"

class AWorldPlayerController;
class UUserWidget;
class UPrimitiveComponent;


UCLASS()
class PROJECTTB_API AEventBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEventBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> EventWidgetClass;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UUserWidget* EventWidget;
	
	UPROPERTY(BlueprintReadOnly, Category="Event")
	TObjectPtr<AWorldPlayerController> CachedPC;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Event")
	bool bOneShot = true;

	UPROPERTY(BlueprintReadOnly, Category="Event")
	bool bIsTriggered = false;

	UPROPERTY(BlueprintReadOnly, Category="Event")
	bool bIsResolved = false;

	UPROPERTY(BlueprintReadOnly, Category="Event")
	bool bIsCompleted = false;

	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnAreaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	// 중복 실행 방지
	UFUNCTION(BlueprintCallable, Category="Event")
	bool CanStartEvent() const;

	UFUNCTION(BlueprintCallable, Category="Event")
	void ResolveEvent();

	UFUNCTION(BlueprintCallable, Category="Event")
	void RequestCloseEvent();

	UFUNCTION(BlueprintCallable, Category="Event")
	void FinishEvent();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="Event")
	void StartEvent();

	UFUNCTION(BlueprintImplementableEvent, Category="Event")
	void OnEventFinished();
	
	
private:
	bool CreateEventWidget();
	void RequestShowEventWidget(AWorldPlayerController* PC);
	void DisableEventActor();
};
