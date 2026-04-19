// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUEEditor.h"
#include "PiUEEditorCommandCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

namespace
{
	const FName PropertyEditorModuleName(TEXT("PropertyEditor"));
	const FName EditorCommandStructName(TEXT("PiUEEditorCommandItem"));
}

void FPiUEEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(PropertyEditorModuleName);
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(EditorCommandStructName, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPiUEEditorCommandCustomization::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FPiUEEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded(PropertyEditorModuleName))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditorModuleName);
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout(EditorCommandStructName);
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

IMPLEMENT_MODULE(FPiUEEditorModule, PiUEEditor)
