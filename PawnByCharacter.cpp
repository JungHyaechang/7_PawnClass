// Fill out your copyright notice in the Description page of Project Settings.


#include "PawnByCharacter.h"
#include "DronePawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "JungPlayerController.h"


APawnByCharacter::APawnByCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// 캡슐 컴포넌트 생성 및 설정
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->InitCapsuleSize(34.0f, 95.0f);
	CapsuleComponent->SetSimulatePhysics(false);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CapsuleComponent->SetCollisionObjectType(ECC_Pawn);
	CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	// 스켈레탈 메시 컴포넌트 생성 및 설정
	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComp->SetupAttachment(CapsuleComponent);
	SkeletalMeshComp->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleComponent->GetUnscaledCapsuleHalfHeight()));

	bUseControllerRotationYaw = false; // 마우스 회전에 따른 Pawn 회전 동기화
	// 스프링암 및 카메라 추가, 설정
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComp->SetupAttachment(CapsuleComponent);
	SpringArmComp->TargetArmLength = 300.0f;
	SpringArmComp->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;

	WalkSpeed = NormalSpeed;
	NormalSpeed = 300.0f;
	SprintMultiplier = 3.0f;
	SprintSpeed = NormalSpeed * SprintMultiplier;
	Acceleration = 5.0f;
	Deceleration = 500.0f;
	Gravity = FVector(0.0f, 0.0f, -980.0f);
	bIsGrounded = true;
	bIsSprinting = false;
	MouseSensitivity = 1.5f;
	JumpForce = 700.0f;
	CurrentVelocity = FVector::ZeroVector;
	TargetVelocity = FVector::ZeroVector;
	bCanEnterDrone = false;
	CurrentDrone = nullptr;
	TargetYaw = 0.0;
	YawInterpSpeed = 15.0f;
}

void APawnByCharacter::BeginPlay()
{
	Super::BeginPlay();

	WalkSpeed = NormalSpeed;
}

void APawnByCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckIfOnGround();

	// 중력 적용
	if (!bIsGrounded)
	{
		// 중력 상한 설정 (최대 낙하 속도)
		const float MaxFallSpeed = -3000.0f;
		CurrentVelocity.Z = FMath::Clamp(CurrentVelocity.Z - FMath::Abs(Gravity.Z) * DeltaTime, MaxFallSpeed, JumpForce);
	}
	else
	{
		if (CurrentVelocity.Z <= 0)
		{
			CurrentVelocity.Z = 0;
		}
	}

	// 캐릭터 이동 업데이트 로직 , 가속 / 감속 처리
	if (!TargetVelocity.IsNearlyZero())
	{
		CurrentVelocity.X = FMath::FInterpTo(CurrentVelocity.X, TargetVelocity.X, DeltaTime, Acceleration);
		CurrentVelocity.Y = FMath::FInterpTo(CurrentVelocity.Y, TargetVelocity.Y, DeltaTime, Acceleration);
	}
	else
	{
		CurrentVelocity.X = FMath::FInterpTo(CurrentVelocity.X, 0.0f, DeltaTime, Deceleration);
		CurrentVelocity.Y = FMath::FInterpTo(CurrentVelocity.Y, 0.0f, DeltaTime, Deceleration);
	}

	FVector FinalVelocity = CurrentVelocity;

	if (!FinalVelocity.IsNearlyZero())
	{
		FVector Newlocation = GetActorLocation() + (FinalVelocity * DeltaTime);
		SetActorLocation(Newlocation, true);
	}
	
	// 캐릭터 회전 업데이트 로직
	if (Controller) // Possess 변환에 따라 컨트
	{
		FRotator CarmeraRotation = Controller->GetControlRotation();
		if (!MoveInput.IsNearlyZero()) // 이게 없으면 입력을 떼는 순간 캐릭터가 부자연스럽게 정면을 바라봄.
		{
			float radians = atan2(-1 * MoveInput.Y, MoveInput.X);
			float angles = -1 * radians * (180 / PI);
			TargetYaw = CarmeraRotation.Yaw + angles;
		}
	}
	
	FRotator CurrentRotaion = GetActorRotation();
	float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentRotaion.Yaw, TargetYaw); // 각도 래핑 문제를 해결하기 위한 각도 보정(최단거리) 
	float NewYaw = CurrentRotaion.Yaw + DeltaYaw * FMath::Clamp(DeltaTime * YawInterpSpeed, 0.0f, 1.0f); // 보간
	CurrentRotaion.Yaw = NewYaw;
	SetActorRotation(CurrentRotaion);
}


void APawnByCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (AJungPlayerController* PlayerController = Cast<AJungPlayerController>(GetController()))
		{
			// MoveAction <-> Move 바인딩
			if (PlayerController->MoveAction)
			{
				EnhancedInput->BindAction(
					PlayerController->MoveAction,
					ETriggerEvent::Triggered,
					this,
					&APawnByCharacter::StartMove
				);

				EnhancedInput->BindAction(
					PlayerController->MoveAction,
					ETriggerEvent::Completed,
					this,
					&APawnByCharacter::StopMove
				);
			}

			// LookAction <-> Look 바인딩
			if (PlayerController->LookAction)
			{
				EnhancedInput->BindAction(
					PlayerController->LookAction,
					ETriggerEvent::Triggered,
					this,
					&APawnByCharacter::Look
				);
			}

			// JumpAction <-> JumpStart, JumpStop 바인딩
			if (PlayerController->JumpAction)
			{
				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Triggered,
					this,
					&APawnByCharacter::StartJump
				);

				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Completed,
					this,
					&APawnByCharacter::StopJump
				);
			}

			// SprintAction <-> SprintStart, SprintStop 바인딩
			if (PlayerController->SprintAction)
			{
				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Triggered,
					this,
					&APawnByCharacter::StartSprint
				);

				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Completed,
					this,
					&APawnByCharacter::StopSprint
				);
			}

			// EnterDrone <-> EnterDrone 바인딩
			if (PlayerController->EnterDrone)
			{
				EnhancedInput->BindAction(
					PlayerController->EnterDrone,
					ETriggerEvent::Triggered,
					this,
					&APawnByCharacter::EnterDrone
				);
			}
		}
	}
}



void APawnByCharacter::StartMove(const FInputActionValue& value)
{
	if (!Controller) return;
	MoveInput = value.Get<FVector2D>();

	FRotator CarmeraRotaion = Controller->GetControlRotation();
	FVector Forward = FRotationMatrix(CarmeraRotaion).GetUnitAxis(EAxis::X) * MoveInput.X;
	FVector Right = FRotationMatrix(CarmeraRotaion).GetUnitAxis(EAxis::Y) * MoveInput.Y;

	TargetVelocity = Forward + Right;
	TargetVelocity = TargetVelocity.GetSafeNormal() * WalkSpeed;

}
/* 처음 캐릭터 회전을 위해 노력했던 로직(실패)
void  APawnByCharacter::StartRotation(const FInputActionValue& value)
{
	if (!Controller) return;
	const FVector2D MoveInput = value.Get<FVector2D>();

	if (!MoveInput.IsNearlyZero())
	{
		FVector Forward = GetActorForwardVector() * MoveInput.X;
		FVector Right = GetActorRightVector() * MoveInput.Y;

		FVector MoveDirection = FVector(Forward.X + Right.X, Forward.Y + Right.Y, 0.0f);

		MoveDirection.Normalize();

		FRotator TargetRotation = MoveDirection.Rotation();
		AddActorLocalRotation(TargetRotation);
	}
}
*/
void APawnByCharacter::StopMove(const FInputActionValue& value)
{
	if (!Controller) return;
	MoveInput = value.Get<FVector2D>();

	if (MoveInput.IsNearlyZero())
	{
		// CurrentVelocity.X = 0;
		// CurrentVelocity.Y = 0;
		TargetVelocity.X = 0;
		TargetVelocity.Y = 0;
		return;
	}
}

void APawnByCharacter::StartJump(const FInputActionValue& value)
{
	bool bJumpPressed = value.Get<bool>();

	if (bJumpPressed && bIsGrounded) // 지면에 있을 때만 점프 가능
	{
		bIsGrounded = false;
		CurrentVelocity.Z = JumpForce;
		// CurrentVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, JumpForce);
	}
}

