#include "nvse/PluginAPI.h"
#include "nvse/CommandTable.h"
#include "nvse/GameAPI.h"
#include "nvse/GameData.h"
#include "nvse/ParamInfos.h"
#include "nvse/GameObjects.h"
#include "nvse/SafeWrite.h"
#include <string>
#include "NiNoudes.h"
//#include "matrix.h"

const UInt32 kRetrieveRootNodeCall = 0x00950BB0;

// Retrieves the 1st/3rd person root ninode
NiNoude* GetRootNode(bool firstPerson) {
	_asm
	{
		movzx edx, firstPerson
		mov eax, 0x011DEA3C // player
		push edx
		mov ecx, [eax]
		call kRetrieveRootNodeCall
	}
}

// Searches the children nodes to find the node with the specified name
NiNoude* FindNode(NiNoude* node, const char* name) {

	if (node && node->m_pcName) {
		if (strcmp(node->m_pcName, name) == 0) {
			return node;

			// Check that the rtti type is NiNode or BSFadeNode
		}
		else if ((UInt32)node->GetType() != 0x011F4428 && (UInt32)node->GetType() != 0x011F9140) {
			return 0;
		}

		int len = node->m_children.numObjs;
		for (int i = 0; i < len; i++) {
			NiNoude* n = FindNode((NiNoude*)node->m_children[i], name);
			if (n != 0) {
				return n;
			}
		}
	}

	return 0;
}

__declspec(naked) UInt32 __fastcall HexToUInt(const char* str)
{
	__asm
	{
		push	esi
		call	StrLen
		test	eax, eax
		jz		done
		lea		esi, [ecx + eax]
		mov		ch, al
		xor eax, eax
		xor cl, cl
		ALIGN 16
		hexToInt:
		dec		esi
			movsx	edx, byte ptr[esi]
			sub		dl, '0'
			js		done
			cmp		dl, 9
			jle		doAdd
			or dl, 0x20
			cmp		dl, '1'
			jl		done
			cmp		dl, '6'
			jg		done
			sub		dl, 0x27
			doAdd:
		shl		edx, cl
			add		eax, edx
			add		cl, 4
			dec		ch
			jnz		hexToInt
			done :
		pop		esi
			retn
	}
}

#define CALL_EAX(addr) __asm mov eax, addr __asm call eax

__declspec(naked) TESIdleForm* AnimData::GetPlayedIdle()
{
	__asm
	{
		mov		eax, [ecx + 0x128]
		test	eax, eax
		jz		noQueued
		mov		eax, [eax + 0x2C]
		test	eax, eax
		jnz		done
		noQueued :
		mov		eax, [ecx + 0x124]
			test	eax, eax
			jz		done
			mov		eax, [eax + 0x2C]
			test	eax, eax
			jz		done
			push	eax
			CALL_EAX(0x4985F0)
			pop		edx
			movzx	eax, al
			dec		eax
			and eax, edx
			done :
		retn
	}
}

bool __fastcall IsPlayerIdlePlaying(TESIdleForm* idleAnim)
{
	PlayerCharacter* pc = PlayerCharacter::GetSingleton();
	//AnimData* animData = thisObj->GetAnimData();
	AnimData* animData3rd = pc->baseProcess->GetAnimData();
	AnimData* animData1st = pc->firstPersonAnimData;
	return (animData3rd && (animData3rd->GetPlayedIdle() == idleAnim)) || (animData1st && (animData1st->GetPlayedIdle() == idleAnim));
}


bool init = false;

PlayerCharacter* pc = 0;
DataHandler* dh = 0;

NiNoude* Root3rdNiNoude = 0;
NiNoude* FakeRoot3rdNiNoude = 0;

TESIdleForm* kickAnim;
const char* modName = "B42Bash.esp";
const char* kickAnimIndex = "800";
UInt32 kickAnimRefID = 0;


int count = 0;

void KickPanardGestion(NVSEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case NVSEMessagingInterface::kMessage_MainGameLoop:
		if (init)
		{
			//ALERTE : ON FAIT UN KICK
			if (!(pc->bThirdPerson) && IsPlayerIdlePlaying(kickAnim))
			{
				// Le corps de la troisième personne n'est pas visible
				if ((Root3rdNiNoude->m_flags & 0x00000001) == 1)
				{
					//Bah on l'affiche
					Root3rdNiNoude->m_flags &= 0xFFFFFFFE;
					
				}
				else
				{
					//On fait notre tambouille avec le calcul puis l'inclinaison du body pour voir le panard en toutes circonstances
					
					//Console_Print(std::to_string(count).c_str());
				}
			}
			else
			{
			//Si on est toujours en première personne et que le corps de la troisième personne est toujours visible
				if (!(pc->bThirdPerson) && (Root3rdNiNoude->m_flags & 0x00000001) == 0)
				{
					//Bah on le désaffiche
					Root3rdNiNoude->m_flags |= 0x00000001;
					count++;
				}
			}
		}
		break;
	case NVSEMessagingInterface::kMessage_ExitToMainMenu:
		init = false;
		break;
	case NVSEMessagingInterface::kMessage_PostLoadGame:

		pc = PlayerCharacter::GetSingleton();
		dh = DataHandler::Get();

		UInt8 modIndex = dh->GetModIndex(modName);
		kickAnimRefID = (HexToUInt(kickAnimIndex) & 0xFFFFFF) | (modIndex << 24);

		kickAnim = (TESIdleForm*)(LookupFormByRefID(kickAnimRefID));

		Root3rdNiNoude = GetRootNode(0);
		FakeRoot3rdNiNoude = FindNode(Root3rdNiNoude, "360Corr0");

		init = true;
		break;
	}
}

NVSEMessagingInterface* g_messagingInterface{};
NVSEInterface* g_nvseInterface{};
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;



bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	_MESSAGE("query");

	// fill out the info structure
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Postal Kick";
	info->version = 1;

	if (nvse->isEditor)
	{
		return false;
	}


	// version checks pass
	// any version compatibility checks should be done here
	return true;
}

bool NVSEPlugin_Load(NVSEInterface* nvse)
{
	_MESSAGE("load");

	g_pluginHandle = nvse->GetPluginHandle();

	// save the NVSE interface in case we need it later
	g_nvseInterface = nvse;

	// register to receive messages from NVSE
	g_messagingInterface = static_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
	g_messagingInterface->RegisterListener(g_pluginHandle, "NVSE", KickPanardGestion);

	// Disables a jump that prevents the 1st person body 
	// from being visible if the 3rd person body is visible
	SafeWrite16(0x00874D86, 0x9090);
	SafeWrite32(0x00874D88, 0x90909090);

	return true;
}
