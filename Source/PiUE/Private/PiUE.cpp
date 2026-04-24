// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUE.h"
#include "Framework/Application/SlateApplication.h"
#include "PiUECommands.h"
#include "PiUEInputProcessor.h"

#define LOCTEXT_NAMESPACE "FPiUEModule"

void FPiUEModule::StartupModule()
{
	FPiUECommands::Register();

	if (FSlateApplication::IsInitialized())
	{
		InputProcessor = MakeShared<FPiUEInputProcessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
	}
}

void FPiUEModule::ShutdownModule()
{
	if (InputProcessor.IsValid())
	{
		InputProcessor->CloseMenu();
	}

	if (FSlateApplication::IsInitialized() && InputProcessor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}

	InputProcessor.Reset();

	FPiUECommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPiUEModule, PiUE)
