// Copyright Solessfir 2026. All Rights Reserved.

#include "SPiUERadialMenu.h"
#include "Brushes/SlateImageBrush.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "StructUtils/InstancedStruct.h"
#include "PiUEActionDispatcher.h"
#include "PiUESettings.h"
#include "PiUETypes.h"
#include "SPiUERadialPanel.h"
#include "SPiUEWedge.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "PiUE"

EPiUEItemMode SPiUERadialMenu::GetCurrentMode()
{
	return (GEditor && GEditor->IsPlaySessionInProgress()) ? EPiUEItemMode::PIE : EPiUEItemMode::Editor;
}

bool SPiUERadialMenu::ItemPassesFilter(const FInstancedStruct& Item, const EPiUEItemMode Mode)
{
	const UScriptStruct* Type = Item.GetScriptStruct();
	if (!Type || !Type->IsChildOf(FPiUEMenuItemBase::StaticStruct()))
	{
		return false;
	}

	const FPiUEMenuItemBase& Base = Item.Get<FPiUEMenuItemBase>();
	if ((Base.Mode & static_cast<uint8>(Mode)) == 0)
	{
		return false;
	}

	if (Type->IsChildOf(FPiUECategoryItem::StaticStruct()))
	{
		const FPiUECategoryItem& Category = Item.Get<FPiUECategoryItem>();
		return HasAnyVisibleItem(Category.Children, Mode);
	}

	return true;
}

bool SPiUERadialMenu::HasAnyVisibleItem(const TArray<FInstancedStruct>& Items, const EPiUEItemMode Mode)
{
	for (const FInstancedStruct& Item : Items)
	{
		if (ItemPassesFilter(Item, Mode))
		{
			return true;
		}
	}
	return false;
}

TArray<const FInstancedStruct*> SPiUERadialMenu::FilterToPointers(const TArray<FInstancedStruct>& Source) const
{
	TArray<const FInstancedStruct*> Result;
	Result.Reserve(Source.Num());
	for (const FInstancedStruct& Item : Source)
	{
		if (ItemPassesFilter(Item, CurrentMode))
		{
			Result.Add(&Item);
		}
	}
	return Result;
}

void SPiUERadialMenu::Construct(const FArguments& InArgs)
{
	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	MenuRadius = Settings->MenuRadius;
	DeadZoneRadius = Settings->DeadZoneRadius;
	CachedWedgeExitDuration = Settings->WedgeExitDuration;
	CachedArcTrackSpeed = Settings->ArcTrackSpeed;
	CachedArcFadeSpeed = Settings->ArcFadeSpeed;
	CachedCategoryHoverMs = Settings->CategoryHoverMs;

	MenuCenterAbsPos = InArgs._MenuCenterAbsPos;
	CurrentMode = GetCurrentMode();

	if (const TArray<FInstancedStruct>* Root = InArgs._RootItems)
	{
		NavStack.Add(FilterToPointers(*Root));
	}

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(Panel, SPiUERadialPanel).Radius(MenuRadius).HighlightColor(Settings->HighlightWedgeTint)
		]
	];

	RebuildForCurrentLevel();
}

void SPiUERadialMenu::BeginTransition(TFunction<void()> NavAction)
{
	if (bTransitionPending)
	{
		return;
	}

	for (const TSharedPtr<SPiUEWedge>& Wedge : Wedges)
	{
		if (Wedge.IsValid())
		{
			Wedge->SetExiting();
		}
	}

	PendingNavAction = MoveTemp(NavAction);
	TransitionCountdown = CachedWedgeExitDuration / 1000.f;
	bTransitionPending = true;
}

void SPiUERadialMenu::RebuildForCurrentLevel()
{
	if (!Panel.IsValid() || NavStack.Num() == 0)
	{
		return;
	}

	Panel->ClearChildren();
	Wedges.Reset();
	DynamicBrushes.Reset();
	HoveredIndex = INDEX_NONE;
	bArcActive = false;
	ArcDisplayAlpha = 0.f;
	CategoryHoverIndex = INDEX_NONE;
	CategoryHoverAccum = 0.f;
	Panel->UpdateArc(0.f, ArcCurrentAngle);

	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	const TArray<const FInstancedStruct*>& Items = NavStack.Top();
	DynamicBrushes.Reserve(Items.Num());

	for (const FInstancedStruct* Item : Items)
	{
		if (!Item)
		{
			continue;
		}
		const UScriptStruct* Type = Item->GetScriptStruct();
		if (!Type || !Type->IsChildOf(FPiUEMenuItemBase::StaticStruct()))
		{
			continue;
		}
		const FPiUEMenuItemBase& Base = Item->Get<FPiUEMenuItemBase>();
		const FLinearColor BaseTint = Base.BackgroundTint.IsSet() ? Base.BackgroundTint.GetValue() : Settings->DefaultWedgeTint;
		AddWedge(Base, BaseTint);
	}

	for (int32 i = 0; i < Wedges.Num(); ++i)
	{
		const float Angle = Panel->GetSlotAngle(i);
		Wedges[i]->SetEnterDirection(FVector2D(FMath::Sin(Angle), -FMath::Cos(Angle)), MenuRadius);
	}

	Invalidate(EInvalidateWidgetReason::Layout);
}

