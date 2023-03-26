
#include "nvse/PluginAPI.h"
#include "nvse/GameForms.h"
#include "nvse/GameObjects.h"
#include "nvse/GameUI.h"
#include "nvse/SafeWrite.h"

#include <windows.h>
#include "shlwapi.h"
#pragma comment (lib, "shlwapi.lib")

#include "matrix.h"
#include "NiNoudes.h"


// ini settings
static float g_fDlgFocusOverride = 0.0;
static float g_fSittingMaxLookingDownOverride = 40.0;
static float g_fVanityModeForceDefaultOverride = 300.0;
static float g_fCameraPosX = 0.0;
static float g_fCameraPosY = 14.0;
static float g_fCameraPosZ = 6.0;
static float g_fCameraZOffset = 12.0;


// Stores if the game is in 1st or 3rd person
static int g_isThirdPerson = 0;

// This should be set when the game force switches to 3rd person (sitting anim, knockout)
// if set the camera should be moved in front of the player's face
static int g_isForceThirdPerson = 0;

// Used to detect when POV switch to enable fake first person view
static int g_detectPOVSwitch = 0;

// Used to detect if you are entering VATS from 3rd person
static int g_detectThirdPersonVATS = 0;

// Used to detect if you are zooming in/out of dialog
static int g_detectDialogZoom = 0;

// Used to check if iron sights should be enabled (when using 3rd person arms)
static int g_enableIronSights = 0;

// Saves the 3rd person camera zoom
static float g_saveThirdPersonZoom = 0;

// Set to disable applying collisions to the animation
static float g_animSkipCollisions = 0;


const UInt32 kRetrieveAnimDataCall = 0x00950A60;

// Retrieves the 3rd person actor anim data
ActorAnimData* GetActorAnimData(Actor* actor) {
	_asm
	{
		push 0
		mov ecx, actor
		call kRetrieveAnimDataCall
	}
}

const UInt32 kApplyAnimDataCall = 0x00493BD0;

// Applies the anim data to the skeleton
void ApplyAnimData(ActorAnimData* animData) {
	_asm
	{
		mov ecx, animData
		call kApplyAnimDataCall
	}
}

const UInt32 kGetCurrentActionCall = 0x008A7570;

// Returns the actor's current action
int GetCurrentAction(Actor* actor) {
	_asm
	{
		mov ecx, actor
		call kGetCurrentActionCall
	}
}


int GetKnockedState(Actor* actor) {
	_asm
	{
		mov edx, actor
		mov ecx, [edx + 0x68] // HighProcess
		mov eax, [ecx]
		mov edx, [eax + 0x40C]
		call edx
	}
}

int GetSitSleepState(Actor* actor) {
	return *((UInt32*)actor + (0x1AC >> 2));
}


const UInt32 kGetMovementFlagsCall = 0x008846E0;

// Gets the actor's movement flags
int GetMovementFlags(Actor* actor) {
	_asm
	{
		mov ecx, actor
		call kGetMovementFlagsCall
	}
}

int IsThirdPerson() {
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	int isThirdPerson = *((UInt8*)player + 0x64A);
	int isThirdPersonFlag = player->bThirdPerson;

	// Transitioning between POV
	if (isThirdPerson != isThirdPersonFlag) {
		return 2;
	}
	return isThirdPerson;
}


// Returns true if tfc is enabled
bool GetToggleFlyCam() {
	_asm
	{
		mov eax, 0x011DEA0C
		mov eax, [eax]
		mov al, [eax + 0x6]
	}
}


// Returns true if the actor is dead
bool GetDead(Actor* actor) {
	_asm
	{
		mov ecx, actor
		mov edx, [ecx]
		mov eax, [edx + 0x22C]
		push 1
		call eax
	}
}


// Returns the current state of the pipboy menu animation
int GetPipboyState() {
	InterfaceManager* im = InterfaceManager::GetSingleton();
	return *((UInt8*)im + 0x4BC);
}

// Returns true if using weapon scope or entering/exiting pipboy menu
int GetForceFirstPersonFlag() {
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	return *((UInt8*)player + 0x64F);
}

