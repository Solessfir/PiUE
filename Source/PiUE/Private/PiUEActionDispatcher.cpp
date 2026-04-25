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

void FPiUEEditorCommandItem::Execute() const
{
	if (CommandContext.IsNone() || CommandName.IsNone())
	{
		return;
	}

	const TSharedPtr<FUICommandInfo> Info = FInputBindingManager::Get().FindCommandInContext(CommandContext, CommandName);
	if (!Info.IsValid())
	{
		UE_LOGFMT(LogPiUE, Warning, "PiUE: editor command {0}.{1} not found.", CommandContext, CommandName);
		return;
	}

	// Try the Level Editor's global command list. Covers PIE, Simulate, view modes, show flags, and most viewport actions.
	// Other-context commands (asset editors, etc.) are out of scope for now.
	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		const TSharedRef<FUICommandList> Commands = LevelEditorModule.GetGlobalLevelEditorActions();
		if (Commands->TryExecuteAction(Info.ToSharedRef()))
		{
			return;
		}
	}

	// Fallback: try the active viewport's own command list (covers viewport-specific bindings).
	if (GCurrentLevelEditingViewportClient)
	{
		const TSharedPtr<SEditorViewport> ViewportWidget = GCurrentLevelEditingViewportClient->GetEditorViewportWidget();
		if (ViewportWidget.IsValid() && ViewportWidget->GetCommandList()->TryExecuteAction(Info.ToSharedRef()))
		{
			return;
		}
	}

	UE_LOGFMT(LogPiUE, Warning, "PiUE: command {0}.{1} not bound on any known command list.", CommandContext, CommandName);
}

void FPiUEConsoleCommandItem::Execute() const
{
	if (Command.IsEmpty() || !GEngine)
	{
		return;
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	GEngine->Exec(World, *Command);
}

void FPiUEEditorUtilityObjectItem::Execute() const
{
	if (Object.IsNull())
	{
		return;
	}

	UEditorUtilityBlueprint* Blueprint = Object.LoadSynchronous();
	if (!Blueprint)
	{
		UE_LOGFMT(LogPiUE, Warning, "PiUE: failed to load EditorUtilityBlueprint {0}.", Object.ToString());
		return;
	}

	UClass* Class = Blueprint->GeneratedClass;
	if (!Class || !Class->IsChildOf<UEditorUtilityObject>())
	{
		UE_LOGFMT(LogPiUE, Warning, "PiUE: EditorUtilityBlueprint {0} has no valid generated class.", Object.ToString());
		return;
	}

	UEditorUtilityObject* Instance = NewObject<UEditorUtilityObject>(GetTransientPackage(), Class);
	if (Instance)
	{
		Instance->Run();
	}
}

void FPiUEEditorUtilityItem::Execute() const
{
	if (Widget.IsNull())
	{
		return;
	}

	UEditorUtilityWidgetBlueprint* WidgetBP = Widget.LoadSynchronous();
	if (!WidgetBP)
	{
		UE_LOGFMT(LogPiUE, Warning, "PiUE: failed to load EditorUtilityWidgetBlueprint {0}.", Widget.ToString());
		return;
	}

	UEditorUtilitySubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>() : nullptr;
	if (Subsystem)
	{
		Subsystem->SpawnAndRegisterTab(WidgetBP);
	}
}

void FPiUEActionDispatcher::Execute(const FInstancedStruct& Item)
{
	if (Item.IsValid())
	{
		Item.Get<FPiUEMenuItemBase>().Execute();
	}
}
