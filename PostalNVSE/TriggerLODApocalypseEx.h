#pragma once

char* formatPointer = 0;

int functionCall = 0x406D00;
__declspec(naked) int TambouilleDuLOD()
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
		//push 0x106daf8
		push formatPointer
		push 0x104							//0x104 = 260, le nombre de caractères MAXIMAL
		mov  ecx, DWORD PTR[ebp + 0x8]
		push ecx
		call functionCall
		add esp,0x20
		pop ebp
		retn
	}
}

int funcDuCheck = 0x456A20;

int finDeLaFonction = 0x6F6F00;
int jmpChargeleLODNormal = 0x6F6ED9;

//Ne pas oublier que le jeu rentre dans cette fonction tout seul lors du chargement d'une sauvegarde DEPUIS LE MAIN MENU SEULEMENT
__declspec(naked) void LODLoopHook()
{
	_asm {
		retourDansLaBoucle:
		mov eax, [formatsIndex]
		mov ecx, [formatsNumber]
		//Si c'est zéro, - donc pas de LOD formats -, on se fait même pas chier à entrer dans la boucle et on charge le LOD normal
		cmp ecx, 0
		je yaRienDuTout
		//PETITE BOUCLE
		mov ecx, [formatsArray]
		mov eax, [ecx + eax * 4]
		mov [formatPointer], eax
		//LOAD DU LOD AVEC LE SUFFIX
		movzx edx, word ptr[ebp + 0x14]
		push edx
		movzx eax, word ptr[ebp + 0x10]
		push eax
		mov ecx, DWORD PTR[ebp + 0x18]
		push ecx
		mov edx, DWORD PTR[ebp - 0x120]
		mov eax, DWORD PTR[edx + 0x44]
		push eax
		lea ecx, [ebp - 0x11c]
		push    ecx
		call TambouilleDuLOD
		//CHECK SI LE LOD PRECEDENT A FONCTIONNE
		add esp, 0x14
		push 0xffffffff
		push 0
		push 0
		lea edx, [ebp - 0x11c]
		push edx
		call funcDuCheck
		add esp, 10h
		test eax, eax
		jnz jmpSiCestGood
		//LE LOD N'A PAS ETE CHARGE, DONC SOIT ON CONTINUE LA BOUCLE POUR UN EVENTUEL CUSTOM LOD SOIT ON QUITTE ET ON LOAD LE LOD NORMAL
		//J'incrémente l'index de 1
		mov eax, [formatsIndex]
		add eax,1
		mov ecx, [formatsNumber]
		//SI YA PLUS D'INDEX DISPO, ET QUE C'EST TOUJOURS PAS BON, ON SE FAIT PLUS CHIER ET ON CHARGE LE LOD NORMAL
		cmp eax, ecx
		jae yaRienDuTout
		//SI Y'A ENCORE UN INDEX DISPO, ON RETOURNE EN HAUT
		mov [formatsIndex],eax
		jmp retourDansLaBoucle
		//Y'A PAS DE LOD FORMAT, DONC PAS DE CUSTOM LOD, DONC ON CHARGE JUSTE LE LOD NORMAL
		yaRienDuTout:
		mov [formatsIndex],0
		jmp jmpChargeleLODNormal
		//LE LOD A ETE CHARGE, ON REINITIALISE L'INDEX POUR LA PROCHAINE CELL ET ON SORT
		jmpSiCestGood:
		mov [formatsIndex],0
		jmp finDeLaFonction
	}
}

int PostApocalypseCall = 0x6F5690;

//En jumpant à cette adresse, je jump directement une seconde fois dans "LODLoopHook" vu que je l'ai hook dans TriggerLODApocalypseExHooks, kino
int toTheLODLoop = 0x6F6E63;

__declspec(naked) void refreshLODHook()
{
	_asm {
		call PostApocalypseCall
		add esp, 0x14
		push 0xffffffff
		push 0
		push 0
		lea edx, [ebp - 0x11c]
		push edx
		call funcDuCheck
		add esp, 10h
		test eax, eax
		jnz jmpSiCestGood2
		jmp toTheLODLoop
		jmpSiCestGood2:
		jmp finDeLaFonction
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

		freeTheFormats(formatsArray, formatsNumber);
		formatsArray = getFormatsFromMap(LODsuffixesMap, formatsNumber);

		*result = 1;
	}
	return true;
}

void TriggerLODApocalypseExHooks()
{
	//WriteRelJump(0x6F6E63, 0x6F6E99);
	//WriteRelJump(0x6F6EBD, (UInt32)ultimeTest2);

	WriteRelJump(0x6F6E63, (UInt32)LODLoopHook);
	WriteRelJump(0x6F6EB8, (UInt32)refreshLODHook);
}