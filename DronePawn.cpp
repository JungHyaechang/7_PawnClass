// Fill out your copyright notice in the Description page of Project Settings.


#include "DronePawn.h"
#include "PawnByCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "UObject/ScriptDelegates.h"
#include "JungPlayerController.h"

ADronePawn::ADronePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBoxComponent"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetSimulatePhysics(false);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionBox->SetGenerateOverlapEvents(true);

	SkeletalMeshComp = CreateDefaultSubobject <USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComp->SetupAttachment(CollisionBox);
	SkeletalMeshComp->SetSimulatePhysics(false);
	SkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SkeletalMeshComp->SetCollisionObjectType(ECC_Pawn);
	SkeletalMeshComp->SetCollisionResponseToAllChannels(ECR_Block);
	SkeletalMeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComp->SetupAttachment(CollisionBox);
	SpringArmComp->TargetArmLength = 700.0f;
	SpringArmComp->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;

	XYaxisNormalSpeed = 1600.0;
	ZaxisNormalSpeed = 2000.0;
	CurrentVelocity = FVector::ZeroVector;
	TargetVelocity = FVector::ZeroVector;
	Gravity = FVector(0.0f, 0.0f, -980.0f);
	AccelerationValue = 1.0f;
	DecelerationValue = 1.0f;
	MouseSensivity = 0.5f;
	TargetRoll = 0.0f;
	TargetPitch = 0.0f;
	RollInterpSpeed = 5.0f;
	PitchInterpSpeed = 3.0f;
	MaxRollAngle = 10.0;
	MaxPitchAngle = -7.0;
	bIsGrounded = true;
}

void ADronePawn::BeginPlay()
{
	Super::BeginPlay();

	// ������ ���ε�
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ADronePawn::OnOverlapBegin);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &ADronePawn::OnOverlapEnd);
	
}

// ������ ���� �� ����
void ADronePawn::OnOverlapBegin(UPrimitiveComponent* OverlappedComponenet, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawnByCharacter* Player = Cast<APawnByCharacter>(OtherActor);
	if (Player && Player->SpringArmComp)
	{
		Player->bCanEnterDrone = true;
		Player->CurrentDrone = this;
		UE_LOG(LogTemp, Warning, TEXT("Begin Overlap, CanEnterDrone! press G button"));

		Player->SpringArmComp->bDoCollisionTest = false;
		Player->SpringArmComp->TargetArmLength = 300.0f;
	}
}

void ADronePawn::OnOverlapEnd(UPrimitiveComponent* OverlappedComponenet, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawnByCharacter* Player = Cast<APawnByCharacter>(OtherActor);
	if (Player)
	{
		AController* PlayerController = Player->GetController();

		if (!PlayerController)
		{
			UE_LOG(LogTemp, Error, TEXT("OnOverlapEnd: Player has no controller!"));
			return;
		}
		
		// EndOverlap �Ǵ� ���� �ٷ� nullptr �̵Ǹ鼭 CurrentDrone�� nullptr �Ǹ鼭 Possess�� �ȵ�. �׷��� 0.1 ���� nullptr�� �ǵ��� �߰��� ����
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [Player,PlayerController]()
			{
				if (PlayerController->GetPawn() != Player)
				{
					UE_LOG(LogTemp, Warning, TEXT("OnOverlapEnd (Delayed Check): Player is still controlling the drone."));
					return;
				}
				Player->bCanEnterDrone = false;
				Player->CurrentDrone = nullptr;
				UE_LOG(LogTemp, Warning, TEXT("OnOverlapEnd (Delayed Check): End Overlap, Can Not EnterDrone."));
			}, 0.1f, false);
	}
}

void ADronePawn::EnableDroneControll()
{
	// �����ϴ� GetController()�� nullptr�� ���ܰ� ���´ٰ� �ؼ� if(GetController()) ���� �߰�
	if (GetController()) 
	{
		AJungPlayerController* PlayerController = Cast<AJungPlayerController>(GetController());
		if (PlayerController)
		{
			EnableInput(PlayerController);

			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->ClearAllMappings(); // ���� �Է� ����
				Subsystem->AddMappingContext(PlayerController->DroneIMC, 0); // DroneIMC�� ����
			}
		}
	}
}

void ADronePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckIfOnGround(); // �ٴ� ����

	// �߷� ����
	if (!bIsGrounded)
	{
		const float MaxFallSpeed = -3000.0f;
		CurrentVelocity.Z = FMath::Clamp(CurrentVelocity.Z - FMath::Abs(Gravity.Z) * DeltaTime, MaxFallSpeed, ZaxisNormalSpeed);
	}
	else
	{
		if (CurrentVelocity.Z <= 0)
		{
			CurrentVelocity.Z = 0;
		}
	}

	FRotator YawRotation(0.0f, GetActorRotation().Yaw, 0.0f);

	FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) * MoveInput.X * XYaxisNormalSpeed;
	FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) * MoveInput.Y * XYaxisNormalSpeed;
	FVector Up = GetActorUpVector() * MoveInput.Z * ZaxisNormalSpeed;
	TargetVelocity = Forward + Right + Up;

	// �ӵ� ���� ó��
	if (!TargetVelocity.IsNearlyZero())
	{
		CurrentVelocity = FMath::VInterpTo(CurrentVelocity, TargetVelocity, DeltaTime, AccelerationValue);
	}
	else
	{
		CurrentVelocity = FMath::VInterpTo(CurrentVelocity, FVector::ZeroVector, DeltaTime, DecelerationValue);
	}

	// �̵� ó��
	if (!CurrentVelocity.IsNearlyZero())
	{
		FVector NewLocation = GetActorLocation() + (CurrentVelocity * DeltaTime);
		SetActorLocation(NewLocation, true);
	}

	// Roll ���� ó��
	FRotator CurrentRotation = GetActorRotation();
	CurrentRotation.Pitch = FMath::FInterpTo(CurrentRotation.Pitch, TargetPitch, DeltaTime, PitchInterpSpeed);
	CurrentRotation.Roll = FMath::FInterpTo(CurrentRotation.Roll, TargetRoll, DeltaTime, RollInterpSpeed);
	SetActorRotation(CurrentRotation);
}

void ADronePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (AJungPlayerController* PlayerController = Cast<AJungPlayerController>(GetController()))
		{
			if (PlayerController->DroneIMC)
			{
				// DroneMove <--> Move �ΰ��� ���¸� ���ε�(Triggered, Completed)
				EnhancedInput->BindAction(
					PlayerController->DroneMove,
					ETriggerEvent::Triggered,
					this,
					&ADronePawn::Move
				);

				// LookAction <--> Look ���콺 ȸ���� ���� ī�޶� ȸ��
				EnhancedInput->BindAction(
					PlayerController->LookAction,
					ETriggerEvent::Triggered,
					this,
					&ADronePawn::Look
				);
			}
		}
	}
}

void ADronePawn::Move(const FInputActionValue& value)
{
	if (!Controller) return;

	MoveInput = value.Get<FVector>();
	
	TargetPitch = MoveInput.X * MaxPitchAngle;
	TargetRoll = MoveInput.Y * MaxRollAngle;
}

void ADronePawn::Look(const FInputActionValue& value)
{
	if (!Controller) return;

	FVector2D Inputvalue2D = value.Get<FVector2D>();
	FRotator CurrentRotation = Controller->GetControlRotation();

	float CameraYaw = CurrentRotation.Yaw + (Inputvalue2D.X * MouseSensivity);
	float CameraPitch = FMath::Clamp(CurrentRotation.Pitch + (Inputvalue2D.Y * MouseSensivity), -80.0f, 80.0f);

	// ī�޶� ȸ��
	FRotator TargetRotation = FRotator(CameraPitch, CameraYaw, 0.0f);
	Controller->SetControlRotation(TargetRotation);
}

// �浹 ���� Ʈ���̽�(����ĳ��Ʈ)
void ADronePawn::CheckIfOnGround()
{
	FHitResult Hit;
	FVector Start = GetActorLocation() - FVector(0.0f, 0.0f, CollisionBox->GetScaledBoxExtent().Z);
	FVector End = Start - FVector(0.0f, 0.0f, 15.0f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	bool bHit = GetWorld()->LineTraceSingleByObjectType(
		Hit,
		Start,
		End,
		ObjectQueryParams,
		Params
	);

	if (bHit && Hit.GetActor())
	{
		FVector ImpactPoint = Hit.ImpactPoint;
		float DistanceToGround = Start.Z - ImpactPoint.Z;

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

