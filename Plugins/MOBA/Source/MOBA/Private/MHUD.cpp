// Fill out your copyright notice in the Description page of Project Settings.
#include "MOBAPrivatePCH.h"
#include "MHUD.h"
#include <algorithm>
#include "EngineUtils.h"
// for GEngine
#include "Engine.h"
#include "UnrealString.h"
#include "NameTypes.h"
#include "WidgetLayoutLibrary.h"


#include "HeroCharacter.h"
#include "MOBAPlayerController.h"
#include "Equipment.h"
#include "MOBAGameState.h"
#include "HeroActionx.h"
#include "SceneObject.h"
#include "Equipment.h"


AMHUD::AMHUD()
{
	LocalController = NULL;
	SequenceNumber = 1;
}

void AMHUD::BeginPlay()
{
	Super::BeginPlay();
	RTSStatus = ERTSStatusEnum::Normal;
	bMouseRButton = false;
	bMouseLButton = false;
	ClickedSelected = false;
	WantPickup = NULL;
	ThrowTexture = NULL;
	for(TActorIterator<AHeroCharacter> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		HeroCanSelection.Add(*ActorItr);
	}
	for(int i = 0; i < 6; ++i)
	{
		FVector2D p1, s1;
		GetEquipmentPosition(i, p1, s1);
		RTS_AddHitBox(p1, s1, FString::Printf(TEXT("Equipment%d"), i + 1), false, 0);
		if(EquipmentMaterial)
		{
			EquipmentDMaterials.Add(UMaterialInstanceDynamic::Create(EquipmentMaterial, this));
		}
	}
	for(int i = 0; i < 4; ++i)
	{
		FVector2D p1, s1;
		GetSkillPosition(i, p1, s1);
		RTS_AddHitBox(p1, s1, FString::Printf(TEXT("Skill%d"), i + 1), false, 0);
		if(SkillMaterial)
		{
			SkillDMaterials.Add(UMaterialInstanceDynamic::Create(SkillMaterial, this));
		}
	}
	if(ThrowMaterial)
	{
		ThrowDMaterial = UMaterialInstanceDynamic::Create(ThrowMaterial, this);
	}
	OnSize();
	bClickHero = false;
	bNeedMouseRDown = false;
	bNeedMouseLDown = false;

}

void AMHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(RemoveSelection.Num() > 0)
	{
		RTSStatus = ERTSStatusEnum::Normal;
		for(AHeroCharacter* EachHero : RemoveSelection)
		{
			EachHero->SelectionOff();
			CurrentSelection.Remove(EachHero);
		}
		RemoveSelection.Empty();
	}
	OnSize();
}

