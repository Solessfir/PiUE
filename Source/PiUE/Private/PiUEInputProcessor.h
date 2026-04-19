// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h"

class SLevelViewport;
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

private:
	/** Returns the summon chord (primary or secondary) whose key matches PressedKey. False if neither matches. */
	static bool GetMatchingSummonChord(const FKey& PressedKey, FInputChord& OutChord);

	/** True when a level editor viewport currently has user focus - used to gate summon to the viewport. */
	static bool IsViewportFocused(const FSlateApplication& SlateApp);

	/** Spawns the menu host window at the cursor. */
	void OpenMenu(const FSlateApplication& SlateApp);

	/** Dismisses the menu host window if open. */
	void CloseMenu();

	TWeakPtr<SLevelViewport> OverlayViewport;
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
