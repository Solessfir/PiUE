// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUEInputProcessor.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/InputBindingManager.h"
#include "Framework/Commands/InputChord.h"
#include "Editor.h"
#include "HAL/PlatformTime.h"
#include "Layout/WidgetPath.h"
#include "LevelEditorViewport.h"
#include "PiUECommands.h"
#include "PiUESettings.h"
#include "PiUETypes.h"
#include "SEditorViewport.h"
#include "SPiUERadialMenu.h"
#include "SPiUERadialPanel.h"
#include "Widgets/SCanvas.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

namespace
{
	bool ChordModifiersMatch(const FInputChord& Chord, const FInputEvent& Event)
	{
		return Event.IsControlDown() == Chord.bCtrl
			&& Event.IsShiftDown()   == Chord.bShift
			&& Event.IsAltDown()     == Chord.bAlt
			&& Event.IsCommandDown() == Chord.bCmd;
	}

	template <typename PredicateType>
	int32 FindRingMatching(const FKey& PressedKey, FInputChord& OutChord, PredicateType&& ChordPasses)
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
				const FInputChord& Chord = *Commands[RingIndex]->GetActiveChord(static_cast<EMultipleKeyBindingIndex>(i));
				if (Chord.IsValidChord() && Chord.Key == PressedKey && ChordPasses(Chord))
				{
					OutChord = Chord;
					return RingIndex;
				}
			}
		}
		return INDEX_NONE;
	}
}

int32 FPiUEInputProcessor::FindMatchingRingIndex(const FKey& PressedKey, const FInputEvent& Event, FInputChord& OutChord)
{
	// Multiple rings may bind the same Key with different modifiers (e.g. Ring1=V, Ring2=Ctrl+V).
	// Iterate every ring's chord and return the first whose key and modifier state both match the event.
	return FindRingMatching(PressedKey, OutChord, [&Event](const FInputChord& Chord) { return ChordModifiersMatch(Chord, Event); });
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

bool FPiUEInputProcessor::IsViewportFocused(const FSlateApplication& SlateApp, const bool bViewportOnly)
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

	if (!bViewportOnly)
	{
		return FindWindowUnderCursor(SlateApp).IsValid();
	}

	return IsTargetViewportTopmost(SlateApp);
}

bool FPiUEInputProcessor::IsTargetViewportTopmost(const FSlateApplication& SlateApp)
{
	const TSharedPtr<SWindow> Cursor = FindWindowUnderCursor(SlateApp);
	if (!Cursor.IsValid())
	{
		return false;
	}

	const FVector2D CursorPos = SlateApp.GetCursorPos();

	// Locate the actual widget tree under the cursor. Geometry-only checks (e.g. IsUnderLocation on the viewport widget)
	// match by rectangle bounds and so falsely succeed when another tab (BP graph, Details, etc.) is docked on top of the viewport's rect.
	// Walking the hovered widget path resolves z-order / occlusion correctly.
	TArray<TSharedRef<SWindow>> AllWindows;
	FSlateApplication::Get().GetAllVisibleWindowsOrdered(AllWindows);
	const FWidgetPath HoveredPath = FSlateApplication::Get().LocateWindowUnderMouse(CursorPos, AllWindows, true);
	if (!HoveredPath.IsValid())
	{
		return false;
	}

	// PIE active: cursor must be inside the game viewport widget tree.
	if (GEditor && GEditor->IsPlaySessionInProgress() && GEngine && GEngine->GameViewport)
	{
		const TSharedPtr<SViewport> GameVPWidget = GEngine->GameViewport->GetGameViewportWidget();
		if (GameVPWidget.IsValid() && HoveredPath.ContainsWidget(GameVPWidget.Get()))
		{
			return true;
		}
		// Fall through: even in PIE the level viewport may also count as a target.
	}

	if (!GCurrentLevelEditingViewportClient)
	{
		return false;
	}

	const TSharedPtr<SEditorViewport> LVPWidget = GCurrentLevelEditingViewportClient->GetEditorViewportWidget();
	if (!LVPWidget.IsValid())
	{
		return false;
	}

	return HoveredPath.ContainsWidget(LVPWidget.Get());
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
	const int32 RingIndex = FindMatchingRingIndex(InKeyEvent.GetKey(), InKeyEvent, SummonChord);
	if (RingIndex == INDEX_NONE)
	{
		return false;
	}

	if (bSummonKeyHeld)
	{
		return InKeyEvent.GetKey() == ActiveSummonKey && Menu.IsValid();
	}

	if (InKeyEvent.IsRepeat())
	{
		return false;
	}

	if (Menu.IsValid())
	{
		CloseMenu();
		return true;
	}

	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	const TArray<FInstancedStruct>* RingItems = Settings->GetRingItems(RingIndex);
	if (!RingItems || !SPiUERadialMenu::HasAnyVisibleItem(*RingItems, SPiUERadialMenu::GetCurrentMode()))
	{
		return false;
	}

	if (!IsViewportFocused(SlateApp, Settings->IsRingViewportOnly(RingIndex)))
	{
		return false;
	}

	OpenMenu(SlateApp, RingIndex);
	if (!Menu.IsValid())
	{
		return false;
	}

	bSummonKeyHeld = true;
	ActiveSummonKey = InKeyEvent.GetKey();
	PressStartTime = FPlatformTime::Seconds();
	return true;
}

