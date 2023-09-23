#pragma once
namespace OnStealEventHandler
{
	constexpr char eventName[] = "PostalOut:OnSteal";

	void __fastcall HandleEvent()
	{
		g_eventInterface->DispatchEvent(eventName, 0);
	}

	void __declspec(naked) OnStealHook()
	{
		static UInt32 const retnAddr = 0x8BFB4F;
		static UInt32 const callWhatever = 0x48EBC0;
		_asm
		{
			call callWhatever
			call HandleEvent
			add esp, 8
			jmp retnAddr
		}
	}


	void WriteHook()
	{
		WriteRelJump(0x8BFB47, (UInt32)OnStealHook);
	}

}