void SPiUERadialMenu::AddWedge(const FPiUEMenuItemBase& Base, const FLinearColor BaseTint)
{
	const FSlateBrush* IconBrushPtr = nullptr;
	if (!Base.Icon.Path.IsEmpty())
	{
		TUniquePtr<FSlateBrush> Brush;
		if (Base.Icon.Path.EndsWith(TEXT(".svg"), ESearchCase::IgnoreCase))
		{
			Brush = MakeUnique<FSlateVectorImageBrush>(Base.Icon.Path, FVector2D(18.f, 18.f));
		}
		else
		{
			Brush = MakeUnique<FSlateDynamicImageBrush>(FName(*Base.Icon.Path), FVector2D(18.f, 18.f));
		}

		IconBrushPtr = Brush.Get();
		DynamicBrushes.Add(MoveTemp(Brush));
	}

	TSharedPtr<SPiUEWedge> Wedge;
	Panel->AddSlot()
	[
		SAssignNew(Wedge, SPiUEWedge)
		.Icon(IconBrushPtr)
		.Label(Base.Label)
		.BaseTint(BaseTint)
		.bBold(Base.bBold)
	];
	Wedges.Add(Wedge);
}

void SPiUERadialMenu::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bTransitionPending)
	{
		TransitionCountdown -= static_cast<float>(InDeltaTime);
		if (TransitionCountdown <= 0.f)
		{
			bTransitionPending = false;
			if (const TFunction<void()> Action = MoveTemp(PendingNavAction))
			{
				Action();
			}
		}
		return;
	}

	if (!Panel.IsValid())
	{
		return;
	}

	const FVector2D CursorScreen = FSlateApplication::Get().GetCursorPos();
	const int32 NewHover = Panel->GetSlotAtDelta(CursorScreen - MenuCenterAbsPos, DeadZoneRadius);

	if (NewHover != HoveredIndex)
	{
		if (Wedges.IsValidIndex(HoveredIndex))
		{
			Wedges[HoveredIndex]->SetHighlighted(false);
		}

		if (Wedges.IsValidIndex(NewHover))
		{
			Wedges[NewHover]->SetHighlighted(true);
		}

		HoveredIndex = NewHover;

		if (NewHover == INDEX_NONE)
		{
			bArcActive = false;
		}
		else
		{
			ArcTargetAngle = Panel->GetSlotAngle(NewHover);
			bArcActive = true;
		}
	}

	if (bArcActive)
	{
		const float Delta = FMath::FindDeltaAngleRadians(ArcCurrentAngle, ArcTargetAngle);
		ArcCurrentAngle += Delta * FMath::Min(1.f, static_cast<float>(InDeltaTime) * CachedArcTrackSpeed);
	}

	const float AlphaTarget = bArcActive ? 1.f : 0.f;
	ArcDisplayAlpha += (AlphaTarget - ArcDisplayAlpha) * FMath::Min(1.f, static_cast<float>(InDeltaTime) * CachedArcFadeSpeed);
	Panel->UpdateArc(ArcDisplayAlpha, ArcCurrentAngle);
}

void SPiUERadialMenu::TryExecuteHoveredAction()
{
	if (!Wedges.IsValidIndex(HoveredIndex))
	{
		return;
	}

	const TArray<const FInstancedStruct*>& Items = NavStack.Top();
	if (!Items.IsValidIndex(HoveredIndex) || !Items[HoveredIndex])
	{
		return;
	}

	const FInstancedStruct& Item = *Items[HoveredIndex];
	const UScriptStruct* Type = Item.GetScriptStruct();

	if (Type && (Type->IsChildOf(FPiUECloseItem::StaticStruct()) || Type->IsChildOf(FPiUECategoryItem::StaticStruct())))
	{
		return;
	}

	FPiUEActionDispatcher::Execute(Item);
}

