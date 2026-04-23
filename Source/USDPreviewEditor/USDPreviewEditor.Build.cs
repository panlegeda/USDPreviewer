using UnrealBuildTool;

public class USDPreviewEditor : ModuleRules
{
	public USDPreviewEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"WorkspaceMenuStructure",
				"LevelEditor",
				"DesktopPlatform",
				"InputCore",
				"ApplicationCore"
			}
		);
	}
}