bool FPiUEInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey().IsMouseButton())
	{
		return false;
	}

	if (!bSummonKeyHeld || InKeyEvent.GetKey() != ActiveSummonKey)
	{
		return false;
	}

	bSummonKeyHeld = false;
	ActiveSummonKey = FKey();

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

bool FPiUEInputProcessor::TryHandleMouseSummonDown(const FSlateApplication& SlateApp, const int32 MouseRingIndex, const FKey& SummonKey)
{
	if (Menu.IsValid())
	{
		bSummonKeyHeld = false;
		CloseMenu();
		return true;
	}

	const UPiUESettings* Settings = GetDefault<UPiUESettings>();
	const TArray<FInstancedStruct>* RingItems = Settings->GetRingItems(MouseRingIndex);
	if (!RingItems || !SPiUERadialMenu::HasAnyVisibleItem(*RingItems, SPiUERadialMenu::GetCurrentMode()))
	{
		return false;
	}

	if (!bSummonKeyHeld && IsViewportFocused(SlateApp, Settings->IsRingViewportOnly(MouseRingIndex)))
	{
		OpenMenu(SlateApp, MouseRingIndex);
		if (Menu.IsValid())
		{
			bSummonKeyHeld = true;
			ActiveSummonKey = SummonKey;
			PressStartTime = FPlatformTime::Seconds();
		}
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
	// Summon via mouse button.
	FInputChord SummonChord;
	const int32 MouseRingIndex = FindMatchingRingIndex(MouseEvent.GetEffectingButton(), MouseEvent, SummonChord);
	if (MouseRingIndex != INDEX_NONE)
	{
		if (bSummonKeyHeld)
		{
			return MouseEvent.GetEffectingButton() == ActiveSummonKey && Menu.IsValid();
		}

		return TryHandleMouseSummonDown(SlateApp, MouseRingIndex, MouseEvent.GetEffectingButton());
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
	if (!bSummonKeyHeld)
	{
		if (bMouseTapCloseArmed && Menu.IsValid() && MouseEvent.GetEffectingButton() == MouseTapCloseKey)
		{
			// Viewport's input preprocessor (higher-priority bucket) eats the closing mouse Down, so we
			// close on Up instead. Skip first ~80ms to absorb hardware duplicate Up events from some mice.
			// Hardcoded, not a UPiUESettings entry: it's a driver quirk, not a user preference. 80ms sits
			// well above observed duplicate intervals (~5-30ms) and well below any intentional second click.
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

	if (MouseEvent.GetEffectingButton() != ActiveSummonKey)
	{
		return false;
	}

	bSummonKeyHeld = false;
	ActiveSummonKey = FKey();

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
		MouseTapCloseKey = MouseEvent.GetEffectingButton();
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
	// Inflate the canvas slot by MenuScale so the render-scaled menu visuals fit without clipping.
	const float HalfSize = (Settings->MenuRadius + SPiUERadialPanel::WedgePadding) * FMath::Max(1.f, Settings->MenuScale);
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
			SAssignNew(MenuContent, SPiUERadialMenu).RootItems(Settings->GetRingItems(RingIndex)).MenuCenterAbsPos(CursorScreen).RootTitle(Settings->GetRingTitle(RingIndex))
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
	bSummonKeyHeld = false;
	ActiveSummonKey = FKey();
	bMouseTapCloseArmed = false;
	MouseTapCloseKey = FKey();
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