void SPiUERadialMenu::TickCloseHover(float DeltaTime)
{
	if (NavStack.Num() <= 1)
	{
		return;
	}

	CategoryHoverAccum += DeltaTime;

	if (CategoryHoverAccum >= CachedCategoryHoverMs / 1000.0)
	{
		CategoryHoverAccum = 0.f;
		CategoryHoverIndex = INDEX_NONE;
		BeginTransition([this]()
		{
			NavStack.Pop();
			RebuildForCurrentLevel();
		});
	}
}

void SPiUERadialMenu::TickCategoryEnterHover(float DeltaTime)
{
	CategoryHoverAccum += DeltaTime;
	if (CategoryHoverAccum >= CachedCategoryHoverMs / 1000.0)
	{
		const int32 NavIndex = HoveredIndex;
		CategoryHoverAccum = 0.f;
		CategoryHoverIndex = INDEX_NONE;
		BeginTransition([this, NavIndex]()
		{
			const TArray<const FInstancedStruct*>& Items = NavStack.Top();
			if (!Items.IsValidIndex(NavIndex) || !Items[NavIndex])
			{
				return;
			}
			const FPiUECategoryItem& Category = Items[NavIndex]->Get<FPiUECategoryItem>();
			NavStack.Add(FilterToPointers(Category.Children));
			RebuildForCurrentLevel();
		});
	}
}

void SPiUERadialMenu::TickCategoryHover(float DeltaTime)
{
	if (HoveredIndex != CategoryHoverIndex)
	{
		CategoryHoverIndex = HoveredIndex;
		CategoryHoverAccum = 0.f;
	}

	if (!Wedges.IsValidIndex(HoveredIndex))
	{
		return;
	}

	const TArray<const FInstancedStruct*>& Items = NavStack.Top();
	if (!Items.IsValidIndex(HoveredIndex) || !Items[HoveredIndex])
	{
		return;
	}

	const UScriptStruct* Type = Items[HoveredIndex]->GetScriptStruct();
	if (!Type)
	{
		return;
	}

	if (Type->IsChildOf(FPiUECloseItem::StaticStruct()))
	{
		TickCloseHover(DeltaTime);
	}
	else if (Type->IsChildOf(FPiUECategoryItem::StaticStruct()))
	{
		TickCategoryEnterHover(DeltaTime);
	}
}

bool SPiUERadialMenu::ConfirmSelection()
{
	if (NavStack.Num() == 0 || !Wedges.IsValidIndex(HoveredIndex))
	{
		return true;
	}

	const TArray<const FInstancedStruct*>& Items = NavStack.Top();
	if (!Items.IsValidIndex(HoveredIndex) || !Items[HoveredIndex])
	{
		return true;
	}

	const FInstancedStruct& Item = *Items[HoveredIndex];
	const UScriptStruct* Type = Item.GetScriptStruct();

	if (Type && Type->IsChildOf(FPiUECloseItem::StaticStruct()))
	{
		return NavigateBack();
	}

	if (Type && Type->IsChildOf(FPiUECategoryItem::StaticStruct()))
	{
		const int32 ClickedIndex = HoveredIndex;
		BeginTransition([this, ClickedIndex]()
		{
			const TArray<const FInstancedStruct*>& InnerItems = NavStack.Top();
			if (!InnerItems.IsValidIndex(ClickedIndex) || !InnerItems[ClickedIndex])
			{
				return;
			}
			const FPiUECategoryItem& Category = InnerItems[ClickedIndex]->Get<FPiUECategoryItem>();
			NavStack.Add(FilterToPointers(Category.Children));
			RebuildForCurrentLevel();
		});
		return false;
	}

	FPiUEActionDispatcher::Execute(Item);
	return true;
}

int32 SPiUERadialMenu::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// Canvas geometry is zero on the first paint (widget not yet laid out). Skip to suppress the one-frame position flash.
	if (!bFirstPaintDone)
	{
		bFirstPaintDone = true;
		return LayerId;
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

bool SPiUERadialMenu::NavigateBack()
{
	if (NavStack.Num() <= 1)
	{
		return true;
	}

	BeginTransition([this]()
	{
		NavStack.Pop();
		RebuildForCurrentLevel();
	});
	return false;
}

#undef LOCTEXT_NAMESPACE
