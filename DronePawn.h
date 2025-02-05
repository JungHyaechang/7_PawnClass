// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DronePawn.generated.h"

class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UBoxComponent;
struct FInputActionValue;


UCLASS()
class ACTORPAWN_API ADronePawn : public APawn
{
	GENERATED_BODY()

public:
	ADronePawn();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkeletalMeshComponent")
	UBoxComponent* CollisionBox;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkeletalMeshComponent")
	USkeletalMeshComponent* SkeletalMeshComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArmComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraComponent;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponenet, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponenet, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UFUNCTION()
	void EnableDroneControll();

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void Move(const FInputActionValue& value);
	UFUNCTION()
	void Look(const FInputActionValue& value);
	UFUNCTION()
	void CheckIfOnGround();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float XYaxisNormalSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ZaxisNormalSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AccelerationValue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float DecelerationValue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FVector CurrentVelocity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MouseSensivity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FVector TargetVelocity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float TargetRoll;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float TargetPitch;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RollInterpSpeed; // Roll 보간속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float PitchInterpSpeed; // Pitch 보간속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxRollAngle; // 최대 Roll 기울기 각도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxPitchAngle; // 최대 Pitch 기울기 각도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FVector Gravity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bIsGrounded;

	FVector MoveInput;

};
