// Copyright Solessfir 2026. All Rights Reserved.

#include "SPiUERadialPanel.h"
#include "Layout/ArrangedChildren.h"
#include "Rendering/DrawElements.h"

SPiUERadialPanel::SPiUERadialPanel()
	: Children(this)
{
}

void SPiUERadialPanel::Construct(const FArguments& InArgs)
{
	Radius = InArgs._Radius;
	HighlightColor = InArgs._HighlightColor;
	SetVisibility(EVisibility::SelfHitTestInvisible);
	Children.AddSlots(MoveTemp(const_cast<TArray<FSlot::FSlotArguments>&>(InArgs._Slots)));
}

SPiUERadialPanel::FScopedWidgetSlotArguments SPiUERadialPanel::AddSlot()
{
	return FScopedWidgetSlotArguments{MakeUnique<FSlot>(), Children, INDEX_NONE};
}

void SPiUERadialPanel::ClearChildren()
{
	Children.Empty();
}

void SPiUERadialPanel::SetRadius(const float InRadius)
{
	Radius = InRadius;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SPiUERadialPanel::UpdateArc(const float InAlpha, const float InAngle)
{
	if (ArcAlpha == InAlpha && ArcAngle == InAngle)
	{
		return;
	}
	ArcAlpha = InAlpha;
	ArcAngle = InAngle;
	Invalidate(EInvalidateWidgetReason::Paint);
}

float SPiUERadialPanel::GetSlotAngle(const int32 SlotIndex) const
{
	return ComputeSlotAngle(SlotIndex, Children.Num());
}

float SPiUERadialPanel::ComputeSlotAngle(const int32 SlotIndex, const int32 SlotCount)
{
	if (SlotCount <= 0)
	{
		return 0.f;
	}

	switch (SlotCount)
	{
	case 1:
		return 0.f;

	case 2:
		return (SlotIndex == 0) ? HALF_PI : -HALF_PI;

	case 3:
		{
			constexpr float Angles3[3] = {0.f, 2.f * PI / 3.f, -2.f * PI / 3.f};
			return Angles3[SlotIndex % 3];
		}

	case 4:
		return static_cast<float>(SlotIndex) * HALF_PI;

	default:
		return (2.f * PI / static_cast<float>(SlotCount)) * static_cast<float>(SlotIndex);
	}
}

FVector2D SPiUERadialPanel::GetSlotAnchor(const int32 SlotIndex) const
{
	const int32 NumSlots = Children.Num();
	if (NumSlots <= 0)
	{
		return FVector2D::ZeroVector;
	}

	const float Angle = ComputeSlotAngle(SlotIndex, NumSlots);
	return FVector2D(FMath::Sin(Angle) * Radius, -FMath::Cos(Angle) * Radius);
}

int32 SPiUERadialPanel::GetSlotAtDelta(const FVector2D& CursorDelta, const float DeadZoneRadius) const
{
	const int32 NumSlots = Children.Num();
	if (NumSlots <= 0 || CursorDelta.SizeSquared() < DeadZoneRadius * DeadZoneRadius)
	{
		return INDEX_NONE;
	}

	const float CursorAngle = FMath::Atan2(CursorDelta.X, -CursorDelta.Y);
	int32 Best = INDEX_NONE;
	float BestDelta = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < NumSlots; ++Index)
	{
		const float Diff = FMath::Abs(FMath::FindDeltaAngleRadians(ComputeSlotAngle(Index, NumSlots), CursorAngle));
		if (Diff < BestDelta)
		{
			BestDelta = Diff;
			Best = Index;
		}
	}
	return Best;
}

FVector2D SPiUERadialPanel::ComputeDesiredSize(float) const
{
	const float Side = (Radius + WedgePadding) * 2.f;
	return FVector2D(Side, Side);
}

int32 SPiUERadialPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 ChildLayer = SPanel::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Center = LocalSize * 0.5f;
	const FPaintGeometry PaintGeo = AllottedGeometry.ToPaintGeometry(LocalSize, FSlateLayoutTransform());
	constexpr float RingRadius = 14.f;
	constexpr int32 RingSegments = 48;

	// Overlap 2 segments past start so antialiased line caps are buried under existing geometry, eliminating the seam.
	CachedRingPoints.Reset();
	CachedRingPoints.Reserve(RingSegments + 3);
	for (int32 i = 0; i <= RingSegments + 1; ++i)
	{
		const float A = (2.f * PI * i) / RingSegments;
		CachedRingPoints.Add(Center + FVector2D(FMath::Sin(A) * RingRadius, -FMath::Cos(A) * RingRadius));
	}
	FSlateDrawElement::MakeLines(OutDrawElements, ChildLayer + 1, PaintGeo, CachedRingPoints, ESlateDrawEffect::None, FLinearColor(0.15f, 0.15f, 0.15f, 0.9f), true, 4.5f);

	// Highlight arc at the animated angle, faded by ArcAlpha.
	const int32 NumSlots = Children.Num();
	if (ArcAlpha > 0.01f && NumSlots > 0)
	{
		const float HalfArc = NumSlots > 1 ? (PI / static_cast<float>(NumSlots)) : PI;
		constexpr int32 ArcSegments = 24;

		FLinearColor ArcColor = HighlightColor;
		ArcColor.A *= ArcAlpha;

		// Full-circle arc (single slot): overlap 2 segments past start to bury end cap under existing geometry.
		const bool bFullCircle = HalfArc >= PI;
		const int32 ArcOverlap = bFullCircle ? 2 : 0;
		CachedArcPoints.Reset();
		CachedArcPoints.Reserve(ArcSegments + 1 + ArcOverlap);
		for (int32 i = 0; i <= ArcSegments + ArcOverlap; ++i)
		{
			const float A = FMath::Lerp(ArcAngle - HalfArc, ArcAngle + HalfArc, static_cast<float>(i) / ArcSegments);
			CachedArcPoints.Add(Center + FVector2D(FMath::Sin(A) * RingRadius, -FMath::Cos(A) * RingRadius));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, ChildLayer + 2, PaintGeo, CachedArcPoints, ESlateDrawEffect::None, ArcColor, true, 5.5f);
	}

	return ChildLayer + 2;
}

void SPiUERadialPanel::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	const int32 NumSlots = Children.Num();
	if (NumSlots <= 0)
	{
		return;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Center = LocalSize * 0.5f;

	for (int32 Index = 0; Index < NumSlots; ++Index)
	{
		const FSlot& Slot = Children[Index];
		const TSharedRef<SWidget> Widget = Slot.GetWidget();
		if (Widget->GetVisibility() == EVisibility::Collapsed)
		{
			continue;
		}

		const FVector2D ChildSize = Widget->GetDesiredSize();
		const float Angle = ComputeSlotAngle(Index, NumSlots);
		const FVector2D Offset(FMath::Sin(Angle) * Radius, -FMath::Cos(Angle) * Radius);
		const FVector2D Position = Center + Offset - ChildSize * 0.5f;

		ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(Widget, Position, ChildSize));
	}
}
