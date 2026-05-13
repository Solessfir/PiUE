// Copyright Solessfir 2026. All Rights Reserved.

#include "SPiUERadialMenu.h"
#include "Brushes/SlateImageBrush.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/SlateIconFinder.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "PiUEActionDispatcher.h"
#include "PiUESettings.h"
#include "PiUETypes.h"
#include "SPiUERadialPanel.h"
#include "SPiUEWedge.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "PiUE"

namespace
{
	constexpr const TCHAR* LevelEditorTabId = TEXT("LevelEditor");

	// Tab ids of standalone editor tabs we surface as wedges (no associated UObject asset).
	const TSet<FName>& GetSettingsAllowList()
	{
		static const TSet<FName> AllowList = {
			FName(TEXT("ProjectSettings")),
			FName(TEXT("EditorSettings"))
		};
		return AllowList;
	}

	// One MRU-sortable candidate built before sort/trim.
	// Holds everything needed to render and dispatch a wedge plus the engine-tracked activation time used for ordering.
	struct FTabCandidate
	{
		FText Label;
		const FSlateBrush* Icon = nullptr;
		double LastActivationTime = 0.0;
		TFunction<void()> Action;
	};
}

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

const FSlateBrush* SPiUERadialMenu::ResolveItemIcon(const FPiUEMenuItemBase& Base)
{
	if (Base.Icon.Path.IsEmpty())
	{
		return nullptr;
	}

	// Memoized within the current level so repeated icon paths share a single FSlateVectorImageBrush.
	if (const FSlateBrush* const* Existing = IconBrushCache.Find(Base.Icon.Path))
	{
		return *Existing;
	}

	// Icon picker only enumerates engine .svg resources; FSlateVectorImageBrush is the only relevant type.
	TUniquePtr<FSlateBrush> Brush = MakeUnique<FSlateVectorImageBrush>(Base.Icon.Path, FVector2D(18.f, 18.f));
	const FSlateBrush* Ptr = Brush.Get();
	DynamicBrushes.Add(MoveTemp(Brush));
	IconBrushCache.Add(Base.Icon.Path, Ptr);
	return Ptr;
}

void SPiUERadialMenu::ExpandOpenTabs(const FPiUEOpenTabsItem& Spread, TArray<FPiUEWedgeEntry>& OutEntries)
{
	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	const FLinearColor SpreadTint = Spread.BackgroundTint.IsSet() ? Spread.BackgroundTint.GetValue() : Settings->DefaultWedgeTint;

	const int32 ClampedMax = FMath::Clamp(Spread.MaxCount, 1, 12);
	const int32 TabBudget = ClampedMax - 1;  // one slot reserved for the Level wedge.

	// Mirror the engine's Ctrl+Tab dialog (SGlobalTabSwitchingDialog): enumerate currently-open asset
	// editors via the subsystem, sort by IAssetEditorInstance::GetLastActivationTime(). Settings tabs
	// (Project Settings, Editor Preferences) are added if currently live, sorted alongside by tab
	// activation time. Closed tabs naturally drop out.
	TArray<FTabCandidate> Candidates;

	if (UAssetEditorSubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
	{
		for (UObject* Asset : Subsystem->GetAllEditedAssets())
		{
			if (!Asset || Asset->GetOuter() == GetTransientPackage())
			{
				continue;
			}
			IAssetEditorInstance* Editor = Subsystem->FindEditorForAsset(Asset, false);
			if (!Editor)
			{
				continue;
			}

			FTabCandidate Candidate;
			Candidate.Label = FText::AsCultureInvariant(Asset->GetName());
			Candidate.Icon = Spread.bShowIcons ? FSlateIconFinder::FindIconBrushForClass(Asset->GetClass()) : nullptr;
			Candidate.LastActivationTime = Editor->GetLastActivationTime();

			const FSoftObjectPath AssetPath(Asset);
			Candidate.Action = [AssetPath]()
			{
				UAssetEditorSubsystem* Sub = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
				if (!Sub)
				{
					return;
				}
				UObject* Live = AssetPath.ResolveObject();
				if (!Live)
				{
					Live = AssetPath.TryLoad();
				}
				if (Live)
				{
					// FindEditorForAsset with bFocusIfOpen=true focuses the existing toolkit. If the editor
					// was closed since the menu was summoned, falls through to OpenEditorForAsset to re-open.
					if (!Sub->FindEditorForAsset(Live, true))
					{
						Sub->OpenEditorForAsset(Live);
					}
				}
			};
			Candidates.Add(MoveTemp(Candidate));
		}
	}

	for (const FName SettingsTabId : GetSettingsAllowList())
	{
		const TSharedPtr<SDockTab> LiveTab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(SettingsTabId));
		if (!LiveTab.IsValid())
		{
			continue;
		}

		FTabCandidate Candidate;
		Candidate.Label = LiveTab->GetTabLabel();
		Candidate.Icon = Spread.bShowIcons ? LiveTab->GetTabIcon() : nullptr;
		Candidate.LastActivationTime = LiveTab->GetLastActivationTime();
		const FName CapturedId = SettingsTabId;
		Candidate.Action = [CapturedId]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId(CapturedId));
		};
		Candidates.Add(MoveTemp(Candidate));
	}

	// Sort newest-first by engine-tracked activation time, then trim to budget.
	Candidates.Sort([](const FTabCandidate& A, const FTabCandidate& B) { return A.LastActivationTime > B.LastActivationTime; });
	if (TabBudget >= 0 && Candidates.Num() > TabBudget)
	{
		Candidates.SetNum(TabBudget, EAllowShrinking::No);
	}

	auto MakeWedgeFromCandidate = [&Spread, &SpreadTint](FTabCandidate& Source)
	{
		FPiUEWedgeEntry Entry;
		Entry.Label = MoveTemp(Source.Label);
		Entry.Icon = Source.Icon;
		Entry.bBold = Spread.bBold;
		Entry.Tint = SpreadTint;
		Entry.Kind = EPiUEWedgeKind::Leaf;
		Entry.Action = MoveTemp(Source.Action);
		return Entry;
	};

	auto MakeLevelEntry = [&Spread, &SpreadTint]()
	{
		FPiUEWedgeEntry Entry;

		// Use the current level's map name when available; fall back to a generic label.
		FText LevelLabel = NSLOCTEXT("PiUE", "LevelWedgeFallback", "Level");
		if (GEditor)
		{
			if (UWorld* World = GEditor->GetEditorWorldContext().World())
			{
				FString MapName = World->GetMapName();
				MapName.RemoveFromStart(World->StreamingLevelsPrefix);
				if (!MapName.IsEmpty())
				{
					LevelLabel = FText::FromString(MapName);
				}
			}
		}

		Entry.Label = LevelLabel;
		Entry.Icon = Spread.bShowIcons ? FSlateIconFinder::FindIconBrushForClass(UWorld::StaticClass()) : nullptr;
		Entry.bBold = Spread.bBold;
		Entry.Tint = SpreadTint;
		Entry.Kind = EPiUEWedgeKind::Leaf;
		Entry.Action = []()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId(LevelEditorTabId));
		};
		return Entry;
	};

	if (Spread.bReversed)
	{
		for (int32 i = Candidates.Num() - 1; i >= 0; --i)
		{
			OutEntries.Add(MakeWedgeFromCandidate(Candidates[i]));
		}
		OutEntries.Add(MakeLevelEntry());
	}
	else
	{
		OutEntries.Add(MakeLevelEntry());
		for (FTabCandidate& Source : Candidates)
		{
			OutEntries.Add(MakeWedgeFromCandidate(Source));
		}
	}
}

