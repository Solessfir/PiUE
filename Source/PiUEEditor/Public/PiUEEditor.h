// Copyright Solessfir 2026. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

class FPiUEEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;
};
