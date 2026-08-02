// Copyright Solessfir 2026. All Rights Reserved.

#include "PiUECommandRouter.h"

#include "Framework/Commands/UICommandInfo.h"
#include "Framework/Commands/UICommandList.h"
#include "Interfaces/IMainFrameModule.h"
#include "LevelEditor.h"
#include "LevelEditorViewport.h"
#include "Modules/ModuleManager.h"
#include "SEditorViewport.h"

namespace
{
	template <typename VisitorType>
	bool VisitKnownCommandLists(VisitorType&& Visitor)
	{
		FModuleManager& ModuleManager = FModuleManager::Get();

		if (ModuleManager.IsModuleLoaded("LevelEditor"))
		{
			FLevelEditorModule& LevelEditorModule = ModuleManager.GetModuleChecked<FLevelEditorModule>("LevelEditor");
			if (Visitor(LevelEditorModule.GetGlobalLevelEditorActions()))
			{
				return true;
			}
		}

		if (GCurrentLevelEditingViewportClient)
		{
			const TSharedPtr<SEditorViewport> ViewportWidget = GCurrentLevelEditingViewportClient->GetEditorViewportWidget();
			if (ViewportWidget.IsValid() && Visitor(ViewportWidget->GetCommandList().ToSharedRef()))
			{
				return true;
			}
		}

		if (ModuleManager.IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrameModule = ModuleManager.GetModuleChecked<IMainFrameModule>("MainFrame");
			if (Visitor(MainFrameModule.GetMainFrameCommandBindings()))
			{
				return true;
			}
		}

		return false;
	}
}

bool FPiUECommandRouter::IsCommandMapped(const TSharedRef<const FUICommandInfo>& Command)
{
	return VisitKnownCommandLists([&Command](const TSharedRef<FUICommandList>& CommandList)
	{
		return CommandList->IsActionMapped(Command);
	});
}

bool FPiUECommandRouter::TryExecuteCommand(const TSharedRef<const FUICommandInfo>& Command)
{
	return VisitKnownCommandLists([&Command](const TSharedRef<FUICommandList>& CommandList)
	{
		return CommandList->TryExecuteAction(Command);
	});
}