void SPiUERadialMenu::BuildLevelEntries(const TArray<FInstancedStruct>& Source, TArray<FPiUEWedgeEntry>& OutEntries)
{
	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	OutEntries.Reserve(Source.Num());

	for (const FInstancedStruct& Item : Source)
	{
		if (!ItemPassesFilter(Item, CurrentMode))
		{
			continue;
		}

		const UScriptStruct* Type = Item.GetScriptStruct();

		if (Type->IsChildOf(FPiUEOpenTabsItem::StaticStruct()))
		{
			ExpandOpenTabs(Item.Get<FPiUEOpenTabsItem>(), OutEntries);
			continue;
		}

		const FPiUEMenuItemBase& Base = Item.Get<FPiUEMenuItemBase>();

		FPiUEWedgeEntry Entry;
		Entry.Label = Base.Label;
		Entry.Icon = ResolveItemIcon(Base);
		Entry.bBold = Base.bBold;
		Entry.Tint = Base.BackgroundTint.IsSet() ? Base.BackgroundTint.GetValue() : Settings->DefaultWedgeTint;

		if (Type->IsChildOf(FPiUECloseItem::StaticStruct()))
		{
			Entry.Kind = EPiUEWedgeKind::Close;
		}
		else if (Type->IsChildOf(FPiUECategoryItem::StaticStruct()))
		{
			const FPiUECategoryItem& Category = Item.Get<FPiUECategoryItem>();
			Entry.Kind = EPiUEWedgeKind::Category;
			Entry.CategoryChildren = &Category.Children;
			Entry.bUseLabelAsTitle = Category.bUseLabelAsTitle;
		}
		else
		{
			Entry.Kind = EPiUEWedgeKind::Leaf;
			const FInstancedStruct* ItemPtr = &Item;
			Entry.Action = [ItemPtr]() { FPiUEActionDispatcher::Execute(*ItemPtr); };
		}

		OutEntries.Add(MoveTemp(Entry));
	}
}

