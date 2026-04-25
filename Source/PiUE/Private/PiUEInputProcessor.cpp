// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUEInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/InputBindingManager.h"
#include "Framework/Commands/InputChord.h"
#include "Editor.h"
#include "HAL/PlatformTime.h"
#include "LevelEditorViewport.h"
#include "PiUECommands.h"
#include "PiUESettings.h"
#include "SEditorViewport.h"
#include "SPiUERadialMenu.h"
#include "SPiUERadialPanel.h"
#include "Widgets/SCanvas.h"
#include "Widgets/SWindow.h"

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

TSharedPtr<SWindow> FPiUEInputProcessor::FindWindowUnderCursor(const FSlateApplication& SlateApp)
{
	const FVector2D CursorPos = SlateApp.GetCursorPos();
	TArray<TSharedRef<SWindow>> AllWindows;
	FSlateApplication::Get().GetAllVisibleWindowsOrdered(AllWindows);

	for (int32 i = AllWindows.Num() - 1; i >= 0; --i)
	{
		const TSharedRef<SWindow>& Win = AllWindows[i];
		if (Win->GetType() == EWindowType::Normal && Win->GetCachedGeometry().IsUnderLocation(CursorPos))
		{
			return Win;
		}
	}

	return nullptr;
}

bool FPiUEInputProcessor::IsViewportFocused(const FSlateApplication& SlateApp, const bool bAvailableAnywhere)
{
	const TSharedPtr<SWidget> FocusedWidget = SlateApp.GetKeyboardFocusedWidget();
	if (FocusedWidget.IsValid())
	{
		const FName WidgetType = FocusedWidget->GetType();
		if (WidgetType == FName(TEXT("SEditableText")) || WidgetType == FName(TEXT("SMultiLineEditableText")))
		{
			return false;
		}
	}

	if (bAvailableAnywhere)
	{
		return FindWindowUnderCursor(SlateApp).IsValid();
	}

	return IsLevelViewportTopmost(SlateApp);
}

bool FPiUEInputProcessor::IsLevelViewportTopmost(const FSlateApplication& SlateApp)
{
	if (!GCurrentLevelEditingViewportClient)
	{
		return false;
	}

	const TSharedPtr<SEditorViewport> LVPWidget = GCurrentLevelEditingViewportClient->GetEditorViewportWidget();
	if (!LVPWidget.IsValid())
	{
		return false;
	}

	const TSharedPtr<SWindow> LVPWindow = FSlateApplication::Get().FindWidgetWindow(LVPWidget.ToSharedRef());
	if (!LVPWindow.IsValid())
	{
		return false;
	}

	return FindWindowUnderCursor(SlateApp) == LVPWindow;
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

static bool ModifiersMatch(const FKeyEvent& InKeyEvent, const FInputChord& SummonChord)
{
	return InKeyEvent.IsControlDown() == SummonChord.bCtrl
		&& InKeyEvent.IsShiftDown()   == SummonChord.bShift
		&& InKeyEvent.IsAltDown()     == SummonChord.bAlt
		&& InKeyEvent.IsCommandDown() == SummonChord.bCmd;
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
	if (RingIndex == INDEX_NONE || !ModifiersMatch(InKeyEvent, SummonChord))
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

	if (!IsViewportFocused(SlateApp, GetDefault<UPiUESettings>()->IsRingAvailableAnywhere(RingIndex)))
	{
		return false;
	}

	if (GEditor && GEditor->IsPlaySessionInProgress())
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

bool FPiUEInputProcessor::TryHandleMouseSummonDown(FSlateApplication& SlateApp, const int32 MouseRingIndex)
{
	if (GEditor && GEditor->IsPlaySessionInProgress())
	{
		return Menu.IsValid();
	}

	if (Menu.IsValid())
	{
		bSummonKeyHeld = false;
		CloseMenu();
		return true;
	}

	if (!bSummonKeyHeld && IsViewportFocused(SlateApp, GetDefault<UPiUESettings>()->IsRingAvailableAnywhere(MouseRingIndex)))
	{
		bSummonKeyHeld = true;
		PressStartTime = FPlatformTime::Seconds();
		OpenMenu(SlateApp, MouseRingIndex);
	}

	return Menu.IsValid();
}

bool FPiUEInputProcessor::HandleMenuClick(const TSharedPtr<SPiUERadialMenu>& PinnedMenu, const FKey& Button)
{
	if (Button == EKeys::RightMouseButton)
	{
		if (PinnedMenu->NavigateBack())
		{
			CloseMenu();
		}
		return true;
	}

	if (Button == EKeys::LeftMouseButton)
	{
		if (PinnedMenu->ConfirmSelection())
		{
			CloseMenu();
		}
		return true;
	}

	return false;
}

bool FPiUEInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	// Summon via mouse button (e.g. Mouse4 secondary bind).
	FInputChord SummonChord;
	const int32 MouseRingIndex = FindMatchingRingIndex(MouseEvent.GetEffectingButton(), SummonChord);
	if (MouseRingIndex != INDEX_NONE)
	{
		return TryHandleMouseSummonDown(SlateApp, MouseRingIndex);
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

	return HandleMenuClick(PinnedMenu, MouseEvent.GetEffectingButton());
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

void FPiUEInputProcessor::AttachMenuOverlay(const TSharedRef<SWindow>& Window, const FVector2D& CursorScreen, const int32 RingIndex)
{
	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	const float HalfSize = Settings->MenuRadius + SPiUERadialPanel::WedgePadding;
	const FVector2D MenuSize(HalfSize * 2.f, HalfSize * 2.f);

	TSharedPtr<SPiUERadialMenu> MenuContent;
	TSharedRef<SCanvas> Overlay = SNew(SCanvas);

	// Compute position lazily using the canvas's own geometry - avoids stale geometry on first frame.
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

	Window->AddOverlaySlot()
	[
		Overlay
	];

	OverlayWindow = Window;
	MenuOverlayWidget = Overlay;
	Menu = MenuContent;
}

void FPiUEInputProcessor::OpenMenu(const FSlateApplication& SlateApp, const int32 RingIndex)
{
	CloseMenu();

	const FVector2D CursorScreen = SlateApp.GetCursorPos();
	const TSharedPtr<SWindow> Window = FindWindowUnderCursor(SlateApp);
	if (!Window.IsValid())
	{
		return;
	}

	AttachMenuOverlay(Window.ToSharedRef(), CursorScreen, RingIndex);
}

void FPiUEInputProcessor::CloseMenu()
{
	bMouseTapCloseArmed = false;
	if (const TSharedPtr<SWindow> Window = OverlayWindow.Pin())
	{
		if (MenuOverlayWidget.IsValid())
		{
			Window->RemoveOverlaySlot(MenuOverlayWidget.ToSharedRef());
		}
	}
	OverlayWindow.Reset();
	MenuOverlayWidget.Reset();
	Menu.Reset();
}
