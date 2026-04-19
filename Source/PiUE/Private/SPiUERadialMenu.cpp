// Copyright Solessfir 2026. All Rights Reserved.

#include "SPiUERadialMenu.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "StructUtils/InstancedStruct.h"
#include "PiUEActionDispatcher.h"
#include "PiUESettings.h"
#include "PiUETypes.h"
#include "SPiUERadialPanel.h"
#include "SPiUEWedge.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "PiUE"

void SPiUERadialMenu::Construct(const FArguments& InArgs)
{
	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	MenuRadius = Settings->MenuRadius;
	DeadZoneRadius = Settings->DeadZoneRadius;

	MenuCenterAbsPos = InArgs._MenuCenterAbsPos;

	const TArray<FInstancedStruct>* Root = InArgs._RootItems;
	if (Root != nullptr)
	{
		NavStack.Add(Root);
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
	TransitionCountdown = 0.13f;
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
	WedgeBrushes.Reset();
	HoveredIndex = INDEX_NONE;
	bArcActive = false;
	ArcDisplayAlpha = 0.f;
	CategoryHoverIndex = INDEX_NONE;
	CategoryHoverAccum = 0.f;
	Panel->UpdateArc(0.f, ArcCurrentAngle);

	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	const TArray<FInstancedStruct>& Items = *NavStack.Top();
	const bool bInSubcategory = NavStack.Num() > 1;

	WedgeBrushes.Reserve(Items.Num());
	Panel->SetHasBackSlot(bInSubcategory);

	for (const FInstancedStruct& Item : Items)
	{
		const UScriptStruct* Type = Item.GetScriptStruct();
		if (Type == nullptr || !Type->IsChildOf(FPiUEMenuItemBase::StaticStruct()))
		{
			continue;
		}

		const FPiUEMenuItemBase& Base = Item.Get<FPiUEMenuItemBase>();
		const FLinearColor BaseTint = Base.BackgroundTint.IsSet() ? Base.BackgroundTint.GetValue() : Settings->DefaultWedgeTint;

		FSlateBrush& IconBrush = WedgeBrushes.AddDefaulted_GetRef();
		if (!Base.Icon.IsNull())
		{
			if (UTexture2D* Tex = Base.Icon.LoadSynchronous())
			{
				IconBrush.SetResourceObject(Tex);
				IconBrush.ImageSize = FVector2D(18.f, 18.f);
				IconBrush.TintColor = FSlateColor(Base.IconTint);
			}
		}

		TSharedPtr<SPiUEWedge> Wedge;
		Panel->AddSlot()
		[
			SAssignNew(Wedge, SPiUEWedge)
			.Icon(&IconBrush)
			.Label(Base.Label)
			.BaseTint(BaseTint)
			.bBold(Base.bBold)
		];
		Wedges.Add(Wedge);
	}

	if (bInSubcategory)
	{
		TSharedPtr<SPiUEWedge> BackWedge;
		Panel->AddSlot()
		[
			SAssignNew(BackWedge, SPiUEWedge)
			.Label(LOCTEXT("BackButton", "Back"))
			.BaseTint(Settings->DefaultWedgeTint)
		];
		Wedges.Add(BackWedge);
	}

	for (int32 i = 0; i < Wedges.Num(); ++i)
	{
		const float Angle = Panel->GetSlotAngle(i);
		Wedges[i]->SetEnterDirection(FVector2D(FMath::Sin(Angle), -FMath::Cos(Angle)), MenuRadius);
	}

	Invalidate(EInvalidateWidgetReason::Layout);
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
		ArcCurrentAngle += Delta * FMath::Min(1.f, static_cast<float>(InDeltaTime) * 18.f);
	}
	const float AlphaTarget = bArcActive ? 1.f : 0.f;
	ArcDisplayAlpha += (AlphaTarget - ArcDisplayAlpha) * FMath::Min(1.f, static_cast<float>(InDeltaTime) * 10.f);
	Panel->UpdateArc(ArcDisplayAlpha, ArcCurrentAngle);
}

