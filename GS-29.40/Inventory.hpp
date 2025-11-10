#pragma once
#include <map>

inline UObject* (*StaticFindObjectO)(UClass* Class, UObject* InOuter, const TCHAR* Name, bool ExactClass);

template <typename T = UObject>
static T* FindObjectFast(const std::string& ObjectName, UClass* Class = nullptr, UObject* InOuter = nullptr)
{
	auto& ObjectNameCut = ObjectName; // ObjectName.substr(ObjectName.find(" ") + 1);
	auto ObjectNameWide = std::wstring(ObjectNameCut.begin(), ObjectNameCut.end()).c_str();

	return (T*)StaticFindObjectO(Class, InOuter, ObjectNameWide, false);
}

inline UObject* (*StaticLoadObjectOriginal)(UClass*, UObject*, const wchar_t* InName, const wchar_t* Filename, uint32_t LoadFlags, UObject* Sandbox, bool bAllowObjectReconciliation, __int64 a8);

template <typename T = UObject>
static inline T* LoadObject(const std::string& NameStr, UClass* Class = nullptr, UObject* Outer = nullptr)
{
	static auto Addr = Util::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B 85 ? ? ? ? 33 FF 48 8B B5 ? ? ? ? 49 8B D8");
	StaticLoadObjectOriginal = decltype(StaticLoadObjectOriginal)(Addr);
	auto NameCWSTR = std::wstring(NameStr.begin(), NameStr.end()).c_str();
	return (T*)StaticLoadObjectOriginal(Class, Outer, NameCWSTR, nullptr, 0, nullptr, false, 0);
}

UFortAbilitySet* LoadAbilitySet(TSoftObjectPtr<UFortAbilitySet> SoftAbilitySet)
{
	/*UFortAbilitySet* AbilitySet = SoftAbilitySet.Get();

	if (!AbilitySet)
	{
		struct Conv_SoftAbilitySetReferenceToObject final
		{
		public:
			TSoftObjectPtr<UFortAbilitySet> SoftAbilitySet;
			UFortAbilitySet* ReturnValue;
		};

		static UFunction* Func = UKismetSystemLibrary::StaticClass()->GetFunction("KismetSystemLibrary", "Conv_SoftObjectReferenceToObject");

		if (Func)
		{
			Conv_SoftAbilitySetReferenceToObject Parms{};

			Parms.SoftAbilitySet = SoftAbilitySet;

			static UKismetSystemLibrary* KismetSystemLibraryDefault = UKismetSystemLibrary::GetDefaultObj();

			if (KismetSystemLibraryDefault)
				KismetSystemLibraryDefault->ProcessEvent(Func, &Parms);

			return Parms.ReturnValue;
		}

		return nullptr;
	}

	return AbilitySet;*/

	UFortAbilitySet* AbilitySet = SoftAbilitySet.Get();

	if (!AbilitySet && SoftAbilitySet.ObjectID.AssetPath.AssetName.IsValid())
	{
		const FString& AssetPathName = UKismetStringLibrary::Conv_NameToString(SoftAbilitySet.ObjectID.AssetPath.AssetName);
		AbilitySet = LoadObject<UFortAbilitySet>(AssetPathName.ToString(), UFortAbilitySet::StaticClass());
	}

	return AbilitySet;
}

namespace Inventory
{
	bool (*SetStateValue)(FFortItemEntry* ItemEntry, EFortItemEntryState StateType, int32 IntValue);

