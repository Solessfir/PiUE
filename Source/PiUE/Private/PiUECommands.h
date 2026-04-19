// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/**
* Global commands exposed by PiUE. Currently a single summon command that the user rebinds in Editor Preferences.
*/
class FPiUECommands : public TCommands<FPiUECommands>
{
	friend TCommands;

public:
	FPiUECommands();

	// Hotkey-bindable command that opens the radial menu.
	TSharedPtr<FUICommandInfo> SummonRadialMenu;

protected:
	virtual void RegisterCommands() override;
};
