#pragma once
#include "Utilities.h"

#if RUNTIME

const UInt32 _NiTMap_Lookup = 0x00853130;

#endif

// 8
struct NiRTTI
{
	const char	* name;
	NiRTTI		* parent;
};

// C
struct NiVector3
{
	float	x, y, z;
};

// 10 - always aligned?
struct NiVector4
{
	float	x, y, z, w;

	inline __m128 operator*(__m128 packedPS) const { return _mm_mul_ps(PS(), packedPS); }

	inline __m128 PS() const { return _mm_loadu_ps(&x); }
};

// 10 - always aligned?
struct NiQuaternion
{
	float	x, y, z, w;
};

// 24
struct NiMatrix33
{
	float	data[9];
};

// 34
struct NiTransform
{
	NiMatrix33	rotate;		// 00
	NiVector3	translate;	// 24
	float		scale;		// 30
};

// 10
struct NiSphere
{
	float	x, y, z, radius;
};

// 1C
struct NiFrustum
{
	float	l;			// 00
	float	r;			// 04
	float	t;			// 08
	float	b;			// 0C
	float	n;			// 10
	float	f;			// 14
	UInt8	o;			// 18
	UInt8	pad19[3];	// 19
};

// 10
struct NiViewport
{
	float	l;
	float	r;
	float	t;
	float	b;
};

// C
struct NiColor
{
	float	r;
	float	g;
	float	b;
};

// 10
struct NiColorAlpha
{
	float	r;
	float	g;
	float	b;
	float	a;
};

// 10
struct NiPlane
{
	NiVector3	nrm;
	float		offset;
};

// 10
// NiTArrays are slightly weird: they can be sparse
// this implies that they can only be used with types that can be NULL?
// not sure on the above, but some code only works if this is true
// this can obviously lead to fragmentation, but the accessors don't seem to care
// weird stuff
template <typename T>
struct NiTArray
{
	void	** _vtbl;		// 00
	T		* data;			// 04
	UInt16	capacity;		// 08 - init'd to size of preallocation
	UInt16	firstFreeEntry;	// 0A - index of the first free entry in the block of free entries at the end of the array (or numObjs if full)
	UInt16	numObjs;		// 0C - init'd to 0
	UInt16	growSize;		// 0E - init'd to size of preallocation

	T operator[](UInt32 idx) {
		if (idx < firstFreeEntry)
			return data[idx];
		return NULL;
	}

	T Get(UInt32 idx) { return (*this)[idx]; }

	UInt16 Length(void) { return firstFreeEntry; }
	void AddAtIndex(UInt32 index, T* item);	// no bounds checking
	void SetCapacity(UInt16 newCapacity);	// grow and copy data if needed
};

#if RUNTIME

template <typename T> void NiTArray<T>::AddAtIndex(UInt32 index, T* item)
{
	ThisStdCall(0x00869640, this, index, item);
}

template <typename T> void NiTArray<T>::SetCapacity(UInt16 newCapacity)
{
	ThisStdCall(0x008696E0, this, newCapacity);
}

#endif

// 18
// an NiTArray that can go above 0xFFFF, probably with all the same weirdness
// this implies that they make fragmentable arrays with 0x10000 elements, wtf
template <typename T>
class NiTLargeArray
{
public:
	NiTLargeArray();
	~NiTLargeArray();

	void	** _vtbl;		// 00
	T		* data;			// 04
	UInt32	capacity;		// 08 - init'd to size of preallocation
	UInt32	firstFreeEntry;	// 0C - index of the first free entry in the block of free entries at the end of the array (or numObjs if full)
	UInt32	numObjs;		// 10 - init'd to 0
	UInt32	growSize;		// 14 - init'd to size of preallocation

	T operator[](UInt32 idx) {
		if (idx < firstFreeEntry)
			return data[idx];
		return NULL;
	}

	T Get(UInt32 idx) { return (*this)[idx]; }

	UInt32 Length(void) { return firstFreeEntry; }
};

// 8
template <typename T>
struct NiTSet
{
	T		* data;		// 00
	UInt16	capacity;	// 04
	UInt16	length;		// 06
};


#define ZERO_BYTES(addr, size) __stosb((UInt8*)(addr), 0, size)

// 10
// this is a NiTPointerMap <UInt32, T_Data>
// todo: generalize key
template <typename T_Data>
class NiTPointerMap
{
public:
	struct Entry
	{
		Entry* next;
		UInt32		key;
		T_Data* data;
	};

	virtual void	Destroy(bool doFree);
	virtual UInt32	CalculateBucket(UInt32 key);
	virtual bool	CompareKey(UInt32 lhs, UInt32 rhs);
	virtual void	FillEntry(Entry* entry, UInt32 key, T_Data data);
	virtual void	FreeKey(Entry* entry);
	virtual Entry* AllocNewEntry();
	virtual void	FreeEntry(Entry* entry);

	UInt32		m_numBuckets;	// 04
	Entry** m_buckets;	// 08
	UInt32		m_numItems;		// 0C

	bool HasKey(UInt32 key) const
	{
		for (Entry* entry = m_buckets[key % m_numBuckets]; entry; entry = entry->next)
			if (key == entry->key) return true;
		return false;
	}

	T_Data* Lookup(UInt32 key) const;
	void Insert(UInt32 key, T_Data value);

