// Copyright Solessfir 2026. All Rights Reserved.

using UnrealBuildTool;

public class PiUEEditor : ModuleRules
{
	public PiUEEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"InputCore",
			"PiUE",
			"PropertyEditor",
			"Slate",
			"SlateCore",
		});
	}
}
