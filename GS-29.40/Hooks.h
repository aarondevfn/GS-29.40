#pragma once
#include <thread>
#include <chrono>
#include <atomic>



struct FObjectKey
{
public:
	int32		ObjectIndex;
	int32		ObjectSerialNumber;
};

struct FSubObjectRegistry
{

};



struct FReplicatedComponentInfo
{
	/** Component that will be replicated */
	UActorComponent* Component = nullptr;

	/** Store the object key info for fast access later */
	FObjectKey Key;

	/** NetCondition of the component */
	ELifetimeCondition NetCondition;

	/** Collection of subobjects replicated with this component */
	FSubObjectRegistry SubObjects;


};


void (*GetPlayerViewPoint)(APlayerController* PlayerController, FVector& out_Location, FRotator& out_Rotation);

struct FObjectHandle {

};

struct FObjectPtr
{
public:
	union
	{
		mutable FObjectHandle Handle;
		UObject* DebugPtr;
	};
};



template <typename T>
struct TObjectPtr
{
public:
	using ElementType = T;

	union
	{
		FObjectPtr ObjectPtr;
		T* DebugPtr;
	};
};

void* GetInterfaceAddress(UObject* Object, UClass* InterfaceClass)
{
	static auto tamere = Util::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 33 DB 48 8B FA 48 8B F1 48 85 D2 74 7C F7 82 ? ? ? ? ? ? ? ? 74 70");
	static void* (*GetInterfaceAddressOriginal)(UObject * a1, UClass * a2) = decltype(GetInterfaceAddressOriginal)(tamere);
	return GetInterfaceAddressOriginal(Object, InterfaceClass);
}

void (*OnDamageServerOG)(ABuildingActor* BuildingActor, float Damage, const FGameplayTagContainer& DamageTags, const FVector& Momentum, const FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser, const FGameplayEffectContextHandle& EffectContext);
void OnDamageServer(ABuildingActor* BuildingActor, float Damage, const FGameplayTagContainer& DamageTags, const FVector& Momentum, const FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser, const FGameplayEffectContextHandle& EffectContext) {
	OnDamageServerOG(
		BuildingActor,
		Damage,
		DamageTags,
		Momentum,
		HitInfo,
		InstigatedBy,
		DamageCauser,
		EffectContext);

	ABuildingSMActor* BuildingSMActor = Cast<ABuildingSMActor>(BuildingActor);
	AFortPlayerController* PlayerController = Cast<AFortPlayerController>(InstigatedBy);
	AFortWeapon* Weapon = Cast<AFortWeapon>(DamageCauser);

	if (!BuildingSMActor || !PlayerController || !Weapon)
		return;

	AFortPlayerPawn* PlayerPawn = PlayerController->MyFortPawn;

	if (!Weapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) ||
		/*!BuildingSMActor->bAllowResourceDrop ||*/
		!PlayerPawn)
		return;

	UFortResourceItemDefinition* ResourceItemDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingSMActor->ResourceType);

	int32 ResourceDropAmount = 0;
	float ResourceRatio = 0.0f;

	if (ResourceItemDefinition)
	{
		if (BuildingSMActor->MaxResourcesToSpawn < 0)
			BuildingSMActor->MaxResourcesToSpawn = BuildingSMActor->DetermineMaxResourcesToSpawn();

		float MaxResourcesToSpawn = BuildingSMActor->MaxResourcesToSpawn;
		float MaxHealth = BuildingSMActor->GetMaxHealth();

		ResourceRatio = MaxResourcesToSpawn / MaxHealth;
		ResourceDropAmount = (int32)(ResourceRatio * Damage);
	}

	bool bHasHealthLeft = BuildingSMActor->HasHealthLeft();
	bool bDestroyed = false;

	if (!bHasHealthLeft)
	{
		bDestroyed = true;

		int32 MinResourceCount = 0;

		if (!ResourceItemDefinition || (MinResourceCount = 1, ResourceRatio == 0.0))
			MinResourceCount = 0;

		if (ResourceDropAmount < MinResourceCount)
			ResourceDropAmount = MinResourceCount;
	}

	if (ResourceDropAmount > 0)
	{
		FFortItemEntry ItemEntry;
		Inventory::MakeItemEntry(&ItemEntry, ResourceItemDefinition, ResourceDropAmount, 0, 0, 0.f);
		Inventory::SetStateValue(&ItemEntry, EFortItemEntryState::DoNotShowSpawnParticles, 1);
		Inventory::SetStateValue(&ItemEntry, EFortItemEntryState::ShouldShowItemToast, 1);
		Inventory::AddInventoryItem(PlayerController, ItemEntry);

		PlayerController->ClientReportDamagedResourceBuilding(BuildingSMActor, BuildingSMActor->ResourceType, ResourceDropAmount, bDestroyed, (Damage == 100.0f));

		//ItemEntry.FreeItemEntry();
	}
}

void ServerRepairBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToRepair)
{
	if (!BuildingActorToRepair ||
		!BuildingActorToRepair->GetLevel()->OwningWorld)
		return;

	int32 RepairCosts = PlayerController->PayRepairCosts(BuildingActorToRepair);
	BuildingActorToRepair->RepairBuilding(PlayerController, RepairCosts);
}

void ServerAttemptInventoryDrop(AFortPlayerController* PlayerController, const FGuid& ItemGuid, int32 Count, bool bTrash) {
	if (Count < 1)
		return;

	AFortPlayerPawn* PlayerPawn = PlayerController->MyFortPawn;

	if (!PlayerPawn || PlayerPawn->bIsSkydiving || PlayerPawn->bIsDBNO)
		return;

	AFortPlayerControllerAthena* PlayerControllerAthena = Cast<AFortPlayerControllerAthena>(PlayerController);

	if (PlayerControllerAthena)
	{
		if (PlayerControllerAthena->IsInAircraft())
			return;
	}

	UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerController->BP_GetInventoryItemWithGuid(ItemGuid));

	if (!WorldItem) {
		return;
	}

	UFortWorldItemDefinition* WorldItemDefinition = Cast<UFortWorldItemDefinition>(WorldItem->ItemEntry.ItemDefinition);

	if (!WorldItemDefinition || !WorldItem->CanBeDropped())
		return;

	if (WorldItem->ItemEntry.Count <= 0)
	{
		UFortWeaponRangedItemDefinition* WeaponRangedItemDefinition = Cast<UFortWeaponRangedItemDefinition>(WorldItemDefinition);

		if (WeaponRangedItemDefinition && WeaponRangedItemDefinition->bPersistInInventoryWhenFinalStackEmpty)
			return;


		PlayerController->RemoveInventoryItem(ItemGuid, Count, true, true);
		return;
	}

	if (PlayerController->RemoveInventoryItem(ItemGuid, Count, false, true)) {
		const float LootSpawnLocationX = 0.0f;
		const float LootSpawnLocationY = 0.0f;
		const float LootSpawnLocationZ = 60.0f;

		const FVector& SpawnLocation = PlayerPawn->K2_GetActorLocation() +
			PlayerPawn->GetActorForwardVector() * LootSpawnLocationX +
			PlayerPawn->GetActorRightVector() * LootSpawnLocationY +
			PlayerPawn->GetActorUpVector() * LootSpawnLocationZ;

		FFortItemEntry NewItemEntry;

		if (WorldItem->ItemEntry.Count == Count)
		{
			NewItemEntry.CopyItemEntryWithReset(&WorldItem->ItemEntry);
		}
		else if (Count < WorldItem->ItemEntry.Count)
		{
			Inventory::MakeItemEntry(
				&NewItemEntry,
				(UFortItemDefinition*)WorldItem->ItemEntry.ItemDefinition,
				Count,
				WorldItem->ItemEntry.Level,
				WorldItem->ItemEntry.LoadedAmmo,
				WorldItem->ItemEntry.Durability
			);
		}

		Inventory::SetStateValue(&NewItemEntry, EFortItemEntryState::DurabilityInitialized, 1);
		Inventory::SetStateValue(&NewItemEntry, EFortItemEntryState::FromDroppedPickup, 1);


		FRotator PlayerRotation = PlayerPawn->K2_GetActorRotation();
		PlayerRotation.Yaw += UKismetMathLibrary::RandomFloatInRange(-60.0f, 60.0f);

		float RandomDistance = UKismetMathLibrary::RandomFloatInRange(500.0f, 600.0f);
		FVector FinalDirection = UKismetMathLibrary::GetForwardVector(PlayerRotation);

		FVector FinalLocation = SpawnLocation + FinalDirection * RandomDistance;

		Inventory::SpawnPickup(
			PlayerController,
			PlayerPawn,
			&NewItemEntry,
			SpawnLocation,
			FinalLocation,
			0,
			true,
			true,
			EFortPickupSourceTypeFlag::Player,
			EFortPickupSpawnSource::TossedByPlayer
		);


	}
	else {
		HLog("sdfsdfsdfsdf");
	}

}



FGameplayAbilitySpecHandle GiveAbilityAndActivateOnceA(UAbilitySystemComponent* component, FGameplayAbilitySpec Spec, FGameplayAbilitySpecHandle Handle) {

	Spec.bActivateOnce = true;

	FGameplayAbilitySpecHandle* AddedAbilityHandle = GiveAbility0(component, &Spec.Handle, &Spec);
	FGameplayAbilitySpec* FoundSpec = FindAbilitySpecFromHandle(component, *AddedAbilityHandle);

	if (FoundSpec)
	{
		FoundSpec->RemoveAfterActivation = true;

		if (!InternalTryActivateAbility(component, Handle, FPredictionKey(), nullptr, nullptr, nullptr)) {

			void (*ClearAbility)(UAbilitySystemComponent * component, const FGameplayAbilitySpecHandle & Handle);
			ClearAbility = decltype(ClearAbility)(Util::FindPattern("48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 56 48 83 EC 20 48 8B 01 48 8B F2 48 8B D9"));
			if (ClearAbility) {
				ClearAbility(component, Handle);
			}
			else {
				HLog("ClearAbility invalid !");
			}

			return FGameplayAbilitySpecHandle();
		}
	}

	return *AddedAbilityHandle;
}




void GiveAbility(UFortAbilitySystemComponent* AbilitySystemComponent, UClass* GameplayAbilityClass) {
	auto Test = (UGameplayAbility*)GameplayAbilityClass->DefaultObject;
	//Test->bHasBlueprintActivate = true;

	//HLog("Test : " << Test->GetName());

	auto GenerateNewSpec = [&]() -> FGameplayAbilitySpec
		{
			FGameplayAbilitySpecHandle Handle{ rand() };

			FGameplayAbilitySpec Spec{ -1, -1, -1, Handle,Test, 1, -1, 0, 0, false, false, false };


			return Spec;
		};

	auto Spec = GenerateNewSpec();

	for (int i = 0; i < AbilitySystemComponent->ActivatableAbilities.Items.Num(); i++)
	{
		auto& CurrentSpec = AbilitySystemComponent->ActivatableAbilities.Items[i];

		if (CurrentSpec.Ability == Spec.Ability)
			return;
	}

	AbilitySystemComponent->K2_GiveAbility(GameplayAbilityClass, 0, -1);

	//GiveAbility0(AbilitySystemComponent, &Spec.Handle, &Spec);
}





