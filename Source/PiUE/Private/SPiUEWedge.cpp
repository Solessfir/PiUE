// Copyright Solessfir 2026. All Rights Reserved.

#include "SPiUEWedge.h"
#include "PiUESettings.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> SPiUEWedge::BuildContent(const FArguments& InArgs)
{
	const FSlateBrush* IconBrush = InArgs._Icon ? InArgs._Icon : FAppStyle::GetBrush("NoBrush");
	const bool bHasIcon = InArgs._Icon != nullptr
		&& (InArgs._Icon->GetResourceName() != NAME_None || InArgs._Icon->GetResourceObject());

	TSharedRef<SHorizontalBox> Content = SNew(SHorizontalBox);

	if (bHasIcon)
	{
		Content->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SBox)
			.WidthOverride(18.f)
			.HeightOverride(18.f)
			[
				SNew(SImage).Image(IconBrush)
			]
		];
	}

	TSharedRef<STextBlock> LabelWidget = SNew(STextBlock)
		.Text(InArgs._Label)
		.ColorAndOpacity(FLinearColor::White);
	if (InArgs._bBold)
	{
		LabelWidget->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 9));
	}
	Content->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[LabelWidget];

	return Content;
}

void SPiUEWedge::Construct(const FArguments& InArgs)
{
	BaseTint = InArgs._BaseTint;
	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	HighlightTint = Settings->HighlightWedgeTint;
	CachedAnimSpeed = Settings->WedgeAnimSpeed;
	CachedHighlightAnimSpeed = Settings->HighlightAnimSpeed;

	constexpr float CornerRadius = 8.f;
	AnimBrush = MakeUnique<FSlateRoundedBoxBrush>(BaseTint, CornerRadius);

	SetVisibility(EVisibility::HitTestInvisible);

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(88.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(BorderWidget, SBorder)
			.BorderImage(AnimBrush.Get())
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FMargin(14.f, 6.f))
			[
				BuildContent(InArgs)
			]
		]
	];
}

void SPiUEWedge::SetHighlighted(bool bInHighlighted)
{
	bHighlighted = bInHighlighted;
}

void SPiUEWedge::SetExiting()
{
	bExiting = true;
}

void SPiUEWedge::SetEnterDirection(const FVector2D InOutwardDir, const float InRadius)
{
	OutwardDir = InOutwardDir;
	OutwardRadius = InRadius;
}

void SPiUEWedge::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Presence: translate from center + fade for enter/exit.
	const float PresenceTarget = bExiting ? 0.f : 1.f;
	PresenceAlpha += (PresenceTarget - PresenceAlpha) * FMath::Min(1.f, InDeltaTime * CachedAnimSpeed);
	if (FMath::IsNearlyEqual(PresenceAlpha, PresenceTarget, 0.001f))
	{
		PresenceAlpha = PresenceTarget;
	}

	SetRenderTransform(FSlateRenderTransform(FTransform2D(OutwardDir * OutwardRadius * (PresenceAlpha - 1.f))));
	SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, PresenceAlpha));

	// Highlight tint lerp. Only invalidate when the tint actually changed.
	const float HighlightTarget = bHighlighted ? 1.f : 0.f;
	const float PrevHighlight = HighlightAlpha;
	HighlightAlpha += (HighlightTarget - HighlightAlpha) * FMath::Min(1.f, InDeltaTime * CachedHighlightAnimSpeed);
	if (FMath::IsNearlyEqual(HighlightAlpha, HighlightTarget, 0.001f))
	{
		HighlightAlpha = HighlightTarget;
	}

	if (AnimBrush.IsValid() && !FMath::IsNearlyEqual(PrevHighlight, HighlightAlpha))
	{
		AnimBrush->TintColor = FSlateColor(FMath::Lerp(BaseTint, HighlightTint, HighlightAlpha));
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}
