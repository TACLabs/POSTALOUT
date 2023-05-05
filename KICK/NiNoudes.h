#pragma once

#include "nvse/GameTypes.h"

class NiTimeController;
class NiExtraData;




struct PropertyNode
{
	PropertyNode	* next;
	PropertyNode	* prev;
	void			* data;
};


class NiRefOubject
{
public:
	NiRefOubject();
	~NiRefOubject();

	virtual void	Destructor(bool freeThis);	// 00
	virtual void	Delete();					// Calls Destructor

//	void		** _vtbl;						// 000
	UInt32		m_uiRefCount;					// 004 - name known
};


class NiOubject : public NiRefOubject
{
public:
	NiOubject();
	~NiOubject();

	virtual NiRTTI *	GetType(void);
};


class NiOubjectNET : public NiOubject
{
public:
	NiOubjectNET();
	~NiOubjectNET();

	const char			* m_pcName;					// 008
	NiTimeController	* m_controller;				// 00C

	NiExtraData			** m_extraDataList;			// 010
	UInt16				m_extraDataListLen;			// 014
	UInt16				m_extraDataListCapacity;	// 016
};


class NiAVObject : public NiOubjectNET
{
public:
	NiAVObject();
	~NiAVObject();

	NiOubject		* m_parent;				// 018
	NiOubject		* m_spCollisionObject;	// 01C
	NiSphere		* m_kWorldBound;		// 020

	PropertyNode	* m_propertyListStart;	// 024
	PropertyNode	* m_propertyListEnd;	// 028
	UInt32			m_propertyListLen;		// 02C

	UInt32			m_flags;				// 030
	NiMatrix33		m_localRotate;			// 034
	NiVector3		m_localTranslate;		// 058
	float			m_localScale;			// 064
	NiMatrix33		m_worldRotate;			// 068
	NiVector3		m_worldTranslate;		// 08C
	float			m_worldScale;			// 098
};


class NiNoude : public NiAVObject
{
public:
	NiNoude();
	~NiNoude();

	NiTArray <NiAVObject *>		m_children;	// 09C
};


class NiShader;

class NiGeometry : public NiAVObject
{
public:
	NiGeometry();
	~NiGeometry();

	NiOubject	* unk09C;	// 09C - NiAlphaProperty
	NiOubject	* unk0A0;	// 0A0 - NiCullingProperty
	NiOubject	* unk0A4;	// 0A4 - NiMaterialProperty
	NiOubject	* unk0A8;	// 0A8 - BSShaderPPLightingProperty, BSShaderNoLightingProperty
	NiOubject	* unk0AC;	// 0AC - NiStencilProperty
	NiOubject	* unk0B0;	// 0B0 - NiTexturingProperty

	UInt32		unk0B4;		// 0B4
	NiOubject	* data;		// 0B8 - NiTriStripsData, NiTriShapeData
	UInt32		unk0BC;		// 0BC
	NiShader	* shader;	// 0C0 - ShadowLightShader, BSShaderNoLighting
};


class NiTriBasedGeom : public NiGeometry
{
public:
	NiTriBasedGeom();
	~NiTriBasedGeom();
};

class NiTriShape : public NiTriBasedGeom
{
public:
	NiTriShape();
	~NiTriShape();
};

class NiTriStrips : public NiTriBasedGeom
{
public:
	NiTriStrips();
	~NiTriStrips();
};


class Actor;
class NiControllerManager;
class NiTextKeyExtraData;


// Seems very similar to Oblivion's version
class NiControullerSequence : public NiOubject
{
public:
	NiControullerSequence();
	~NiControullerSequence();

	enum
	{
		kState_Inactive = 0,
		kState_Animating,
		kState_EaseIn,
		kState_EaseOut,
		kState_TransSource,
		kState_TransDest,
		kState_MorphSource
	};

	enum
	{
		kCycle_Loop = 0,
		kCycle_Reverse,
		kCycle_Clamp,
	};

	// 10
	struct Unk014
	{
		NiRefOubject	* unk00;	// 00
		NiRefOubject	* unk04;	// 04
		UInt32		unk08;		// 08
		UInt8		unk0C;		// 0C
		UInt8		unk0D;		// 0D
		UInt8		pad0E[2];	// 0E
	};

	// 10
	struct Unk018
	{
		NiRefOubject	* unk00;	// 00
		UInt16		unk04;		// 04
		UInt16		unk06;		// 06
		UInt16		unk08;		// 08
		UInt16		unk0A;		// 0A
		UInt16		unk0C;		// 0C
		UInt8		pad0E[2];	// 0E
	};

	char				* filePath;		// 008
	UInt32				arraySize;		// 00C
	UInt32				unk010;			// 010
	Unk014				* unk014;		// 014
	Unk018				* unk018;		// 018
	float				weight;			// 01C
	NiTextKeyExtraData	* unk020;		// 020
	UInt32				cycleType;		// 024
	float				freq;			// 028
	float				begin;			// 02C
	float				end;			// 030
	float				last;			// 034
	float				weightLast;		// 038
	float				lastScaled;		// 03C
	NiControllerManager * controllerMgr;	// 040
	UInt32				state;			// 044
	float				offset;			// 048
	float				start;			// 04C - offset * -1?
	float				end2;			// 050
	UInt32				unk054;			// 054
	UInt32				unk058;			// 058
	char				* accumRoot;	// 05C - bone? (seen "Bip01")
	NiNoude				* niNode060;	// 060
	// Not tested 064 and above
};

class BSAnimGroupSequonce : public NiControullerSequence
{
public:
	BSAnimGroupSequonce();
	~BSAnimGroupSequonce();
};


class ActorAnimData
{
public:
	ActorAnimData();
	~ActorAnimData();

	UInt32					unk000;						// 000
	Actor					* actor;					// 004
	NiNoude					* nSceneRoot;				// 008
	NiNoude					* nBip01;					// 00C
	UInt32					unk010[(0x028 - 0x010) >> 2];	// 010
	NiNoude					* nPelvis;					// 028
	NiNoude					* nBip01Copy;				// 02C - Same as nBip01?
	NiNoude					* nLForearm;				// 030
	NiNoude					* nHead;					// 034
	NiNoude					* nWeapon;					// 038
	UInt32					unk03C;						// 03C
	UInt32					unk040;						// 040
	NiNoude					* nNeck1;					// 044
	UInt32					unk048;						// 048
	UInt32					unk04C[(0x0D8 - 0x04C) >> 2];	// 04C
	NiControllerManager		* manager;					// 0D8
	UInt32					unk0DC;						// 0DC
	BSAnimGroupSequonce		* animSequences[8];			// 0E0
	BSAnimGroupSequonce		* unk100;					// 100
	UInt32					unk104;						// 104
	UInt32					unk108;						// 108
	float					unk10C;						// 10C
	float					unk110;						// 110
	float					unk114;						// 114
	float					unk118;						// 118
	float					unk11C;						// 11C
	UInt16					unk120;						// 120
	UInt16					unk122;						// 122
	UInt32					unk124[(0x13C - 0x124) >> 2];	// 124
	UInt32					unk13C;						// 13C
};