	FFortBaseWeaponStats* GetWeaponStats(UFortWeaponItemDefinition* ItemDefinition)
	{
		if (!ItemDefinition)
			return nullptr;

		if (ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) ||
			ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) ||
			ItemDefinition->IsA(UFortIngredientItemDefinition::StaticClass()))
			return nullptr;

		FDataTableRowHandle* WeaponStatHandle = &ItemDefinition->WeaponStatHandle;

		if (!WeaponStatHandle)
			return nullptr;

		FFortBaseWeaponStats BaseWeaponStats;
		UFortKismetLibrary::GetWeaponStatsRow(*WeaponStatHandle, &BaseWeaponStats);

		return &BaseWeaponStats;
	}

	void MakeItemEntry(FFortItemEntry* ItemEntry, UFortItemDefinition* ItemDefinition, int32 Count, int32 Level, int32 LoadedAmmo, float Durability)
	{
		if (!ItemEntry || !ItemDefinition)
			return;

		ItemEntry->CreateDefaultItemEntry(ItemDefinition, Count, Level);

		UFortWorldItemDefinition* WorldItemDefinition = Cast<UFortWorldItemDefinition>(ItemDefinition);

		if (WorldItemDefinition)
		{
			int32 ItemLevel = 0;

			ItemEntry->Level = (Level == -1) ? ItemLevel : Level;
			ItemEntry->Durability = (Durability == -1.0f) ? WorldItemDefinition->GetMaxDurability(ItemLevel) : Durability;

			UFortWeaponItemDefinition* WeaponItemDefinition = Cast<UFortWeaponItemDefinition>(WorldItemDefinition);

			if (WeaponItemDefinition)
			{
				FFortBaseWeaponStats* BaseWeaponStats = GetWeaponStats(WeaponItemDefinition);

				/*int32 (*GetWeaponClipSize)(UFortWeaponItemDefinition* WeaponItemDefinition, int32 WeaponLevel) = decltype(GetWeaponClipSize)(0x21da37c + uintptr_t(GetModuleHandle(0)));
				int32 WeaponClipSize = GetWeaponClipSize(WeaponItemDefinition, ItemLevel);*/

				if (WeaponItemDefinition->bUsesPhantomReserveAmmo && false)
				{
					/*ItemEntry->PhantomReserveAmmo = (WeaponClipSize - BaseWeaponStats->ClipSize);
					ItemEntry->LoadedAmmo = BaseWeaponStats ? BaseWeaponStats->ClipSize : 0;*/
				}
				else
					ItemEntry->LoadedAmmo = BaseWeaponStats->ClipSize;

				//WeaponItemDefinition->TryGetInitialAmmunitionQuantities();
			}
		}
	}

	void ModifyCountItem(AFortInventory* Inventory, const FGuid& ItemGuid, int32 NewCount)
	{
		if (!Inventory)
			return;

		for (int32 i = 0; i < Inventory->Inventory.ItemInstances.Num(); i++)
		{
			UFortWorldItem* ItemInstance = Inventory->Inventory.ItemInstances[i];
			if (!ItemInstance) continue;

			FFortItemEntry* ItemEntry = &Inventory->Inventory.ItemInstances[i]->ItemEntry;

			if (ItemEntry->ItemGuid == ItemGuid)
			{
				ItemEntry->SetCount(NewCount);
				break;
			}
		}

		for (int32 i = 0; i < Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* ReplicatedItemEntry = &Inventory->Inventory.ReplicatedEntries[i];

			if (ReplicatedItemEntry->ItemGuid == ItemGuid)
			{
				ReplicatedItemEntry->Count = NewCount;
				Inventory->Inventory.MarkItemDirty(ReplicatedItemEntry);
				break;
			}
		}
	}

	void ModifyLoadedAmmoItem(AFortInventory* Inventory, const FGuid& ItemGuid, int32 NewLoadedAmmo)
	{
		if (!Inventory)
			return;

		for (int32 i = 0; i < Inventory->Inventory.ItemInstances.Num(); i++)
		{
			UFortWorldItem* ItemInstance = Inventory->Inventory.ItemInstances[i];
			if (!ItemInstance) continue;

			FFortItemEntry* ItemEntry = &Inventory->Inventory.ItemInstances[i]->ItemEntry;

			if (ItemEntry->ItemGuid == ItemGuid)
			{
				ItemEntry->SetLoadedAmmo(NewLoadedAmmo);
				break;
			}
		}

		for (int32 i = 0; i < Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* ReplicatedItemEntry = &Inventory->Inventory.ReplicatedEntries[i];

			if (ReplicatedItemEntry->ItemGuid == ItemGuid)
			{
				ReplicatedItemEntry->LoadedAmmo = NewLoadedAmmo;
				Inventory->Inventory.MarkItemDirty(ReplicatedItemEntry);
				break;
			}
		}
	}

	bool IsInventoryFull(AFortInventory* Inventory)
	{
		if (!Inventory)
			return false;

		AFortPlayerController* PlayerController = Cast<AFortPlayerController>(Inventory->Owner);
		if (!PlayerController) return false;

		int32 (*GetInventoryCapacity)(void* InventoryOwnerInterfaceAddress, EFortInventoryType InventoryType) = decltype(GetInventoryCapacity)(0x9a2c6b8 + uintptr_t(GetModuleHandle(0)));
		int32 InventoryCapacity = GetInventoryCapacity(PlayerController->GetInventoryOwner(), Inventory->InventoryType);

		int32 (*GetInventorySize)(void* InventoryOwnerInterfaceAddress, EFortInventoryType InventoryType) = decltype(GetInventorySize)(0x9a2cbd8 + uintptr_t(GetModuleHandle(0)));
		int32 InventorySize = GetInventorySize(PlayerController->GetInventoryOwner(), Inventory->InventoryType);

		return (InventorySize >= InventoryCapacity);
	}

	UFortWorldItem* AddItem(AFortInventory* Inventory, FFortItemEntry ItemEntry)
	{
		if (!Inventory || !Inventory->Owner)
			return nullptr;

		UFortItemDefinition* ItemDefinition = Cast<UFortItemDefinition>(ItemEntry.ItemDefinition);
		if (!ItemDefinition) return nullptr;

		UFortWorldItem* WorldItem = Cast<UFortWorldItem>(ItemDefinition->CreateTemporaryItemInstanceBP(ItemEntry.Count, ItemEntry.Level));
		if (!WorldItem) return nullptr;

		WorldItem->ItemEntry.CopyItemEntryWithReset(&ItemEntry);

		AFortPlayerController* PlayerController = Cast<AFortPlayerController>(Inventory->Owner);

		WorldItem->SetOwningControllerForTemporaryItem(PlayerController);

		Inventory->AddWorldItem(WorldItem);

		if (PlayerController)
		{
			UFortGadgetItemDefinition* GadgetItemDefinition = Cast<UFortGadgetItemDefinition>(WorldItem->ItemEntry.ItemDefinition);

			if (GadgetItemDefinition)
			{
				void (*ApplyGadgetData)(UFortGadgetItemDefinition* GadgetItemDefinition, void* InventoryOwner, class UFortWorldItem* WorldItem, bool bIdk) = decltype(ApplyGadgetData)(0x9a204ac + uintptr_t(GetModuleHandle(0)));
				ApplyGadgetData(GadgetItemDefinition, PlayerController->GetInventoryOwner(), WorldItem, true);
			}
		}

		return WorldItem;
	}

	void RemoveItem(AFortInventory* Inventory, const FGuid& ItemGuid)
	{
		if (!Inventory)
			return;

		AFortPlayerController* PlayerController = Cast<AFortPlayerController>(Inventory->Owner);

		if (!PlayerController)
			return;

		UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerController->BP_GetInventoryItemWithGuid(ItemGuid));

		if (!WorldItem)
			return;

		UFortGadgetItemDefinition* GadgetItemDefinition = Cast<UFortGadgetItemDefinition>(WorldItem->ItemEntry.ItemDefinition);

		if (GadgetItemDefinition)
		{
			void (*RemoveGadgetData)(UFortGadgetItemDefinition* GadgetItemDefinition, void* InventoryOwner, class UFortWorldItem* WorldItem) = decltype(RemoveGadgetData)(0x4c2ad2c + uintptr_t(GetModuleHandle(0)));
			RemoveGadgetData(GadgetItemDefinition, PlayerController->GetInventoryOwner(), WorldItem);
		}

		Inventory->RecentlyRemoved.Add(WorldItem);

		for (int32 i = 0; i < Inventory->Inventory.ItemInstances.Num(); i++)
		{
			UFortWorldItem* ItemInstance = Inventory->Inventory.ItemInstances[i];
			if (!ItemInstance) continue;

			FFortItemEntry* ItemEntry = &Inventory->Inventory.ItemInstances[i]->ItemEntry;
			if (!ItemEntry) continue;

			if (ItemEntry->ItemGuid == ItemGuid)
			{
				Inventory->Inventory.ItemInstances.Remove(i);
				break;
			}
		}

		for (int32 i = 0; i < Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* ReplicatedItemEntry = &Inventory->Inventory.ReplicatedEntries[i];
			if (!ReplicatedItemEntry) continue;

			if (ReplicatedItemEntry->ItemGuid == ItemGuid)
			{
				Inventory->Inventory.ReplicatedEntries.Remove(i);
				break;
			}
		}

		Inventory->Inventory.MarkArrayDirty();
		Inventory->HandleInventoryLocalUpdate();
	}

	void ResetInventory(AFortInventory* Inventory, bool bRemoveAll = true)
	{
		if (!Inventory)
			return;

		if (bRemoveAll)
		{
			if (Inventory->Inventory.ItemInstances.IsValid())
				Inventory->Inventory.ItemInstances.Free();

			if (Inventory->Inventory.ReplicatedEntries.IsValid())
				Inventory->Inventory.ReplicatedEntries.Free();

			Inventory->Inventory.MarkArrayDirty();
			Inventory->HandleInventoryLocalUpdate();
		}
		else
		{
			TArray<FGuid> ItemGuidToRemoves;

			for (int32 i = 0; i < Inventory->Inventory.ItemInstances.Num(); i++)
			{
				UFortWorldItem* ItemInstance = Inventory->Inventory.ItemInstances[i];
				if (!ItemInstance) continue;

				FFortItemEntry* ItemEntry = &ItemInstance->ItemEntry;
				if (!ItemEntry) continue;

				if (!ItemInstance->CanBeDropped())
					continue;

				ItemGuidToRemoves.Add(ItemEntry->ItemGuid);
			}

			for (int32 i = 0; i < ItemGuidToRemoves.Num(); i++)
				Inventory::RemoveItem(Inventory, ItemGuidToRemoves[i]);
		}
	}

	AFortPickup* GetClosestPickup(AFortPickup* PickupToCombine, float MaxDistance)
	{
		if (!PickupToCombine)
			return nullptr;

		UFortWorldItemDefinition* WorldItemDefinitionCombine = Cast<UFortWorldItemDefinition>(PickupToCombine->PrimaryPickupItemEntry.ItemDefinition);

		if (WorldItemDefinitionCombine)
		{
			TArray<AActor*> Actors;
			UGameplayStatics::GetAllActorsOfClass(PickupToCombine, AFortPickup::StaticClass(), &Actors);

			AFortPickup* ClosestPickup = nullptr;

			for (int32 i = 0; i < Actors.Num(); i++)
			{
				AFortPickup* Pickup = Cast<AFortPickup>(Actors[i]);
				if (!Pickup) continue;

				if (Pickup->bActorIsBeingDestroyed || Pickup->bPickedUp)
					continue;

				if (Pickup == PickupToCombine)
					continue;

				const float Distance = PickupToCombine->GetDistanceTo(Pickup);

				if (Distance > MaxDistance)
					continue;

				if (Pickup->PickupLocationData.CombineTarget)
					continue;

				FFortItemEntry* PrimaryPickupItemEntry = &Pickup->PrimaryPickupItemEntry;
				if (!PrimaryPickupItemEntry) continue;

				UFortWorldItemDefinition* WorldItemDefinition = Cast<UFortWorldItemDefinition>(PrimaryPickupItemEntry->ItemDefinition);
				if (!WorldItemDefinition) continue;

				if (WorldItemDefinition != WorldItemDefinitionCombine)
					continue;

				if (PrimaryPickupItemEntry->Count >= WorldItemDefinition->MaxStackSize.GetValueAtLevel(0))
					continue;

				ClosestPickup = Pickup;
				break;
			}

			if (Actors.IsValid())
				Actors.Free();

			return ClosestPickup;
		}

		return nullptr;
	}

	bool CombineNearestPickup(AFortPickup* PickupToCombine, float MaxDistance)
	{
		if (!PickupToCombine)
			return false;

		AFortPickup* ClosestPickup = Inventory::GetClosestPickup(PickupToCombine, MaxDistance);

		if (ClosestPickup)
		{
			PickupToCombine->PickupLocationData.CombineTarget = ClosestPickup;
			PickupToCombine->PickupLocationData.FlyTime = 0.25f;
			PickupToCombine->PickupLocationData.LootFinalPosition = (FVector_NetQuantize10)ClosestPickup->K2_GetActorLocation();
			PickupToCombine->PickupLocationData.LootInitialPosition = (FVector_NetQuantize10)PickupToCombine->K2_GetActorLocation();
			PickupToCombine->PickupLocationData.FinalTossRestLocation = (FVector_NetQuantize10)ClosestPickup->K2_GetActorLocation();

			PickupToCombine->OnRep_PickupLocationData();
			PickupToCombine->FlushNetDormancy();

			return true;
		}

		return false;
	}

	AFortPickup* SpawnPickup(UObject* WorldContextObject, AFortPawn* ItemOwner, FFortItemEntry *ItemEntry, FVector SpawnLocation, FVector FinalLocation, int32 OverrideMaxStackCount = 0, bool bToss = true, bool bShouldCombinePickupsWhenTossCompletes = true, EFortPickupSourceTypeFlag InPickupSourceTypeFlags = EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource InPickupSpawnSource = EFortPickupSpawnSource::Unset)
	{
		HLog("ItemDefinition : " << ItemEntry->ItemDefinition->GetName());
		HLog("Count : " << ItemEntry->Count);

		AFortPickup* (*SimpleCreatePickup)(UObject* WorldContextObject, FFortItemEntry* ItemEntry, const FVector& SpawnLocation, const FRotator& SpawnRotation) = decltype(SimpleCreatePickup)(0xa0a16b8 + uintptr_t(GetModuleHandle(0)));
		AFortPickup* Pickup = SimpleCreatePickup(WorldContextObject, ItemEntry, SpawnLocation, FRotator());

		HLog("Pickup : " << Pickup->GetName());

		if (Pickup)
		{
			if (ItemOwner)
			{
				Pickup->PawnWhoDroppedPickup.ObjectIndex = ItemOwner->Index;
				Pickup->PawnWhoDroppedPickup.ObjectSerialNumber = 0;
			}

			Pickup->PickupSourceTypeFlags = InPickupSourceTypeFlags;
			Pickup->PickupSpawnSource = InPickupSpawnSource;
			Pickup->bRandomRotation = true;

			Pickup->TossPickup(
				FinalLocation,
				ItemOwner,
				OverrideMaxStackCount,
				bToss,
				bShouldCombinePickupsWhenTossCompletes,
				InPickupSourceTypeFlags,
				InPickupSpawnSource
			);
		}

		return Pickup;
	}

	UFortWorldItem* AddInventoryItem(AFortPlayerController* PlayerController, FFortItemEntry ItemEntry, FGuid CurrentItemGuid = FGuid(), bool bReplaceWeapon = true)
	{
		if (!PlayerController || !ItemEntry.ItemDefinition)
			return nullptr;

		AFortInventory* WorldInventory = Cast<AFortInventory>(PlayerController->WorldInventory.GetObjectRef());
		if (!WorldInventory) return nullptr;

		if (ItemEntry.Count <= 0)
			return nullptr;

		TArray<UFortItem*> ItemArray;
		PlayerController->BP_FindItemInstancesFromDefinition(Cast<UFortItemDefinition>(ItemEntry.ItemDefinition), FGuid(), ItemArray);

		if (ItemArray.Num() > 0)
		{
			int32 ItemCountToAdd = ItemEntry.Count;

			for (int32 i = 0; i < ItemArray.Num(); i++)
			{
				UFortWorldItem* WorldItem = Cast<UFortWorldItem>(ItemArray[i]);
				if (!WorldItem) continue;

				UFortItemDefinition* ItemDefinition = Cast<UFortItemDefinition>(WorldItem->ItemEntry.ItemDefinition);
				if (!ItemDefinition) continue;

				int32 CurrentCount = WorldItem->ItemEntry.Count;
				int32 MaxStackSize = ItemDefinition->MaxStackSize.GetValueAtLevel(0);

				if (CurrentCount < MaxStackSize)
				{
					int32 NewCount = UKismetMathLibrary::Min(CurrentCount + ItemCountToAdd, MaxStackSize);

					ModifyCountItem(WorldInventory, WorldItem->ItemEntry.ItemGuid, NewCount);

					ItemCountToAdd -= (NewCount - CurrentCount);

					if (ItemCountToAdd <= 0)
						return WorldItem;
				}
			}

			if (ItemCountToAdd < 0)
				ItemCountToAdd = 0;

			ItemEntry.Count = ItemCountToAdd;
		}

		if (ItemEntry.Count <= 0)
			return nullptr;

		UFortWorldItemDefinition* WorldItemDefinition = Cast<UFortWorldItemDefinition>(ItemEntry.ItemDefinition);

		if (!WorldItemDefinition)
			return nullptr;

		int32 MaxStackSize = WorldItemDefinition->MaxStackSize.GetValueAtLevel(0);

		if (ItemEntry.Count > MaxStackSize)
		{
			while (ItemEntry.Count > 0)
			{
				int32 NewCount = UKismetMathLibrary::Min(ItemEntry.Count, MaxStackSize);

				FFortItemEntry NewItemEntry;
				MakeItemEntry(&NewItemEntry, WorldItemDefinition, NewCount, ItemEntry.Level, ItemEntry.LoadedAmmo, ItemEntry.Durability);

				AddInventoryItem(PlayerController, NewItemEntry, CurrentItemGuid, false);

				ItemEntry.Count -= NewCount;
			}

			return nullptr;
		}

		FVector SpawnLocation = FVector({ 0, 0, 0 });
		FVector FinalLocation = FVector({ 0, 0, 0 });

		AFortPlayerPawn* PlayerPawn = PlayerController->MyFortPawn;

		if (PlayerPawn)
		{
			SpawnLocation = PlayerPawn->K2_GetActorLocation();

			SpawnLocation.Z += 40.0f;

			float RandomAngle = UKismetMathLibrary::RandomFloatInRange(-60.0f, 60.0f);

			FRotator RandomRotation = PlayerPawn->K2_GetActorRotation();
			RandomRotation.Yaw += RandomAngle;

			float RandomDistance = UKismetMathLibrary::RandomFloatInRange(500.0f, 600.0f);
			FVector Direction = UKismetMathLibrary::GetForwardVector(RandomRotation);

			FinalLocation = SpawnLocation + Direction * RandomDistance;
		}

		UFortWorldItem* NewWorldItem = nullptr;

		if (IsInventoryFull(WorldInventory) && WorldItemDefinition->bInventorySizeLimited && PlayerPawn)
		{
			if (!bReplaceWeapon)
			{
				SpawnPickup(PlayerController, PlayerPawn, &ItemEntry, SpawnLocation, FinalLocation);
				return NewWorldItem;
			}

			UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerController->BP_GetInventoryItemWithGuid(CurrentItemGuid));

			if (!WorldItem)
			{
				AFortWeapon* Weapon = PlayerPawn->CurrentWeapon;

				if (Weapon)
					WorldItem = Cast<UFortWorldItem>(PlayerController->BP_GetInventoryItemWithGuid(Weapon->ItemEntryGuid));
			}

			if (!WorldItem)
			{
				SpawnPickup(PlayerController, PlayerPawn, &ItemEntry, SpawnLocation, FinalLocation);
				return NewWorldItem;
			}

			UFortWorldItemDefinition* WorldItemDefinition = Cast<UFortWorldItemDefinition>(WorldItem->ItemEntry.ItemDefinition);

			if (!WorldItemDefinition || !WorldItem->CanBeDropped())
			{
				SpawnPickup(PlayerController, PlayerPawn, &ItemEntry, SpawnLocation, FinalLocation);
				return NewWorldItem;
			}

			UFortWeaponRangedItemDefinition* WeaponRangedItemDefinition = Cast<UFortWeaponRangedItemDefinition>(WorldItemDefinition);

			if (WeaponRangedItemDefinition && WeaponRangedItemDefinition->bPersistInInventoryWhenFinalStackEmpty)
			{
				int32 ItemQuantity = UFortKismetLibrary::K2_GetItemQuantityOnPlayer(PlayerController, WeaponRangedItemDefinition, FGuid());

				if (ItemQuantity == 0)
				{
					SpawnPickup(PlayerController, PlayerPawn, &ItemEntry, SpawnLocation, FinalLocation);
					return NewWorldItem;
				}
			}

			PlayerController->ServerAttemptInventoryDrop(WorldItem->ItemEntry.ItemGuid, WorldItem->ItemEntry.Count, false);

			if (!IsInventoryFull(WorldInventory))
			{
				NewWorldItem = AddItem(WorldInventory, ItemEntry);

				if (NewWorldItem)
				{
					AFortPlayerControllerAthena* PlayerControllerAthena = Cast<AFortPlayerControllerAthena>(PlayerController);

					if (WorldItem->ItemEntry.ItemGuid == PlayerPawn->CurrentWeapon->ItemEntryGuid && PlayerControllerAthena)
						PlayerControllerAthena->ClientEquipItem(NewWorldItem->ItemEntry.ItemGuid, true);
				}
			}
		}
		else if (!IsInventoryFull(WorldInventory) && WorldItemDefinition->bInventorySizeLimited)
		{
			NewWorldItem = AddItem(WorldInventory, ItemEntry);
		}
		else if (!WorldItemDefinition->bInventorySizeLimited)
		{
			UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerController->BP_FindExistingItemForDefinition(WorldItemDefinition, FGuid(), false));

			if (WorldItem && !WorldItemDefinition->bAllowMultipleStacks && PlayerPawn)
			{
				SpawnPickup(PlayerController, PlayerPawn, &ItemEntry, SpawnLocation, FinalLocation);
				return NewWorldItem;
			}
			else
			{
				NewWorldItem = AddItem(WorldInventory, ItemEntry);
			}
		}

		return NewWorldItem;
	}

	struct Item
	{
		std::string GamePath;
		int Count = 1;
	};

	inline std::vector<Item> LootPlayers
	{

		{"/Game/Items/ResourcePickups/WoodItemData.WoodItemData",50},
		{"/Game/Items/ResourcePickups/StoneItemData.StoneItemData",50},
		{"/Game/Items/ResourcePickups/MetalItemData.MetalItemData",50},

		{"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium",50},
		{"/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells",20},
		{"/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets",10},
		{ "/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy",50 },
		{ "/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight",50 }
	};

	void InitBaseInventory(AFortPlayerController* PlayerController) {

		AFortInventory* WorldInventory = Cast<AFortInventory>(PlayerController->WorldInventory.GetObjectRef());
		if (!WorldInventory) return;

		for (auto& item : LootPlayers) 
		{
			UFortItemDefinition* Definition = FindObjectFast<UFortItemDefinition>(item.GamePath);

			FFortItemEntry ItemEntry;
			MakeItemEntry(&ItemEntry, Definition, item.Count, -1, -1, -1.0f);

			AddItem(WorldInventory, ItemEntry);
		}
	}

	void SetupInventory(AFortPlayerController* PlayerController, UFortWeaponMeleeItemDefinition* PickaxeItemDefinition)
	{
		if (!PlayerController)
			return;

		AFortInventory* WorldInventory = Cast<AFortInventory>(PlayerController->WorldInventory.GetObjectRef());
		if (!WorldInventory) return;

		TArray<FItemAndCount> StartingItems = Cast<AFortGameModeAthena>(UGameplayStatics::GetGameMode(PlayerController))->StartingItems;

		for (int32 i = 0; i < StartingItems.Num(); i++)
		{
			FItemAndCount StartingItem = StartingItems[i];

			UFortWorldItemDefinition* WorldItemDefinition = Cast<UFortWorldItemDefinition>(StartingItem.Item);

			if (!WorldItemDefinition || StartingItem.Count <= 0)
				continue;

			if (WorldItemDefinition->IsA(UFortSmartBuildingItemDefinition::StaticClass()))
				continue;

			FFortItemEntry ItemEntry;
			MakeItemEntry(&ItemEntry, WorldItemDefinition, StartingItem.Count, -1, -1, -1.0f);

			AddItem(WorldInventory, ItemEntry);
		}

		if (PickaxeItemDefinition)
		{
			FFortItemEntry ItemEntry;
			MakeItemEntry(&ItemEntry, PickaxeItemDefinition, 1, 0, 0, 0.f);

			AddItem(WorldInventory, ItemEntry);
		}
	}

	void InitInventory()
	{
		SetStateValue = decltype(SetStateValue)(InSDKUtils::GetImageBase() + 0x93fca94);
	}
}