void AMHUD::DrawHUD()
{
	Super::DrawHUD();

	if(RTSStatus == ERTSStatusEnum::Normal && bMouseLButton && IsGameRegion(CurrentMouseXY))
	{
		// selection box
		if(FVector2D::DistSquared(InitialMouseXY, CurrentMouseXY) > 25)
		{
			for(AHeroCharacter* EachHero : HeroCanSelection)
			{
				FVector pos = this->Project(EachHero->GetActorLocation());
				EachHero->ScreenPosition.X = pos.X;
				EachHero->ScreenPosition.Y = pos.Y;
				bool res = CheckInSelectionBox(EachHero->ScreenPosition);
				if(res && !EachHero->isSelection)
				{
					EachHero->SelectionOn();
				}
				else if(!res && EachHero->isSelection)
				{
					EachHero->SelectionOff();
				}
			}

			float maxX, maxY;
			float minX, minY;
			maxX = std::max(InitialMouseXY.X, CurrentMouseXY.X);
			maxY = std::max(InitialMouseXY.Y, CurrentMouseXY.Y);
			minX = std::min(InitialMouseXY.X, CurrentMouseXY.X);
			minY = std::min(InitialMouseXY.Y, CurrentMouseXY.Y);
			DrawLine(minX, minY, maxX, minY, SelectionBoxLineColor);
			DrawLine(maxX, minY, maxX, maxY, SelectionBoxLineColor);
			DrawLine(maxX, maxY, minX, maxY, SelectionBoxLineColor);
			DrawLine(minX, maxY, minX, minY, SelectionBoxLineColor);

			DrawRect(SelectionBoxFillColor, minX, minY, maxX - minX - 1, maxY - minY - 1);
		}
	}
	for(AHeroCharacter* EachHero : HeroCanSelection)
	{
		FVector2D headpos = FVector2D(this->Project(EachHero->PositionOnHead->GetComponentLocation()));
		FVector2D footpos = FVector2D(this->Project(EachHero->PositionUnderFoot->GetComponentLocation()));
		footpos.Y += 35;
		float  hpBarLength = EachHero->HPBarLength;
		float  halfHPBarLength = hpBarLength * .5f;
		headpos += HPBarOffset;
		DrawRect(HPBarBackColor, headpos.X - halfHPBarLength - 1, headpos.Y - 1, hpBarLength + 2, HPBarHeight + 2);
		DrawRect(HPBarForeColor, headpos.X - halfHPBarLength, headpos.Y, hpBarLength * EachHero->GetHPPercent(), HPBarHeight);
		float maxhp = EachHero->CurrentMaxHP;
		if(maxhp < 1500)
		{
			for(float i = 100; i < maxhp; i += 100)
			{
				float xpos = headpos.X - halfHPBarLength + hpBarLength * (i / maxhp);
				DrawLine(xpos, headpos.Y, xpos, headpos.Y + HPBarHeight, HPBarBackColor);
			}
		}
		else
		{
			for(float i = 500; i < maxhp; i += 500)
			{
				float xpos = headpos.X - halfHPBarLength + hpBarLength * (i / maxhp);
				DrawLine(xpos, headpos.Y, xpos, headpos.Y + HPBarHeight, HPBarBackColor);
			}
		}
		DrawText(EachHero->HeroName, FLinearColor(1, 1, 1), footpos.X - EachHero->HeroName.Len()*.5f * 15, footpos.Y);
	}
	if(CurrentSelection.Num() > 0)
	{
		if(RTSStatus == ERTSStatusEnum::ThrowEquipment)
		{
			ThrowDMaterial->SetTextureParameterValue(TEXT("InputTexture"), ThrowTexture);
			DrawMaterialSimple(ThrowDMaterial, CurrentMouseXY.X, CurrentMouseXY.Y,
			                   100 * ViewportScale, 100 * ViewportScale);
		}
		AHeroCharacter* selectHero = CurrentSelection[0];
		if(SkillMaterial)
		{
			for(int32 idx = 0; idx < 4; ++idx)
			{
				FMHitBox* skhb = FindHitBoxByName(FString::Printf(TEXT("Skill%d"), idx + 1));

				if(skhb && SkillDMaterials.Num() > idx && selectHero->Skill_Texture.Num() > idx)
				{
					SkillDMaterials[idx]->SetTextureParameterValue(TEXT("InputTexture"), selectHero->Skill_Texture[idx]);
					SkillDMaterials[idx]->SetScalarParameterValue(TEXT("Alpha"), selectHero->GetSkillCDPercent(idx));
					DrawMaterialSimple(SkillDMaterials[idx], skhb->Coords.X * ViewportScale, skhb->Coords.Y * ViewportScale,
					                   skhb->Size.X * ViewportScale, skhb->Size.Y * ViewportScale);
				}
			}
		}

		if(EquipmentMaterial)
		{
			for(int32 idx = 0; idx < 6; ++idx)
			{
				FMHitBox* skhb = FindHitBoxByName(FString::Printf(TEXT("Equipment%d"), idx + 1));

				if(skhb)
				{
					if(EquipmentDMaterials.Num() > idx && selectHero->Equipments.Num() > idx && selectHero->Equipments[idx])
					{
						EquipmentDMaterials[idx]->SetTextureParameterValue(TEXT("InputTexture"), selectHero->Equipments[idx]->Head);
						EquipmentDMaterials[idx]->SetScalarParameterValue(TEXT("Alpha"), selectHero->GetSkillCDPercent(idx));
						DrawMaterialSimple(EquipmentDMaterials[idx], skhb->Coords.X * ViewportScale, skhb->Coords.Y * ViewportScale,
						                   skhb->Size.X * ViewportScale, skhb->Size.Y * ViewportScale);
					}
					else
					{
						DrawRect(SelectionBoxFillColor, skhb->Coords.X * ViewportScale, skhb->Coords.Y * ViewportScale,
						         skhb->Size.X * ViewportScale, skhb->Size.Y * ViewportScale);
					}
				}
			}
		}
	}
}