	void DumpLoads()
	{
		int loadsArray[0x80];
		ZERO_BYTES(loadsArray, sizeof(loadsArray));
		Entry** pBucket = m_buckets, * entry;
		UInt32 maxLoad = 0, entryCount;
		for (Entry** pEnd = m_buckets + m_numBuckets; pBucket != pEnd; pBucket++)
		{
			entryCount = 0;
			entry = *pBucket;
			while (entry)
			{
				entryCount++;
				entry = entry->next;
			}
			loadsArray[entryCount]++;
			if (maxLoad < entryCount)
				maxLoad = entryCount;
		}
		//PrintDebug("Size = %d\nBuckets = %d\n----------------\n", m_numItems, m_numBuckets);
		//for (UInt32 iter = 0; iter <= maxLoad; iter++)
			//PrintDebug("%d:\t%05d (%.4f%%)", iter, loadsArray[iter], 100.0 * (double)loadsArray[iter] / m_numItems);
	}

	class Iterator
	{
		NiTPointerMap* table;
		Entry** bucket;
		Entry* entry;

		void FindNonEmpty()
		{
			for (Entry** end = &table->m_buckets[table->m_numBuckets]; bucket != end; bucket++)
				if (entry = *bucket) break;
		}

	public:
		Iterator(NiTPointerMap& _table) : table(&_table), bucket(table->m_buckets), entry(nullptr) { FindNonEmpty(); }

		explicit operator bool() const { return entry != nullptr; }
		void operator++()
		{
			entry = entry->next;
			if (!entry)
			{
				bucket++;
				FindNonEmpty();
			}
		}
		T_Data* Get() const { return entry->data; }
		UInt32 Key() const { return entry->key; }
	};

	Iterator Begin() { return Iterator(*this); }
};

#define CALL_EAX(addr) __asm mov eax, addr __asm call eax

template <typename T_Data>
__declspec(naked) T_Data* NiTPointerMap<T_Data>::Lookup(UInt32 key) const
{
	__asm
	{
		mov		eax, [esp + 4]
		xor edx, edx
		div		dword ptr[ecx + 4]
		mov		eax, [ecx + 8]
		mov		eax, [eax + edx * 4]
		test	eax, eax
		jz		done
		mov		edx, [esp + 4]
		ALIGN 16
		iterHead:
		cmp[eax + 4], edx
			jz		found
			mov		eax, [eax]
			test	eax, eax
			jnz		iterHead
			retn	4
			found:
		mov		eax, [eax + 8]
			done :
			retn	4
	}
}

template <typename T_Data>
__declspec(naked) void NiTPointerMap<T_Data>::Insert(UInt32 key, T_Data value)
{
	__asm
	{
		mov		eax, [esp + 4]
		xor edx, edx
		div		dword ptr[ecx + 4]
		mov		eax, [ecx + 8]
		lea		eax, [eax + edx * 4]
		push	eax
		inc		dword ptr[ecx + 0xC]
		CALL_EAX(0x43A010)
		pop		ecx
		mov		edx, [ecx]
		mov[eax], edx
		mov		edx, [esp + 4]
		mov[eax + 4], edx
		mov		edx, [esp + 8]
		mov[eax + 8], edx
		mov[ecx], eax
		retn	8
	}
}


// 10
// todo: NiTPointerMap should derive from this
// cleaning that up now could cause problems, so it will wait
template <typename T_Key, typename T_Data>
class NiTMapBase
{
public:
	NiTMapBase();
	~NiTMapBase();

	struct Entry
	{
		Entry	* next;	// 000
		T_Key	key;	// 004
		T_Data	data;	// 008
	};

	virtual NiTMapBase<T_Key, T_Data>*	Destructor(bool doFree);						// 000
	virtual UInt32						Hash(T_Key key);								// 001
	virtual void						Equal(T_Key key1, T_Key key2);					// 002
	virtual void						FillEntry(Entry entry, T_Key key, T_Data data);	// 003
	virtual	void						Unk_004(void * arg0);							// 004
	virtual	void						Unk_005(void);									// 005
	virtual	void						Unk_006();										// 006

	//void	** _vtbl;	// 0
	UInt32	numBuckets;	// 4
	Entry	** buckets;	// 8
	UInt32	numItems;	// C
#if RUNTIME
	DEFINE_MEMBER_FN_LONG(NiTMapBase, Lookup, bool, _NiTMap_Lookup, T_Key key, T_Data * dataOut);
#endif
};

// 14
template <typename T_Data>
class NiTStringPointerMap : public NiTPointerMap <T_Data>
{
public:
	NiTStringPointerMap();
	~NiTStringPointerMap();

	UInt32	unk010;
};

// not sure how much of this is in NiTListBase and how much is in NiTPointerListBase
// 10
template <typename T>
class NiTListBase
{
public:
	NiTListBase();
	~NiTListBase();

	struct Node
	{
		Node	* next;
		Node	* prev;
		T		* data;
	};

	virtual void	Destructor(void);
	virtual Node *	AllocateNode(void);
	virtual void	FreeNode(Node * node);

//	void	** _vtbl;	// 000
	Node	* start;	// 004
	Node	* end;		// 008
	UInt32	numItems;	// 00C
};

// 10
template <typename T>
class NiTPointerListBase : public NiTListBase <T>
{
public:
	NiTPointerListBase();
	~NiTPointerListBase();
};

// 10
template <typename T>
class NiTPointerList : public NiTPointerListBase <T>
{
public:
	NiTPointerList();
	~NiTPointerList();
};

// 4
template <typename T>
class NiPointer
{
public:
	NiPointer(T *init) : data(init)		{	}

	T	* data;

	const T&	operator *() const { return *data; }
	T&			operator *() { return *data; }

	operator const T*() const { return data; }
	operator T*() { return data; }
};

// 14
template <typename T>
class BSTPersistentList
{
public:
	BSTPersistentList();
	~BSTPersistentList();

	virtual void	Destroy(bool destroy);

//	void	** _vtbl;	// 00
	UInt32	unk04;		// 04
	UInt32	unk08;		// 08
	UInt32	unk0C;		// 0C
	UInt32	unk10;		// 10
};
