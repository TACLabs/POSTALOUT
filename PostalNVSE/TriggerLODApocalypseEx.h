#pragma once

int functionCall = 0x406D00;
__declspec(naked) int TambouilleduLOD()
{
	_asm {
		push ebp
		mov ebp, esp
		movsx eax,word ptr [ebp+0x18]
		push eax
		movsx ecx,word ptr [ebp+0x14]
		push ecx
		mov edx,dword ptr [ebp+0x10]
		push edx
		mov eax, dword ptr[ebp + 0xC]
		mov edx, [eax]
		mov ecx, dword ptr[ebp + 0xC]
		mov eax, DWORD PTR[edx + 0x130]
		call eax
		push eax
		mov ecx, dword ptr[ebp + 0xC]
		mov edx,[ecx]
		mov ecx, dword ptr[ebp + 0xC]
		mov eax, DWORD PTR[edx + 0x130]
		call eax
		push eax
		push 0x106daf8
		push 0x104
		mov  ecx, DWORD PTR[ebp + 0x8]
		push ecx
		call functionCall
		add esp,0x20
		pop ebp
		retn
	}
}


bool Cmd_TriggerLODApocalypseEx_Execute(COMMAND_ARGS) {
	char suffix[MAX_PATH];
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

/*
void hann()
{
	Console_Print("LOAD");
}

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

	WriteRelJump(0x6F6E63, 0x6F6E99);				//LOAD LE POSTAPOCALYPSE LOAD, BIEN COMME IL FAUT, DES LE LANCEMENT DU JEU
	WriteRelCall(0x6F6EB8, (UInt32)TambouilleduLOD); //AI REUSSI A REMPLACER UNE FONCTION DANS LE SOURCE CODE DU JEU
    
}