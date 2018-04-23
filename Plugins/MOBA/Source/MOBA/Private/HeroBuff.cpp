﻿ // Fill out your copyright notice in the Description page of Project Settings.

#include "MOBAPrivatePCH.h"
#include "HeroBuff.h"
#include "UnrealNetwork.h"
#include "HeroCharacter.h"
#include "MOBAPlayerController.h"

AHeroBuff::AHeroBuff(const FObjectInitializer& ObjectInitializer)
	: Super(FObjectInitializer::Get())
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	Interval = 0;
	IntervalCounting = 0;
	IntervalCount = 0;
	Particle = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("Particle0"));
	Particle->SetupAttachment(RootComponent);
}

AHeroBuff* AHeroBuff::NewHeroBuff()
{
	return NewObject<AHeroBuff>();
}

TArray<AHeroBuff*> AHeroBuff::CloneArray(TArray<AHeroBuff*> input)
{
	TArray<AHeroBuff*> res;
	for (AHeroBuff* hb : input)
	{
		res.Add(hb->Clone());
	}
	return res;
}

AHeroBuff* AHeroBuff::Clone()
{
	AHeroBuff* data = NewObject<AHeroBuff>();
	data->Priority = Priority;
	data->Name = Name;
	data->Head = Head;
	data->BuffState = BuffState;
	data->CanStacks = CanStacks;
	data->Stacks = Stacks;
	data->Duration = Duration;
	return data;
}

void AHeroBuff::BeginPlay()
{
	Super::BeginPlay();
	MaxDuration = Duration;
}

void AHeroBuff::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Forever)
	{
		Duration -= DeltaTime;
		ParticleDuration -= DeltaTime;
		RealDuration -= DeltaTime;
	}
	if (ParticleDuration <= 0)
	{
		Particle->Deactivate();
	}
	if (RealDuration <= 0 && !IsPendingKillPending())
	{
		this->Destroy();
	}
	if (Interval > 0 && Duration >= 0)
	{
		AuraCount += DeltaTime;
		if (AuraCount > 0.1)
		{
			AuraCount = 0;
			TArray<AHeroCharacter*> tmp;
			if (BuffUniqueMap.Contains(HEROU::AuraRadiusEnemy))
			{
				float range = BuffUniqueMap[HEROU::AuraRadiusEnemy];
				TArray<AHeroCharacter*> Enemys = AHeroCharacter::localPC->FindRadiusActorByLocation(
					BuffTarget[0], GetActorLocation(), range, ETeamFlag::TeamEnemy, true);
				for (AHeroCharacter* EachHero : Enemys)
				{
					tmp.Add(EachHero);
				}
			}
			if (BuffUniqueMap.Contains(HEROU::AuraRadiusFriends))
			{
				float range = BuffUniqueMap[HEROU::AuraRadiusFriends];
				TArray<AHeroCharacter*> Enemys = AHeroCharacter::localPC->FindRadiusActorByLocation(
					BuffTarget[0], GetActorLocation(), range, ETeamFlag::TeamFriends, true);
				for (AHeroCharacter* EachHero : Enemys)
				{
					tmp.Add(EachHero);
				}
			}
		}
		
		IntervalCounting += DeltaTime;
		if (IntervalCounting >= Interval)
		{
			IntervalCount++;
			IntervalCounting = 0;
			OnInterval(IntervalCount);
		}
	}
}


void AHeroBuff::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroBuff, BuffTarget);
}
