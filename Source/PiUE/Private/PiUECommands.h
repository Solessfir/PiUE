// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/**
* Global commands exposed by PiUE. Five independently bindable summon commands, one per ring.
*/
class FPiUECommands : public TCommands<FPiUECommands>
{
	friend TCommands;

public:
	FPiUECommands();

	TSharedPtr<FUICommandInfo> SummonRadialMenu1;
	TSharedPtr<FUICommandInfo> SummonRadialMenu2;
	TSharedPtr<FUICommandInfo> SummonRadialMenu3;
	TSharedPtr<FUICommandInfo> SummonRadialMenu4;
	TSharedPtr<FUICommandInfo> SummonRadialMenu5;

protected:
	virtual void RegisterCommands() override;
};
