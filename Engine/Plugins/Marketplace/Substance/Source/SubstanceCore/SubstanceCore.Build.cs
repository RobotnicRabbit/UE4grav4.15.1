// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
using System;
using System.IO;
using System.Collections.Generic;

namespace UnrealBuildTool.Rules
{
public class SubstanceCore : ModuleRules
{
	public SubstanceCore(TargetInfo Target)
	{
		//Internal defines
		Definitions.Add("WITH_SUBSTANCE=1");
		Definitions.Add("SUBSTANCE_PLATFORM_BLEND=1");
		Definitions.Add("SUBSTANCE_CORE_DEBUG_TOOLS=0");

		//PCH file
		bEnforceIWYU = false;

		//Thread usage
		if (Target.Platform == UnrealTargetPlatform.Win32 ||
		        Target.Platform == UnrealTargetPlatform.Win64 ||
		        Target.Platform == UnrealTargetPlatform.XboxOne)
		{
			Definitions.Add("AIR_USE_WIN32_SYNC=1");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac ||
		         Target.Platform == UnrealTargetPlatform.PS4 ||
		         Target.Platform == UnrealTargetPlatform.Linux)
		{
			Definitions.Add("AIR_USE_PTHREAD_SYNC=1");
		}

		//Exposing the include path to substance core publicly. Even though the include paths are public, the libs should no longer be exposed.
		PrivateIncludePaths.Add("SubstanceCore/Private");
		PrivateIncludePaths.Add("SubstanceCore/Private/include");

		//Exposing public include paths
		PublicIncludePaths.Add("SubstanceCore/Public");
		PublicIncludePaths.Add("SubstanceCore/Classes");

		//Module dependencies
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Projects",
			"Slate",
			"SlateCore",
			"SubstanceEngine"
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"Engine",
			"RenderCore",
			"RHI",
			"ImageWrapper",
			"SessionServices",
			"RHI"
		});

		//Editor specific configuration
		bool IncludePS4Files = false;

		if (UEBuildConfiguration.bBuildEditor == true)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"AssetTools",
				"ContentBrowser",
				"Settings",
				"TargetPlatform",
				"MainFrame"
			});

			//Used for ps4 cooking
			string SDKDir = System.Environment.GetEnvironmentVariable("SCE_ORBIS_SDK_DIR");
			if ((SDKDir != null) && (SDKDir.Length > 0))
			{
				PublicIncludePaths.Add(SDKDir + "/target/include_common");
				PublicLibraryPaths.Add(SDKDir + "/host_tools/lib");

				PublicAdditionalLibraries.Add("libSceGpuAddress.lib");
				PublicDelayLoadDLLs.Add("libSceGpuAddress.dll");
				PublicAdditionalLibraries.Add("libSceGnm.lib");
				PublicDelayLoadDLLs.Add("libSceGnm.dll");

				//Toggle on our flag if we are building for PS4
				IncludePS4Files = true;
			}
		}


		//Overwrite PS4 SDK if the files don't exist
		string ConsoleFilePath = "SubstanceCore\\Private\\SubstanceCoreConsoleSpecific.h";
		string BasePath = Path.Combine("..", "Plugins", "Runtime", "Substance", "Source");
		ConsoleFilePath = Path.Combine(BasePath, ConsoleFilePath);
		IncludePS4Files = File.Exists(ConsoleFilePath) && IncludePS4Files;

		Definitions.Add("SUBSTANCE_HAS_PS4_SDK=" + (IncludePS4Files ? "1" : "0"));
	}
}
}
