// Copyright Solessfir 2026. All Rights Reserved.

using UnrealBuildTool;

public class PiUE : ModuleRules
{
	public PiUE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange([
			"Core",
			"CoreUObject",
			"Engine"
		]);

		PrivateDependencyModuleNames.AddRange([
			"ApplicationCore",
			"Blutility",
			"DeveloperSettings",
			"EditorFramework",
			"EditorSubsystem",
			"InputCore",
			"LevelEditor",
			"MainFrame",
			"Projects",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UMG",
			"UMGEditor",
			"UnrealEd"
		]);
	}
}
