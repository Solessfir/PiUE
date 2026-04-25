// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h"

class SWindow;
class SPiUERadialMenu;

/**
* Slate-level input preprocessor for the PiUE summon hotkey.
* Intercepts key/button-down to spawn the radial menu and key/button-up to confirm/close it.
* Supports both keyboard and mouse-button summon chords. Also handles Esc (cancel) and right-mouse (navigate up).
*/
class FPiUEInputProcessor : public IInputProcessor, public TSharedFromThis<FPiUEInputProcessor>
{
public:
	// Begin IInputProcessor interface
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual const TCHAR* GetDebugName() const override { return TEXT("PiUE"); }
	// End IInputProcessor interface

	/** Dismisses the menu host window if open. Safe to call from ShutdownModule. */
	void CloseMenu();

private:
	/** Returns ring index (0-4) whose command chord matches PressedKey, writing the matched chord to OutChord. Returns INDEX_NONE if no match. */
	static int32 FindMatchingRingIndex(const FKey& PressedKey, FInputChord& OutChord);

	/** Gates summon. bAvailableAnywhere=true: any editor window (text fields excluded). false: level viewport only. */
	static bool IsViewportFocused(const FSlateApplication& SlateApp, bool bAvailableAnywhere);

	/** Returns true if the topmost normal window under the cursor is the level viewport window. */
	static bool IsLevelViewportTopmost(const FSlateApplication& SlateApp);

	/** Spawns the menu host window at the cursor showing the specified ring. */
	void OpenMenu(const FSlateApplication& SlateApp, const int32 RingIndex);

	/** Returns the topmost normal editor window under the cursor, or nullptr if none found. */
	static TSharedPtr<SWindow> FindWindowUnderCursor(const FSlateApplication& SlateApp);

	/** Creates the canvas overlay and menu widget and attaches them to the given window. */
	void AttachMenuOverlay(const TSharedRef<SWindow>& Window, const FVector2D& CursorScreen, int32 RingIndex);

	/** Handles a mouse-button summon press: opens or closes the menu for the given ring. */
	bool TryHandleMouseSummonDown(FSlateApplication& SlateApp, int32 MouseRingIndex);

	/** Dispatches an LMB confirm or RMB navigate-back while the menu is open in tap mode. Returns whether the event was handled. */
	bool HandleMenuClick(const TSharedPtr<SPiUERadialMenu>& PinnedMenu, const FKey& Button);

	TWeakPtr<SWindow> OverlayWindow;
	TSharedPtr<SWidget> MenuOverlayWidget;
	TWeakPtr<SPiUERadialMenu> Menu;

	/** Platform time (seconds) at which the summon key went down. */
	double PressStartTime = 0.0;

	/** True while the summon key is held down. Used to ignore auto-repeat key-down events. */
	bool bSummonKeyHeld = false;

	/** Set after a mouse-button tap opens the menu. The Down event for the closing press can be swallowed by the viewport,
	    so we close on the Up instead, using TapOpenTime to debounce duplicate hardware Up events. */
	bool bMouseTapCloseArmed = false;
	double TapOpenTime = 0.0;
};
