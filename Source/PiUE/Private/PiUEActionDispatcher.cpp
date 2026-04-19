// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUEActionDispatcher.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityObject.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Framework/Commands/InputBindingManager.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Framework/Commands/UICommandList.h"
#include "LevelEditor.h"
#include "LevelEditorViewport.h"
#include "Modules/ModuleManager.h"
#include "SEditorViewport.h"
#include "StructUtils/InstancedStruct.h"
#include "PiUETypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogPiUE, Log, All);

namespace PiUE
{
	static bool ExecuteEditorCommand(const FPiUEEditorCommandItem& Item)
	{
		if (Item.CommandContext.IsNone() || Item.CommandName.IsNone())
		{
			return false;
		}

		const TSharedPtr<FUICommandInfo> Info = FInputBindingManager::Get().FindCommandInContext(Item.CommandContext, Item.CommandName);
		if (!Info.IsValid())
		{
			UE_LOGFMT(LogPiUE, Warning, "PiUE: editor command {0}.{1} not found.", Item.CommandContext, Item.CommandName);
			return false;
		}

		// Try the Level Editor's global command list. Covers PIE, Simulate, view modes, show flags, and most viewport actions.
		// Other-context commands (asset editors, etc.) are out of scope for now.
		if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
		{
			FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
			const TSharedRef<FUICommandList> Commands = LevelEditorModule.GetGlobalLevelEditorActions();
			if (Commands->TryExecuteAction(Info.ToSharedRef()))
			{
				return true;
			}
		}

		// Fallback: try the active viewport's own command list (covers viewport-specific bindings).
		if (GCurrentLevelEditingViewportClient != nullptr)
		{
			const TSharedPtr<SEditorViewport> ViewportWidget = GCurrentLevelEditingViewportClient->GetEditorViewportWidget();
			if (ViewportWidget.IsValid() && ViewportWidget->GetCommandList()->TryExecuteAction(Info.ToSharedRef()))
			{
				return true;
			}
		}

		UE_LOGFMT(LogPiUE, Warning, "PiUE: command {0}.{1} not bound on any known command list.", Item.CommandContext, Item.CommandName);
		return false;
	}

	static bool ExecuteConsoleCommand(const FPiUEConsoleCommandItem& Item)
	{
		if (Item.Command.IsEmpty() || GEngine == nullptr)
		{
			return false;
		}

		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		return GEngine->Exec(World, *Item.Command);
	}

	static bool ExecuteEditorUtilityObject(const FPiUEEditorUtilityObjectItem& Item)
	{
		if (Item.Object.IsNull())
		{
			return false;
		}

		UEditorUtilityBlueprint* Blueprint = Item.Object.LoadSynchronous();
		if (Blueprint == nullptr)
		{
			UE_LOGFMT(LogPiUE, Warning, "PiUE: failed to load EditorUtilityBlueprint {0}.", Item.Object.ToString());
			return false;
		}

		UClass* Class = Blueprint->GeneratedClass;
		if (Class == nullptr || !Class->IsChildOf<UEditorUtilityObject>())
		{
			UE_LOGFMT(LogPiUE, Warning, "PiUE: EditorUtilityBlueprint {0} has no valid generated class.", Item.Object.ToString());
			return false;
		}

		UEditorUtilityObject* Instance = NewObject<UEditorUtilityObject>(GetTransientPackage(), Class);
		if (Instance == nullptr)
		{
			return false;
		}

		Instance->Run();
		return true;
	}

	static bool ExecuteEditorUtility(const FPiUEEditorUtilityItem& Item)
	{
		if (Item.Widget.IsNull())
		{
			return false;
		}

		UEditorUtilityWidgetBlueprint* WidgetBP = Item.Widget.LoadSynchronous();
		if (WidgetBP == nullptr)
		{
			UE_LOGFMT(LogPiUE, Warning, "PiUE: failed to load EditorUtilityWidgetBlueprint {0}.", Item.Widget.ToString());
			return false;
		}

		UEditorUtilitySubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>() : nullptr;
		if (Subsystem == nullptr)
		{
			return false;
		}

		Subsystem->SpawnAndRegisterTab(WidgetBP);
		return true;
	}
}

bool FPiUEActionDispatcher::Execute(const FInstancedStruct& Item)
{
	const UScriptStruct* Type = Item.GetScriptStruct();
	if (Type == nullptr)
	{
		return false;
	}

	if (Type->IsChildOf(FPiUEEditorCommandItem::StaticStruct()))
	{
		return PiUE::ExecuteEditorCommand(Item.Get<FPiUEEditorCommandItem>());
	}
	if (Type->IsChildOf(FPiUEConsoleCommandItem::StaticStruct()))
	{
		return PiUE::ExecuteConsoleCommand(Item.Get<FPiUEConsoleCommandItem>());
	}
	if (Type->IsChildOf(FPiUEEditorUtilityObjectItem::StaticStruct()))
	{
		return PiUE::ExecuteEditorUtilityObject(Item.Get<FPiUEEditorUtilityObjectItem>());
	}
	if (Type->IsChildOf(FPiUEEditorUtilityItem::StaticStruct()))
	{
		return PiUE::ExecuteEditorUtility(Item.Get<FPiUEEditorUtilityItem>());
	}

	// Categories are navigational only, not executable.
	return false;
}