void SPiUERadialMenu::Construct(const FArguments& InArgs)
{
	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	MenuRadius = Settings->MenuRadius;
	DeadZoneRadius = Settings->DeadZoneRadius;
	MenuScale = Settings->MenuScale;
	CachedWedgeExitDuration = Settings->WedgeExitDuration;
	CachedArcTrackSpeed = Settings->ArcTrackSpeed;
	CachedArcFadeSpeed = Settings->ArcFadeSpeed;
	CachedCategoryHoverMs = Settings->CategoryHoverMs;

	MenuCenterAbsPos = InArgs._MenuCenterAbsPos;
	CurrentMode = GetCurrentMode();

	if (const TArray<FInstancedStruct>* Root = InArgs._RootItems)
	{
		NavStack.Add(Root);
		NavTitles.Add(InArgs._RootTitle);
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

	// Uniform visual scale around the menu center. Layout (Panel.Radius, wedge sizes) stays unscaled;
	// hit math compensates by dividing the cursor delta by MenuScale in Tick.
	if (!FMath::IsNearlyEqual(MenuScale, 1.f))
	{
		SetRenderTransform(FSlateRenderTransform(FScale2D(MenuScale)));
		SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}

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

void SPiUERadialMenu::AddWedgeForEntry(const FPiUEWedgeEntry& Entry)
{
	TSharedPtr<SPiUEWedge> Wedge;
	Panel->AddSlot()
	[
		SAssignNew(Wedge, SPiUEWedge)
		.Icon(Entry.Icon)
		.Label(Entry.Label)
		.BaseTint(Entry.Tint)
		.bBold(Entry.bBold)
	];
	Wedges.Add(Wedge);
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
	IconBrushCache.Reset();
	CurrentEntries.Reset();
	HoveredIndex = INDEX_NONE;
	bArcActive = false;
	ArcDisplayAlpha = 0.f;
	CategoryHoverIndex = INDEX_NONE;
	CategoryHoverAccum = 0.f;
	Panel->UpdateArc(0.f, ArcCurrentAngle);
	Panel->SetTitle(NavTitles.Num() > 0 ? NavTitles.Top() : FText::GetEmpty());

	const TArray<FInstancedStruct>* Source = NavStack.Top();
	if (!Source)
	{
		return;
	}

	BuildLevelEntries(*Source, CurrentEntries);

	for (const FPiUEWedgeEntry& Entry : CurrentEntries)
	{
		AddWedgeForEntry(Entry);
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

	// Visuals are render-scaled; cursor positions are in unscaled screen space, so divide the delta to compare against unscaled radii.
	const float InvScale = MenuScale > 0.f ? 1.f / MenuScale : 1.f;
	const int32 NewHover = Panel->GetSlotAtDelta((CursorScreen - MenuCenterAbsPos) * InvScale, DeadZoneRadius);

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
	if (!CurrentEntries.IsValidIndex(HoveredIndex))
	{
		return;
	}

	const FPiUEWedgeEntry& Entry = CurrentEntries[HoveredIndex];
	if (Entry.Kind == EPiUEWedgeKind::Leaf && Entry.Action)
	{
		Entry.Action();
	}
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
			if (NavTitles.Num() > 0)
			{
				NavTitles.Pop();
			}
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

		if (!CurrentEntries.IsValidIndex(NavIndex))
		{
			return;
		}
		const TArray<FInstancedStruct>* Children = CurrentEntries[NavIndex].CategoryChildren;
		if (!Children)
		{
			return;
		}
		const FText EnteredTitle = CurrentEntries[NavIndex].bUseLabelAsTitle ? CurrentEntries[NavIndex].Label : FText::GetEmpty();

		BeginTransition([this, Children, EnteredTitle]()
		{
			NavStack.Add(Children);
			NavTitles.Add(EnteredTitle);
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

	if (!CurrentEntries.IsValidIndex(HoveredIndex))
	{
		return;
	}

	switch (CurrentEntries[HoveredIndex].Kind)
	{
		case EPiUEWedgeKind::Close:    TickCloseHover(DeltaTime); break;
		case EPiUEWedgeKind::Category: TickCategoryEnterHover(DeltaTime); break;
		default: break;
	}
}

bool SPiUERadialMenu::ConfirmSelection()
{
	if (NavStack.Num() == 0 || !CurrentEntries.IsValidIndex(HoveredIndex))
	{
		return true;
	}

	const FPiUEWedgeEntry& Entry = CurrentEntries[HoveredIndex];

	if (Entry.Kind == EPiUEWedgeKind::Close)
	{
		return NavigateBack();
	}

	if (Entry.Kind == EPiUEWedgeKind::Category)
	{
		const TArray<FInstancedStruct>* Children = Entry.CategoryChildren;
		const FText EnteredTitle = Entry.bUseLabelAsTitle ? Entry.Label : FText::GetEmpty();
		BeginTransition([this, Children, EnteredTitle]()
		{
			if (!Children)
			{
				return;
			}
			NavStack.Add(Children);
			NavTitles.Add(EnteredTitle);
			RebuildForCurrentLevel();
		});
		return false;
	}

	if (Entry.Action)
	{
		Entry.Action();
	}
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
		if (NavTitles.Num() > 0)
		{
			NavTitles.Pop();
		}
		RebuildForCurrentLevel();
	});
	return false;
}

#undef LOCTEXT_NAMESPACE
