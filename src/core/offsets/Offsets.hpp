#pragma once

#include "GeneratedOffsets.hpp"

namespace offsets
{
	// client.dll
	inline DWORD entityList;
	inline DWORD viewMatrix;
	inline DWORD localPlayerController;
	inline DWORD globalVars;
	inline DWORD plantedC4;
	inline DWORD weaponC4;

	// engine2.dll
	inline DWORD buildNumber;

	namespace controller {
		constexpr std::ptrdiff_t m_iPing = generated_offsets::m_iPing; // uint32
		constexpr std::ptrdiff_t m_hPawn = generated_offsets::m_hPawn; // CHandle<C_BasePlayerPawn>
		constexpr std::ptrdiff_t m_steamID = generated_offsets::m_steamID; // uint64
		constexpr std::ptrdiff_t m_iszPlayerName = generated_offsets::m_iszPlayerName; // char[128]
		constexpr std::ptrdiff_t m_bIsLocalPlayerController = generated_offsets::m_bIsLocalPlayerController; // bool
		constexpr std::ptrdiff_t m_pInGameMoneyServices = generated_offsets::m_pInGameMoneyServices; // CCSPlayerController_InGameMoneyServices*
		constexpr std::ptrdiff_t m_iAccount = generated_offsets::m_iAccount; // int32 - CCSPlayerController_InGameMoneyServices 
	}

	namespace pawn {
		constexpr std::ptrdiff_t m_vOldOrigin = generated_offsets::m_vOldOrigin; // Vector
		constexpr std::ptrdiff_t m_iHealth = generated_offsets::m_iHealth; // int32
		constexpr std::ptrdiff_t m_iTeamNum = generated_offsets::m_iTeamNum; // uint8
		constexpr std::ptrdiff_t m_bIsScoped = generated_offsets::m_bIsScoped; // bool
		constexpr std::ptrdiff_t m_ArmorValue = generated_offsets::m_ArmorValue; // int32
		constexpr std::ptrdiff_t m_bIsDefusing = generated_offsets::m_bIsDefusing; // bool
		constexpr std::ptrdiff_t m_vecAbsVelocity = generated_offsets::m_vecAbsVelocity; // Vector

		constexpr std::ptrdiff_t m_pGameSceneNode = generated_offsets::m_pGameSceneNode; // CGameSceneNode*
		
		constexpr std::ptrdiff_t m_entitySpottedState = generated_offsets::m_entitySpottedState; // EntitySpottedState_t
		constexpr std::ptrdiff_t m_bSpottedByMask = generated_offsets::m_bSpottedByMask; // uint32[2] - EntitySpottedState_t
		
		constexpr std::ptrdiff_t m_flFlashOverlayAlpha = generated_offsets::m_flFlashOverlayAlpha; // float32 - C_CSPlayerPawnBase 
		constexpr std::ptrdiff_t m_angEyeAngles = generated_offsets::m_angEyeAngles; // QAngle
		
		constexpr std::ptrdiff_t m_pWeaponServices = generated_offsets::m_pWeaponServices; // CPlayer_WeaponServices*
		constexpr std::ptrdiff_t m_hActiveWeapon = generated_offsets::m_hActiveWeapon; // CHandle<C_BasePlayerWeapon> - CPlayer_WeaponServices
		constexpr std::ptrdiff_t m_AttributeManager = generated_offsets::m_AttributeManager; // C_AttributeContainer - C_EconEntity (parent of C_BasePlayerWeapon)
		constexpr std::ptrdiff_t m_Item = generated_offsets::m_Item; // C_EconItemView - C_AttributeContainer
		constexpr std::ptrdiff_t m_iItemDefinitionIndex = generated_offsets::m_iItemDefinitionIndex; // uint16 - C_EconItemView
		constexpr std::ptrdiff_t m_iClip1 = generated_offsets::m_iClip1; // int32 - C_BasePlayerWeapon
		constexpr std::ptrdiff_t m_bInReload = generated_offsets::m_bInReload; // bool - C_CSWeaponBase
		constexpr std::ptrdiff_t m_pObserverServices = generated_offsets::m_pObserverServices; // CPlayer_ObserverServices*
	}

	namespace bomb {
		constexpr std::ptrdiff_t m_isPlanted = 0x8; // unk
		constexpr std::ptrdiff_t m_bC4Activated = generated_offsets::m_bC4Activated; // bool
		constexpr std::ptrdiff_t m_nBombSite = generated_offsets::m_nBombSite; // int32
		constexpr std::ptrdiff_t m_bBeingDefused = generated_offsets::m_bBeingDefused; // bool
		constexpr std::ptrdiff_t m_flDefuseCountDown = generated_offsets::m_flDefuseCountDown; // GameTime_t

		constexpr std::ptrdiff_t m_vecAbsOrigin = generated_offsets::m_vecAbsOrigin; // VectorWS - CGameSceneNode 
	}

	namespace bone {
		constexpr std::ptrdiff_t m_modelState = generated_offsets::m_modelState; // CModelState
	}

	namespace observerServices {
		constexpr std::ptrdiff_t m_iObserverMode = generated_offsets::m_iObserverMode;
		constexpr std::ptrdiff_t m_hObserverTarget = generated_offsets::m_hObserverTarget;
		constexpr std::ptrdiff_t m_bForcedObserverMode = generated_offsets::m_bForcedObserverMode; // bool
	}

	namespace global {
		constexpr std::ptrdiff_t maxClients = 0x10;
		constexpr std::ptrdiff_t currentMapName = 0x180;
		constexpr std::ptrdiff_t currentTime = 0x2C;
	}

	namespace signatures
	{
		const std::string viewMatrix = "48 8D 0D ?? ?? ?? ?? 48 C1 E0 06";
		const std::string globalVars = "48 89 15 ?? ?? ?? ?? 48 89 42";
		const std::string entityList = "48 8B 0D ?? ?? ?? ?? 48 89 7C 24 ?? 8B FA C1 EB";
		const std::string localPlayerController = "48 8B 05 ?? ?? ?? ?? 41 89 BE";
		const std::string plantedC4 = "48 8b 1d ?? ?? ?? ?? 45 32 f6";
		const std::string weaponC4 =
			"48 89 05 ?? ?? ?? ?? "
			"F7 C1 ?? ?? ?? ?? "
			"74 ?? "
			"81 E1 ?? ?? ?? ?? "
			"89 0D ?? ?? ?? ?? "
			"8B 05 ?? ?? ?? ?? "
			"89 1D ?? ?? ?? ?? "
			"EB ?? "
			"48 8B 15 ?? ?? ?? ?? "
			"48 8B 5C 24 ?? "
			"FF C0 "
			"89 05 ?? ?? ?? ?? "
			"48 8B C6 48 89 34 EA 80 BE";

#if 0
		const std::string localPlayerPawn = "48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 83 EC ?? 8B 0D";

		const std::string csgoInput = "48 89 05 ?? ?? ?? ?? 0F 57 C0 0F 11 05";
		const std::string viewAngles = "F2 42 0F 10 84 28 ?? ?? ?? ??";
#endif

		const std::string buildNumber = "89 05 ?? ?? ?? ?? 48 8d 0d ?? ?? ?? ?? ff 15 ?? ?? ?? ?? 48 8b 0d";

	}
}