bool AMHUD::CheckInSelectionBox(FVector2D pos)
{
	float maxX, maxY;
	float minX, minY;
	maxX = std::max(InitialMouseXY.X, CurrentMouseXY.X);
	maxY = std::max(InitialMouseXY.Y, CurrentMouseXY.Y);
	minX = std::min(InitialMouseXY.X, CurrentMouseXY.X);
	minY = std::min(InitialMouseXY.Y, CurrentMouseXY.Y);

	if(minX < pos.X && pos.X < maxX &&
	        minY < pos.Y && pos.Y < maxY)
	{
		return true;
	}
	return false;
}

void AMHUD::ClearAllSelection()
{
	for(AHeroCharacter* EachHero : CurrentSelection)
	{
		EachHero->SelectionOff();
	}
	CurrentSelection.Empty();
}

FMHitBox* AMHUD::FindHitBoxByName(const FString& name)
{
	for(int32 Index = 0; Index < RTS_HitBoxMap.Num(); ++Index)
	{
		if(RTS_HitBoxMap[Index].GetName() == name)
		{
			return &RTS_HitBoxMap[Index];
		}
	}
	return nullptr;
}

void AMHUD::RTS_AddHitBox(FVector2D Position, FVector2D Size, const FString& Name, bool bConsumesInput,
                             int32 Priority)
{
	bool bAdded = false;
	for(int32 Index = 0; Index < RTS_HitBoxMap.Num(); ++Index)
	{
		if(RTS_HitBoxMap[Index].GetPriority() < Priority)
		{
			RTS_HitBoxMap.Insert(FMHitBox(Position, Size, Name, bConsumesInput, Priority), Index);
			bAdded = true;
			break;
		}
	}
	if(!bAdded)
	{
		RTS_HitBoxMap.Add(FMHitBox(Position, Size, Name, bConsumesInput, Priority));
	}
}

bool AMHUD::IsGameRegion(FVector2D pos)
{
	for(FMHitBox& HitBox : RTS_HitBoxMap)
	{
		if(HitBox.Contains(pos, ViewportScale))
		{
			return false;
		}
	}
	return true;
}

bool AMHUD::IsUIRegion(FVector2D pos)
{
	return !IsGameRegion(pos);
}

void AMHUD::AssignSelectionHeroPickup(AEquipment* equ)
{
	if(LocalController && CurrentSelection.Num() > 0)
	{
		FHeroAction act;
		act.ActionStatus = EHeroActionStatus::MoveToPickup;
		act.TargetEquipment = equ;
		act.SequenceNumber = SequenceNumber++;
		for (AHeroCharacter* EachHero : CurrentSelection)
		{
			if (bLeftShiftDown)
			{
				LocalController->ServerAppendHeroAction(EachHero, act);
			}
			else
			{
				LocalController->ServerSetHeroAction(EachHero, act);
			}
		}
	}
}

void AMHUD::HeroAttackHero(AHeroCharacter* hero)
{
	bClickHero = true;
	if(LocalController)
	{
		TArray<AHeroCharacter*> HeroGoAttack;
		for(AHeroCharacter* EachHero : CurrentSelection)
		{
			if(EachHero->TeamId != hero->TeamId)
			{
				HeroGoAttack.Add(EachHero);
			}
		}
		if(HeroGoAttack.Num() > 0)
		{
			AMOBAGameState* ags = Cast<AMOBAGameState>(UGameplayStatics::GetGameState(GetWorld()));
			FHeroAction act;
			act.ActionStatus = EHeroActionStatus::AttackActor;
			act.TargetActor = hero;
			act.SequenceNumber = SequenceNumber++;

			for (AHeroCharacter* EachHero : HeroGoAttack)
			{
				if (bLeftShiftDown)
				{
					LocalController->ServerAppendHeroAction(EachHero, act);
				}
				else
				{
					LocalController->ServerSetHeroAction(EachHero, act);
				}
			}
		}
	}
}

