// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PiUETypes.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"

class SPiUERadialPanel;
class SPiUEWedge;
struct FInstancedStruct;
struct FPiUEMenuItemBase;
struct FPiUEOpenTabsItem;

/** What kind of action a wedge dispatches when confirmed. Drives selection logic in the menu. */
enum class EPiUEWedgeKind : uint8
{
	Leaf,      // run Action and close
	Category,  // navigate into CategoryChildren on enter
	Close      // navigate back one level
};

/**
* Self-contained wedge description: everything the menu needs to render and dispatch one slot.
* Built per-rebuild from either a source FInstancedStruct (static items) or an Open Tabs spread (dynamic).
*/
struct FPiUEWedgeEntry
{
	FText Label;
	const FSlateBrush* Icon = nullptr;
	bool bBold = false;
	FLinearColor Tint = FLinearColor::Black;
	EPiUEWedgeKind Kind = EPiUEWedgeKind::Leaf;

	/** Invoked on confirm for Leaf wedges. Empty for Category / Close. */
	TFunction<void()> Action;

	/** Source children array for Category wedges. Stable pointer into Settings CDO; nullptr otherwise. */
	const TArray<FInstancedStruct>* CategoryChildren = nullptr;

	/** Category-only: when true, this wedge's Label is pushed as the menu title on enter. */
	bool bUseLabelAsTitle = false;
};

/**
* Root widget for the PiUE radial menu. Hosts a radial panel of wedges.
* Navigates into/out of category items without destroying the host window.
*/
class SPiUERadialMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPiUERadialMenu) {}
		/** Items shown at the root level of the menu. Usually UPiUESettings::RootItems. */
		SLATE_ARGUMENT(const TArray<FInstancedStruct>*, RootItems)
		/** Absolute screen position of the cursor when the menu was opened. Used as the dead-zone center. */
		SLATE_ARGUMENT(FVector2D, MenuCenterAbsPos)
		/** Title drawn above the small center ring at root level. Sub-rings show the entered category's Label instead. Empty = no title. */
		SLATE_ARGUMENT(FText, RootTitle)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Returns the current execution mode (Editor or PIE), used for filtering items at summon time. */
	static EPiUEItemMode GetCurrentMode();

	/** Returns true if the item is visible in the given mode. Categories pass only if at least one descendant passes. */
	static bool ItemPassesFilter(const FInstancedStruct& Item, EPiUEItemMode Mode);

	/** Returns true if any item in the array (recursively, for categories) is visible in the given mode. */
	static bool HasAnyVisibleItem(const TArray<FInstancedStruct>& Items, EPiUEItemMode Mode);

	/** Confirms the currently highlighted wedge. Enters categories, dispatches leaf items, returns true if the menu should close. */
	bool ConfirmSelection();

	/** Navigates one level up. Returns true if the menu should close (already at root). */
	bool NavigateBack();

	/** Executes the hovered wedge only if it is a leaf action. No-op for categories, back button, and dead zone. */
	void TryExecuteHoveredAction();

	/** Accumulates hover time on a category wedge and navigates into it after the threshold. Only active in hold mode. */
	void TickCategoryHover(float DeltaTime);

	// Begin SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	// End SWidget interface

private:
	/** Rebuilds the radial panel from the source items on top of the stack. */
	void RebuildForCurrentLevel();

	/** Marks current wedges exiting, waits for TransitionCountdown, then runs NavAction and rebuilds. */
	void BeginTransition(TFunction<void()> NavAction);

	/** Accumulates hover time on a Close wedge and triggers navigate-back on threshold. */
	void TickCloseHover(float DeltaTime);

	/** Accumulates hover time on a Category wedge and triggers navigate-in on threshold. */
	void TickCategoryEnterHover(float DeltaTime);

	/**
	* Builds the wedge list for one menu level from a source items array.
	* Filters by current mode, expands FPiUEOpenTabsItem inline, and converts each surviving item into an FPiUEWedgeEntry.
	* Allocates dynamic brushes (for Icon SVG / dynamic image paths) into DynamicBrushes for the lifetime of this level.
	*/
	void BuildLevelEntries(const TArray<FInstancedStruct>& Source, TArray<FPiUEWedgeEntry>& OutEntries);

	/** Appends Level + open-tab wedges generated from an Open Tabs spread to OutEntries. */
	static void ExpandOpenTabs(const FPiUEOpenTabsItem& Spread, TArray<FPiUEWedgeEntry>& OutEntries);

	/** Resolves an icon brush for a base item (runtime override, then dynamic SVG / image, then null). */
	const FSlateBrush* ResolveItemIcon(const FPiUEMenuItemBase& Base);

	/** Adds a wedge widget to the panel using the entry's visual properties. */
	void AddWedgeForEntry(const FPiUEWedgeEntry& Entry);

	/** Navigation stack of source pointers. Each level points to a stable TArray<FInstancedStruct> (Settings CDO root items or a Category's Children). */
	TArray<const TArray<FInstancedStruct>*> NavStack;

	/** Title for each nav level. Index 0 = ring title from settings, deeper levels = entered category's Label. Parallel to NavStack. */
	TArray<FText> NavTitles;

	/** Wedge entries for the currently displayed level. Rebuilt every RebuildForCurrentLevel. */
	TArray<FPiUEWedgeEntry> CurrentEntries;

	/** Snapshot of execution mode at menu open. Drives item visibility filtering. */
	EPiUEItemMode CurrentMode = EPiUEItemMode::Editor;

	TSharedPtr<SPiUERadialPanel> Panel;
	TArray<TSharedPtr<SPiUEWedge>> Wedges;
	TArray<TUniquePtr<FSlateBrush>> DynamicBrushes;

	/** Per-level memo: icon path -> brush already allocated into DynamicBrushes. Avoids redundant allocation when the same icon repeats across wedges in one level. */
	TMap<FString, const FSlateBrush*> IconBrushCache;

	/** Index of the wedge currently under the cursor (INDEX_NONE = dead zone). */
	int32 HoveredIndex = INDEX_NONE;

	float ArcCurrentAngle = 0.f;
	float ArcTargetAngle = 0.f;
	float ArcDisplayAlpha = 0.f;
	bool bArcActive = false;

	int32 CategoryHoverIndex = INDEX_NONE;
	float CategoryHoverAccum = 0.f;

	/** Absolute screen position of the cursor at menu open. Center reference for dead-zone, bypasses stale geometry. */
	FVector2D MenuCenterAbsPos = FVector2D::ZeroVector;

	mutable bool bFirstPaintDone = false;
	bool bTransitionPending = false;
	float TransitionCountdown = 0.f;
	TFunction<void()> PendingNavAction;

	/** Cached settings snapshot values to avoid repeated CDO access. */
	float MenuRadius = 120.f;
	float DeadZoneRadius = 25.f;
	float MenuScale = 1.f;
	float CachedWedgeExitDuration = 130.f;
	float CachedArcTrackSpeed = 18.f;
	float CachedArcFadeSpeed = 10.f;
	double CachedCategoryHoverMs = 1000.0;
};
