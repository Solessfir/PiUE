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
	UI_COMMAND(SummonRadialMenu1, "Summon PiUE Ring 1", "Opens PiUE ring 1 at the cursor.", EUserInterfaceActionType::Button, FInputChord(EKeys::V), FInputChord(EKeys::ThumbMouseButton));
	UI_COMMAND(SummonRadialMenu2, "Summon PiUE Ring 2", "Opens PiUE ring 2 at the cursor.", EUserInterfaceActionType::Button, FInputChord(), FInputChord());
	UI_COMMAND(SummonRadialMenu3, "Summon PiUE Ring 3", "Opens PiUE ring 3 at the cursor.", EUserInterfaceActionType::Button, FInputChord(), FInputChord());
	UI_COMMAND(SummonRadialMenu4, "Summon PiUE Ring 4", "Opens PiUE ring 4 at the cursor.", EUserInterfaceActionType::Button, FInputChord(), FInputChord());
	UI_COMMAND(SummonRadialMenu5, "Summon PiUE Ring 5", "Opens PiUE ring 5 at the cursor.", EUserInterfaceActionType::Button, FInputChord(), FInputChord());
}

#undef LOCTEXT_NAMESPACE