void AMHUD::HeroAttackSceneObject(ASceneObject* SceneObj)
{
	bClickHero = true;
	if (LocalController)
	{
		if (CurrentSelection.Num() > 0)
		{
			AMOBAGameState* ags = Cast<AMOBAGameState>(UGameplayStatics::GetGameState(GetWorld()));
			FHeroAction act;
			act.ActionStatus = EHeroActionStatus::AttackSceneObject;
			act.TargetActor = SceneObj;
			act.SequenceNumber = SequenceNumber++;

			for (AHeroCharacter* EachHero : CurrentSelection)
			{
				if (bLeftShiftDown)
				{
					LocalController->ServerAppendHeroAction(EachHero, act);
				}
				else
				{
					LocalController->ServerSetHeroAction(EachHero, act);
				}

			}
		}
	}
}


void AMHUD::KeyboardCallUseSkill(int32 idx)
{
	if (CurrentSelection.Num() > 0)
	{
		bool res = CurrentSelection[0]->ShowSkillHint(idx);
		if (res)
		{
			RTSStatus = ERTSStatusEnum::SkillHint;
		}
	}
}

FVector AMHUD::GetCurrentDirection()
{
	if (CurrentSelection.Num() > 0)
	{
		return CurrentSelection[0]->CurrentSkillDirection;
	}
	return FVector();
}

FRotator AMHUD::GetCurrentRotator()
{
	if(CurrentSelection.Num() > 0)
	{
		return CurrentSelection[0]->CurrentSkillDirection.Rotation();
	}
	return FRotator();
}

void AMHUD::OnSize()
{
	ViewportScale = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
}

void AMHUD::OnMouseMove(FVector2D pos, FVector pos3d)
{
	CurrentMouseXY = pos;
	// 如果沒有點到任何東西就不更新滑鼠點到的位置
	if (pos3d != FVector::ZeroVector)
	{
		CurrentMouseHit = pos3d;
	}
}

void AMHUD::OnRMouseDown(FVector2D pos)
{
	AMOBAGameState* ags = Cast<AMOBAGameState>(UGameplayStatics::GetGameState(GetWorld()));
	// hitbox用
	for(FMHitBox& HitBox : RTS_HitBoxMap)
	{
		if(HitBox.Contains(pos, ViewportScale))
		{
			RButtonDownHitBox = HitBox.GetName();
			break;
		}
	}
	// 右鍵事件
	if(!bClickHero)
	{
		//localController->AddHeroToClearWantQueue(CurrentSelection);
	}
	if(IsGameRegion(pos) && LocalController && !bClickHero)
	{
		switch(RTSStatus)
		{
		case ERTSStatusEnum::Normal:
		{
			if(CurrentSelection.Num() > 0)
			{
				if(WantPickup)
				{
					//localController->AddHeroToPickupQueue(WantPickup->GetActorLocation(), CurrentSelection[0], WantPickup);
					WantPickup = NULL;
				}
				else
				{
					// 確認 shift 鍵來插旗
					FHeroAction act;
					act.ActionStatus = EHeroActionStatus::MoveToPosition;
					act.TargetVec1 = CurrentMouseHit;
					act.SequenceNumber = SequenceNumber++;
					for (AHeroCharacter* EachHero : CurrentSelection)
					{
						if (CurrentMouseHit != FVector::ZeroVector)
						{
							if (bLeftShiftDown)
							{
								LocalController->ServerAppendHeroAction(EachHero, act);
							}
							else
							{
								LocalController->ServerSetHeroAction(EachHero, act);
							}
						}
					}
					
				}
			}
		}
		break;
		case ERTSStatusEnum::Move:
			break;
		case ERTSStatusEnum::Attack:
		{

		}
		break;
		case ERTSStatusEnum::ThrowEquipment:
			break;
		case ERTSStatusEnum::SkillHint:
		{
			// 取消技能
			if(IsGameRegion(CurrentMouseXY))
			{
				if (CurrentSelection.Num() > 0)
				{
					CurrentSelection[0]->HideSkillHint();
					RTSStatus = ERTSStatusEnum::Normal;
					//localController->AddHeroToMoveQueue(CurrentMouseHit, CurrentSelection);
				}
			}
		}
		default:
			break;
		}
	}
}

