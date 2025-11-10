#pragma once

namespace DecoTool
{
	void InitDecoTool()
	{
		AFortDecoTool* FortDecoToolDefault = AFortDecoTool::GetDefaultObj();
		UClass* FortDecoToolClass = AFortDecoTool::StaticClass();
	}
}