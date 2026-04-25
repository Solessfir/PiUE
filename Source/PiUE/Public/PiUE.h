// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FPiUEInputProcessor;

/**
* PiUE editor module. Registers the summon command, the Slate input preprocessor, and the settings page.
*/
class FPiUEModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

private:
	TSharedPtr<FPiUEInputProcessor> InputProcessor;
};
