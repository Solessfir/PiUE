// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"

struct FInstancedStruct;
class SPiUERadialPanel;
class SPiUEWedge;

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
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

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
	/** Rebuilds the radial panel from the items on top of the stack. */
	void RebuildForCurrentLevel();

	/** Marks current wedges exiting, waits for TransitionCountdown, then runs NavAction and rebuilds. */
	void BeginTransition(TFunction<void()> NavAction);

	/** Navigation stack - each entry points at the items array for a level. */
	TArray<const TArray<FInstancedStruct>*> NavStack;

	TSharedPtr<SPiUERadialPanel> Panel;
	TArray<TSharedPtr<SPiUEWedge>> Wedges;
	TArray<TUniquePtr<FSlateBrush>> DynamicBrushes;

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
};