void GiveFortAbilitySet(UFortAbilitySystemComponent* AbilitySystemComponent, UFortAbilitySet* FortAbilitySet)
{
	if (!AbilitySystemComponent || !FortAbilitySet)
		return;

	for (int32 i = 0; i < FortAbilitySet->GameplayAbilities.Num(); i++)
	{
		TSubclassOf<UFortGameplayAbility> GameplayAbility = FortAbilitySet->GameplayAbilities[i];
		GiveAbility(AbilitySystemComponent, GameplayAbility);
	}

	for (int32 i = 0; i < FortAbilitySet->GrantedGameplayEffects.Num(); i++)
	{
		FGameplayEffectApplicationInfoHard GrantedGameplayEffect = FortAbilitySet->GrantedGameplayEffects[i];

		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(GrantedGameplayEffect.GameplayEffect, GrantedGameplayEffect.Level, EffectContext);
	}
}


bool (*LocalSpawnPlayActorOG)();
bool LocalSpawnPlayActorHook()
{
	HLog("LocalSpawnPlayActorHook!");
	return true;
}

void (*InitializeUIOG)();
void InitializeUIHook() {}

#include <intrin.h>

bool (*InitBase)(UNetDriver* NetDriver, bool bInitAsClient, void* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error);
bool azerty = true;








bool activateReplication = false;








class FNetHandle
{
public:

	struct FInternalValue
	{
		uint32 Id;
		uint32 Epoch;
	};


	union
	{
		FObjectKey Value;
		FInternalValue InternalValue;
	};
};


int GetSizeOfClass(UObject* Class) { return Class ? *(int*)(__int64(Class) + (0x50 + 8)) : 0; };

void SpawnVehicles()
{
	UWorld* World = UWorld::GetWorld();

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(World, AFortAthenaVehicleSpawner::StaticClass(), &Actors);

	HLog("SpawnVehicles Num: " << Actors.Num());

	for (int32 i = 0; i < Actors.Num(); i++)
	{
		AFortAthenaVehicleSpawner* AthenaVehicleSpawner = Cast<AFortAthenaVehicleSpawner>(Actors[i]);
		if (!AthenaVehicleSpawner) continue;

		UClass* VehicleClass = AthenaVehicleSpawner->GetVehicleClass();
		if (!VehicleClass) continue;

		FVector SpawnLocation = AthenaVehicleSpawner->K2_GetActorLocation();
		FRotator SpawnRotation = AthenaVehicleSpawner->K2_GetActorRotation();

		AFortAthenaVehicle* AthenaVehicle = Cast<AFortAthenaVehicle>(SpawnActor(World, VehicleClass, SpawnLocation, SpawnRotation));
		if (!AthenaVehicle) continue;

		AthenaVehicleSpawner->OnConstructVehicle(AthenaVehicle);
	}
}

namespace Hooks
{
	int GetNetModeDetour()
	{
		return 1; // 1
	}

	int ActorGetNetModeDetour()
	{
		return 1; // 1
	}

	void (*TickFlushOG)(UNetDriver* NetDriver, float DeltaSeconds);
	void TickFlushHook(UNetDriver* NetDriver, float DeltaSeconds)
	{
		TArray<UNetConnection*> ClientConnections = NetDriver->ClientConnections;

		if (ClientConnections.Num() > 0 && !ClientConnections[0]->InternalAck)
		{
			if (Globals::bIsUsingIris)
			{
				if (NetDriver->ReplicationSystem)
				{
					UpdateReplicationViews(NetDriver);
					SendClientMoveAdjustments(NetDriver);
					PreSendUpdate(NetDriver->ReplicationSystem, FSendUpdateParams{ .SendPass = EReplicationSystemSendPass::TickFlush, .DeltaSeconds = DeltaSeconds });
				}
			}
			else
				ServerReplicateActors(NetDriver, DeltaSeconds);
		}

		static UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = nullptr;

		if (GamePhaseLogic == nullptr)
			GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(NetDriver);

		if (GamePhaseLogic)
			BattleRoyaleGamePhaseLogic::Tick(GamePhaseLogic, DeltaSeconds);

		TickFlushOG(NetDriver, DeltaSeconds);
	}

	bool (*KickPlayerOG)();
	bool KickPlayerHook()
	{
		HLog("KickPlayerHook");
		return 0;
	}

	void ListenTahLesFous()
	{
		auto BaseAdress = (uintptr_t)GetModuleHandle(0);
		*(bool*)(BaseAdress + 0x126357DB) = false;
		*(bool*)(BaseAdress + 0x12615755) = true;

		auto GameViewport = Globals::FortEngine->GameViewport;
		Globals::World = GameViewport->World;

		auto BeaconHost = (AOnlineBeaconHost*)SpawnActor(Globals::World, AOnlineBeaconHost::StaticClass(), FVector(), FRotator());
		BeaconHost->ListenPort = 7777;

		if (InitHost(BeaconHost)) {
			auto GameNetDriver = Globals::KismetStringLibrary->Conv_StringToName(L"GameNetDriver");

			BeaconHost->NetDriver->bIsUsingIris = Globals::bIsUsingIris;

			Globals::World->NetDriver = BeaconHost->NetDriver;
			Globals::World->NetDriver->NetDriverName = GameNetDriver;

			FString Error;
			auto InURL = FURL();
			InURL.Port = 7777;

			if (InitListen(Globals::World->NetDriver, Globals::World, InURL, true, Error)) {
				SetWorld = decltype(SetWorld)(Globals::World->NetDriver->VTable[0x7E]);

				SetWorld(Globals::World->NetDriver, Globals::World);

				Globals::World->LevelCollections[0].NetDriver = Globals::World->NetDriver;
				Globals::World->LevelCollections[1].NetDriver = Globals::World->NetDriver;
				HLog("InitListen");

				static auto UpdateReplicationViewspattern = Util::FindPattern("48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 0F 10 0D ? ? ? ? 48 8D 05 ? ? ? ? 45 33 E4");
				static auto PreSendUpdatepattern = Util::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 40 44 8B 05 ? ? ? ? 48 8B DA 48 8B F9 48 8D 15 ? ? ? ? 48 8D 4C 24 ? E8 ? ? ? ? 8B 03 4C 8B 47 28 41 89 80 ? ? ? ? 48 8B 77 28 8B 86 ? ? ? ? 83 F8 02");

				UpdateReplicationViews = decltype(UpdateReplicationViews)(UpdateReplicationViewspattern);
				PreSendUpdate = decltype(PreSendUpdate)(PreSendUpdatepattern);

				START_DETOUR;
				DetourAttach(TickFlushOG, TickFlushHook);
				DetourAttach(KickPlayerOG, KickPlayerHook);
				END_DETOUR;
			}
		}
	}

	void ProcessEvent(UObject* object, UFunction* func, void* Parameters)
	{

		static bool bIsStarted = false;

		if (!bIsStarted)
		{
			if (func->GetName().find("Tick") != std::string::npos)
			{
				if (GetAsyncKeyState(VK_F1) & 0x1)
				{
					auto PlayerController = Globals::GPS->GetPlayerController(Globals::World, 0);
					if (PlayerController) {
						PlayerController->SwitchLevel(L"Helios_terrain");
					}

					START_DETOUR;
					DetourAttach(InternalGetNetMode, GetNetModeDetour);
					END_DETOUR;

					bIsStarted = true;
				}

			}
		}

		return ProcessEventO(object, func, Parameters);
	}



	bool Local_SpawnPlayActorDetour() {
		HLog("Local_SpawnPlayActorDetour ! ");
		return true;
	}
	void RequestExitWithStatusDetour()
	{
		printf_s("Ta capter ca a crash, ImageBase: %p\n", uintptr_t(GetModuleHandle(0)));

		HLog("APAWN : " << (uintptr_t)_ReturnAddress() - InSDKUtils::GetImageBase());
		return;
	}

	bool hgrthrthrthhgrthrthrth() {
		HLog("hgrthrthrthhgrthrthrth ! ");
		return false;
	}

	void (*ABCBCBCB)();

	void ABCBCBCBDetour() {
		HLog("ABCBCBCBDetour")
	}


	UClass** (*FortGameSessionTuConnais)(__int64 a1, UClass** OutGameSessionClass);

	UClass** FortGameSessionTuConnaisDetour(__int64 a1, UClass** OutGameSessionClass) {
		HLog("FortGameSessionTuConnaisDetour")
			* OutGameSessionClass = AFortGameSessionDedicated::StaticClass();
		return OutGameSessionClass;
	}


	void (*sub_7FF73E427CD0)(__int64 a1, __int64* a2); // crash after 5 minutes
	void sub_7FF73E427CD0Detour(__int64 a1, __int64* a2) {
		return;
	}

	void (*ModifyLoadedAmmo)(void* a1, const FGuid& ItemGuid, int32 CorrectAmmo);
	void ModifyLoadedAmmoHook(void* a1, const FGuid& ItemGuid, int32 CorrectAmmo)
	{
		AFortPlayerController* PlayerController = (AFortPlayerController*)(__int64(a1) - 0x8C0);

		auto WorldInventory = (AFortInventory*)PlayerController->WorldInventory.ObjectPointer;
		auto ItemInstances = WorldInventory->Inventory.ItemInstances;
		auto ReplicatedEntries = WorldInventory->Inventory.ReplicatedEntries;

		for (int i = 0; i < ItemInstances.Num(); i++) {
			auto ItemInstance = ItemInstances[i];
			if (ItemInstance->GetItemGuid() == ItemGuid) {
				ItemInstance->ItemEntry.LoadedAmmo = CorrectAmmo;
				WorldInventory->Inventory.MarkItemDirty(&ItemInstance->ItemEntry);
				break;
			}
		}
		for (int i = 0; i < ReplicatedEntries.Num(); i++) {
			auto ReplicatedEntrie = &ReplicatedEntries[i];
			if (ReplicatedEntrie->ItemGuid == ItemGuid) {
				ReplicatedEntrie->LoadedAmmo = CorrectAmmo;
				WorldInventory->Inventory.MarkItemDirty(ReplicatedEntrie);
				break;
			}
		}
	}

	void GetPlayerViewPointDetour(AFortPlayerController* PlayerController, FVector& out_Location, FRotator& out_Rotation)
	{
		APawn* Pawn = PlayerController->Pawn;
		ASpectatorPawn* SpectatorPawn = PlayerController->SpectatorPawn;

		if (Pawn)
		{
			out_Location = Pawn->K2_GetActorLocation();
			out_Rotation = PlayerController->GetControlRotation();
			return;
		}
		else if (SpectatorPawn)
		{
			out_Location = SpectatorPawn->K2_GetActorLocation();
			out_Rotation = ((APlayerController*)SpectatorPawn->Owner)->GetControlRotation();
			return;
		}
		else if (!SpectatorPawn && !Pawn)
		{
			out_Location = PlayerController->LastSpectatorSyncLocation;
			out_Rotation = PlayerController->LastSpectatorSyncRotation;
			return;
		}

		return GetPlayerViewPoint(PlayerController, out_Location, out_Rotation);
	}


	bool (*CanActivateAbilityOG)();
	bool CanActivateAbility()
	{
		return true;
	}

