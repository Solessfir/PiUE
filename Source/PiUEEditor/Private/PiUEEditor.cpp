// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUEEditor.h"
#include "PiUECommandPickerCustomization.h"
#include "PiUEIconPathCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

namespace
{
	// FName instances are constructed inside accessors (not at namespace scope) to avoid the static
	// initialization order fiasco - FName depends on the global name table which itself initializes lazily.
	const FName& PropertyEditorModuleName() { static const FName Name(TEXT("PropertyEditor")); return Name; }
	const FName& EditorCommandStructName()  { static const FName Name(TEXT("PiUEEditorCommandItem")); return Name; }
	const FName& IconPathStructName()       { static const FName Name(TEXT("PiUEIconPath")); return Name; }
}

void FPiUEEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(PropertyEditorModuleName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(EditorCommandStructName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPiUECommandPickerCustomization::MakeInstance));
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(IconPathStructName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPiUEIconPathCustomization::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FPiUEEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded(PropertyEditorModuleName()))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditorModuleName());
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout(EditorCommandStructName());
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout(IconPathStructName());
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

IMPLEMENT_MODULE(FPiUEEditorModule, PiUEEditor)