void AMHUD::OnRMousePressed1(FVector2D pos)
{
	bClickHero = false;
	ClickStatus = ERTSClickEnum::LastRightClick;
	if(!bMouseRButton)
	{
		bNeedMouseRDown = true;
	}
	bMouseRButton = true;
}

void AMHUD::OnRMousePressed2(FVector2D pos)
{
	if(bNeedMouseRDown)
	{
		bNeedMouseRDown = false;
		OnRMouseDown(pos);
		return;
	}
	bMouseRButton = true;
	// 裝備事件
	for(FMHitBox& HitBox : RTS_HitBoxMap)
	{
		if(HitBox.Contains(pos, ViewportScale))
		{
			RTS_HitBoxRButtonPressed(HitBox.GetName());
			if(HitBox.ConsumesInput())
			{
				break;  //Early out if this box consumed the click
			}
		}
	}
	RTS_MouseRButtonPressed();
}

void AMHUD::OnRMouseReleased(FVector2D pos)
{
	// hitbox用
	for(FMHitBox& HitBox : RTS_HitBoxMap)
	{
		if(HitBox.Contains(pos, ViewportScale))
		{
			RButtonUpHitBox = HitBox.GetName();
			break;
		}
	}
	bMouseRButton = false;
	// 裝備事件
	for(FMHitBox& HitBox : RTS_HitBoxMap)
	{
		if(HitBox.Contains(pos, ViewportScale))
		{
			RTS_HitBoxRButtonReleased(HitBox.GetName());
			if(HitBox.ConsumesInput())
			{
				break;  //Early out if this box consumed the click
			}
		}
	}
	// 如果有點到物品
	if(CurrentSelection.Num() > 0)
	{
		AHeroCharacter* Selection = CurrentSelection[0];
		if(RButtonUpHitBox.Len() > 1 && RButtonUpHitBox == RButtonDownHitBox)
		{
			EquipmentIndex = FCString::Atoi(*RButtonUpHitBox.Right(1)) - 1;
			if(Selection->Equipments[EquipmentIndex])
			{
				RTSStatus = ERTSStatusEnum::ThrowEquipment;
				ThrowTexture = Selection->Equipments[EquipmentIndex]->Head;
			}
			RButtonUpHitBox = RButtonDownHitBox = FString();
		}
	}
	bClickHero = false;
}

void AMHUD::OnLMouseDown(FVector2D pos)
{
	// hitbox用
	for(FMHitBox& HitBox : RTS_HitBoxMap)
	{
		if(HitBox.Contains(pos, ViewportScale))
		{
			LButtonDownHitBox = HitBox.GetName();
			break;
		}
	}
}

void AMHUD::OnLMousePressed1(FVector2D pos)
{
	bClickHero = false;
	ClickStatus = ERTSClickEnum::LastLeftClick;
	if(!bMouseLButton)
	{
		bNeedMouseLDown = true;
	}
	bMouseLButton = true;
}

