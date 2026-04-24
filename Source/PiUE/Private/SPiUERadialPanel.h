// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SPanel.h"
#include "Layout/Children.h"

/**
 * Lays out child widgets radially around a center point (angle from "up", clockwise).
 *  N == 1: [Top], N == 2: [Right, Left], N == 3: [Top, BR, BL], N == 4: cardinal, N >= 5: equal step
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
	void SetRadius(const float InRadius);

	/** Updates the animated arc. InAlpha 0=hidden 1=full; InAngle is radians (0=up, clockwise). */
	void UpdateArc(const float InAlpha, const float InAngle);

	/** Returns the center angle (radians, 0=up, CW) for the given slot index. */
	float GetSlotAngle(const int32 SlotIndex) const;

	/**
	 * Returns the slot index for a cursor delta (cursor minus menu center, in screen space). INDEX_NONE if inside dead zone.
	 * Does not use cached geometry - safe to call before first layout pass.
	 */
	int32 GetSlotAtDelta(const FVector2D& CursorDelta, const float DeadZoneRadius) const;

	/** Computes the local-space anchor point for a given slot index (used for hit-testing and painting). */
	FVector2D GetSlotAnchor(const int32 SlotIndex) const;

	/** Extra space beyond the ring radius reserved so wedges at the extremes are not clipped. Shared with overlay sizing. */
	static constexpr float WedgePadding = 80.f;

	// Begin SWidget interface
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FChildren* GetChildren() override { return &Children; }
	// End SWidget interface

private:
	static float ComputeSlotAngle(const int32 SlotIndex, const int32 SlotCount);

	TPanelChildren<FSlot> Children;
	float Radius = 120.f;
	float ArcAngle = 0.f;
	float ArcAlpha = 0.f;
	FLinearColor HighlightColor = FLinearColor(0.1f, 0.5f, 0.9f, 0.95f);

	mutable TArray<FVector2D> CachedRingPoints;
	mutable TArray<FVector2D> CachedArcPoints;
};