void SPiUERadialMenu::TryExecuteHoveredAction()
{
	if (!Wedges.IsValidIndex(HoveredIndex))
	{
		return;
	}

	// Back button is last slot in a subcategory - not an action.
	if (NavStack.Num() > 1 && HoveredIndex == Wedges.Num() - 1)
	{
		return;
	}

	const TArray<FInstancedStruct>& Items = *NavStack.Top();
	if (!Items.IsValidIndex(HoveredIndex))
	{
		return;
	}

	const FInstancedStruct& Item = Items[HoveredIndex];
	const UScriptStruct* Type = Item.GetScriptStruct();

	if (Type != nullptr && Type->IsChildOf(FPiUECategoryItem::StaticStruct()))
	{
		return;
	}

	FPiUEActionDispatcher::Execute(Item);
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

	// Back button: last slot in a subcategory.
	const bool bIsBackButton = NavStack.Num() > 1 && HoveredIndex == Wedges.Num() - 1;
	if (bIsBackButton)
	{
		CategoryHoverAccum += DeltaTime;
		if (CategoryHoverAccum >= GetDefault<UPiUESettings>()->CategoryHoverMs / 1000.0)
		{
			CategoryHoverAccum = 0.f;
			CategoryHoverIndex = INDEX_NONE;
			BeginTransition([this]()
			{
				NavStack.Pop();
				RebuildForCurrentLevel();
			});
		}
		return;
	}

	const TArray<FInstancedStruct>& Items = *NavStack.Top();
	if (!Items.IsValidIndex(HoveredIndex))
	{
		return;
	}

	const FInstancedStruct& Item = Items[HoveredIndex];
	const UScriptStruct* Type = Item.GetScriptStruct();
	if (Type == nullptr || !Type->IsChildOf(FPiUECategoryItem::StaticStruct()))
	{
		return;
	}

	CategoryHoverAccum += DeltaTime;
	if (CategoryHoverAccum >= GetDefault<UPiUESettings>()->CategoryHoverMs / 1000.0)
	{
		const int32 NavIndex = HoveredIndex;
		CategoryHoverAccum = 0.f;
		CategoryHoverIndex = INDEX_NONE;
		BeginTransition([this, NavIndex]()
		{
			const TArray<FInstancedStruct>& Items = *NavStack.Top();
			if (!Items.IsValidIndex(NavIndex))
			{
				return;
			}
			const FPiUECategoryItem& Category = Items[NavIndex].Get<FPiUECategoryItem>();
			NavStack.Add(&Category.Children);
			RebuildForCurrentLevel();
		});
	}
}

bool SPiUERadialMenu::ConfirmSelection()
{
	if (NavStack.Num() == 0 || !Wedges.IsValidIndex(HoveredIndex))
	{
		return true;
	}

	// Back wedge is the last slot when in a subcategory.
	if (NavStack.Num() > 1 && HoveredIndex == Wedges.Num() - 1)
	{
		return NavigateBack();
	}

	const TArray<FInstancedStruct>& Items = *NavStack.Top();
	if (!Items.IsValidIndex(HoveredIndex))
	{
		return true;
	}

	const FInstancedStruct& Item = Items[HoveredIndex];
	const UScriptStruct* Type = Item.GetScriptStruct();

	if (Type != nullptr && Type->IsChildOf(FPiUECategoryItem::StaticStruct()))
	{
		const int32 ClickedIndex = HoveredIndex;
		BeginTransition([this, ClickedIndex]()
		{
			const TArray<FInstancedStruct>& Items = *NavStack.Top();
			if (!Items.IsValidIndex(ClickedIndex))
			{
				return;
			}
			const FPiUECategoryItem& Category = Items[ClickedIndex].Get<FPiUECategoryItem>();
			NavStack.Add(&Category.Children);
			RebuildForCurrentLevel();
		});
		return false;
	}

	FPiUEActionDispatcher::Execute(Item);
	return true;
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
