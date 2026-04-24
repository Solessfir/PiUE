// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUEInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/InputBindingManager.h"
#include "Framework/Commands/InputChord.h"
#include "HAL/PlatformTime.h"
#include "LevelEditorViewport.h"
#include "PiUECommands.h"
#include "PiUESettings.h"
#include "SEditorViewport.h"
#include "SLevelViewport.h"
#include "SPiUERadialMenu.h"
#include "Widgets/SCanvas.h"

int32 FPiUEInputProcessor::FindMatchingRingIndex(const FKey& PressedKey, FInputChord& OutChord)
{
	const TSharedPtr<FUICommandInfo> Commands[5] =
	{
		FPiUECommands::Get().SummonRadialMenu1,
		FPiUECommands::Get().SummonRadialMenu2,
		FPiUECommands::Get().SummonRadialMenu3,
		FPiUECommands::Get().SummonRadialMenu4,
		FPiUECommands::Get().SummonRadialMenu5,
	};

	for (int32 RingIndex = 0; RingIndex < 5; ++RingIndex)
	{
		if (!Commands[RingIndex].IsValid())
		{
			continue;
		}
		for (int32 i = 0; i < 2; ++i)
		{
			const FInputChord Chord = *Commands[RingIndex]->GetActiveChord(static_cast<EMultipleKeyBindingIndex>(i));
			if (Chord.IsValidChord() && Chord.Key == PressedKey)
			{
				OutChord = Chord;
				return RingIndex;
			}
		}
	}
	return INDEX_NONE;
}

bool FPiUEInputProcessor::IsViewportFocused(const FSlateApplication& SlateApp)
{
	if (GCurrentLevelEditingViewportClient == nullptr)
	{
		return false;
	}

	const TSharedPtr<SEditorViewport> ViewportWidget = GCurrentLevelEditingViewportClient->GetEditorViewportWidget();
	if (!ViewportWidget.IsValid())
	{
		return false;
	}

	return ViewportWidget->GetCachedGeometry().IsUnderLocation(SlateApp.GetCursorPos());
}

void FPiUEInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	if (!bSummonKeyHeld)
	{
		return;
	}

	const TSharedPtr<SPiUERadialMenu> PinnedMenu = Menu.Pin();
	if (PinnedMenu.IsValid())
	{
		PinnedMenu->TickCategoryHover(DeltaTime);
	}
}

bool FPiUEInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	// Esc while menu is open cancels.
	if (Menu.IsValid() && InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseMenu();
		return true;
	}

	if (InKeyEvent.GetKey().IsMouseButton())
	{
		return false;
	}

	FInputChord SummonChord;
	const int32 RingIndex = FindMatchingRingIndex(InKeyEvent.GetKey(), SummonChord);
	if (RingIndex == INDEX_NONE)
	{
		return false;
	}

	// Verify modifier state matches the bound chord to avoid stealing plain key presses.
	const bool bModifiersMatch =
		InKeyEvent.IsControlDown() == SummonChord.bCtrl &&
		InKeyEvent.IsShiftDown()   == SummonChord.bShift &&
		InKeyEvent.IsAltDown()     == SummonChord.bAlt &&
		InKeyEvent.IsCommandDown() == SummonChord.bCmd;

	if (!bModifiersMatch)
	{
		return false;
	}

	if (InKeyEvent.IsRepeat() || bSummonKeyHeld)
	{
		return Menu.IsValid();
	}

	if (Menu.IsValid())
	{
		CloseMenu();
		return true;
	}

	if (!IsViewportFocused(SlateApp))
	{
		return false;
	}

	if (GEditor != nullptr && GEditor->IsPlaySessionInProgress())
	{
		return false;
	}

	bSummonKeyHeld = true;
	PressStartTime = FPlatformTime::Seconds();
	OpenMenu(SlateApp, RingIndex);
	return true;
}

bool FPiUEInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey().IsMouseButton())
	{
		return false;
	}

	FInputChord SummonChord;
	if (FindMatchingRingIndex(InKeyEvent.GetKey(), SummonChord) == INDEX_NONE)
	{
		return false;
	}

	if (!bSummonKeyHeld)
	{
		return false;
	}

	bSummonKeyHeld = false;

	const TSharedPtr<SPiUERadialMenu> PinnedMenu = Menu.Pin();
	if (!PinnedMenu.IsValid())
	{
		return false;
	}

	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	const double ElapsedMs = (FPlatformTime::Seconds() - PressStartTime) * 1000.0;

	if (ElapsedMs < Settings->TapThreshold)
	{
		return true;
	}

	PinnedMenu->TryExecuteHoveredAction();
	CloseMenu();
	return true;
}

