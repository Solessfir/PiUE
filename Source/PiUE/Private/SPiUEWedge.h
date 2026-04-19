// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/SCompoundWidget.h"

class SBorder;
struct FSlateBrush;

/**
* Single item rendered in the pie. Shows an optional icon and a label.
*/
class SPiUEWedge : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPiUEWedge)
		: _Icon(nullptr)
		, _bBold(false)
	{
		_Visibility = EVisibility::HitTestInvisible;
	}
		SLATE_ARGUMENT(const FSlateBrush*, Icon)
		SLATE_ARGUMENT(FText, Label)
		SLATE_ARGUMENT(FLinearColor, BaseTint)
		SLATE_ARGUMENT(bool, bBold)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Toggles the highlight target. Color transition is animated via Tick. */
	void SetHighlighted(bool bInHighlighted);

	/** Starts the exit animation. Widget translates toward center and fades to zero. */
	void SetExiting();

	/** Sets the radial direction and distance used for the enter/exit translation animation. Call after all slots are added so angles are final. */
	void SetEnterDirection(const FVector2D InOutwardDir, const float InRadius);

	// Begin SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	// End SWidget interface

private:
	bool bHighlighted = false;
	bool bExiting = false;
	float HighlightAlpha = 0.f;
	float PresenceAlpha = 0.f;
	FVector2D OutwardDir = FVector2D::ZeroVector;
	float OutwardRadius = 0.f;
	FLinearColor BaseTint = FLinearColor::Black;
	FLinearColor HighlightTint = FLinearColor::White;

	TUniquePtr<FSlateRoundedBoxBrush> AnimBrush;

	TSharedPtr<SBorder> BorderWidget;
};
