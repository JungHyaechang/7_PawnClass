// Fill out your copyright notice in the Description page of Project Settings.


#include "JungPlayerController.h"
#include "EnhancedInputSubsystems.h"


AJungPlayerController::AJungPlayerController()
	: CharacterIMC(nullptr),
	DroneIMC(nullptr),
	MoveAction(nullptr),
	LookAction(nullptr),
	JumpAction(nullptr),
	SprintAction(nullptr),
	DroneMove(nullptr),
	EnterDrone(nullptr)
{
}


void AJungPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			// 캐릭터 IMC 활성화
			if (CharacterIMC)
			{
				Subsystem->AddMappingContext(CharacterIMC, 0);
			}
		}
	}
}







