// Copyright Epic Games, Inc. All Rights Reserved.

#include "VelinasPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "VelinasCharacter.h"
#include "Engine/World.h"
#include "Engine/SkeletalMeshSocket.h"

#include "EngineUtils.h"

#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/GameplayStatics.h"

AVelinasPlayerController::AVelinasPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AVelinasPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if(bInputPressed)
	{
		FollowTime += DeltaTime;

		// Look for the touch location
		FVector HitLocation = FVector::ZeroVector;
		FHitResult Hit;
		if(bIsTouch)
		{
			GetHitResultUnderFinger(ETouchIndex::Touch1, ECC_Visibility, true, Hit);
		}
		else
		{
			GetHitResultUnderCursor(ECC_Visibility, true, Hit);
		}
		HitLocation = Hit.Location;

		// Direct the Pawn towards that location
		APawn* const MyPawn = GetPawn();
		if(MyPawn)
		{
			FVector WorldDirection = (HitLocation - MyPawn->GetActorLocation()).GetSafeNormal();
			MyPawn->AddMovementInput(WorldDirection, 1.f, false);
		}
	}
	else
	{
		FollowTime = 0.f;
	}
}

void AVelinasPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &AVelinasPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &AVelinasPlayerController::OnSetDestinationReleased);

	InputComponent->BindAxis("MoveUp", this, &AVelinasPlayerController::OnMoveUp);
	InputComponent->BindAxis("MoveRight", this, &AVelinasPlayerController::OnMoveRight);

	InputComponent->BindAction("Jump", IE_Pressed, this, &AVelinasPlayerController::OnJumpPressed);
	InputComponent->BindAction("Jump", IE_Released, this, &AVelinasPlayerController::OnJumpReleased);

	InputComponent->BindAction("Equip", IE_Pressed, this, &AVelinasPlayerController::OnEquip);

	// support touch devices 
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AVelinasPlayerController::OnTouchPressed);
	InputComponent->BindTouch(EInputEvent::IE_Released, this, &AVelinasPlayerController::OnTouchReleased);
}

void AVelinasPlayerController::OnMovement(float AxisValue, EAxis::Type Axis)
{
	APawn* const PlayerPawn = GetPawn();
	// Find out which way is forward
	const FRotator Rotation = this->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// Get forward vector
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(Axis);
	PlayerPawn->AddMovementInput(Direction, AxisValue);
}

void AVelinasPlayerController::OnMoveUp(float AxisValue)
{
	this->OnMovement(AxisValue, EAxis::X);
}

void AVelinasPlayerController::OnMoveRight(float AxisValue)
{
	this->OnMovement(AxisValue, EAxis::Y);
}

void AVelinasPlayerController::OnJumpPressed()
{
	ACharacter *const PlayerCharacter = this->GetCharacter();
	PlayerCharacter->bPressedJump = true;
}

void AVelinasPlayerController::OnJumpReleased()
{
	ACharacter* const PlayerCharacter = this->GetCharacter();
	PlayerCharacter->bPressedJump = false;
}

void AVelinasPlayerController::OnEquip()
{
	AVelinasCharacter* const PlayerCharacter = (AVelinasCharacter*)this->GetCharacter();
	for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		// Follow iterator object to my actual actor pointer
		AActor* MyActor = *ActorItr;
		if (MyActor->ActorHasTag("Weapon"))
		{
			// Actor with tag "Weapon", a.k.a Dagger
			PlayerCharacter->RightHandEquipment = MyActor;
			USkeletalMeshSocket *socket = PlayerCharacter->GetMesh()->SkeletalMesh->FindSocket(FName("hand_rSocket"));
			FTransform sTransform = socket->GetSocketTransform(PlayerCharacter->GetMesh());

			MyActor->SetActorTransform(sTransform);

			if (socket != nullptr && socket != NULL)
			{
				socket->AttachActor(MyActor, PlayerCharacter->GetMesh());
			}
		}
	}
}


void AVelinasPlayerController::OnSetDestinationPressed()
{
	// We flag that the input is being pressed
	bInputPressed = true;
	// Just in case the character was moving because of a previous short press we stop it
	StopMovement();
}

void AVelinasPlayerController::OnSetDestinationReleased()
{
	// Player is no longer pressing the input
	bInputPressed = false;

	// If it was a short press
	if(FollowTime <= ShortPressThreshold)
	{
		// We look for the location in the world where the player has pressed the input
		FVector HitLocation = FVector::ZeroVector;
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, true, Hit);
		HitLocation = Hit.Location;

		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitLocation);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, HitLocation, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}
}

void AVelinasPlayerController::OnTouchPressed(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	bIsTouch = true;
	OnSetDestinationPressed();
}

void AVelinasPlayerController::OnTouchReleased(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	bIsTouch = false;
	OnSetDestinationReleased();
}
