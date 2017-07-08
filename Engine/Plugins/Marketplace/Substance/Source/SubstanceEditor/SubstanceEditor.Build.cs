// Copyright 2014-2016 Allegorithmic, Inc. All Rights Reserved.
using UnrealBuildTool;

/**
 * Substance core is added as a private dependency as we do not want to expose substance core to other
 * modules that will use the substance editor module. We are still adding the framework/lib headers for our private includes
 * as substance core will depend on these and so that this module will know about the framework types.
 **/

public class SubstanceEditor : ModuleRules
{
	public SubstanceEditor(TargetInfo Target)
	{
		//PCH file
		bEnforceIWYU = false;

		//Include paths
		PrivateIncludePaths.Add("SubstanceEditor/Private");
		PrivateIncludePaths.Add("SubstanceCore/Private/include");

		//Module public dependencies
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AssetTools",
			"BlueprintGraph",
			"ContentBrowser",
			"Core",
			"CoreUObject",
			"DesktopPlatform",
			"EditorStyle",
			"EditorWidgets",
			"Engine",
			"InputCore",
			"KismetCompiler",
			"LevelEditor",
			"MainFrame",
			"PropertyEditor",
			"RenderCore",
			"RHI",
			"TextureEditor",
			"UnrealEd",
		});

		//Module private dependencies
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AppFramework",
			"Slate",
			"SlateCore",
			"Projects",
			"WorkspaceMenuStructure",
			"ShaderCore",
			"SubstanceCore"
		});

	}
}
