#include "nvse/PluginAPI.h"
#include "nvse/CommandTable.h"
#include "nvse/GameAPI.h"
#include "nvse/ParamInfos.h"
#include "nvse/GameObjects.h"
#include "nvse/SafeWrite.h"
#include <string>
#include "NiNoudes.h"
#include "matrix.h"

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

bool init = false;
PlayerCharacter* pPC = 0;
NiNoude* nRoot3rd = 0;
NiNoude* nFakeRoot3rd = 0;
float ancienneValeur = 0.0f;

void KickPanardInclinaison(NVSEMessagingInterface::Message* msg)
{

	switch (msg->type)
	{

	case NVSEMessagingInterface::kMessage_MainGameLoop:
		if (init)
		{
			if (!(pPC->bThirdPerson) && (nRoot3rd->m_flags & 0x00000001) == 0)
			{
				/*
				float angleX = (pPC->rotX * 180) / 3.14;

				NiVector3 rot;
				rot.x = 0;
				rot.y = angleX;
				rot.z = 0;

				NiMatrix33 rotMat;
				EulerToMatrix(&rotMat, &rot);

				nFakeRoot3rd->m_localRotate.data[0] = rotMat.data[0];
				nFakeRoot3rd->m_localRotate.data[1] = rotMat.data[1];
				nFakeRoot3rd->m_localRotate.data[2] = rotMat.data[2];
				nFakeRoot3rd->m_localRotate.data[3] = rotMat.data[3];
				nFakeRoot3rd->m_localRotate.data[4] = rotMat.data[4];
				nFakeRoot3rd->m_localRotate.data[5] = rotMat.data[5];
				nFakeRoot3rd->m_localRotate.data[6] = rotMat.data[6];
				nFakeRoot3rd->m_localRotate.data[7] = rotMat.data[7];
				nFakeRoot3rd->m_localRotate.data[8] = rotMat.data[8];
				*/
				*(float*)0x011CD8D0 = 40.0f;
			}
			else
			{
				*(float*)0x011CD8D0 = ancienneValeur;
			}
		}
		break;

	case NVSEMessagingInterface::kMessage_ExitToMainMenu:
		init = false;
		break;
	case NVSEMessagingInterface::kMessage_PostLoadGame:
		pPC = PlayerCharacter::GetSingleton();
		nRoot3rd = GetRootNode(0);
		nFakeRoot3rd = FindNode(nRoot3rd, "360Corr0");

		float* ptr = reinterpret_cast<float*>(0x11CD8D0);
		ancienneValeur = *ptr;

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

	//g_pluginHandle = nvse->GetPluginHandle();

	// save the NVSE interface in case we need it later
	//g_nvseInterface = nvse;

	// register to receive messages from NVSE
	//g_messagingInterface = static_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
	//g_messagingInterface->RegisterListener(g_pluginHandle, "NVSE", KickPanardInclinaison);

	// Disables a jump that prevents the 1st person body 
	// from being visible if the 3rd person body is visible
	SafeWrite16(0x00874D86, 0x9090);
	SafeWrite32(0x00874D88, 0x90909090);

	return true;
}