bool FPiUEInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	// Summon via mouse button (e.g. Mouse4 secondary bind).
	FInputChord SummonChord;
	const int32 MouseRingIndex = FindMatchingRingIndex(MouseEvent.GetEffectingButton(), SummonChord);
	if (MouseRingIndex != INDEX_NONE)
	{
		if (GEditor == nullptr || !GEditor->IsPlaySessionInProgress())
		{
			if (Menu.IsValid())
			{
				bSummonKeyHeld = false;
				CloseMenu();
				return true;
			}
			if (!bSummonKeyHeld && IsViewportFocused(SlateApp))
			{
				bSummonKeyHeld = true;
				PressStartTime = FPlatformTime::Seconds();
				OpenMenu(SlateApp, MouseRingIndex);
			}
		}
		return Menu.IsValid();
	}

	const TSharedPtr<SPiUERadialMenu> PinnedMenu = Menu.Pin();
	if (!PinnedMenu.IsValid())
	{
		return false;
	}

	// Key still held = hold mode in progress; clicks pass through, release will execute.
	if (bSummonKeyHeld)
	{
		return false;
	}

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		const bool bShouldClose = PinnedMenu->NavigateBack();
		if (bShouldClose)
		{
			CloseMenu();
		}
		return true;
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const bool bShouldClose = PinnedMenu->ConfirmSelection();
		if (bShouldClose)
		{
			CloseMenu();
		}
		return true;
	}

	return false;
}

bool FPiUEInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	FInputChord SummonChord;
	if (FindMatchingRingIndex(MouseEvent.GetEffectingButton(), SummonChord) == INDEX_NONE)
	{
		return false;
	}

	if (!bSummonKeyHeld)
	{
		if (bMouseTapCloseArmed && Menu.IsValid())
		{
			// The viewport's input preprocessor (higher-priority type bucket) consumes ThumbMouseButton Down
			// when the menu overlay is active, so the closing press never reaches HandleMouseButtonDownEvent.
			// Instead we close on the Up, but skip the first ~80ms window to absorb hardware duplicate Up
			// events that some mice fire immediately after the real Up.
			constexpr double DuplicateDebounceMs = 80.0;
			const double MsSinceOpen = (FPlatformTime::Seconds() - TapOpenTime) * 1000.0;
			if (MsSinceOpen > DuplicateDebounceMs)
			{
				bMouseTapCloseArmed = false;
				CloseMenu();
				return true;
			}
		}
		return false;
	}

	bSummonKeyHeld = false;

	const TSharedPtr<SPiUERadialMenu> PinnedMenu = Menu.Pin();
	if (!PinnedMenu.IsValid())
	{
		return false;
	}

	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	const double ElapsedMs = (FPlatformTime::Seconds() - PressStartTime) * 1000.0;

	if (ElapsedMs < Settings->TapThreshold)
	{
		bMouseTapCloseArmed = true;
		TapOpenTime = FPlatformTime::Seconds();
		return true;
	}

	PinnedMenu->TryExecuteHoveredAction();
	CloseMenu();
	return true;
}

void FPiUEInputProcessor::OpenMenu(const FSlateApplication& SlateApp, int32 RingIndex)
{
	CloseMenu();

	if (GCurrentLevelEditingViewportClient == nullptr)
	{
		return;
	}

	const TSharedPtr<SLevelViewport> ViewportWidget = StaticCastSharedPtr<SLevelViewport>(GCurrentLevelEditingViewportClient->GetEditorViewportWidget());
	if (!ViewportWidget.IsValid())
	{
		return;
	}

	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	const FVector2D CursorScreen = SlateApp.GetCursorPos();
	const float HalfSize = Settings->MenuRadius + 80.f;
	const FVector2D MenuSize(HalfSize * 2.f, HalfSize * 2.f);

	TSharedPtr<SPiUERadialMenu> MenuContent;
	TSharedRef<SCanvas> Overlay = SNew(SCanvas);

	// Compute position lazily using the canvas's own geometry - avoids the SLevelViewport/inner-overlay coordinate mismatch.
	TWeakPtr<SCanvas> WeakOverlay = Overlay;
	auto ComputePos = [WeakOverlay, CursorScreen, HalfSize]() -> FVector2D
	{
		if (const TSharedPtr<SCanvas> Pinned = WeakOverlay.Pin())
		{
			return Pinned->GetCachedGeometry().AbsoluteToLocal(CursorScreen) - FVector2D(HalfSize, HalfSize);
		}
		return FVector2D::ZeroVector;
	};

	Overlay->AddSlot()
		.Position(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateLambda(ComputePos)))
		.Size(TAttribute<FVector2D>(MenuSize))
		[
			SAssignNew(MenuContent, SPiUERadialMenu).RootItems(Settings->GetRingItems(RingIndex)).MenuCenterAbsPos(CursorScreen)
		];

	ViewportWidget->AddOverlayWidget(Overlay);

	OverlayViewport = ViewportWidget;
	MenuOverlayWidget = Overlay;
	Menu = MenuContent;
}

void FPiUEInputProcessor::CloseMenu()
{
	bMouseTapCloseArmed = false;
	if (const TSharedPtr<SLevelViewport> Viewport = OverlayViewport.Pin())
	{
		if (MenuOverlayWidget.IsValid())
		{
			Viewport->RemoveOverlayWidget(MenuOverlayWidget.ToSharedRef());
		}
	}
	OverlayViewport.Reset();
	MenuOverlayWidget.Reset();
	Menu.Reset();
}
