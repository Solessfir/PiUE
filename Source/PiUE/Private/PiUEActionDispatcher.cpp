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
#include "StructUtils/InstancedStruct.h"
#include "PiUECommandRouter.h"
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

	if (FPiUECommandRouter::TryExecuteCommand(Info.ToSharedRef()))
	{
		return;
	}

	UE_LOGFMT(LogPiUE, Warning, "PiUE: command {0}.{1} is unavailable in the current editor context.", CommandContext, CommandName);
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

	const UEditorUtilityBlueprint* Blueprint = Object.LoadSynchronous();
	if (!Blueprint)
	{
		UE_LOGFMT(LogPiUE, Warning, "PiUE: failed to load EditorUtilityBlueprint {0}.", Object.ToString());
		return;
	}

	const UClass* Class = Blueprint->GeneratedClass;
	if (!Class || !Class->IsChildOf<UEditorUtilityObject>())
	{
		UE_LOGFMT(LogPiUE, Warning, "PiUE: EditorUtilityBlueprint {0} has no valid generated class.", Object.ToString());
		return;
	}

	if (UEditorUtilityObject* Instance = NewObject<UEditorUtilityObject>(GetTransientPackage(), Class))
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

	if (UEditorUtilitySubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>() : nullptr)
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
