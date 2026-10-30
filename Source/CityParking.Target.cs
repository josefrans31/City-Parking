using UnrealBuildTool;
using System.Collections.Generic;

public class CityParkingTarget : TargetRules
{
	public CityParkingTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CityParking");
	}
}
