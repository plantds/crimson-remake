// Copyright (c) 2020 Tension Graphics AB

using UnrealBuildTool;
using System.Collections.Generic;

public class CrimsonTarget : TargetRules
{
	public CrimsonTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "Crimson" } );
	}
}
