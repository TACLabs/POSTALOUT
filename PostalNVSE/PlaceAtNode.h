#pragma once

__forceinline TESObjectREFR* PlaceAtMe(TESObjectREFR* refr, TESForm* form, UInt32 count, UInt32 distance, UInt32 direction, float health)
{
	return CdeclCall<TESObjectREFR*>(0x5C4B30, refr, form, count, distance, direction, health);
}

bool Cmd_PlaceAtNode_Execute(COMMAND_ARGS)
{
	REFR_RES = 0;
	TESForm *form;
	char nodeName[0x40];
	TESObjectREFR* tempPosMarker = s_tempPosMarker;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &nodeName, &form) && thisObj)
	{
		if (TESObjectCELL* cell = thisObj->parentCell)
		{
			tempPosMarker->parentCell = cell;
			if (NiNoude* niNoude = FindNode((NiNoude*)thisObj->GetNiNode(), nodeName))
			{
				tempPosMarker->posX = niNoude->m_worldTranslate.x;
				tempPosMarker->posY = niNoude->m_worldTranslate.y;
				tempPosMarker->posZ = niNoude->m_worldTranslate.z;

				if (TESObjectREFR* placedRef = PlaceAtMe(tempPosMarker, form, 1, 0, 0, 1))
					REFR_RES = placedRef->refID;
			}
		}
	}

	return true;
}