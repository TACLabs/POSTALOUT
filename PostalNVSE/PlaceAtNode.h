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
	UInt32 count;
	TESObjectREFR* tempPosMarker = s_tempPosMarker;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &nodeName, &form, &count) && thisObj)
	{
		if (TESObjectCELL* cell = thisObj->parentCell)
		{
			tempPosMarker->parentCell = cell;
			//récup les coordinates du NODE
			tempPosMarker->posX = 0.0f;
			tempPosMarker->posY = 0.0f;
			tempPosMarker->posZ = 0.0f;
			
			if (TESObjectREFR* placedRef = PlaceAtMe(tempPosMarker, form, count, 0, 0, 1))
				REFR_RES = placedRef->refID;
		}
	}

	return true;
}