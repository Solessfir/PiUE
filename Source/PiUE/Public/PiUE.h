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
	// Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface interface

private:
	TSharedPtr<FPiUEInputProcessor> InputProcessor;
};
