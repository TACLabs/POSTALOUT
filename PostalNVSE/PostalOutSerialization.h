#pragma once
//Grand V aux développeurs de JohnnyGuitar NVSE Plugin pour la plupart du code ci-joint

bool (*_WriteRecord)(UInt32 type, UInt32 version, const void* buffer, UInt32 length);
bool (*_WriteRecordData)(const void* buffer, UInt32 length);
bool (*_GetNextRecordInfo)(UInt32* type, UInt32* version, UInt32* length);
UInt32(*_ReadRecordData)(void* buffer, UInt32 length);
bool (*_ResolveRefID)(UInt32 refID, UInt32* outRefID);
bool (*_OpenRecord)(UInt32 type, UInt32 version);
#define SERIALIZATION_VERSION 1

std::unordered_map<std::string, bool> LODsuffixesMap;

enum RecordIDs
{
	kRecordID_LODsuffixes = 'POLS',
};

void SaveGameCallback(void*)
{
	if (!LODsuffixesMap.empty())
	{
		_OpenRecord(kRecordID_LODsuffixes, SERIALIZATION_VERSION);
		//Ecrit la SIZE du MAP
		UInt16 mapLen = LODsuffixesMap.size();
		_WriteRecordData(&mapLen, sizeof(UInt16));
		for (auto it : LODsuffixesMap) {

			//Ecrit la LENGTH du STRING du MAP ITEM
			UInt16 len = it.first.length();
			_WriteRecordData(&len, sizeof(UInt16));

			//Ecrit le STRING du MAP ITEM
			_WriteRecordData(it.first.c_str(), it.first.length());

			//Ecrit le STATE du MAP ITEM
			_WriteRecordData(&it.second, sizeof(bool));
		}
	}
}

void LoadGameCallback(void*)
{
	UInt32 type, version, length;
	while (_GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case kRecordID_LODsuffixes: 
		{
			//Recupere la SIZE du MAP
			UInt16 mapLen = 0;
			_ReadRecordData(&mapLen, sizeof(UInt16));
			if (mapLen > 0)
			{
				char buffer[MAX_PATH] = { 0 };
				for (int i = 0; i < mapLen; i++)
				{
					//Récupère la LENGTH du STRING
					UInt16 len = 0;
					_ReadRecordData(&len, sizeof(UInt16));

					//Récupère le STRING et mets un caractère de fin de chaine qui est le ZERO
					_ReadRecordData(buffer, len);
					buffer[len] = 0;

					std::string sSuffix = std::string(buffer);

					//Récupère le STATE
					bool state = false;
					_ReadRecordData(&state, sizeof(bool));

					LODsuffixesMap[sSuffix] = state;
				}
			}
		}
			default:
				break;
		}
	}

	/*
	for (const auto& paire : LODsuffixesMap) {
		Console_Print("%s - %d", paire.first.c_str(), paire.second);
	}
	*/
}

void PostalOutSerializationInit(const NVSEInterface* nvse)
{
	NVSESerializationInterface* serialization = (NVSESerializationInterface*)nvse->QueryInterface(kInterface_Serialization);
	_WriteRecord = serialization->WriteRecord;
	_WriteRecordData = serialization->WriteRecordData;
	_GetNextRecordInfo = serialization->GetNextRecordInfo;
	_ReadRecordData = serialization->ReadRecordData;
	_ResolveRefID = serialization->ResolveRefID;
	_OpenRecord = serialization->OpenRecord;
	serialization->SetPreLoadCallback(nvse->GetPluginHandle(), LoadGameCallback);
	serialization->SetSaveCallback(nvse->GetPluginHandle(), SaveGameCallback);
}

