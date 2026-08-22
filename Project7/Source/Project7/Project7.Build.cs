// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Project7 : ModuleRules
{
	public Project7(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput"
		});
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Test"
		});
		
	}
}
