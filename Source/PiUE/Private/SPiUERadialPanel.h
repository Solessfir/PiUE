// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SPanel.h"
#include "Layout/Children.h"

/**
 * Lays out child widgets radially around a center point (angle from "up", clockwise).
 *  Normal: N == 1: [Top], N == 2: [Right, Left], N == 3: [Top, BR, BL], N == 4: cardinal, N >= 5: equal step
 *  Back-slot mode: last slot fixed at PI (6 o'clock); remaining N-1 slots arc evenly centered at top.
 */
class SPiUERadialPanel : public SPanel
{
public:
	/** Individual panel slot. */
	class FSlot : public TSlotBase<FSlot>
	{
	public:
		FSlot()
			: TSlotBase<FSlot>()
		{}
	};

	using FScopedWidgetSlotArguments = typename TPanelChildren<FSlot>::FScopedWidgetSlotArguments;

	SLATE_BEGIN_ARGS(SPiUERadialPanel)
		: _Radius(120.f)
		, _HighlightColor(FLinearColor(0.1f, 0.5f, 0.9f, 0.95f))
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}
		SLATE_ARGUMENT(float, Radius)
		SLATE_ARGUMENT(FLinearColor, HighlightColor)
		SLATE_SLOT_ARGUMENT(FSlot, Slots)
	SLATE_END_ARGS()

	SPiUERadialPanel();

	void Construct(const FArguments& InArgs);

	/** Adds a new slot. Returns its index in the children list. */
	FScopedWidgetSlotArguments AddSlot();

	/** Removes all slots. */
	void ClearChildren();

	/** Sets the ring radius in local-space pixels. */
	void SetRadius(float InRadius);

	/** When true, last slot is fixed at PI (6 o'clock) as a back button. Triggers layout invalidation. */
	void SetHasBackSlot(bool bValue);

	/** Updates the animated arc. InAlpha 0=hidden 1=full; InAngle is radians (0=up, clockwise). */
	void UpdateArc(float InAlpha, float InAngle);

	/** Returns the center angle (radians, 0=up, CW) for the given slot index. */
	float GetSlotAngle(int32 SlotIndex) const;

	/**
	 * Returns the slot index for a cursor delta (cursor minus menu center, in screen space). INDEX_NONE if inside dead zone.
	 * Does not use cached geometry - safe to call before first layout pass.
	 */
	int32 GetSlotAtDelta(const FVector2D& CursorDelta, float DeadZoneRadius) const;

	/** Computes the local-space anchor point for a given slot index (used for hit-testing and painting). */
	FVector2D GetSlotAnchor(int32 SlotIndex) const;

	// Begin SWidget interface
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FChildren* GetChildren() override { return &Children; }
	// End SWidget interface

private:
	static float ComputeSlotAngle(int32 SlotIndex, int32 SlotCount, bool bHasBackSlot);

	TPanelChildren<FSlot> Children;
	float Radius = 120.f;
	float ArcAngle = 0.f;
	float ArcAlpha = 0.f;
	bool bHasBackSlot = false;
	FLinearColor HighlightColor = FLinearColor(0.1f, 0.5f, 0.9f, 0.95f);
};