int GetCutsceneControlsDisabled() {
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	int disabledControlFlags = *((UInt32*)player + (0x680 >> 2));
	return ((disabledControlFlags & PlayerCharacter::kControlFlag_Movement) && (disabledControlFlags & PlayerCharacter::kControlFlag_Look));
}


const UInt32 kInMenuModeCall = 0x00702360;

// Returns true if you are in menumode
bool InMenuMode() {
	_asm
	{
		call kInMenuModeCall
	}
}


const UInt32 kMenuModeCall = 0x00702680;

// Returns true if you are in the specified menumode
bool MenuMode(int menumode) {
	_asm
	{
		mov eax, menumode
		push 0
		push eax
		call kMenuModeCall
		add esp, 8
	}
}

bool InDialogMenu() {
	return (MenuMode(1009) || g_detectDialogZoom);
}

bool InVATSMenu() {
	return MenuMode(1056);
}

bool InRaceSexMenu() {
	return MenuMode(1036);
}


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


// Searches the parent node to find the node with the specified name
NiNoude* FindParentNode(NiNoude* node, const char* name) {
	while (node && node->m_pcName) {
		if (strcmp(node->m_pcName, name) == 0) {
			return node;
		}

		node = (NiNoude*)node->m_parent;
	}

	return 0;
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


// Updates skeleton using forward kinematics
void FKSolve(NiNoude* node) {

	if (node) {

		// Determine the NiObject type from RTTI
		int nodeType = 0;

		NiRTTI* rtti = node->GetType();
		while (rtti != 0) {
			// NiNode
			if ((UInt32)rtti == 0x011F4428) {
				nodeType = 2;
				break;
				// NiAVObject
			}
			else if ((UInt32)rtti == 0x011F4280) {
				nodeType = 1;
				break;
			}

			rtti = rtti->parent;
		}

		if (nodeType == 0) {
			return;
		}

		if (node->m_parent) {
			NiNoude* parent = (NiNoude*)node->m_parent;
			NiVector3 v;

			// Translation
			MatrixVectorMultiply(&v, &(parent->m_worldRotate), &(node->m_localTranslate));
			node->m_worldTranslate.x = parent->m_worldTranslate.x + v.x * parent->m_worldScale;
			node->m_worldTranslate.y = parent->m_worldTranslate.y + v.y * parent->m_worldScale;
			node->m_worldTranslate.z = parent->m_worldTranslate.z + v.z * parent->m_worldScale;

			// Rotation
			MatrixMultiply(&(node->m_worldRotate), &(parent->m_worldRotate), &(node->m_localRotate));

			// Scale
			node->m_worldScale = parent->m_worldScale * node->m_localScale;
		}
		else {
			// no parent so world translation, rotation, scale is the same as local
			node->m_worldTranslate.x = node->m_localTranslate.x;
			node->m_worldTranslate.y = node->m_localTranslate.y;
			node->m_worldTranslate.z = node->m_localTranslate.z;

			node->m_worldRotate.data[0] = node->m_localRotate.data[0];
			node->m_worldRotate.data[1] = node->m_localRotate.data[1];
			node->m_worldRotate.data[2] = node->m_localRotate.data[2];
			node->m_worldRotate.data[3] = node->m_localRotate.data[3];
			node->m_worldRotate.data[4] = node->m_localRotate.data[4];
			node->m_worldRotate.data[5] = node->m_localRotate.data[5];
			node->m_worldRotate.data[6] = node->m_localRotate.data[6];
			node->m_worldRotate.data[7] = node->m_localRotate.data[7];
			node->m_worldRotate.data[8] = node->m_localRotate.data[8];

			node->m_worldScale = node->m_localScale;
		}

		// Apply recursively to children nodes
		if (nodeType == 2) {
			int len = node->m_children.numObjs;
			for (int i = 0; i < len; i++) {
				FKSolve((NiNoude*)node->m_children[i]);
			}
		}
	}
}


// Sets visibility of head/arms nodes
void UpdateSkeletonNodes(bool toggleNodes) {

	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	ActorAnimData* animData = GetActorAnimData(player);

	NiNoude* nLClavicle = FindNode(GetRootNode(0), "Bip01 L Clavicle");
	NiNoude* nRClavicle = FindNode(GetRootNode(0), "Bip01 R Clavicle");

	//MON AJOUT
	NiNoude* nSpine = FindNode(GetRootNode(0), "Bip01 Spine");
	NiNoude* nRThigh = FindNode(GetRootNode(0), "Bip01 R Thigh");
	NiNoude* nRFoot = FindNode(GetRootNode(0), "Bip01 R Foot");
	NiNoude* nRCalf = FindNode(GetRootNode(0), "Bip01 R Calf");
	NiNoude* nRToe = FindNode(GetRootNode(0), "Bip01 R Toe0");
	//FIN DE MON AJOUT

	NiNoude* nHead = animData->nHead;

	if (toggleNodes) {
		nLClavicle->m_localScale = 1;
		nRClavicle->m_localScale = 1;

		//MON AJOUT
		nSpine->m_localScale = 1;
		nRThigh->m_localScale = 1;
		nRFoot->m_localScale = 1;
		nRCalf->m_localScale = 1;
		nRToe->m_localScale = 1;
		//FIN DE MON AJOUT

		nHead->m_localScale = 1;
	}
	else {
		nLClavicle->m_localScale = 0.001;
		nRClavicle->m_localScale = 0.001;

		//MON AJOUT
		nSpine->m_localScale = 0.001;
		nRThigh->m_localScale = 0.001;
		nRFoot->m_localScale = 0.001;
		nRCalf->m_localScale = 0.001;
		nRToe->m_localScale = 0.001;
		//FIN DE MON AJOUT

		nHead->m_localScale = 0.001;
	}
}


// Aligns the 1st person skeleton with 3rd person skeleton
void TranslateFirstPerson() {

	// Calculate displacement of first person camera
	NiNoude* nRoot1st = GetRootNode(1);
	NiNoude* nCamera1st = FindNode(nRoot1st, "Camera1st");
	NiVector3 vFirstPerson;
	NiVector3 vThirdPerson;

	vFirstPerson.x = nCamera1st->m_worldTranslate.x - nRoot1st->m_worldTranslate.x;
	vFirstPerson.y = nCamera1st->m_worldTranslate.y - nRoot1st->m_worldTranslate.y;
	vFirstPerson.z = nCamera1st->m_worldTranslate.z - nRoot1st->m_worldTranslate.z;


	// Calculate displacement of third person camera
	NiNoude* nRoot3rd = GetRootNode(0);
	NiNoude* nHead = FindNode(nRoot3rd, "Bip01 Head");
	NiVector3* tHead = &(nHead->m_worldTranslate);
	NiVector3 v0, v1;

	v0.x = g_fCameraPosX * nRoot3rd->m_worldScale;
	v0.y = g_fCameraPosY * nRoot3rd->m_worldScale;
	v0.z = g_fCameraPosZ * nRoot3rd->m_worldScale;
	MatrixVectorMultiply(&v1, &(nRoot3rd->m_worldRotate), &v0);


	vThirdPerson.x = (tHead->x + v1.x) - nRoot3rd->m_worldTranslate.x;
	vThirdPerson.y = (tHead->y + v1.y) - nRoot3rd->m_worldTranslate.y;
	vThirdPerson.z = (tHead->z + v1.z) - nRoot3rd->m_worldTranslate.z;


	// Calculate the difference between the root node positions
	NiVector3 vTranslateRoot;
	vTranslateRoot.x = nRoot3rd->m_localTranslate.x - nRoot1st->m_localTranslate.x;
	vTranslateRoot.y = nRoot3rd->m_localTranslate.y - nRoot1st->m_localTranslate.y;
	vTranslateRoot.z = nRoot3rd->m_localTranslate.z - nRoot1st->m_localTranslate.z;


	// Translate 1st person root node to new position
	nRoot1st->m_localTranslate.x += vThirdPerson.x - vFirstPerson.x + vTranslateRoot.x;
	nRoot1st->m_localTranslate.y += vThirdPerson.y - vFirstPerson.y + vTranslateRoot.y;
	nRoot1st->m_localTranslate.z += vThirdPerson.z - vFirstPerson.z + vTranslateRoot.z;
}


// Translates the 3rd person skeleton to correct position
void TranslateThirdPerson() {
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	NiNoude* nRoot3rd = GetRootNode(0);
	int sitSleepFlag = GetSitSleepState(player);

	// Translate 3rd person back to avoid seeing into objects
	if (!InDialogMenu() && !InVATSMenu() && sitSleepFlag == 0) {
			NiNoude* nRoot1st = GetRootNode(1);
			NiNoude* nCamera1st = FindNode(nRoot1st, "Camera1st");

			ActorAnimData* animData = GetActorAnimData(player);
			NiNoude* nHead3rd = animData->nHead;

			NiVector3 v0, v1;
			v0.x = g_fCameraPosX * nRoot3rd->m_worldScale;
			v0.y = g_fCameraPosY * nRoot3rd->m_worldScale;
			v0.z = g_fCameraPosZ * nRoot3rd->m_worldScale;
			MatrixVectorMultiply(&v1, &(nRoot3rd->m_worldRotate), &v0);

			nRoot3rd->m_localTranslate.x += nCamera1st->m_worldTranslate.x - (nHead3rd->m_worldTranslate.x + v1.x);
			nRoot3rd->m_localTranslate.y += nCamera1st->m_worldTranslate.y - (nHead3rd->m_worldTranslate.y + v1.y);
			nRoot3rd->m_localTranslate.z += nCamera1st->m_worldTranslate.z - (nHead3rd->m_worldTranslate.z + v1.z);
	}
}


// Updates the player 3rd person skeleton
void UpdateAnimation() {
	NiNoude* nRoot3rd = GetRootNode(0);

	// ensure body is disabled when in tfc mode
	if (GetToggleFlyCam()) {
		if ((nRoot3rd->m_flags & 0x00000001) == 0) {
			nRoot3rd->m_flags |= 0x00000001;
		}
	}

	NiVector3 vLocalTranslate;
	vLocalTranslate.x = nRoot3rd->m_localTranslate.x;
	vLocalTranslate.y = nRoot3rd->m_localTranslate.y;
	vLocalTranslate.z = nRoot3rd->m_localTranslate.z;

	TranslateThirdPerson();

	UpdateSkeletonNodes(0);
	ActorAnimData* animData = GetActorAnimData(PlayerCharacter::GetSingleton());
	ApplyAnimData(animData);
	UpdateSkeletonNodes(1);
	

	// Fix the camera moving backwards during Dialog/VATS menu bug
	if (InMenuMode()) {
		nRoot3rd->m_localTranslate.x = vLocalTranslate.x;
		nRoot3rd->m_localTranslate.y = vLocalTranslate.y;
		nRoot3rd->m_localTranslate.z = vLocalTranslate.z;
	}
}


void UpdateActorAnim(ActorAnimData* animData) {
	if (g_isThirdPerson == 0 && animData->actor == PlayerCharacter::GetSingleton()) {
		// 1st person animation data
		if (animData != GetActorAnimData(PlayerCharacter::GetSingleton())) {

			// Checks if you are zooming in/out of dialog
			float dialogZoomPercent = *(float*)0x011E0D48;
			if (dialogZoomPercent != 0) {
				g_detectDialogZoom = 1;
			}
			else {
				g_detectDialogZoom = 0;
			}

			if (InDialogMenu() == 0 && InRaceSexMenu() == 0 && GetCutsceneControlsDisabled() == 0 && GetDead(PlayerCharacter::GetSingleton()) == 0) {
				UpdateAnimation();

				// Update 1st person animData
				ApplyAnimData(animData);
			}
		}
	}
}


const UInt32 kAnimationHook = 0x0088882F;
const UInt32 kAnimationReturn = 0x00888834;

static _declspec(naked) void HookAnimation(void)
{
	_asm
	{
		//overwritten code
		call kApplyAnimDataCall

		pushad
		mov ecx, [ebp + 0x8]
		push ecx
		call UpdateActorAnim
		add esp, 4
		popad

		jmp kAnimationReturn
	}
}


void UpdateSwitchPOV() {
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	g_isThirdPerson = player->bThirdPerson;

	NiNoude* nRoot1st = GetRootNode(1);
	NiNoude* nRoot3rd = GetRootNode(0);

	if (g_detectPOVSwitch) {
		g_isForceThirdPerson = 1;
	}
	else {
		g_isForceThirdPerson = 0;
	}

	g_detectThirdPersonVATS = 0;

	if (g_saveThirdPersonZoom) {
		if (player->bThirdPerson == 0) {
			float* thirdPersonZoom = (float*)0x011E0B5C;
			*thirdPersonZoom = g_saveThirdPersonZoom;
			g_saveThirdPersonZoom = 0;
		}
	}


	*((UInt8*)player + 0x64B) = player->bThirdPerson;

	if ((player->bThirdPerson || GetForceFirstPersonFlag()) && GetPipboyState() == 0) {
		nRoot1st->m_flags |= 0x00000001;
		nRoot3rd->m_flags &= 0xFFFFFFFE;
	}
	else {
		nRoot1st->m_flags &= 0xFFFFFFFE;
		nRoot3rd->m_flags &= 0xFFFFFFFE;
	}
}


const UInt32 kSwitchPOVHook = 0x009520D8;
const UInt32 kSwitchPOVReturn = 0x009520DF;

static _declspec(naked) void HookSwitchPOV(void)
{
	_asm
	{
		//overwritten code
		mov fs : [0] , ecx

		pushad
		call UpdateSwitchPOV
		popad

		jmp kSwitchPOVReturn
	}
}


void UpdateCamera(NiNoude* nCamera) {

	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	g_isThirdPerson = IsThirdPerson();

	NiNoude* nRoot1st = GetRootNode(1);
	NiNoude* nRoot3rd = GetRootNode(0);

		// Controls 3rd person body visibility
		if (IsThirdPerson() == 0 && InRaceSexMenu() == 0 && (InDialogMenu() || InVATSMenu() || GetCutsceneControlsDisabled())) {
			if ((nRoot3rd->m_flags & 0x00000001) == 0) {
				nRoot3rd->m_flags |= 0x00000001;
			}
		}
		else {
			if ((nRoot3rd->m_flags & 0x00000001) == 1) {
				nRoot3rd->m_flags &= 0xFFFFFFFE;
			}
		}


	UInt8 testAnim1 = *((UInt8*)player + 0x798); // Sit Down
	UInt8 testAnim2 = *((UInt8*)player + 0x799); // Sit GetUp
	UInt8 testAnim3 = *((UInt8*)player + 0x79B); // Perk Animation
	if (g_saveThirdPersonZoom != 0 && testAnim1 == 0 && testAnim2 == 0 && testAnim3 == 0) {
		float* thirdPersonZoom = (float*)0x011E0B5C;
		*thirdPersonZoom = g_saveThirdPersonZoom;
		g_saveThirdPersonZoom = 0;
	}


	if ((IsThirdPerson() && !g_isForceThirdPerson) || InDialogMenu() || GetCutsceneControlsDisabled()) {
		return;
	}

	ActorAnimData* animData = GetActorAnimData(player);

	NiNoude* nHead = animData->nHead;
	NiVector3* vHead = &(nHead->m_worldTranslate);
	NiVector3* vCamera = &(nCamera->m_localTranslate);
	NiVector3 v0, v1;

	// Positions the camera during fake first person
	if (g_isForceThirdPerson || GetSitSleepState(player) >= 8) {
		v0.x = g_fCameraPosX * nRoot3rd->m_worldScale;
		v0.y = g_fCameraPosY * nRoot3rd->m_worldScale;
		v0.z = g_fCameraPosZ * nRoot3rd->m_worldScale;
		MatrixVectorMultiply(&v1, &(nHead->m_worldRotate), &v0);

		if (g_isForceThirdPerson && (GetDead(player) || GetKnockedState(player))) {


			UpdateSkeletonNodes(1); //MON AJOUT
			nRoot3rd->m_localTranslate.z += g_fCameraZOffset * nRoot3rd->m_worldScale;

			g_animSkipCollisions = 1;
			ActorAnimData* animData = GetActorAnimData(player);
			ApplyAnimData(animData);
			g_animSkipCollisions = 0;
		}

		vCamera->x = vHead->x + v1.x;
		vCamera->y = vHead->y + v1.y;
		vCamera->z = vHead->z + v1.z;

		
			NiMatrix33 mCameraRot;
			mCameraRot.data[0] = 0;
			mCameraRot.data[1] = 0;
			mCameraRot.data[2] = 1;
			mCameraRot.data[3] = 0;
			mCameraRot.data[4] = 1;
			mCameraRot.data[5] = 0;
			mCameraRot.data[6] = -1;
			mCameraRot.data[7] = 0;
			mCameraRot.data[8] = 0;

			MatrixMultiply(&(nCamera->m_localRotate), &(nHead->m_worldRotate), &mCameraRot);

	}
	else if (GetSitSleepState(player)) {

		v0.x = g_fCameraPosX * nRoot3rd->m_worldScale;
		v0.y = g_fCameraPosY * nRoot3rd->m_worldScale;
		v0.z = g_fCameraPosZ * nRoot3rd->m_worldScale;
		MatrixVectorMultiply(&v1, &(nRoot3rd->m_worldRotate), &v0);

		vCamera->x = vHead->x + v1.x;
		vCamera->y = vHead->y + v1.y;
		vCamera->z = vHead->z + v1.z;
	}

}


const UInt32 kCameraHook = 0x0094BDDA;
const UInt32 kCameraReturn = 0x0094BDDF;
const UInt32 kGetCameraNodeCall = 0x00558310;

static _declspec(naked) void HookCamera(void)
{
	_asm
	{
		//overwritten code
		push 0
		push 0
		push ecx

		pushad

		// Returns the camera node
		mov ecx, [ebp - 0x68]
		call kGetCameraNodeCall

		push eax
		call UpdateCamera
		add esp, 4

		popad

		jmp kCameraReturn
	}
}


const UInt32 kForcePOVCall = 0x00950340;





const UInt32 kSwitchPOVCall = 0x00951A10;




const UInt32 kAnimCameraRotateHook = 0x0094550C;

bool UpdateAnimCameraRotate() {
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	UInt8 testAnim1 = *((UInt8*)player + 0x798); // Sit Down
	UInt8 testAnim2 = *((UInt8*)player + 0x799); // Sit GetUp
	UInt8 testAnim3 = *((UInt8*)player + 0x79A); // Drink Water
	UInt8 testAnim4 = *((UInt8*)player + 0x79B); // Perk Animation
	bool animPlaying = testAnim1 || testAnim2 || testAnim3 || testAnim4;
	if (animPlaying) {
		int isThirdPerson = *((UInt8*)player + 0x64A);
		if ((isThirdPerson == 0 && (testAnim3 || testAnim4)) || g_isForceThirdPerson || (g_isForceThirdPerson == 0 && isThirdPerson)) {
			return 1;
		}
	}

	return 0;
}


void UpdateResetSkeletonOnDeath() {
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	int isThirdPerson = *((UInt8*)player + 0x64A);

	if (isThirdPerson == 0) {
		UpdateSkeletonNodes(1);

		NiNoude* nRoot3rd = GetRootNode(0);
		FKSolve(nRoot3rd);
	}
}

const UInt32 kResetSkeletonOnDeathHook = 0x0089EA68;
const UInt32 kResetSkeletonOnDeathReturn = 0x0089EA6D;
const UInt32 kResetSkeletonOnDeathCall = 0x008324E0;

static _declspec(naked) void HookResetSkeletonOnDeath(void) {
	_asm
	{
		// overwritten code
		call kResetSkeletonOnDeathCall

		pushad
		call UpdateResetSkeletonOnDeath
		popad

		jmp kResetSkeletonOnDeathReturn
	}
}


void UpdateFixForcePOVZoom() {
	float* thirdPersonZoom = (float*)0x011E0B5C;
	g_saveThirdPersonZoom = *thirdPersonZoom;
}

const UInt32 kFixForcePOVZoomHook = 0x00950387;
const UInt32 kFixForcePOVZoomReturn = 0x0095038C;

static _declspec(naked) void HookFixForcePOVZoom(void) {
	_asm
	{
		pushad
		call UpdateFixForcePOVZoom
		popad

		// overwritten code
		mov ecx, 0x011CD498
		jmp kFixForcePOVZoomReturn
	}
}


int UpdateFixWeaponZoom() {
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	int isThirdPerson = player->bThirdPerson;
	return !isThirdPerson;
}

const UInt32 kFixWeaponZoomHook = 0x009504BC;
const UInt32 kFixWeaponZoomReturn = 0x009504C1;

// Fix using the weapon scope in 1st person switching you to 3rd person
static _declspec(naked) void HookFixWeaponZoom(void)
{
	_asm
	{
		call UpdateFixWeaponZoom
		jmp kFixWeaponZoomReturn
	}
}


const UInt32 kFixIntBodyDisappearingHook = 0x0054A1C9;
const UInt32 kFixIntBodyDisappearingReturn = 0x0054A1CE;

// Fix a bug with the 3rd person body disappearing sometimes when in interiors
static _declspec(naked) void HookFixIntBodyDisappearing(void)
{
	_asm
	{
		mov eax, 0x011DEA3C // player
		push 0
		mov ecx, [eax]
		call kRetrieveRootNodeCall
		jmp kFixIntBodyDisappearingReturn
	}
}


const UInt32 kApplyDarknessShader01Call = 0x00450B80;
const UInt32 kApplyDarknessShader02Call = 0x00B5D9F0;

void ApplyDarknessShader(bool firstPerson) {
	NiNoude* nRoot = GetRootNode(firstPerson);
	_asm
	{
		mov eax, nRoot
		push 1
		push eax
		push 0
		call kApplyDarknessShader01Call
		add esp, 4
		mov ecx, eax
		call kApplyDarknessShader02Call
	}
}


const UInt32 kFixDarknessShaderHook = 0x00943841;
const UInt32 kFixDarknessShaderReturn = 0x00943865;

// Fix a bug with the 3rd person body appearing black when in interiors
static _declspec(naked) void HookFixDarknessShader(void)
{
	_asm
	{
		push 1
		call ApplyDarknessShader
		add esp, 4

		push 0
		call ApplyDarknessShader
		add esp, 4

		jmp kFixDarknessShaderReturn
	}
}


const UInt32 kFixBloodShaderHook = 0x0088EC59;
const UInt32 kFixBloodShaderReturn = 0x0088EC61;

// Fix a bug where blood does not appear 
// on the 3rd person body when getting hit
static _declspec(naked) void HookFixBloodShader(void)
{
	_asm
	{
		mov edx, 0x011DEA3C // player
		mov edx, [edx]
		cmp edx, ecx
		jne not_player

		push 0
		call kRetrieveRootNodeCall
		jmp kFixBloodShaderReturn

		not_player :
		// overwritten code
		mov edx, [eax + 0x1D0]
			call edx
			jmp kFixBloodShaderReturn
	}
}


// Mute 3rd person sounds if in 1st person
int CheckFixSounds(ActorAnimData* animData) {
	PlayerCharacter* player = PlayerCharacter::GetSingleton();
	ActorAnimData* playerAnimData = GetActorAnimData(player);
	NiNoude* nRoot1st = GetRootNode(1);
	NiNoude* nRoot3rd = GetRootNode(0);
	bool bothVisible = (!(nRoot1st->m_flags & 0x00000001) && !(nRoot3rd->m_flags & 0x00000001));
	return (bothVisible && (animData == playerAnimData));
}


const UInt32 kFixAnimSoundsHook = 0x00493654;
const UInt32 kFixAnimSoundsReturn01 = 0x0049365A;
const UInt32 kFixAnimSoundsReturn02 = 0x00493686;

// Fix the double sound bug
static _declspec(naked) void HookFixAnimSounds(void)
{
	_asm
	{
		mov ecx, [ebp - 0x18C] // actor anim data
		push ecx
		call CheckFixSounds
		add esp, 4

		test eax, eax
		jne skip_player_3rd_sound

		// overwritten code
		mov eax, [ebp - 0x110] // actor
		jmp kFixAnimSoundsReturn01

		skip_player_3rd_sound :
		jmp kFixAnimSoundsReturn02
	}
}


const UInt32 kAnimCollisionHook = 0x00C817F2;
const UInt32 kAnimCollisionReturn01 = 0x00C8180D;
const UInt32 kAnimCollisionReturn02 = 0x00C817F8;

// Allows disabling applying the collision object to animation
static _declspec(naked) void HookAnimCollision(void)
{
	_asm
	{
		//overwritten code
		cmp byte ptr[esi + 0x14], 0
		jne skip_collision

		push eax
		mov eax, g_animSkipCollisions
		test eax, eax
		pop eax
		jne skip_collision

		jmp kAnimCollisionReturn01

		skip_collision :
		jmp kAnimCollisionReturn02
	}
}


const UInt32 kDialogFocusOverrideHook = 0x009533FC;
const UInt32 kDialogFocusOverrideReturn = 0x00953403;

static _declspec(naked) void HookDialogFocusOverride(void)
{
	_asm
	{
		lea ecx, g_fDlgFocusOverride
		fld[ecx]
		jmp kDialogFocusOverrideReturn
	}
}


// Gets the corrent root node for bullet/casing spawn
// when using iron sights with 3rd person arms
NiNoude* GetRootNodeIronSights() {
	if (g_isThirdPerson == 0 && g_enableIronSights) {
		return GetRootNode(1);
	}
	else {
		return GetRootNode(0);
	}
}



extern "C" {

	bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
	{
		//_MESSAGE("query");

		// fill out the info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "EnhancedCamera";
		info->version = 1;


		// version checks
		if (!nvse->isEditor)
		{
			if (nvse->nvseVersion < NVSE_VERSION_INTEGER) {
				//_ERROR("NVSE version too old (got %08X expected at least %08X)", nvse->nvseVersion, NVSE_VERSION_INTEGER);
				return false;
			}
		}
		else
		{
			return false; // not using the editor
		}

		// version checks pass
		return true;
	}


	bool NVSEPlugin_Load(const NVSEInterface* nvse)
	{

		if (!nvse->isEditor) {
			WriteRelJump(kSwitchPOVHook, (UInt32)HookSwitchPOV);
			WriteRelJump(kCameraHook, (UInt32)HookCamera);

			WriteRelJump(kAnimationHook, (UInt32)HookAnimation);

			// Disables a jump that prevents the 1st person body 
			// from being visible if the 3rd person body is visible
			SafeWrite16(0x00874D86, 0x9090);
			SafeWrite32(0x00874D88, 0x90909090);

			WriteRelJump(kFixWeaponZoomHook, (UInt32)HookFixWeaponZoom);
			WriteRelJump(kFixIntBodyDisappearingHook, (UInt32)HookFixIntBodyDisappearing);
			WriteRelJump(kFixDarknessShaderHook, (UInt32)HookFixDarknessShader);
			WriteRelJump(kFixBloodShaderHook, (UInt32)HookFixBloodShader);
			WriteRelJump(kFixAnimSoundsHook, (UInt32)HookFixAnimSounds);

			// Fix DOF visual bug
			SafeWrite8(0x00870EC3, 0xEB);

			// Fix jerky 3rd person body animations when in 1st person bug
			SafeWrite16(0x008BCEAD, 0xB8);
			SafeWrite32(0x008BCEAE, 0x00000001);

			

			// Fix missing head/arms when player dies in first person
			WriteRelJump(kResetSkeletonOnDeathHook, (UInt32)HookResetSkeletonOnDeath);

			// Hook used to disable applying the collision object to the animation
			WriteRelJump(kAnimCollisionHook, (UInt32)HookAnimCollision);

			// Prevents chase cam distance from being reset during certain animations
			WriteRelJump(kFixForcePOVZoomHook, (UInt32)HookFixForcePOVZoom);


			// Fix body becoming disabled when opening pipboy
			SafeWrite8(0x009504F2, 0xEB);
			SafeWrite8(0x0086FFF4, 0x00);

			// Override dialog focus
			if (g_fDlgFocusOverride != 0) {
				WriteRelJump(kDialogFocusOverrideHook, (UInt32)HookDialogFocusOverride);
			}

			// Override sitting max looking down angle
			if (g_fSittingMaxLookingDownOverride != 40) {
				*(float*)0x011CD8D0 = g_fSittingMaxLookingDownOverride;
			}

			// Override 3rd person force zoom distance setting
			if (g_fVanityModeForceDefaultOverride != 300) {
				*(float*)0x0011CD49C = g_fVanityModeForceDefaultOverride;
			}
		}

		return true;
	}

};