	bool (*ReadyToStartMatchOG)(AFortGameModeAthena* GameModeAthena, FFrame* Stack, bool* Ret);
	bool ReadyToStartMatchHook(AFortGameModeAthena* GameModeAthena, FFrame* Stack, bool* Ret)
	{
		if (!GameModeAthena->IsA(AFortGameModeAthena::StaticClass()))
			return ReadyToStartMatchOG(GameModeAthena, Stack, Ret);

		Stack->Code += Stack->Code != nullptr;

		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(GameModeAthena->GameState);
		UWorld* World = UWorld::GetWorld();

		if (!GameStateAthena || !World)
		{
			*Ret = false;
			return *Ret;
		}

		static bool bOGMAN = false;

		if (!bOGMAN)
		{
			auto GameViewport = Globals::FortEngine->GameViewport;
			Globals::World = GameViewport->World;

			bOGMAN = true;
		}

		const FName WaitingToStartName = UKismetStringLibrary::Conv_StringToName(L"WaitingToStart");

		if (GameStateAthena->MatchState == WaitingToStartName)
		{
			if (/*World->NetDriver && */!GameStateAthena->CurrentPlaylistInfo.BasePlaylist)
			{
				//UFortPlaylistAthena* PlaylistAthena = UObject::FindObjectSlow<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");
				//UFortPlaylistAthena* PlaylistAthena = UObject::FindObjectSlow<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultDuo.Playlist_DefaultDuo");
				UFortPlaylistAthena* PlaylistAthena = UObject::FindObjectSlow<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSquad.Playlist_DefaultSquad");
				//UFortPlaylistAthena* PlaylistAthena = UObject::FindObjectSlow<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultTrios.Playlist_DefaultTrios");
				//UFortPlaylistAthena* PlaylistAthena = UObject::FindObjectSlow<UFortPlaylistAthena>("FortPlaylistAthena Playlist_Respawn_24.Playlist_Respawn_24");

				//UFortPlaylistAthena* PlaylistAthena = FindObjectFast<UFortPlaylistAthena>("/Game/Athena/Playlists/Trios/Playlist_Trios.Playlist_Trios");

				if (!PlaylistAthena)
				{
					HLog("Playlist Not found");
					*Ret = false;
					return *Ret;
				}

				GameStateAthena->CurrentPlaylistInfo.BasePlaylist = PlaylistAthena;
				GameStateAthena->CurrentPlaylistInfo.OverridePlaylist = PlaylistAthena;
				GameStateAthena->CurrentPlaylistInfo.PlaylistReplicationKey++;
				GameStateAthena->CurrentPlaylistInfo.MarkArrayDirty();
				GameStateAthena->OnRep_CurrentPlaylistInfo();

				GameModeAthena->CurrentPlaylistId = PlaylistAthena->PlaylistId;
				GameModeAthena->CurrentPlaylistName = PlaylistAthena->PlaylistName;

				GameModeAthena->WarmupRequiredPlayerCount = 2;
				GameModeAthena->bDisableAI = false;

				GameStateAthena->CurrentPlaylistId = PlaylistAthena->PlaylistId;
				GameStateAthena->OnRep_CurrentPlaylistId();

				GameStateAthena->bIsLargeTeamGame = PlaylistAthena->bIsLargeTeamGame;

				UFortZoneTheme* ZoneTheme = GameStateAthena->ZoneTheme;

				if (ZoneTheme)
				{
					ZoneTheme->PlaylistId = PlaylistAthena->PlaylistId;
					ZoneTheme->TeamSize = PlaylistAthena->MaxTeamSize;
					ZoneTheme->TeamCount = PlaylistAthena->MaxTeamCount;
					ZoneTheme->MaxPartySize = PlaylistAthena->MaxSocialPartySize;
					ZoneTheme->MaxPlayers = PlaylistAthena->MaxPlayers;
				}

				AFortGameSession* FortGameSession = GameModeAthena->FortGameSession;

				if (FortGameSession)
				{
					FortGameSession->MaxPlayers = PlaylistAthena->MaxPlayers;
					FortGameSession->MaxPartySize = PlaylistAthena->MaxSocialPartySize;

					FortGameSession->SessionName = UKismetStringLibrary::Conv_StringToName(L"OGDeMaladeSession");
				}

				UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameModeAthena);

				if (GamePhaseLogic)
				{
					GamePhaseLogic->AirCraftBehavior = PlaylistAthena->AirCraftBehavior;

					void (*SetGamePhase)(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic, EAthenaGamePhase GamePhase) = decltype(SetGamePhase)(0xa584478 + uintptr_t(GetModuleHandle(0)));
					SetGamePhase(GamePhaseLogic, EAthenaGamePhase::Setup);

					GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::Setup;
				}

				GameModeAthena->bWorldIsReady = true;
			}

			if (!World->NetDriver && false)
			{

			}
		}

		bool bIsReadyToStartMatch = (GameModeAthena->bWorldIsReady &&
			GameStateAthena->bPlaylistDataIsLoaded &&
			GameModeAthena->MatchState == WaitingToStartName &&
			(GameModeAthena->NumPlayers + GameModeAthena->NumBots) >= GameModeAthena->WarmupRequiredPlayerCount);

		if (bIsReadyToStartMatch)
		{
			UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameModeAthena);
			AFortAthenaMapInfo* AthenaMapInfo = GameStateAthena->MapInfo;

			if (GamePhaseLogic && AthenaMapInfo)
			{
				void (*InitializeFlightPath)(AFortAthenaMapInfo * AthenaMapInfo, AFortGameStateAthena * GameStateAthena, UFortGameStateComponent_BattleRoyaleGamePhaseLogic * GamePhaseLogic, int a4, int a5, int a6, int a7) = decltype(InitializeFlightPath)(0x8e1b778 + uintptr_t(GetModuleHandle(0)));
				InitializeFlightPath(AthenaMapInfo, GameStateAthena, GamePhaseLogic, 0, 0, 0, 0);

				SpawnVehicles();

				BattleRoyaleGamePhaseLogic::StartWarmupPhase(GamePhaseLogic);
			}

			const FName InProgressName = UKismetStringLibrary::Conv_StringToName(L"InProgress");

			GameModeAthena->MatchState = InProgressName;
			GameModeAthena->K2_OnSetMatchState(InProgressName);

			GameModeAthena->StartPlay();
			GameModeAthena->StartMatch();