void APawnByCharacter::StopJump(const FInputActionValue& value)
{
	bool bJumpReleased = !value.Get<bool>();

	if (bJumpReleased && CurrentVelocity.Z > 0) // 공중에 있을 때와 점프키에서 손을 땠을 때 StopJump
	{
		CurrentVelocity.Z *= 0.5f;
	}
}

void APawnByCharacter::CheckIfOnGround()
{
	// 지면 체크를 위한 레이캐스트
	FHitResult Hit;
	FVector Start = GetActorLocation() - FVector(0.0f, 0.0f, CapsuleComponent->GetScaledCapsuleHalfHeight());
	FVector End = Start - FVector(0.0f, 0.0f, 10.0f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this); // 자기 자신은 충돌 무시

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic); // 땅, 벽 등 정적인 객체에 대한 충돌 채널
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic); // 움직이는 플랫폼에 대한 충돌 채널

	bool bHit = GetWorld()->LineTraceSingleByObjectType(
		Hit,
		Start,
		End,
		ObjectQueryParams,
		Params
	);

	if (bHit && Hit.GetActor())
	{
		// UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s"), *Hit.GetActor()->GetName());
		FVector ImpactPoint = Hit.ImpactPoint;
		float DistanceToGround = (Start.Z - ImpactPoint.Z);

		if (DistanceToGround <= 10.0f)
		{
			bIsGrounded = true;
		}
		else
		{
			bIsGrounded = false;
		}
	}
	else
	{
		bIsGrounded = false;
	}
}

void APawnByCharacter::StartSprint(const FInputActionValue& value)
{
	bool bSprintPressed = value.Get<bool>();

	if (bSprintPressed && !bIsSprinting)
	{
		bIsSprinting = true;
		WalkSpeed = SprintSpeed;
	}
}

void APawnByCharacter::StopSprint(const FInputActionValue& value)
{
	bool bSprintReleased = !value.Get<bool>();

	if (bSprintReleased && bIsSprinting) // 키를 때고 달리고 있을 때만 실행
	{
		bIsSprinting = false;
		WalkSpeed = NormalSpeed;
	} 
}

void APawnByCharacter::Look(const FInputActionValue& value)
{
	if (!Controller) return;

	const FVector2D LookInput = value.Get<FVector2D>();
	FRotator CurrentRotation = Controller->GetControlRotation();

	float NewYaw = CurrentRotation.Yaw + (LookInput.X * MouseSensitivity);
	float NewPitch = FMath::Clamp(CurrentRotation.Pitch + (LookInput.Y * MouseSensitivity), -80.0f, 80.0f); // 최대, 최소 각도 조절

	Controller->SetControlRotation(FRotator(NewPitch, NewYaw, 0.0f));

	// 시점 변환 시 좌표 확인 로그
	// UE_LOG(LogTemp, Warning, TEXT("Yaw: %f, Pitch: %f"), NewYaw, NewPitch);
}

void APawnByCharacter::EnterDrone(const FInputActionValue& value)
{
	if (bCanEnterDrone && CurrentDrone)
	{
		AJungPlayerController* PlayerController = Cast<AJungPlayerController>(GetController());
		if (PlayerController)
		{
			// 캐릭터 숨기기
			this->SetActorHiddenInGame(true);
			// 충돌 무시
			this->SetActorEnableCollision(false);
			// 입력 비활성화
			this->DisableInput(PlayerController);

			PlayerController->Possess(CurrentDrone); // 오버랩 됐을 때 CurrentDrone == DronePawn, 아닐 땐 nullptr;	
			if (CurrentDrone)
			{
				// Possess 확인 로그
				if (PlayerController->GetPawn() == CurrentDrone) 
				{
					UE_LOG(LogTemp, Warning, TEXT("Possess Success"));
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Possess Fail"));
				}

				CurrentDrone->EnableDroneControll();
				PlayerController->SetViewTargetWithBlend(CurrentDrone, 0.5f);
				UE_LOG(LogTemp, Warning, TEXT("Camera switched"));
			}
		}
	}
}
