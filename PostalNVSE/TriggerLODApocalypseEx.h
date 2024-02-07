#pragma once

bool Cmd_TriggerLODApocalypseEx_Execute(COMMAND_ARGS) {
	char suffix[MAX_PATH];
	bool state = true;
	*result = 0;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &suffix)) {
		std::string sSuffix = suffix;
		if (bool(LODsuffixesMap.count(sSuffix)))
		{
			LODsuffixesMap[sSuffix] = !LODsuffixesMap[sSuffix];
		}
		else
		{
			LODsuffixesMap[sSuffix] = true;
		}
		if (IsConsoleMode()) Console_Print("%s - %d", sSuffix.c_str(), LODsuffixesMap[sSuffix]);
		*result = 1;
	}
	return true;
}


void hann()
{
	Console_Print("LOAD");
}

/*
int retourTest1 = 0x6F6F00;
int doOnce = 0;
__declspec(naked) void test1()
{
	if (doOnce == 0)
	{
		hann();
		doOnce = 1;
	}

	__asm {
		jmp retourTest1
	}
}
*/

void TriggerLODApocalypseExHooks()
{
	//WriteRelJump(0x6F6E63, (UInt32)test1);		//PETIT TEST
	//WriteRelJump(0x6F6E63, 0x6F6E99);				LOAD LE POSTAPOCALYPSE LOAD, BIEN COMME IL FAUT, DES LE LANCEMENT DU JEU
}