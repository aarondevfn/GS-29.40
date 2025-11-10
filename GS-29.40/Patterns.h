#pragma once

namespace Patterns
{
	//// Pawn
	constexpr const char* SetPickupTarget = "48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 A8 0F 29 78 98 44 0F 29 40 ? 44 0F 29 88 ? ? ? ? 44 0F 29 90 ? ? ? ? 44 0F 29 98 ? ? ? ? 44 0F 29 A0 ? ? ? ? 44 0F 29 A8 ? ? ? ? 44 0F 29 B0 ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 80 B9 ? ? ? ? ? 0F 28 FA F3 0F 11 7C 24 ? 49 8B D9";

	//// PlayerController
	constexpr const char* CantBuild = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 70 48 8B 1A 4D 8B F1 4D 8B F8 48 8B FA 48 8B F1 BD ? ? ? ? 48 85 DB";
	constexpr const char* ReplaceBuildingActor = "48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 A8 0F 29 78 98 44 0F 29 40 ? 44 0F 29 88 ? ? ? ? 44 0F 29 90 ? ? ? ? 44 0F 29 98 ? ? ? ? 44 0F 29 A0 ? ? ? ? 44 0F 29 A8 ? ? ? ? 44 0F 29 B0 ? ? ? ? 44 0F 29 B8 ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B AD ? ? ? ? 48 8D 3D ? ? ? ? 33 C0";

	// Beacon
	constexpr const char* InitHost = "48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B F1 4C 8D 25 ? ? ? ? 4D 8B C4";
	constexpr const char* InitListen = "4C 8B DC 49 89 5B 08 49 89 73 10 57 48 83 EC 40 48 8B 7C 24 ? 49 8B F0 48 8B 01 48 8B D9 49 89 7B E0 45 88 4B D8 4D 8B C8";
	constexpr const char* TickFlush = "48 89 5C 24 ? 55 56 57 41 56 41 57 48 8B EC 48 83 EC 60 45 33 F6 0F 29 74 24 ? 44 38 35 ? ? ? ? 4C 8D 3D ? ? ? ? 0F 28 F1 48 8B F9";

	// Replication
	constexpr const char* SendClientAdjustment = "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 8B 91 ? ? ? ? 48 8B D9 83 FA FF 74 46 3B 91 ? ? ? ? 74 3E 44 8A 89 ? ? ? ? 44 8B 81 ? ? ? ? 89 91 ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? ? 74 1C";
	constexpr const char* CreateChannelByName = "40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 45 33 ED 48 8D 05 ? ? ? ? 44 38 2D ? ? ? ? 45 8B F1"; 
	constexpr const char* SetChannelActor = "48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 45 33 FF 48 8D 35 ? ? ? ? 44 89 BD ? ? ? ? 48 8B D9 48 8B 41 28 45 8B E0"; 
	constexpr const char* CallPreReplication = "48 85 D2 0F 84 ? ? ? ? 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC 20 F6 41 58 30 48 8D 81 ? ? ? ? 4C 8B FA 48 8B F9 75 16";
	constexpr const char* ReplicateActor = "40 55 41 54 41 55 41 56 41 57 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 89 B5 ? ? ? ? 48 89 BD ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 33 FF"; 

	// Abilities
	constexpr const char* InternalTryActivateAbility = "48 89 5C 24 ? 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B 85 ? ? ? ? 8B FA";
	constexpr const char* GiveAbility = "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 49 8B 40 10 45 33 F6"; 

	// Inventory
	constexpr const char* CreatePickupData = "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 89 11 48 8B F9 48 83 C1 08 49 8B D0 49 8B D9 E8 ? ? ? ? 0F 10 03 48 8B 44 24 ? 0F 11 87 ? ? ? ? F2 0F 10 4B ?";
	constexpr const char* CreatePickupFromData = "48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 10 0F 28 0D ? ? ? ? 48 8D 35 ? ? ? ? 0F 28 15 ? ? ? ? 4C 8D 44 24 ?";
	constexpr const char* SimpleCreatePickup = "48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 49 8B D9 49 8B F8 48 8B F2 E8 ? ? ? ? 33 D2 48 85 C0 74 5C";
	constexpr const char* CreateItemEntry = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 8A 41 22 48 8D B9 ? ? ? ? 33 F6 48 C7 41 ? ? ? ? ? 48 89 71 18 83 CD FF 89 29 24 F0 89 69 04 0C 10";
	constexpr const char* CreateDefaultItemEntry = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 83 CD FF 33 F6 45 85 C0 89 29 8B C6 89 69 04 41 0F 49 C0 89 69 08";
	constexpr const char* CopyItemEntry = "48 89 5C 24 ? 57 48 83 EC 20 8B 42 0C 48 8B DA 89 41 0C 48 8B F9 8B 42 10 48 83 C2 18 89 41 10 48 83 C1 18 E8 ? ? ? ? 0F B7 43 20 48 8D 53 38 66 89 47 20 8A 47 22";
	constexpr const char* FreeItemEntry = "40 53 48 83 EC 20 48 8B D9 48 81 C1 ? ? ? ? E8 ? ? ? ? 48 8B 8B ? ? ? ? 48 85 C9 75 68 48 8D 8B ? ? ? ? E8 ? ? ? ? 48 8B 8B ? ? ? ? 48 85 C9 75 57 48 8B 8B ? ? ? ? 48 85 C9 75 52";

	// Others									
	constexpr const char* InternalGetNetMode = "48 83 EC 28 48 83 79 ? ? 48 8B D1 75 3B 48 8B 89 ? ? ? ? 48 85 C9 74 46 80 B9 ? ? ? ? ? 75 15 48 8B 81 ? ? ? ? 48 85 C0 74 24 83 B8 ? ? ? ? ? 74 1B";;
	constexpr const char* DispatchRequest = "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 41 56 41 57 48 8D 68 A1 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 48 8D 35 ? ? ? ? 41 8B D8 4C 8B F2 4C 8B F9 72 76";
}