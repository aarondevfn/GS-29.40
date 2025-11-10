#pragma once

#include <algorithm>
#include <time.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <sstream>

namespace Globals
{
	UFortEngine* FortEngine;
	UGameplayStatics* GPS;
	UKismetStringLibrary* KismetStringLibrary;
	UWorld* World;

	bool bIsUsingIris = true;
}

std::vector<std::string> split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);

	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}

	return tokens;
}

template<typename T = UObject>
static T* Cast(UObject* Object)
{
	if (Object && Object->IsA(T::StaticClass()))
	{
		return (T*)Object;
	}

	return nullptr;
}

void TimerFunction(int interval, std::function<void()> callback)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(interval));
	callback();
}

static void HookFunctionExec(UFunction* Function, void* FuncHook, void** FuncOG)
{
	if (!Function)
		return;

	auto& ExecFunction = Function->ExecFunction;

	if (FuncOG)
		*FuncOG = ExecFunction;

	ExecFunction = (UFunction::FNativeFuncPtr)FuncHook;
}

static void HookVTable(UObject* Class, int Index, void* FuncHook, void** FuncOG, const std::string& FuncName)
{
	if (!Class)
	{
		return;
	}

	uintptr_t Address = uintptr_t(Class->VTable[Index]);

	if (FuncOG)
		*FuncOG = Class->VTable[Index];

	DWORD dwProtection;
	VirtualProtect(&Class->VTable[Index], 8, PAGE_EXECUTE_READWRITE, &dwProtection);

	Class->VTable[Index] = FuncHook;

	DWORD dwTemp;
	VirtualProtect(&Class->VTable[Index], 8, dwProtection, &dwTemp);
}

static int32_t FindIndexVTable(const void* ObjectInstance, uintptr_t Address, int32_t MinIndex = 0x0, int32_t MaxIndex = 0x300)
{
	if (!ObjectInstance) return -1;

	void** VTable = *reinterpret_cast<void***>(const_cast<void*>(ObjectInstance));

	int32_t Index = MinIndex;

	for (int32 i = 0; i < MaxIndex; i++)
	{
		if (uintptr_t(VTable[Index]) == Address)
			return Index;

		Index++;
	}

	return -1;
}

TArray<UFortItemDefinition*> GetAllItems()
{
	TArray<UFortItemDefinition*> AllItems;

	for (int32 i = 0; i < UObject::GObjects->Num(); i++)
	{
		UObject* GObject = UObject::GObjects->GetByIndex(i);

		UFortItemDefinition* ItemDefinition = Cast<UFortItemDefinition>(GObject);
		if (!ItemDefinition) continue;

		AllItems.Add(ItemDefinition);
	}

	return AllItems;
}

UFortEngine* GetFortEngine()
{
	return Cast<UFortEngine>(UEngine::GetEngine());
}

AFortGameModeAthena* GetGameMode()
{
	UWorld* World = UWorld::GetWorld();

	if (World)
	{
		AFortGameModeAthena* GameModeAthena = Cast<AFortGameModeAthena>(World->AuthorityGameMode);

		if (GameModeAthena)
			return GameModeAthena;
	}

	return nullptr;
}

AFortGameStateAthena* GetGameState()
{
	UWorld* World = UWorld::GetWorld();

	if (World)
	{
		AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(World->GameState);

		if (GameState)
			return GameState;
	}

	return nullptr;
}

UFortPlaylistAthena* GetPlaylist()
{
	AFortGameStateAthena* GameState = GetGameState();

	if (GameState)
	{
		UFortPlaylistAthena* Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;

		if (Playlist)
			return Playlist;
	}

	return nullptr;
}

UGameDataBR* GetGameDataBR()
{
	UFortAssetManager* AssetManager = Cast<UFortAssetManager>(GetFortEngine()->AssetManager);

	if (AssetManager)
	{
		UGameDataBR* GameDataBR = AssetManager->GameDataBR;

		if (GameDataBR)
			return GameDataBR;
	}

	return nullptr;
}

UFortGameData* GetGameDataCommon()
{
	UFortAssetManager* AssetManager = Cast<UFortAssetManager>(GetFortEngine()->AssetManager);

	if (AssetManager)
	{
		UFortGameData* GameDataCommon = AssetManager->GameDataCommon;

		if (GameDataCommon)
			return GameDataCommon;
	}

	return nullptr;
}

template <typename T>
static T* GetDataTableRowFromName(UDataTable* DataTable, FName RowName)
{
	if (!DataTable)
		return nullptr;

	auto& RowMap = *(UC::TMap<FName, uint8*>*)(__int64(DataTable) + (0x28 + sizeof(UScriptStruct*)));

	for (int i = 0; i < RowMap.Elements.Elements.Num(); ++i)
	{
		auto& Pair = RowMap.Elements.Elements.Data[i].ElementData.Value;

		if (Pair.Key() != RowName)
			continue;

		return (T*)Pair.Value();
	}

	return nullptr;
}