			HLog("ReadyToStartMatch - Start Match Successful!");
		}

		*Ret = bIsReadyToStartMatch;
		return *Ret;
	}

	APawn* SpawnDefaultPawnForHook(AFortGameModeAthena* GameModeAthena, FFrame* Stack, APawn** Ret)
	{
		AFortPlayerControllerAthena* NewPlayer;
		AActor* StartSpot;

		Stack->StepCompiledIn(&NewPlayer);
		Stack->StepCompiledIn(&StartSpot);

		Stack->Code += Stack->Code != nullptr;

		if (!NewPlayer || !StartSpot)
		{
			*Ret = nullptr;
			return *Ret;
		}


		HLog("SpawnDefaultPawnForHook called!");

		UClass* PawnClass = GameModeAthena->GetDefaultPawnClassForController(NewPlayer);
		AFortPlayerStateAthena* PlayerStateAthena = Cast<AFortPlayerStateAthena>(NewPlayer->PlayerState);
		UWorld* World = UWorld::GetWorld();

		if (!PawnClass || !PlayerStateAthena || !World)
		{
			*Ret = nullptr;
			return *Ret;
		}

		ULocalPlayer* FirstLocalPlayer = World->OwningGameInstance->LocalPlayers[0];

		if (FirstLocalPlayer)
		{
			if (FirstLocalPlayer->PlayerController == NewPlayer)
			{
				static bool bIsOG = false;

				if (!bIsOG)
				{
					ListenTahLesFous();
					bIsOG = true;
				}

				*Ret = nullptr;
				return *Ret;
			}
		}

		UCheatManager* CheatManager = Cast<UCheatManager>(NewPlayer->CheatManager);

		if (!CheatManager)
		{
			NewPlayer->CheatManager = Cast<UCheatManager>(UGameplayStatics::SpawnObject(UCheatManager::StaticClass(), NewPlayer));
			NewPlayer->CheatManager->ReceiveInitCheatManager();
		}

		AFortPlayerPawn* (*SpawnDefaultPawnFor)(AFortGameMode* GameMode, AController* NewPlayer, AActor* StartSpot) = decltype(SpawnDefaultPawnFor)(0x97e9d00 + uintptr_t(GetModuleHandle(0)));
		AFortPlayerPawnAthena* PlayerPawnAthena = /*Cast<AFortPlayerPawnAthena>(SpawnActor(UWorld::GetWorld(), PawnClass, StartSpot->K2_GetActorLocation()))*/Cast<AFortPlayerPawnAthena>(SpawnDefaultPawnFor(GameModeAthena, NewPlayer, StartSpot));

		if (PlayerPawnAthena)
		{
			if (!NewPlayer->WorldInventory.GetObjectRef())
			{
				AFortInventory* WorldInventory = Cast<AFortInventory>(SpawnActor(World, NewPlayer->WorldInventoryClass));

				if (WorldInventory)
				{
					WorldInventory->InventoryType = EFortInventoryType::World;
					WorldInventory->Instigator = PlayerPawnAthena;
					WorldInventory->OnRep_Instigator();

					WorldInventory->SetOwner(NewPlayer);
					WorldInventory->OnRep_Owner();

					NewPlayer->WorldInventory.ObjectPointer = WorldInventory;
					NewPlayer->WorldInventory.InterfacePointer = GetInterfaceAddress(WorldInventory, IFortInventoryInterface::StaticClass());

					NewPlayer->ViewTargetInventory.ObjectPointer = WorldInventory;
					NewPlayer->ViewTargetInventory.InterfacePointer = GetInterfaceAddress(WorldInventory, IFortInventoryInterface::StaticClass());

					NewPlayer->bHasInitializedWorldInventory = true;
				}
			}

			ApplyCharacterCustomization(PlayerStateAthena, PlayerPawnAthena);

			UFortAbilitySystemComponent* AbilitySystemComponent = PlayerStateAthena->AbilitySystemComponent;

			if (AbilitySystemComponent)
			{
				UFortAbilitySet* GAS_AthenaPlayer = LoadObject<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer", UFortAbilitySet::StaticClass());

				HLog("GAS_AthenaPlayer: " << GAS_AthenaPlayer->GetName());

				GiveFortAbilitySet(AbilitySystemComponent, GAS_AthenaPlayer);

				UFortAbilitySet* AS_TacticalSprint = LoadObject<UFortAbilitySet>("/TacticalSprintGame/Gameplay/AS_TacticalSprint.AS_TacticalSprint", UFortAbilitySet::StaticClass());

				HLog("AS_TacticalSprint: " << AS_TacticalSprint->GetName());

				GiveFortAbilitySet(AbilitySystemComponent, AS_TacticalSprint);

				/*int32 Idx = 0;

				for (int32 i = 0; i < UObject::GObjects->Num(); i++)
				{
					UFortAbilitySet* AbilitySet = Cast<UFortAbilitySet>(UObject::GObjects->GetByIndex(i));
					if (!AbilitySet) continue;

					GiveFortAbilitySet(AbilitySystemComponent, AbilitySet);

					HLog(Idx << " AbilitySet: " << UKismetSystemLibrary::GetPathName(AbilitySet).ToString());
					Idx++;
				}
				
				Idx = 0;

				for (int32 i = 0; i < UObject::GObjects->Num(); i++)
				{
					UGameplayAbility* GameplayAbility = Cast<UGameplayAbility>(UObject::GObjects->GetByIndex(i));
					if (!GameplayAbility) continue;

					GiveAbility(AbilitySystemComponent, GameplayAbility->Class);

					HLog(Idx << " GameplayAbility: " << UKismetSystemLibrary::GetPathName(GameplayAbility).ToString());
					Idx++;
				}*/

				UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameModeAthena);

				if (GamePhaseLogic && false)
				{
					AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(GamePhaseLogic->GE_OutsideSafeZone, 0, FGameplayEffectContextHandle());
				}
			}

			UAthenaPickaxeItemDefinition* PickaxeItemDefinition = NewPlayer->CosmeticLoadoutPC.Pickaxe;

			if (PickaxeItemDefinition)
			{
				Inventory::SetupInventory(NewPlayer, PickaxeItemDefinition->WeaponDefinition);
				Inventory::InitBaseInventory(NewPlayer);
			}

			NewPlayer->bInfiniteAmmo = true;
			NewPlayer->bBuildFree = true;

			NewPlayer->bEnableBroadcastRemoteClientInfo = true;

			NewPlayer->BroadcastRemoteClientInfo->bActive = true;
			NewPlayer->BroadcastRemoteClientInfo->OnRep_bActive();

			HLog("BroadcastRemoteClientInfo: " << NewPlayer->BroadcastRemoteClientInfo->GetName());
		}

		PlayerStateAthena->SquadID = PlayerStateAthena->TeamIndex;
		PlayerStateAthena->OnRep_SquadId();

		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(GameModeAthena->GameState);

		if (GameStateAthena)
		{
			bool bFoundMemberInfo = false;
			for (int32 i = 0; i < GameStateAthena->GameMemberInfoArray.Members.Num(); i++)
			{
				FGameMemberInfo* MemberInfo = &GameStateAthena->GameMemberInfoArray.Members[i];
				if (!MemberInfo) continue;

				bool bEqualEqual_UniqueNetId = UFortKismetLibrary::EqualEqual_UniqueNetIdReplUniqueNetIdRepl(MemberInfo->MemberUniqueId, PlayerStateAthena->UniqueID);
				if (!bEqualEqual_UniqueNetId) continue;

				MemberInfo->SquadID = PlayerStateAthena->SquadID;
				MemberInfo->TeamIndex = PlayerStateAthena->TeamIndex;

				GameStateAthena->GameMemberInfoArray.MarkItemDirty(MemberInfo);

				bFoundMemberInfo = true;
			}

			if (!bFoundMemberInfo)
			{
				FGameMemberInfo MemberInfo;
				MemberInfo.ReplicationID = -1;
				MemberInfo.ReplicationKey = -1;
				MemberInfo.MostRecentArrayReplicationKey = -1;

				MemberInfo.SquadID = PlayerStateAthena->SquadID;
				MemberInfo.TeamIndex = PlayerStateAthena->TeamIndex;
				MemberInfo.MemberUniqueId = PlayerStateAthena->UniqueID;

				GameStateAthena->GameMemberInfoArray.Members.Add(MemberInfo);
				GameStateAthena->GameMemberInfoArray.MarkArrayDirty();
			}
		}

		PlayerStateAthena->SeasonLevelUIDisplay = NewPlayer->XPComponent->CurrentLevel;
		PlayerStateAthena->OnRep_SeasonLevelUIDisplay();

		NewPlayer->XPComponent->bRegisteredWithQuestManager = true;
		NewPlayer->XPComponent->OnRep_bRegisteredWithQuestManager();

		*Ret = PlayerPawnAthena;
		return *Ret;
	}

	void ServerAttemptAircraftJumpHook(UFortControllerComponent_Aircraft* ControllerComponent_Aircraft, FFrame* Stack, void* Ret)
	{
		FRotator ClientRotation;

		Stack->StepCompiledIn(&ClientRotation);

		Stack->Code += Stack->Code != nullptr;

		AFortPlayerControllerAthena* PlayerControllerAthena = Cast<AFortPlayerControllerAthena>(ControllerComponent_Aircraft->GetOwner());
		UWorld* World = UWorld::GetWorld();

		if (!PlayerControllerAthena || !World)
			return;

		AFortGameModeAthena* GameModeAthena = Cast<AFortGameModeAthena>(World->AuthorityGameMode);

		if (GameModeAthena && PlayerControllerAthena->IsInAircraft())
			GameModeAthena->RestartPlayer(PlayerControllerAthena);
	}

	void (*DispatchRequestOG)(UMcpProfileGroup* McpProfileGroup, __int64 a2, int32 a3);
	void DispatchRequestHook(UMcpProfileGroup* McpProfileGroup, __int64 a2, int32 a3)
	{
		DispatchRequestOG(McpProfileGroup, a2, 3);
	}

	void ServerCreateBuildingActorHook(AFortPlayerControllerAthena* PlayerControllerAthena, FCreateBuildingActorData CreateBuildingData)
	{
		UWorld* World = UWorld::GetWorld();
		if (!World) return;

		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(World->GameState);
		if (!GameStateAthena) return;

		UBuildingStructuralSupportSystem* StructuralSupportSystem = GameStateAthena->StructuralSupportSystem;
		if (!StructuralSupportSystem) return;

		UClass* BuildingClass = CreateBuildingData.BuildingClassData.BuildingClass.Get();

		if (!BuildingClass)
		{
			TSubclassOf<ABuildingActor> SubBuildingClass{};
			SubBuildingClass.ClassPtr = PlayerControllerAthena->BroadcastRemoteClientInfo->RemoteBuildableClass;

			CreateBuildingData.BuildingClassData.BuildingClass = SubBuildingClass;
			BuildingClass = CreateBuildingData.BuildingClassData.BuildingClass;
		}

		if (!BuildingClass)
			return;

		TArray<ABuildingActor*> ExistingBuildings;
		EFortBuildPreviewMarkerOptionalAdjustment MarkerOptionalAdjustment;

		EFortStructuralGridQueryResults GridQueryResults = StructuralSupportSystem->CanAddBuildingActorClassToGrid(
			PlayerControllerAthena,
			BuildingClass,
			CreateBuildingData.BuildLoc,
			CreateBuildingData.BuildRot,
			CreateBuildingData.bMirrored,
			&ExistingBuildings,
			&MarkerOptionalAdjustment,
			false
		);

		if (GridQueryResults == EFortStructuralGridQueryResults::CanAdd)
		{
			for (int32 i = 0; i < ExistingBuildings.Num(); i++)
			{
				ABuildingActor* ExistingBuilding = ExistingBuildings[i];
				if (!ExistingBuilding) continue;

				ExistingBuilding->K2_DestroyActor();
			}

			ABuildingSMActor* BuildingSMActor = Cast<ABuildingSMActor>(SpawnActor(UWorld::GetWorld(), CreateBuildingData.BuildingClassData.BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot));
			if (!BuildingSMActor) return;

			BuildingSMActor->CurrentBuildingLevel = CreateBuildingData.BuildingClassData.UpgradeLevel;

			int32(*PayBuildingCosts)(AFortPlayerController * PlayerController, const FBuildingClassData & BuildingClassData) = decltype(PayBuildingCosts)(0xa081740 + uintptr_t(GetModuleHandle(0)));
			PayBuildingCosts(PlayerControllerAthena, CreateBuildingData.BuildingClassData);

			BuildingSMActor->InitializeKismetSpawnedBuildingActor(BuildingSMActor, PlayerControllerAthena, true, nullptr, true);
			BuildingSMActor->bPlayerPlaced = true;

			AFortPlayerStateAthena* PlayerStateAthena = Cast<AFortPlayerStateAthena>(PlayerControllerAthena->PlayerState);

			if (PlayerStateAthena)
			{
				BuildingSMActor->PlacedByPlayer(PlayerStateAthena);

				BuildingSMActor->SetTeam(PlayerStateAthena->TeamIndex);
				BuildingSMActor->OnRep_Team();
			}
		}
	}

	void ServerBeginEditingBuildingActorHook(AFortPlayerControllerAthena* PlayerControllerAthena, ABuildingSMActor* BuildingActorToEdit)
	{
		if (!BuildingActorToEdit)
			return;

		AFortPlayerPawn* PlayerPawn = PlayerControllerAthena->MyFortPawn;
		if (!PlayerPawn) return;

		if (/*BuildingActorToEdit->CheckBeginEditBuildingActor(PlayerController)*/ true)
		{
			AFortPlayerStateZone* PlayerStateZone = Cast<AFortPlayerStateZone>(PlayerControllerAthena->PlayerState);
			if (!PlayerStateZone) return;

			BuildingActorToEdit->SetEditingPlayer(PlayerStateZone);

			static UFortEditToolItemDefinition* EditToolItemDefinition = FindObjectFast<UFortEditToolItemDefinition>("/Game/Items/Weapons/BuildingTools/EditTool.EditTool");
			UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerControllerAthena->BP_FindExistingItemForDefinition(EditToolItemDefinition, FGuid(), false));

			if (WorldItem &&
				EditToolItemDefinition)
			{
				bool (*ServerExecuteWeapon)(UFortWeaponItemDefinition * WeaponItemDefinition, UFortWorldItem * WorldItem, AFortPlayerController * PlayerController) = decltype(ServerExecuteWeapon)(0xa4c6c68 + uintptr_t(GetModuleHandle(0)));
				ServerExecuteWeapon(EditToolItemDefinition, WorldItem, PlayerControllerAthena);
			}
		}
	}

	void ServerEditBuildingActorHook(AFortPlayerControllerAthena* PlayerControllerAthena, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored)
	{
		if (!BuildingActorToEdit ||
			BuildingActorToEdit->EditingPlayer != PlayerControllerAthena->PlayerState ||
			BuildingActorToEdit->bDestroyed)
			return;

		BuildingActorToEdit->SetEditingPlayer(nullptr);

		int32 CurrentBuildingLevel = BuildingActorToEdit->CurrentBuildingLevel;

		ABuildingSMActor* (*ReplaceBuildingActor)(ABuildingSMActor * BuildingSMActor, int32 a2, TSubclassOf<ABuildingSMActor> BuildingClass, int32 BuildingLevel, int32 RotationIterations, bool bMirrored, AFortPlayerController * PlayerController) = decltype(ReplaceBuildingActor)(0x929e128 + uintptr_t(GetModuleHandle(0)));
		ReplaceBuildingActor(BuildingActorToEdit, 1, NewBuildingClass, CurrentBuildingLevel, RotationIterations, bMirrored, PlayerControllerAthena);
	}

	void ServerEndEditingBuildingActorHook(AFortPlayerControllerAthena* PlayerControllerAthena, ABuildingSMActor* BuildingActorToStopEditing)
	{
		if (!BuildingActorToStopEditing)
			return;

		AFortPlayerPawn* PlayerPawn = PlayerControllerAthena->MyFortPawn;
		if (!PlayerPawn) return;

		if (BuildingActorToStopEditing->EditingPlayer != PlayerControllerAthena->PlayerState ||
			BuildingActorToStopEditing->bDestroyed)
			return;

		BuildingActorToStopEditing->SetEditingPlayer(nullptr);
	}

	void ServerExecuteInventoryItemHook(AFortPlayerControllerAthena* PlayerControllerAthena, const FGuid& ItemGuid)
	{
		AFortPlayerPawn* PlayerPawn = PlayerControllerAthena->MyFortPawn;

		if (!PlayerPawn || PlayerPawn->bIsDBNO)
			return;

		UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerControllerAthena->BP_GetInventoryItemWithGuid(ItemGuid));
		if (!WorldItem) return;

		UFortWorldItemDefinition* WorldItemDefinition = Cast<UFortWorldItemDefinition>(WorldItem->ItemEntry.ItemDefinition);
		if (!WorldItemDefinition) return;

		UFortGadgetItemDefinition* GadgetItemDefinition = Cast<UFortGadgetItemDefinition>(WorldItemDefinition);

		if (GadgetItemDefinition)
		{
			bool (*ServerExecuteGadget)(UFortGadgetItemDefinition * GadgetItemDefinition, UFortWorldItem * WorldItem, AFortPlayerController * PlayerController) = decltype(ServerExecuteGadget)(0x9a3b5f0 + uintptr_t(GetModuleHandle(0)));
			ServerExecuteGadget(GadgetItemDefinition, WorldItem, PlayerControllerAthena);
			return;
		}

		UFortWeaponItemDefinition* WeaponItemDefinition = Cast<UFortWeaponItemDefinition>(WorldItemDefinition);
		if (!WeaponItemDefinition) return;

		UFortContextTrapItemDefinition* ContextTrapItemDefinition = Cast<UFortContextTrapItemDefinition>(WeaponItemDefinition);

		if (ContextTrapItemDefinition)
		{
			bool (*ServerExecuteContextTrap)(UFortContextTrapItemDefinition * ContextTrapItemDefinition, UFortWorldItem * WorldItem, AFortPlayerController * PlayerController) = decltype(ServerExecuteContextTrap)(0x9a3b40c + uintptr_t(GetModuleHandle(0)));
			ServerExecuteContextTrap(ContextTrapItemDefinition, WorldItem, PlayerControllerAthena);
			return;
		}

		UFortDecoItemDefinition* DecoItemDefinition = Cast<UFortDecoItemDefinition>(WeaponItemDefinition);

		if (DecoItemDefinition)
		{
			bool (*ServerExecuteDeco)(UFortDecoItemDefinition * DecoItemDefinition, UFortWorldItem * WorldItem, AFortPlayerController * PlayerController) = decltype(ServerExecuteDeco)(0x9a3b4e4 + uintptr_t(GetModuleHandle(0)));
			ServerExecuteDeco(DecoItemDefinition, WorldItem, PlayerControllerAthena);
			return;
		}

		UFortBuildingItemDefinition* BuildingItemDefinition = Cast<UFortBuildingItemDefinition>(WeaponItemDefinition);

		if (BuildingItemDefinition)
		{
			bool (*ServerExecuteBuilding)(UFortBuildingItemDefinition * BuildingItemDefinition, UFortWorldItem * WorldItem, AFortPlayerController * PlayerController) = decltype(ServerExecuteBuilding)(0x9a3b394 + uintptr_t(GetModuleHandle(0)));
			ServerExecuteBuilding(BuildingItemDefinition, WorldItem, PlayerControllerAthena);
			return;
		}

		bool (*ServerExecuteWeapon)(UFortWeaponItemDefinition* WeaponItemDefinition, UFortWorldItem * WorldItem, AFortPlayerController * PlayerController) = decltype(ServerExecuteWeapon)(0xa4c6c68 + uintptr_t(GetModuleHandle(0)));
		ServerExecuteWeapon(WeaponItemDefinition, WorldItem, PlayerControllerAthena);
	}

	void ServerPlayEmoteItemHook(AFortPlayerControllerAthena* PlayerControllerAthena, UFortMontageItemDefinitionBase* EmoteAsset, float EmoteRandomNumber)
	{
		if (!EmoteAsset)
			return;

		AFortPlayerPawn* PlayerPawn = PlayerControllerAthena->MyFortPawn;

		if (!PlayerPawn || PlayerPawn->bIsSkydiving || PlayerPawn->bIsDBNO)
			return;

		UFortAbilitySystemComponent* AbilitySystemComponent = PlayerPawn->AbilitySystemComponent;
		if (!AbilitySystemComponent) return;

		UAthenaDanceItemDefinition* DanceItem = Cast<UAthenaDanceItemDefinition>(EmoteAsset);

		if (DanceItem)
		{
			PlayerPawn->bMovingEmote = DanceItem->bMovingEmote;
			PlayerPawn->EmoteWalkSpeed = DanceItem->WalkForwardSpeed;
			PlayerPawn->bMovingEmoteForwardOnly = DanceItem->bMoveForwardOnly;
			PlayerPawn->bEmoteUsesSecondaryFire = DanceItem->bUsesSecondaryFireInput;

			PlayerPawn->bMovingEmoteSkipLandingFX = DanceItem->bMovingEmoteSkipLandingFX;
			PlayerPawn->bMovingEmoteFollowingOnly = DanceItem->bMoveFollowingOnly;
			PlayerPawn->GroupEmoteSyncValue = DanceItem->bGroupAnimationSync;
		}

		auto EmoteAssetObjectItem = UObject::GObjects->GetItemByIndex(EmoteAsset->Index);
		TWeakObjectPtr<UObject> WeakEmoteAsset{};
		WeakEmoteAsset.ObjectIndex = EmoteAsset->Index;
		WeakEmoteAsset.ObjectSerialNumber = EmoteAssetObjectItem ? EmoteAssetObjectItem->SerialNumber : 0;

		FGameplayAbilitySpecHandle Handle{ rand() };

		FGameplayAbilitySpec Spec{ -1, -1, -1 };
		Spec.Ability = UGAB_Emote_Generic_C::GetDefaultObj();
		Spec.Level = 0;
		Spec.InputID = -1;
		Spec.Handle = Handle;
		Spec.SourceObject = WeakEmoteAsset;

		GiveAbilityAndActivateOnceA(AbilitySystemComponent, Spec, Handle);
	}

	void ServerAcknowledgePossessionHook(AFortPlayerControllerAthena* PlayerControllerAthena, APawn* P)
	{
		if (!PlayerControllerAthena || !P)
			return;

		if (PlayerControllerAthena->AcknowledgedPawn == P)
			return;

		PlayerControllerAthena->AcknowledgedPawn = P;
	}

	void ServerHandlePickupInfoHook(AFortPlayerPawnAthena* PlayerPawnAthena, AFortPickup* PickUp, const FFortPickupRequestInfo& Params_0)
	{
		if (!PickUp || PlayerPawnAthena->bIsDBNO)
			return;

		float FlyTime = Params_0.FlyTime / PlayerPawnAthena->PickupSpeedMultiplier;

		PickUp->PickupLocationData.PickupGuid = PlayerPawnAthena->CurrentWeapon ? PlayerPawnAthena->CurrentWeapon->ItemEntryGuid : FGuid();
		SetPickupTarget(PickUp, PlayerPawnAthena, FlyTime, PickUp->PickupLocationData.StartDirection, PickUp->PickupLocationData.bPlayPickupSound);
	}

	void ServerSendZiplineStateHook(AFortPlayerPawnAthena* PlayerPawnAthena, FZiplinePawnState& InZiplineState)
	{
		if (InZiplineState.AuthoritativeValue > PlayerPawnAthena->ZiplineState.AuthoritativeValue)
		{
			FZiplinePawnState OldZiplineState = PlayerPawnAthena->ZiplineState;

			PlayerPawnAthena->ZiplineState = InZiplineState;

			if (!PlayerPawnAthena->ZiplineState.bIsZiplining && PlayerPawnAthena->ZiplineState.bJumped)
			{
				float ZiplineJumpActivateDelay = PlayerPawnAthena->ZiplineJumpActivateDelay.GetValueAtLevel(0);

				if ((float)(UGameplayStatics::GetTimeSeconds(PlayerPawnAthena) - PlayerPawnAthena->ZiplineState.TimeZipliningBegan) < ZiplineJumpActivateDelay)
					return;

				EEvaluateCurveTableResult Result;

				float ZiplineJumpDampening;
				UDataTableFunctionLibrary::EvaluateCurveTableRow(PlayerPawnAthena->ZiplineJumpDampening.CurveTable, PlayerPawnAthena->ZiplineJumpDampening.RowName, 0, &Result, &ZiplineJumpDampening, FString());

				float ZiplineJumpStrength;
				UDataTableFunctionLibrary::EvaluateCurveTableRow(PlayerPawnAthena->ZiplineJumpStrength.CurveTable, PlayerPawnAthena->ZiplineJumpStrength.RowName, 0, &Result, &ZiplineJumpStrength, FString());

				const FVector& Velocity = PlayerPawnAthena->GetVelocity();

				FVector LaunchVelocity = FVector({ -750.0f, -750.0f, ZiplineJumpStrength });

				if ((float)(ZiplineJumpDampening * Velocity.X) >= -750.0f)
					LaunchVelocity.X = fminf(ZiplineJumpDampening * Velocity.X, 750.0f);

				if ((float)(ZiplineJumpDampening * Velocity.Y) >= -750.0f)
					LaunchVelocity.Y = fminf(ZiplineJumpDampening * Velocity.Y, 750.0f);

				PlayerPawnAthena->LaunchCharacter(LaunchVelocity, false, false);
			}

			void (*OnRep_ZiplineState)(AFortPlayerPawn* PlayerPawn) = decltype(OnRep_ZiplineState)(0x9fb5c38 + uintptr_t(GetModuleHandle(0)));
			OnRep_ZiplineState(PlayerPawnAthena);
		}
	}

	void MovingEmoteStoppedHook(AFortPawn* Pawn, FFrame* Stack, void* Ret)
	{
		Stack->Code += Stack->Code != nullptr;

		Pawn->bMovingEmote = false;
		Pawn->bMovingEmoteForwardOnly = false;

		Pawn->bMovingEmoteSkipLandingFX = false;
		Pawn->bMovingEmoteFollowingOnly = false;
	}

	void (*AddInventoryItemOG)(AFortPickup* Pickup, void* InventoryOwner, bool bDestroyPickup);
	void AddInventoryItemHook(AFortPickup* Pickup, void* InventoryOwner, bool bDestroyPickup)
	{
		AddInventoryItemOG(Pickup, InventoryOwner, bDestroyPickup);

		if (!InventoryOwner)
			return;

		AFortPlayerController* PlayerController = (AFortPlayerController*)(__int64(InventoryOwner) - 0x8C0);
		if (!PlayerController) return;

		Inventory::AddInventoryItem(PlayerController, Pickup->PrimaryPickupItemEntry, Pickup->PickupLocationData.PickupGuid);
	}

	void OnAircraftEnteredDropZoneHook(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic, FFrame* Stack, void* Ret)
	{
		AFortAthenaAircraft* FortAthenaAircraft;

		Stack->StepCompiledIn(&FortAthenaAircraft);

		Stack->Code += Stack->Code != nullptr;

		if (!FortAthenaAircraft)
			return;

		void (*UnlockAircraft)(UFortGameStateComponent_BattleRoyaleGamePhaseLogic * GamePhaseLogic, bool bLockAicraft) = decltype(UnlockAircraft)(0xa5842e8 + uintptr_t(GetModuleHandle(0)));
		UnlockAircraft(GamePhaseLogic, false);

		GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::BusFlying;
		GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::BusFlying);
	}

	void OnAircraftExitedDropZoneHook(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic, FFrame* Stack, void* Ret)
	{
		AFortAthenaAircraft* FortAthenaAircraft;

		Stack->StepCompiledIn(&FortAthenaAircraft);

		Stack->Code += Stack->Code != nullptr;

		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(GamePhaseLogic->GetOwner());

		if (!GameStateAthena || !FortAthenaAircraft)
			return;
		AFortGameModeAthena* GameModeAthena = Cast<AFortGameModeAthena>(GameStateAthena->AuthorityGameMode);
		if (!GameModeAthena) return;

		TArray<AFortPlayerController*> AllFortPlayerControllers = UFortKismetLibrary::GetAllFortPlayerControllers(GameModeAthena, true, true);

		for (int32 i = 0; i < AllFortPlayerControllers.Num(); i++)
		{
			AFortPlayerControllerAthena* PlayerControllerAthena = Cast<AFortPlayerControllerAthena>(AllFortPlayerControllers[i]);
			if (!PlayerControllerAthena) continue;

			if (PlayerControllerAthena->IsInAircraft())
				GameModeAthena->RestartPlayer(PlayerControllerAthena);
		}
	}

	void OnAircraftFlightEndedHook(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic, FFrame* Stack, void* Ret)
	{
		AFortAthenaAircraft* FortAthenaAircraft;

		Stack->StepCompiledIn(&FortAthenaAircraft);

		Stack->Code += Stack->Code != nullptr;

		if (!FortAthenaAircraft)
			return;

		FortAthenaAircraft->K2_DestroyActor();
	}

	void (*OnDeathServerOG)(AFortPlayerPawnAthena* PlayerPawnAthena, float Damage, FGameplayTagContainer& DamageTags, const FVector& Momentum, FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser, const FGameplayEffectContextHandle& EffectContext);
	void OnDeathServerHook(AFortPlayerPawnAthena* PlayerPawnAthena, float Damage, FGameplayTagContainer& DamageTags, const FVector& Momentum, FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser, const FGameplayEffectContextHandle& EffectContext)
	{
		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(UGameplayStatics::GetGameState(PlayerPawnAthena));
		AFortGameModeAthena* GameModeAthena = Cast<AFortGameModeAthena>(UGameplayStatics::GetGameMode(PlayerPawnAthena));

		UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(PlayerPawnAthena);

		if (!GameStateAthena || !GameModeAthena || !GamePhaseLogic)
			return OnDeathServerOG(PlayerPawnAthena, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

		AFortPlayerControllerAthena* KilledPlayerController = Cast<AFortPlayerControllerAthena>(PlayerPawnAthena->Controller);
		AFortPlayerStateAthena* KilledPlayerState = Cast<AFortPlayerStateAthena>(PlayerPawnAthena->PlayerState);

		if (!KilledPlayerController || !KilledPlayerState)
			return OnDeathServerOG(PlayerPawnAthena, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

		AFortPlayerControllerAthena* KillerPlayerController = Cast<AFortPlayerControllerAthena>(InstigatedBy);
		AFortPlayerStateAthena* KillerPlayerState = Cast<AFortPlayerStateAthena>(InstigatedBy ? InstigatedBy->PlayerState : nullptr);
		AFortPlayerPawnAthena* KillerPlayerPawn = Cast<AFortPlayerPawnAthena>(InstigatedBy ? InstigatedBy->Pawn : nullptr);

		FGameplayTagContainer DeathTags = *(FGameplayTagContainer*)(__int64(PlayerPawnAthena) + 0x2860);
		const EDeathCause DeathCause = AFortPlayerStateAthena::ToDeathCause(DeathTags, PlayerPawnAthena->bIsDBNO);
		const float Distance = KillerPlayerPawn ? KillerPlayerPawn->GetDistanceTo(PlayerPawnAthena) : 0.0f;

		auto RealFinisherPlayerState = KillerPlayerState ? KillerPlayerState : KilledPlayerState;

		KilledPlayerState->DeathInfo.FinisherOrDowner.ObjectIndex = RealFinisherPlayerState->Index;
		KilledPlayerState->DeathInfo.FinisherOrDowner.ObjectSerialNumber = 0;

		KilledPlayerState->DeathInfo.bDBNO = PlayerPawnAthena->bIsDBNO;
		KilledPlayerState->DeathInfo.DeathCause = DeathCause;
		KilledPlayerState->DeathInfo.Distance = (DeathCause == EDeathCause::FallDamage) ? PlayerPawnAthena->LastFallDistance : Distance;
		KilledPlayerState->DeathInfo.DeathLocation = PlayerPawnAthena->K2_GetActorLocation();
		KilledPlayerState->DeathInfo.DeathTags = DeathTags;

		KilledPlayerState->DeathInfo.bInitialized = true;
		KilledPlayerState->OnRep_DeathInfo();

		if (KillerPlayerState && KillerPlayerState != KilledPlayerState)
		{
			KillerPlayerState->KillScore++;
			KillerPlayerState->ClientReportKill(KilledPlayerState);
			KillerPlayerState->OnRep_Kills();

			AFortTeamInfo* KillerPlayerTeam = KillerPlayerState->PlayerTeam.Get();

			if (KillerPlayerTeam)
			{
				for (int32 i = 0; i < KillerPlayerTeam->TeamMembers.Num(); i++)
				{
					AController* KillerTeamController = KillerPlayerTeam->TeamMembers[i];
					if (!KillerTeamController) continue;

					AFortPlayerStateAthena* KillerTeamPlayerState = Cast<AFortPlayerStateAthena>(KillerTeamController->PlayerState);
					if (!KillerTeamPlayerState) continue;

					KillerTeamPlayerState->TeamKillScore++;
					KillerTeamPlayerState->OnRep_TeamKillScore();

					KillerTeamPlayerState->ClientReportTeamKill(KillerTeamPlayerState->TeamKillScore);
				}
			}
		}

		if (!GameStateAthena->IsRespawningAllowed(KilledPlayerState) && !PlayerPawnAthena->bIsDBNO)
		{
			AFortPlayerStateAthena* CorrectKillerPlayerState = (KillerPlayerState && KillerPlayerState == KilledPlayerState) ? nullptr : KillerPlayerState;
			UFortWeaponItemDefinition* KillerWeaponItemDefinition = nullptr;

			if (DamageCauser)
			{
				AFortProjectileBase* ProjectileBase = Cast<AFortProjectileBase>(DamageCauser);
				AFortWeapon* Weapon = Cast<AFortWeapon>(DamageCauser);

				if (ProjectileBase)
				{
					AFortWeapon* ProjectileBaseWeapon = Cast<AFortWeapon>(ProjectileBase->Owner);

					if (ProjectileBaseWeapon)
						KillerWeaponItemDefinition = ProjectileBaseWeapon->WeaponData;
				}
				else if (Weapon)
					KillerWeaponItemDefinition = Weapon->WeaponData;
			}

			bool bMatchEnded = GameModeAthena->HasMatchEnded();
			int32 OldPlayersLeft = GameStateAthena->PlayersLeft;

			void (*RemoveFromAlivePlayers)(AFortGameModeAthena * GameModeAthena, AFortPlayerControllerAthena * PlayerControllerAthena, AFortPlayerStateAthena * PlayerStateAthena, AFortPlayerPawnAthena * PlayerPawnAthena, UFortWeaponItemDefinition * WeaponItemDefinition, EDeathCause DeathCause, char a7) = decltype(RemoveFromAlivePlayers)(0x8e94750 + uintptr_t(GetModuleHandle(0)));
			RemoveFromAlivePlayers(GameModeAthena, KilledPlayerController, CorrectKillerPlayerState, KillerPlayerPawn, KillerWeaponItemDefinition, DeathCause, 0);

			if (GamePhaseLogic->GamePhase > EAthenaGamePhase::Warmup)
			{
				auto SendMatchReport = [&](AFortPlayerControllerAthena* PlayerControllerAthena, int32 Place) -> void
					{
						UAthenaPlayerMatchReport* PlayerMatchReport = PlayerControllerAthena->MatchReport;

						if (PlayerMatchReport)
						{
							if (PlayerMatchReport->bHasTeamStats)
							{
								FAthenaMatchTeamStats& TeamStats = PlayerMatchReport->TeamStats;
								TeamStats.Place = Place;
								TeamStats.TotalPlayers = GameStateAthena->TotalPlayers;

								PlayerControllerAthena->ClientSendTeamStatsForPlayer(TeamStats);
							}

							if (PlayerMatchReport->bHasMatchStats)
							{
								FAthenaMatchStats& MatchStats = PlayerMatchReport->MatchStats;

								PlayerControllerAthena->ClientSendMatchStatsForPlayer(MatchStats);
							}

							if (PlayerMatchReport->bHasRewards)
							{
								FAthenaRewardResult& EndOfMatchResults = PlayerMatchReport->EndOfMatchResults;
								EndOfMatchResults.LevelsGained = 5;
								EndOfMatchResults.BookLevelsGained = 10;
								EndOfMatchResults.TotalSeasonXpGained = 15;
								EndOfMatchResults.TotalBookXpGained = 20;
								EndOfMatchResults.PrePenaltySeasonXpGained = 25;

								PlayerControllerAthena->ClientSendEndBattleRoyaleMatchForPlayer(true, EndOfMatchResults);
							}

							PlayerControllerAthena->ClientSendEndMatchReportHeartbeat();
						}
					};

				if (KillerPlayerState)
				{
					AFortTeamInfo* KilledPlayerTeam = KilledPlayerState->PlayerTeam.Get();

					if (KilledPlayerTeam && KilledPlayerState->IsSquadDead())
					{
						for (int32 i = 0; i < KilledPlayerTeam->TeamMembers.Num(); i++)
						{
							AFortPlayerControllerAthena* KilledTeamController = Cast<AFortPlayerControllerAthena>(KilledPlayerTeam->TeamMembers[i]);
							if (!KilledTeamController) continue;

							AFortPlayerStateAthena* KilledTeamPlayerState = Cast<AFortPlayerStateAthena>(KilledTeamController->PlayerState);
							if (!KilledTeamPlayerState) continue;

							SendMatchReport(KilledTeamController, OldPlayersLeft);
						}
					}
				}

				if (GameStateAthena->TeamsLeft <= 1 && !bMatchEnded)
				{
					AFortTeamInfo* PlayerTeam = KillerPlayerState ? KillerPlayerState->PlayerTeam.Get() : KilledPlayerState->PlayerTeam.Get();

					if (PlayerTeam)
					{
						for (int32 i = 0; i < PlayerTeam->TeamMembers.Num(); i++)
						{
							AFortPlayerControllerAthena* TeamController = Cast<AFortPlayerControllerAthena>(PlayerTeam->TeamMembers[i]);
							if (!TeamController) continue;

							AFortPlayerStateAthena* TeamPlayerState = Cast<AFortPlayerStateAthena>(TeamController->PlayerState);
							if (!TeamPlayerState) continue;

							TeamPlayerState->Place = GameStateAthena->TeamsLeft;
							TeamPlayerState->OnRep_Place();

							APawn* FinisherPawn = KillerPlayerPawn ? KillerPlayerPawn : PlayerPawnAthena;

							TeamController->ClientNotifyWon(FinisherPawn, KillerWeaponItemDefinition, DeathCause);
							TeamController->ClientNotifyTeamWon(FinisherPawn, KillerWeaponItemDefinition, DeathCause);

							int32 PlayersLeft = KillerPlayerPawn ? GameStateAthena->PlayersLeft : OldPlayersLeft;

							static UAthenaGadgetItemDefinition* VictoryCrown = FindObjectFast<UAthenaGadgetItemDefinition>("/VictoryCrownsGameplay/Items/AGID_VictoryCrown.AGID_VictoryCrown");

							if (VictoryCrown)
							{
								int32 ItemQuantity = UFortKismetLibrary::K2_GetItemQuantityOnPlayer(TeamController, VictoryCrown, FGuid());

								if (ItemQuantity <= 0)
									UFortKismetLibrary::K2_GiveItemToPlayer(TeamController, VictoryCrown, FGuid(), 1, false);
							}

							SendMatchReport(TeamController, PlayersLeft);
						}
					}

					GameModeAthena->EndMatch();
				}
			}

			KilledPlayerController->ServerDropAllItems(nullptr);
			/*Inventory::RemoveAllItemsFromInventory(KilledPlayerController->WorldInventory);
			Inventory::UpdateInventory(KilledPlayerController->WorldInventory);*/
		}

		OnDeathServerOG(PlayerPawnAthena, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	}

	bool RemoveInventoryItemDetour(void* InventoryOwner, const FGuid& ItemGuid, int32 Count, bool bForceRemoveFromQuickBars, bool bForceRemoval, bool bForcePersistWhenEmpty) {
		AFortPlayerController* PlayerController = AFortPlayerController::GetPlayerControllerFromInventoryOwner(InventoryOwner);
		if (!PlayerController) return false;

		if (Count == 0)
			return true;

		if (PlayerController->bInfiniteAmmo && !bForceRemoval)
			return true;

		UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerController->BP_GetInventoryItemWithGuid(ItemGuid));
		if (!WorldItem) return false;

		FFortItemEntry ItemEntry = WorldItem->ItemEntry;

		auto WorldInventory = (AFortInventory*)PlayerController->WorldInventory.GetObjectRef();

		if (Count == -1)
		{
			Inventory::RemoveItem(WorldInventory, ItemGuid);
			return true;
		}
		if (Count >= ItemEntry.Count)
		{
			UFortWeaponRangedItemDefinition* WeaponRangedItemDefinition = Cast<UFortWeaponRangedItemDefinition>(ItemEntry.ItemDefinition);

			if (WeaponRangedItemDefinition && (WeaponRangedItemDefinition->bPersistInInventoryWhenFinalStackEmpty || bForcePersistWhenEmpty))
			{

			}

			Inventory::RemoveItem(WorldInventory, ItemGuid);
		}
		else if (Count < ItemEntry.Count)
		{
			int32 NewCount = ItemEntry.Count - Count;

			Inventory::ModifyCountItem(WorldInventory, ItemGuid, NewCount);
		}
		else
			return false;

		return true;
	}

	void ServerCheatHook(AFortPlayerControllerAthena* PlayerControllerAthena, const FString& Msg)
	{
		HLog("ServerCheatHook");

		if (!Msg.IsValid())
			return;

		std::string Command = Msg.ToString();
		std::vector<std::string> ParsedCommand = split(Command, ' ');

		if (ParsedCommand.empty())
			return;

		std::string Action = ParsedCommand[0];
		std::transform(Action.begin(), Action.end(), Action.begin(),
			[](unsigned char c) { return std::tolower(c); });

		FString Message = L"Unknown Command";

		UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(PlayerControllerAthena);
		AFortPlayerStateAthena* PlayerStateAthena = Cast<AFortPlayerStateAthena>(PlayerControllerAthena->PlayerState);
		AFortPlayerPawnAthena* PlayerPawnAthena = Cast<AFortPlayerPawnAthena>(PlayerControllerAthena->MyFortPawn);
		UCheatManager* CheatManager = PlayerControllerAthena->CheatManager;

		if (!GamePhaseLogic || !PlayerStateAthena || !CheatManager)
			return;

		HLog("Action: " << Action);
		HLog("ParsedCommand.size(): " << ParsedCommand.size());

		if (Action == "startaircraft")
		{
			BattleRoyaleGamePhaseLogic::StartAircraftPhase(GamePhaseLogic);
			Message = L"StartAirCraft command executed successfully!";
		}
		else if (Action == "pausesafezone")
		{
			AFortSafeZoneIndicator* SafeZoneIndicator = GamePhaseLogic->SafeZoneIndicator;

			if (SafeZoneIndicator)
			{
				SafeZoneIndicator->bPaused = !SafeZoneIndicator->bPaused;
			}

			Message = L"PauseSafeZone command executed successfully!";
		}
		else if (Action == "skipsafezone")
		{
			AFortSafeZoneIndicator* SafeZoneIndicator = GamePhaseLogic->SafeZoneIndicator;

			if (SafeZoneIndicator)
			{
				SafeZoneIndicator->SafeZoneFinishShrinkTime = 0.0f;
			}

			Message = L"SkipSafeZone command executed successfully!";
		}
		else if (Action == "buildfree")
		{
			PlayerControllerAthena->bBuildFree = PlayerControllerAthena->bBuildFree ? false : true;
			Message = PlayerControllerAthena->bBuildFree ? L"BuildFree on" : L"BuildFree off";
		}
		else if (Action == "infiniteammo")
		{
			PlayerControllerAthena->bInfiniteAmmo = PlayerControllerAthena->bInfiniteAmmo ? false : true;
			Message = PlayerControllerAthena->bInfiniteAmmo ? L"InfiniteAmmo on" : L"InfiniteAmmo off";
		}
		else if (Action == "god")
		{
			CheatManager->God();
			Message = L"null";
		}
		else if (Action == "destroytarget")
		{
			CheatManager->DestroyTarget();
			Message = L"Target successfully destroyed!";
		}
		else if (Action == "tp")
		{
			CheatManager->Teleport();
			Message = L"Teleportation successful!";
		}
		else if (Action == "bugitgo" && ParsedCommand.size() >= 4)
		{
			if (CheatManager)
			{
				try
				{
					float X = std::stof(ParsedCommand[1]);
					float Y = std::stof(ParsedCommand[2]);
					float Z = std::stof(ParsedCommand[3]);

					CheatManager->BugItGo(X, Y, Z, 0.f, 0.f, 0.f);
					Message = L"BugItGo command executed successfully!";
				}
				catch (const std::invalid_argument& e)
				{
					Message = L"Invalid coordinates provided!";
				}
				catch (const std::out_of_range& e)
				{
					Message = L"Coordinates out of range!";
				}
			}
			else
			{
				Message = L"CheatManager not found!";
			}
		}
		else if (Action == "summon" && ParsedCommand.size() >= 2)
		{
			if (PlayerPawnAthena)
			{
				const std::string& ClassName = ParsedCommand[1];

				int32 AmountToSpawn = 1;

				if (ParsedCommand.size() >= 3)
				{
					bool bIsAmountToSpawnInt = std::all_of(ParsedCommand[2].begin(), ParsedCommand[2].end(), ::isdigit);

					if (bIsAmountToSpawnInt)
						AmountToSpawn = std::stoi(ParsedCommand[2]);
				}

				UClass* Class = LoadObject<UClass>(ClassName, UClass::StaticClass());

				if (Class)
				{
					const float LootSpawnLocationX = 300;
					const float LootSpawnLocationY = 0;
					const float LootSpawnLocationZ = 0;

					FVector SpawnLocation = PlayerPawnAthena->K2_GetActorLocation() +
						PlayerPawnAthena->GetActorForwardVector() * LootSpawnLocationX +
						PlayerPawnAthena->GetActorRightVector() * LootSpawnLocationY +
						PlayerPawnAthena->GetActorUpVector() * LootSpawnLocationZ;

					for (int32 j = 0; j < AmountToSpawn; j++)
						SpawnActor(UWorld::GetWorld(), Class, SpawnLocation);

					Message = L"Summon successful!";
				}
				else
				{
					Message = L"Class not found!";
				}
			}
			else
			{
				Message = L"PlayerPawnAthena not found!";
			}
		}
		else if (Action == "resetabilities")
		{
			if (PlayerStateAthena && PlayerStateAthena->AbilitySystemComponent)
			{
				UFortAbilitySystemComponent* AbilitySystemComponent = PlayerStateAthena->AbilitySystemComponent;

				AbilitySystemComponent->ClearAllAbilities();

				for (int32 i = 0; i < AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal.Num(); i++)
				{
					FActiveGameplayEffect ActiveGameplayEffect = AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal[i];
					if (!ActiveGameplayEffect.Spec.Def) continue;

					AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveGameplayEffect.Handle, 1);
				}

				static auto FortAbilitySet = FindObjectFast<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer");
				GiveFortAbilitySet(AbilitySystemComponent, FortAbilitySet);

				UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerStateAthena);

				Message = L"Abilities successfully reset!";
			}
			else
			{
				Message = L"PlayerState/AbilitySystemComponent not found!";
			}
		}
		else if (Action == "resetinventory")
		{
			AFortInventory* WorldInventory = Cast<AFortInventory>(PlayerControllerAthena->WorldInventory.GetObjectRef());

			if (WorldInventory)
			{
				Inventory::ResetInventory(WorldInventory);

				UFortWeaponMeleeItemDefinition* PickaxeItemDefinition = nullptr;

				UAthenaPickaxeItemDefinition* AthenaPickaxeItemDefinition = PlayerControllerAthena->CosmeticLoadoutPC.Pickaxe;

				if (AthenaPickaxeItemDefinition)
					PickaxeItemDefinition = AthenaPickaxeItemDefinition->WeaponDefinition;

				Inventory::SetupInventory(PlayerControllerAthena, PickaxeItemDefinition);

				Message = L"Inventory successfully reset!";
			}
			else
			{
				Message = L"WorldInventory not found!";
			}
		}
		else if (Action == "giveitem" && ParsedCommand.size() >= 2)
		{
			std::string ItemDefinitionName = ParsedCommand[1];

			std::transform(ItemDefinitionName.begin(), ItemDefinitionName.end(), ItemDefinitionName.begin(),
				[](unsigned char c) { return std::tolower(c); });

			int32 NumberToGive = 1;
			bool bNotifyPlayer = true;

			if (ParsedCommand.size() >= 3)
			{
				bool bIsNumberToGiveInt = std::all_of(ParsedCommand[2].begin(), ParsedCommand[2].end(), ::isdigit);

				if (bIsNumberToGiveInt)
					NumberToGive = std::stoi(ParsedCommand[2]);
			}

			if (ParsedCommand.size() >= 4)
			{
				bool bIsNotifyPlayerInt = std::all_of(ParsedCommand[3].begin(), ParsedCommand[3].end(), ::isdigit);

				if (bIsNotifyPlayerInt)
					bNotifyPlayer = std::stoi(ParsedCommand[3]);
			}

			TArray<UFortItemDefinition*> AllItems = GetAllItems();

			if (NumberToGive <= 10000 && NumberToGive > 0)
			{
				bool bItemFound = false;
				for (int32 i = 0; i < AllItems.Num(); i++)
				{
					UFortWorldItemDefinition* ItemDefinition = Cast<UFortWorldItemDefinition>(AllItems[i]);

					if (!ItemDefinition)
						continue;

					std::string ItemDefinitionName2 = ItemDefinition->GetName();

					std::transform(ItemDefinitionName2.begin(), ItemDefinitionName2.end(), ItemDefinitionName2.begin(),
						[](unsigned char c) { return std::tolower(c); });

					if (ItemDefinitionName2 != ItemDefinitionName)
						continue;

					UFortKismetLibrary::K2_GiveItemToPlayer(PlayerControllerAthena, ItemDefinition, FGuid(), NumberToGive, bNotifyPlayer);
					bItemFound = true;
					break;
				}

				if (bItemFound)
				{
					Message = L"Item give success!";
				}
				else
				{
					Message = L"Item definition not found!";
				}
			}
			else
			{
				Message = L"Invalid number to give (NumberToGive <= 10000 && NumberToGive > 0)";
			}

			if (AllItems.IsValid())
				AllItems.Free();
		}
		else
		{
			UKismetSystemLibrary::ExecuteConsoleCommand(PlayerControllerAthena, Msg, PlayerControllerAthena);
		}

		if (Message != L"null")
			PlayerControllerAthena->ClientMessage(Message, FName(), 1);
	}

	void InitDetour()
	{
		void (*RequestExitWithStatus)();
		RequestExitWithStatus = decltype(RequestExitWithStatus)(Util::FindPattern("4C 8B DC 49 89 5B 08 49 89 6B 10 49 89 73 18 49 89 7B 20 41 56 48 83 EC 30 80 3D ? ? ? ? ? 49 8B C0 0F B6 F2 8B DE 44 0F B6 F1 72 2B"));

		bool (*hgrthrthrth)(); // gamesession id
		hgrthrthrth = decltype(hgrthrthrth)(Util::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B FA 48 8B F1 E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B D0 48 89 44 24 ? E8 ? ? ? ? 45 33 F6"));


		// pop up
		ABCBCBCB = decltype(ABCBCBCB)(Util::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 45 0F B6 F0 44 88 44 24 ? 4C 8B FA 48 89 55 C8 4C 8B E1 E8 ? ? ? ? 33 FF"));

		auto BaseAdress = (uintptr_t)GetModuleHandle(0);
		//2AF3D64


		TickFlushOG = decltype(TickFlushOG)(BaseAdress + 0x178b198);
		KickPlayerOG = decltype(KickPlayerOG)(BaseAdress + 0x9cd7e90);

		FortGameSessionTuConnais = decltype(FortGameSessionTuConnais)(BaseAdress + 0x2AF3D64);


		sub_7FF73E427CD0 = decltype(sub_7FF73E427CD0)(Util::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B E9 B9 ? ? ? ? 4C 8B E2"));
		CanActivateAbilityOG = decltype(CanActivateAbilityOG)(BaseAdress + 0x7ae8c40);
		LocalSpawnPlayActorOG = decltype(LocalSpawnPlayActorOG)(BaseAdress + 0x208637c);
		InitializeUIOG = decltype(InitializeUIOG)(BaseAdress + 0x262deac);

		auto FortPlayerControllerAthenaDefault = AFortPlayerControllerAthena::GetDefaultObj();
		auto IdkDefault = (void*)(__int64(FortPlayerControllerAthenaDefault) + 0x8C0);
		HookVTable((UObject*)IdkDefault, 0x1C, ModifyLoadedAmmoHook, (LPVOID*)(&ModifyLoadedAmmo), "ModifyLoadedAmmoHook");

		HookVTable(FortPlayerControllerAthenaDefault, 0xFF, GetPlayerViewPointDetour, (LPVOID*)(&GetPlayerViewPoint), "GetPlayerViewPointDetour");


		// --------- UFortAbilitySystemComponentAthena --------- \\

		auto AbilitySystemComponentDefault = UAbilitySystemComponent::GetDefaultObj();
		int32 IndexVTableOnRep_ReplicatedAnimMontage = FindIndexVTable(AbilitySystemComponentDefault, 0x1c4bbec + BaseAdress, 0x90, 0x200);

		HLog("IndexVTableOnRep_ReplicatedAnimMontage: " << IndexVTableOnRep_ReplicatedAnimMontage);

		auto FortAbilitySystemComponentAthenaDefault = UFortAbilitySystemComponentAthena::GetDefaultObj();
		HookVTable(FortAbilitySystemComponentAthenaDefault, (IndexVTableOnRep_ReplicatedAnimMontage - 0x1), InternalServerTryActiveAbility, nullptr, "UAbilitySystemComponent::InternalServerTryActiveAbility");


		// --------- AFortGameModeAthena --------- \\

		UFunction* ReadyToStartMatchFunc = FindObjectFast<UFunction>("/Script/Engine.GameMode.ReadyToStartMatch");
		HookFunctionExec(ReadyToStartMatchFunc, ReadyToStartMatchHook, (LPVOID*)(&ReadyToStartMatchOG));

		UFunction* SpawnDefaultPawnForFunc = FindObjectFast<UFunction>("/Script/Engine.GameModeBase.SpawnDefaultPawnFor");
		HookFunctionExec(SpawnDefaultPawnForFunc, SpawnDefaultPawnForHook, nullptr);

		// --------- UFortControllerComponent_Aircraft --------- \\

		UFunction* ServerAttemptAircraftJumpFunc = FindObjectFast<UFunction>("/Script/FortniteGame.FortControllerComponent_Aircraft.ServerAttemptAircraftJump");
		HookFunctionExec(ServerAttemptAircraftJumpFunc, ServerAttemptAircraftJumpHook, nullptr);

		// --------- UFortGameStateComponent_BattleRoyaleGamePhaseLogic --------- \\

		UFunction* OnAircraftEnteredDropZoneFunc = FindObjectFast<UFunction>("/Script/FortniteGame.FortGameStateComponent_BattleRoyaleGamePhaseLogic.OnAircraftEnteredDropZone");
		HookFunctionExec(OnAircraftEnteredDropZoneFunc, OnAircraftEnteredDropZoneHook, nullptr);

		UFunction* OnAircraftExitedDropZoneFunc = FindObjectFast<UFunction>("/Script/FortniteGame.FortGameStateComponent_BattleRoyaleGamePhaseLogic.OnAircraftExitedDropZone");
		HookFunctionExec(OnAircraftExitedDropZoneFunc, OnAircraftExitedDropZoneHook, nullptr);

		UFunction* OnAircraftFlightEndedFunc = FindObjectFast<UFunction>("/Script/FortniteGame.FortGameStateComponent_BattleRoyaleGamePhaseLogic.OnAircraftFlightEnded");
		HookFunctionExec(OnAircraftFlightEndedFunc, OnAircraftFlightEndedHook, nullptr);

		// --------- AFortPlayerControllerAthena --------- \\

		HookVTable(FortPlayerControllerAthenaDefault, 0x1280 / 8, ServerCreateBuildingActorHook, nullptr, "AFortPlayerControllerAthena::ServerCreateBuildingActor");
		HookVTable(FortPlayerControllerAthenaDefault, 0x1178 / 8, ServerExecuteInventoryItemHook, nullptr, "AFortPlayerControllerAthena::ServerExecuteInventoryItem");
		HookVTable(FortPlayerControllerAthenaDefault, 0x12B8 / 8, ServerBeginEditingBuildingActorHook, nullptr, "AFortPlayerControllerAthena::ServerBeginEditingBuildingActor");
		HookVTable(FortPlayerControllerAthenaDefault, 0x1290 / 8, ServerEditBuildingActorHook, nullptr, "AFortPlayerControllerAthena::ServerEditBuildingActor");
		HookVTable(FortPlayerControllerAthenaDefault, 0x12A8 / 8, ServerEndEditingBuildingActorHook, nullptr, "AFortPlayerControllerAthena::ServerEndEditingBuildingActor");
		HookVTable(FortPlayerControllerAthenaDefault, 0xFD8 / 8, ServerPlayEmoteItemHook, nullptr, "AFortPlayerControllerAthena::ServerPlayEmoteItem");
		HookVTable(FortPlayerControllerAthenaDefault, 0x998 / 8, ServerAcknowledgePossessionHook, nullptr, "AFortPlayerControllerAthena::ServerAcknowledgePossession");
		HookVTable(FortPlayerControllerAthenaDefault, 0x11D0 / 8, ServerAttemptInventoryDrop, nullptr, "AFortPlayerController::ServerAttemptInventoryDrop");
		HookVTable(FortPlayerControllerAthenaDefault, 0x1260 / 8, ServerRepairBuildingActor, nullptr, "AFortPlayerController::ServerRepairBuildingActor");
		HookVTable(FortPlayerControllerAthenaDefault, 0xFC8 / 8, ServerCheatHook, nullptr, "AFortPlayerController::ServerCheat");

		// --------- AFortPlayerPawnAthena --------- \\

		AFortPlayerPawnAthena* FortPlayerPawnAthenaDefault = AFortPlayerPawnAthena::GetDefaultObj();

		HookVTable(FortPlayerPawnAthenaDefault, 0x1248 / 8, ServerHandlePickupInfoHook, nullptr, "AFortPlayerPawnAthena::ServerHandlePickupInfo");
		HookVTable(FortPlayerPawnAthenaDefault, 0x12C0 / 8, ServerSendZiplineStateHook, nullptr, "AFortPlayerPawnAthena::ServerSendZiplineState");

		UFunction* MovingEmoteStoppedFunc = FindObjectFast<UFunction>("/Script/FortniteGame.FortPawn.MovingEmoteStopped");
		HookFunctionExec(MovingEmoteStoppedFunc, MovingEmoteStoppedHook, nullptr);

		OnDeathServerOG = decltype(OnDeathServerOG)(BaseAdress + 0x9f79d20);

		// --------- AFortPickup --------- \\

		AddInventoryItemOG = decltype(AddInventoryItemOG)(BaseAdress + 0x9a57960);


		DispatchRequestOG = decltype(DispatchRequestOG)(BaseAdress + 0x7c2ceb0);

		// --------- ABuildingActor --------- \\
		// 0x921a51c 0x9f79bb4
		OnDamageServerOG = decltype(OnDamageServerOG)(BaseAdress + 0x921a51c);

		bool (*RemoveInventoryItem)(void* InventoryOwner, const struct FGuid& ItemGuid, int32 Count, bool bForceRemoveFromQuickBars, bool bForceRemoval, bool bForcePersistWhenEmpty) = decltype(RemoveInventoryItem)(0xA087F14 + uintptr_t(GetModuleHandle(0)));

		START_DETOUR;
		DetourAttach(RemoveInventoryItem, RemoveInventoryItemDetour);
		DetourAttach(ProcessEventO, ProcessEvent);
		DetourAttach(RequestExitWithStatus, RequestExitWithStatusDetour);
		DetourAttach(hgrthrthrth, hgrthrthrthhgrthrthrth);
		DetourAttach(sub_7FF73E427CD0, sub_7FF73E427CD0Detour);
		DetourAttach(CanActivateAbilityOG, CanActivateAbility);
		DetourAttach(InitializeUIOG, InitializeUIHook);
		DetourAttach(DispatchRequestOG, DispatchRequestHook);
		DetourAttach(AddInventoryItemOG, AddInventoryItemHook);
		DetourAttach(OnDeathServerOG, OnDeathServerHook);
		DetourAttach(OnDamageServerOG, OnDamageServer);
		END_DETOUR;

		HLog("InitDetour Success !");
	}
}