void AMHUD::OnLMousePressed2(FVector2D pos)
{
	if(bNeedMouseLDown)
	{
		bNeedMouseLDown = false;
		OnLMouseDown(pos);
		// 設定SelectionBox初始位置
		if(RTSStatus == ERTSStatusEnum::Normal)
		{
			if(IsGameRegion(CurrentMouseXY)) // 取消選英雄
			{
				InitialMouseXY = pos;
				if(!ClickedSelected)
				{
					ClearAllSelection();
				}
				ClickedSelected = false;
				UnSelectHero();
			}
		}
		else if(RTSStatus == ERTSStatusEnum::SkillHint) // 放技能
		{
			CurrentSelection[0]->HideSkillHint();
			FHeroAction act;
			act.ActionStatus = EHeroActionStatus::SpellToDirection;
			act.TargetIndex1 = CurrentSelection[0]->GetCurrentSkillIndex();
			act.TargetVec1 = GetCurrentDirection();
			act.TargetVec2 = CurrentMouseHit;
			act.SequenceNumber = SequenceNumber++;
			AMOBAGameState* ags = Cast<AMOBAGameState>(UGameplayStatics::GetGameState(GetWorld()));
			if (bLeftShiftDown)
			{
				LocalController->ServerAppendHeroAction(CurrentSelection[0], act);
			}
			else
			{
				LocalController->ServerSetHeroAction(CurrentSelection[0], act);
			}
			RTSStatus = ERTSStatusEnum::ToNormal;
		}
		return;
	}
	// 顯示技能提示
	if(CurrentSelection.Num() > 0)
	{
		for(FMHitBox& HitBox : RTS_HitBoxMap)
		{
			if(HitBox.GetName().Left(5) == TEXT("Skill"))
			{
				if(HitBox.Contains(pos, ViewportScale))
				{
					int32 idx = FCString::Atoi(*HitBox.GetName().Right(1)) - 1;
					bool res = CurrentSelection[0]->ShowSkillHint(idx);
					if(res)
					{
						RTSStatus = ERTSStatusEnum::SkillHint;
					}
				}
			}
		}
	}
	// 發事件給BP
	for(FMHitBox& HitBox : RTS_HitBoxMap)
	{
		if(HitBox.Contains(pos, ViewportScale))
		{
			RTS_HitBoxLButtonPressed(HitBox.GetName());
			if(HitBox.ConsumesInput())
			{
				break;  //Early out if this box consumed the click
			}
		}
	}
}

void AMHUD::OnLMouseReleased(FVector2D pos)
{
	// hitbox用
	for(FMHitBox& HitBox : RTS_HitBoxMap)
	{
		if(HitBox.Contains(pos, ViewportScale))
		{
			LButtonUpHitBox = HitBox.GetName();
			break;
		}
	}
	bMouseLButton = false;
	// 選英雄
	if(RTSStatus == ERTSStatusEnum::Normal)
	{
		if(IsGameRegion(CurrentMouseXY))
		{
			if(CurrentSelection.Num() > 0)
			{
				SelectedHero(CurrentSelection[0]);
				// 網路連線設定 owner
				//CurrentSelection[0]->SetOwner(localController);
			}
		}
	}
	// 丟物品
	if(RTSStatus == ERTSStatusEnum::ThrowEquipment)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, FString::Printf(TEXT("CurrentSelection.Num %d"),
		                                 CurrentSelection.Num()));
		if (CurrentSelection.Num() > 0)
		{
			FHeroAction act;
			act.ActionStatus = EHeroActionStatus::MoveToThrowEqu;
			act.TargetVec1 = CurrentMouseHit;
			act.TargetIndex1 = EquipmentIndex;
			act.SequenceNumber = SequenceNumber++;
			AMOBAGameState* ags = Cast<AMOBAGameState>(UGameplayStatics::GetGameState(GetWorld()));
			for (AHeroCharacter* EachHero : CurrentSelection)
			{
				if (bLeftShiftDown)
				{
					LocalController->ServerAppendHeroAction(EachHero, act);
				}
				else
				{
					LocalController->ServerSetHeroAction(EachHero, act);
				}
			}
			RTSStatus = ERTSStatusEnum::Normal;
			ThrowTexture = NULL;
		}
	}
	// 發事件給BP
	for(FMHitBox& HitBox : RTS_HitBoxMap)
	{
		if(HitBox.Contains(pos, ViewportScale))
		{
			RTS_HitBoxLButtonReleased(HitBox.GetName());
			if(HitBox.ConsumesInput())
			{
				break;  //Early out if this box consumed the click
			}
		}
	}
	if(RTSStatus == ERTSStatusEnum::ToNormal)
	{
		RTSStatus = ERTSStatusEnum::Normal;
	}
}

void AMHUD::OnSelectedHero(AHeroCharacter* hero)
{
	if (CurrentSelection.Num() > 0 && CurrentSelection[0] == hero)
	{

	}
	else
	{
		for (AHeroCharacter* EachHero : CurrentSelection)
		{
			if (hero != EachHero)
			{
				RemoveSelection.Add(EachHero);
			}
		}
		if (!CurrentSelection.Contains(hero))
		{
			CurrentSelection.Add(hero);
		}
	}
	ClickedSelected = true;
}

