// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUECommands.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "PiUE"

FPiUECommands::FPiUECommands()
	: TCommands<FPiUECommands>(TEXT("PiUE"), LOCTEXT("PiUE", "PiUE"), NAME_None, FAppStyle::GetAppStyleSetName())
{
}

void FPiUECommands::RegisterCommands()
{
	UI_COMMAND(SummonRadialMenu, "Summon PiUE Radial Menu", "Opens the PiUE radial menu at the cursor while held.", EUserInterfaceActionType::Button, FInputChord(EKeys::V), FInputChord(EKeys::ThumbMouseButton));
}

#undef LOCTEXT_NAMESPACE
