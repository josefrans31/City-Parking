using UnrealBuildTool;
using System.Collections.Generic;

public class CityParkingEditorTarget : TargetRules
{
	public CityParkingEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CityParking");
	}
}
