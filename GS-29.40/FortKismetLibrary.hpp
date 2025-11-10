#pragma once

namespace FortKismetLibrary
{
    void K2_GiveBuildingResourceHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, void* Ret)
    {
        AFortPlayerController* Controller;
        EFortResourceType ResourceType;
        int32 ResourceAmount;

        Stack->StepCompiledIn(&Controller);
        Stack->StepCompiledIn(&ResourceType);
        Stack->StepCompiledIn(&ResourceAmount);

        Stack->Code += Stack->Code != nullptr;

        UFortResourceItemDefinition* ResourceItemDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(ResourceType);

        if (!Controller || !ResourceItemDefinition)
            return;

        UFortKismetLibrary::K2_GiveItemToPlayer(Controller, ResourceItemDefinition, FGuid(), ResourceAmount, false);
    }

    void K2_GiveItemToPlayerHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, void* Ret)
    {
        AFortPlayerController* PlayerController;
        UFortWorldItemDefinition* ItemDefinition;
        FGuid ItemVariantGuid;
        int32 NumberToGive;
        bool bNotifyPlayer;

        Stack->StepCompiledIn(&PlayerController);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&ItemVariantGuid);
        Stack->StepCompiledIn(&NumberToGive);
        Stack->StepCompiledIn(&bNotifyPlayer);

        Stack->Code += Stack->Code != nullptr;

        FFortItemEntry ItemEntry;
        Inventory::MakeItemEntry(&ItemEntry, ItemDefinition, NumberToGive, -1, -1, -1.0f);
        ItemEntry.ItemVariantGuid = ItemVariantGuid;

        AFortPlayerPawn* PlayerPawn = PlayerController->MyFortPawn;

        if (bNotifyPlayer && PlayerPawn)
        {
            const FVector& SpawnLocation = PlayerPawn->K2_GetActorLocation();

            AFortPickup* Pickup = Inventory::SpawnPickup(
                PlayerController,
                nullptr,
                &ItemEntry,
                SpawnLocation,
                SpawnLocation,
                0,
                false,
                false
            );

            if (Pickup)
            {
                const FVector& StartDirection = FVector({ 0, 0, 0 });
                SetPickupTarget(Pickup, PlayerPawn, -1, StartDirection, true);
            }
        }
        else
        {
            Inventory::SetStateValue(&ItemEntry, EFortItemEntryState::ShouldShowItemToast, 1);
            Inventory::AddInventoryItem(PlayerController, ItemEntry);
        }
    }

    int32 K2_RemoveItemFromPlayerHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, int32* Ret)
    {
        AFortPlayerController* PlayerController;
        UFortWorldItemDefinition* ItemDefinition;
        FGuid ItemVariantGuid;
        int32 AmountToRemove;
        bool bForceRemoval;

        Stack->StepCompiledIn(&PlayerController);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&ItemVariantGuid);
        Stack->StepCompiledIn(&AmountToRemove);
        Stack->StepCompiledIn(&bForceRemoval);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        if (!PlayerController || !ItemDefinition || AmountToRemove == 0)
        {
            *Ret = 0;
            return *Ret;
        }

        UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerController->BP_FindExistingItemForDefinition(ItemDefinition, ItemVariantGuid, false));

        if (WorldItem)
        {
            int32 ItemCount = WorldItem->ItemEntry.Count;
            int32 FinalCount = ItemCount - AmountToRemove;

            if (FinalCount < 0)
                FinalCount = 0;

            PlayerController->RemoveInventoryItem(WorldItem->ItemEntry.ItemGuid, AmountToRemove, false, bForceRemoval);

            *Ret = FinalCount;
            return *Ret;
        }

        *Ret = 0;
        return *Ret;
    }

    void K2_GiveItemToAllPlayersHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, void* Ret)
    {
        UObject* WorldContextObject;
        UFortWorldItemDefinition* ItemDefinition;
        FGuid ItemVariantGuid;
        int32 NumberToGive;
        bool bNotifyPlayer;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&ItemVariantGuid);
        Stack->StepCompiledIn(&NumberToGive);
        Stack->StepCompiledIn(&bNotifyPlayer);

        Stack->Code += Stack->Code != nullptr;

        if (!WorldContextObject || !ItemDefinition || NumberToGive <= 0)
            return;

        TArray<AFortPlayerController*> AllFortPlayerControllers = UFortKismetLibrary::GetAllFortPlayerControllers(WorldContextObject, true, false);

        for (int32 i = 0; i < AllFortPlayerControllers.Num(); i++)
        {
            AFortPlayerController* PlayerController = AllFortPlayerControllers[i];
            if (!PlayerController) continue;

            UFortKismetLibrary::K2_GiveItemToPlayer(PlayerController, ItemDefinition, ItemVariantGuid, NumberToGive, bNotifyPlayer);
        }
    }

    int32 K2_RemoveFortItemFromPlayerHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, int32* Ret)
    {
        AFortPlayerController* PlayerController;
        UFortItem* Item;
        int32 AmountToRemove;
        bool bForceRemoval;

        Stack->StepCompiledIn(&PlayerController);
        Stack->StepCompiledIn(&Item);
        Stack->StepCompiledIn(&AmountToRemove);
        Stack->StepCompiledIn(&bForceRemoval);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        if (!PlayerController || !Item || AmountToRemove == 0)
        {
            *Ret = 0;
            return *Ret;
        }

        UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerController->BP_GetInventoryItemWithGuid(Item->GetItemGuid()));

        if (WorldItem)
        {
            int32 ItemCount = WorldItem->ItemEntry.Count;
            int32 FinalCount = ItemCount - AmountToRemove;

            if (FinalCount < 0)
                FinalCount = 0;

            PlayerController->RemoveInventoryItem(WorldItem->ItemEntry.ItemGuid, AmountToRemove, false, bForceRemoval);

            *Ret = FinalCount;
            return *Ret;
        }

        *Ret = 0;
        return *Ret;
    }

    void K2_RemoveItemFromAllPlayersHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, void* Ret)
    {
        UObject* WorldContextObject;
        UFortWorldItemDefinition* ItemDefinition;
        FGuid ItemVariantGuid;
        int32 AmountToRemove;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&ItemVariantGuid);
        Stack->StepCompiledIn(&AmountToRemove);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        if (!WorldContextObject || !ItemDefinition || AmountToRemove == 0)
            return;

        TArray<AFortPlayerController*> AllFortPlayerControllers = UFortKismetLibrary::GetAllFortPlayerControllers(WorldContextObject, true, false);

        for (int32 i = 0; i < AllFortPlayerControllers.Num(); i++)
        {
            AFortPlayerController* PlayerController = AllFortPlayerControllers[i];
            if (!PlayerController) continue;;

            UFortKismetLibrary::K2_RemoveItemFromPlayer(PlayerController, ItemDefinition, ItemVariantGuid, AmountToRemove, false);
        }
    }

    int32 K2_RemoveItemFromPlayerByGuidHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, int32* Ret)
    {
        AFortPlayerController* PlayerController;
        FGuid ItemGuid;
        int32 AmountToRemove;
        bool bForceRemoval;

        Stack->StepCompiledIn(&PlayerController);
        Stack->StepCompiledIn(&ItemGuid);
        Stack->StepCompiledIn(&AmountToRemove);
        Stack->StepCompiledIn(&bForceRemoval);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        if (!PlayerController || AmountToRemove == 0)
        {
            *Ret = 0;
            return *Ret;
        }

        UFortWorldItem* WorldItem = Cast<UFortWorldItem>(PlayerController->BP_GetInventoryItemWithGuid(ItemGuid));

        if (WorldItem)
        {
            int32 ItemCount = WorldItem->ItemEntry.Count;
            int32 FinalCount = ItemCount - AmountToRemove;

            if (FinalCount < 0)
                FinalCount = 0;

            PlayerController->RemoveInventoryItem(WorldItem->ItemEntry.ItemGuid, AmountToRemove, false, bForceRemoval);

            *Ret = FinalCount;
            return *Ret;
        }

        *Ret = 0;
        return *Ret;
    }

    int32 K2_RemoveItemsFromPlayerByIntStateValueHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, int32* Ret)
    {
        AFortPlayerController* PlayerController;
        EFortItemEntryState StateType;
        int32 StateValue;
        bool bForceRemoval;

        Stack->StepCompiledIn(&PlayerController);
        Stack->StepCompiledIn(&StateType);
        Stack->StepCompiledIn(&StateValue);
        Stack->StepCompiledIn(&bForceRemoval);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = 0;
        return *Ret;
    }

    int32 K2_RemoveItemsFromPlayerByNameStateValueHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, int32* Ret)
    {
        AFortPlayerController* PlayerController;
        EFortItemEntryState StateType;
        FName StateValue;
        bool bForceRemoval;

        Stack->StepCompiledIn(&PlayerController);
        Stack->StepCompiledIn(&StateType);
        Stack->StepCompiledIn(&StateValue);
        Stack->StepCompiledIn(&bForceRemoval);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = 0;
        return *Ret;
    }

    AFortPickup* K2_SpawnPickupInWorldHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, AFortPickup** Ret)
    {
        UObject* WorldContextObject;
        UFortWorldItemDefinition* ItemDefinition;
        int32 NumberToSpawn;
        FVector Position;
        FVector Direction;
        int32 OverrideMaxStackCount;
        bool bToss;
        bool bRandomRotation;
        bool bBlockedFromAutoPickup;
        int32 PickupInstigatorHandle;
        EFortPickupSourceTypeFlag SourceType;
        EFortPickupSpawnSource Source;
        AFortPlayerController* OptionalOwnerPC;
        bool bPickupOnlyRelevantToOwner;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&NumberToSpawn);
        Stack->StepCompiledIn(&Position);
        Stack->StepCompiledIn(&Direction);
        Stack->StepCompiledIn(&OverrideMaxStackCount);
        Stack->StepCompiledIn(&bToss);
        Stack->StepCompiledIn(&bRandomRotation);
        Stack->StepCompiledIn(&bBlockedFromAutoPickup);
        Stack->StepCompiledIn(&PickupInstigatorHandle);
        Stack->StepCompiledIn(&SourceType);
        Stack->StepCompiledIn(&Source);
        Stack->StepCompiledIn(&OptionalOwnerPC);
        Stack->StepCompiledIn(&bPickupOnlyRelevantToOwner);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        if (!WorldContextObject || !ItemDefinition || NumberToSpawn < 1)
        {
            *Ret = nullptr;
            return *Ret;
        }

        FFortItemEntry ItemEntry;
        Inventory::MakeItemEntry(&ItemEntry, ItemDefinition, NumberToSpawn, -1, -1, -1.0f);
       
        *Ret = Inventory::SpawnPickup(WorldContextObject, nullptr, &ItemEntry, Position, Position, OverrideMaxStackCount, bToss, true, SourceType, Source);
        return *Ret;
    }

    AFortPickup* K2_SpawnPickupInWorldWithClassHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, AFortPickup** Ret)
    {
        UObject* WorldContextObject;
        UFortWorldItemDefinition* ItemDefinition;
        TSubclassOf<AFortPickup> PickupClass;
        int32 NumberToSpawn;
        FVector Position;
        FVector Direction;
        int32 OverrideMaxStackCount;
        bool bToss;
        bool bRandomRotation;
        bool bBlockedFromAutoPickup;
        int32 PickupInstigatorHandle;
        EFortPickupSourceTypeFlag SourceType;
        EFortPickupSpawnSource Source;
        AFortPlayerController* OptionalOwnerPC;
        bool bPickupOnlyRelevantToOwner;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&PickupClass);
        Stack->StepCompiledIn(&NumberToSpawn);
        Stack->StepCompiledIn(&Position);
        Stack->StepCompiledIn(&Direction);
        Stack->StepCompiledIn(&OverrideMaxStackCount);
        Stack->StepCompiledIn(&bToss);
        Stack->StepCompiledIn(&bRandomRotation);
        Stack->StepCompiledIn(&bBlockedFromAutoPickup);
        Stack->StepCompiledIn(&PickupInstigatorHandle);
        Stack->StepCompiledIn(&SourceType);
        Stack->StepCompiledIn(&Source);
        Stack->StepCompiledIn(&OptionalOwnerPC);
        Stack->StepCompiledIn(&bPickupOnlyRelevantToOwner);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = nullptr;
        return *Ret;
    }

    AFortPickup* K2_SpawnPickupInWorldWithClassAndItemEntryHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, AFortPickup** Ret)
    {
        UObject* WorldContextObject;
        FFortItemEntry ItemEntry;
        TSubclassOf<AFortPickup> PickupClass;
        FVector Position;
        FVector Direction;
        int32 OverrideMaxStackCount;
        bool bToss;
        bool bRandomRotation;
        bool bBlockedFromAutoPickup;
        EFortPickupSourceTypeFlag SourceType;
        EFortPickupSpawnSource Source;
        AFortPlayerController* OptionalOwnerPC;
        bool bPickupOnlyRelevantToOwner;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&ItemEntry);
        Stack->StepCompiledIn(&PickupClass);
        Stack->StepCompiledIn(&Position);
        Stack->StepCompiledIn(&Direction);
        Stack->StepCompiledIn(&OverrideMaxStackCount);
        Stack->StepCompiledIn(&bToss);
        Stack->StepCompiledIn(&bRandomRotation);
        Stack->StepCompiledIn(&bBlockedFromAutoPickup);
        Stack->StepCompiledIn(&SourceType);
        Stack->StepCompiledIn(&Source);
        Stack->StepCompiledIn(&OptionalOwnerPC);
        Stack->StepCompiledIn(&bPickupOnlyRelevantToOwner);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = Inventory::SpawnPickup(WorldContextObject, nullptr, &ItemEntry, Position, Position, OverrideMaxStackCount, bToss, true, SourceType, Source);
        return *Ret;
    }

    AFortPickup* K2_SpawnPickupInWorldWithClassAndLevelHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, AFortPickup** Ret)
    {
        UObject* WorldContextObject;
        UFortWorldItemDefinition* ItemDefinition;
        int32 WorldLevel;
        TSubclassOf<AFortPickup> PickupClass;
        int32 NumberToSpawn;
        FVector Position;
        FVector Direction;
        int32 OverrideMaxStackCount;
        bool bToss;
        bool bRandomRotation;
        bool bBlockedFromAutoPickup;
        int32 PickupInstigatorHandle;
        EFortPickupSourceTypeFlag SourceType;
        EFortPickupSpawnSource Source;
        AFortPlayerController* OptionalOwnerPC;
        bool bPickupOnlyRelevantToOwner;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&WorldLevel);
        Stack->StepCompiledIn(&PickupClass);
        Stack->StepCompiledIn(&NumberToSpawn);
        Stack->StepCompiledIn(&Position);
        Stack->StepCompiledIn(&Direction);
        Stack->StepCompiledIn(&OverrideMaxStackCount);
        Stack->StepCompiledIn(&bToss);
        Stack->StepCompiledIn(&bRandomRotation);
        Stack->StepCompiledIn(&bBlockedFromAutoPickup);
        Stack->StepCompiledIn(&PickupInstigatorHandle);
        Stack->StepCompiledIn(&SourceType);
        Stack->StepCompiledIn(&Source);
        Stack->StepCompiledIn(&OptionalOwnerPC);
        Stack->StepCompiledIn(&bPickupOnlyRelevantToOwner);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = nullptr;
        return *Ret;
    }

    TArray<AFortPickup*> K2_SpawnPickupInWorldWithLootTierHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, TArray<AFortPickup*>* Ret)
    {
        UObject* WorldContextObject;
        FName LootTierName;
        FVector Position;
        int32 OverrideMaxStackCount;
        AActor* OptionalASCInterface;
        bool bToss;
        bool bTossWithVelocity;
        FVector TossVelocity;
        EFortPickupSourceTypeFlag SourceType;
        EFortPickupSpawnSource Source;
        int32 LootWorldLevel;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&LootTierName);
        Stack->StepCompiledIn(&Position);
        Stack->StepCompiledIn(&OverrideMaxStackCount);
        Stack->StepCompiledIn(&OptionalASCInterface);
        Stack->StepCompiledIn(&bToss);
        Stack->StepCompiledIn(&bTossWithVelocity);
        Stack->StepCompiledIn(&TossVelocity);
        Stack->StepCompiledIn(&SourceType);
        Stack->StepCompiledIn(&Source);
        Stack->StepCompiledIn(&LootWorldLevel);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = {};
        return *Ret;
    }

    bool SpawnInstancedPickupInWorldHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, bool* Ret)
    {
        UObject* WorldContextObject;
        UFortWorldItemDefinition* ItemDefinition;
        int32 NumberToSpawn;
        FVector Position;
        FVector Direction;
        int32 OverrideMaxStackCount;
        bool bToss;
        bool bRandomRotation;
        bool bBlockedFromAutoPickup;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&NumberToSpawn);
        Stack->StepCompiledIn(&Position);
        Stack->StepCompiledIn(&Direction);
        Stack->StepCompiledIn(&OverrideMaxStackCount);
        Stack->StepCompiledIn(&bToss);
        Stack->StepCompiledIn(&bRandomRotation);
        Stack->StepCompiledIn(&bBlockedFromAutoPickup);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = false;
        return *Ret;
    }

    AFortPickup* SpawnItemVariantPickupInWorldHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, AFortPickup** Ret)
    {
        UObject* WorldContextObject;
        FSpawnItemVariantParams Params_0;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&Params_0);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = nullptr;
        return *Ret;
    }

    AFortPickup* K2_SpawnPickupInWorldWithLevelHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, AFortPickup** Ret)
    {
        UObject* WorldContextObject;
        UFortWorldItemDefinition* ItemDefinition;
        int32 WorldLevel;
        int32 NumberToSpawn;
        FVector Position;
        FVector Direction;
        int32 OverrideMaxStackCount;
        bool bToss;
        bool bRandomRotation;
        bool bBlockedFromAutoPickup;
        int32 PickupInstigatorHandle;
        EFortPickupSourceTypeFlag SourceType;
        EFortPickupSpawnSource Source;
        AFortPlayerController* OptionalOwnerPC;
        bool bPickupOnlyRelevantToOwner;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&WorldLevel);
        Stack->StepCompiledIn(&NumberToSpawn);
        Stack->StepCompiledIn(&Position);
        Stack->StepCompiledIn(&Direction);
        Stack->StepCompiledIn(&OverrideMaxStackCount);
        Stack->StepCompiledIn(&bToss);
        Stack->StepCompiledIn(&bRandomRotation);
        Stack->StepCompiledIn(&bBlockedFromAutoPickup);
        Stack->StepCompiledIn(&PickupInstigatorHandle);
        Stack->StepCompiledIn(&SourceType);
        Stack->StepCompiledIn(&Source);
        Stack->StepCompiledIn(&OptionalOwnerPC);
        Stack->StepCompiledIn(&bPickupOnlyRelevantToOwner);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = nullptr;
        return *Ret;
    }

    bool PickLootDropsHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, bool* Ret)
    {
        UObject* WorldContextObject;
        UC::TArray<FFortItemEntry> OutLootToDrop;
        FName TierGroupName;
        int32 WorldLevel;
        int32 ForcedLootTier;
        FGameplayTagContainer OptionalLootTags;

        Stack->StepCompiledIn(&WorldContextObject);
        UC::TArray<FFortItemEntry>& LootToDrops = Stack->StepCompiledInRef<UC::TArray<FFortItemEntry>>(&OutLootToDrop);
        Stack->StepCompiledIn(&TierGroupName);
        Stack->StepCompiledIn(&WorldLevel);
        Stack->StepCompiledIn(&ForcedLootTier);
        Stack->StepCompiledIn(&OptionalLootTags);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        LootToDrops.Clear();

        *Ret = false;
        return *Ret;
    }

    bool PickLootDropsWithNamedWeightsHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, bool* Ret)
    {
        UObject* WorldContextObject;
        TArray<FFortItemEntry> OutLootToDrop;
        FName TierGroupName;
        int32 WorldLevel;
        TMap<FName, float> NamedWeightsMap;
        int32 ForcedLootTier;
        FGameplayTagContainer OptionalLootTags;

        Stack->StepCompiledIn(&WorldContextObject);
        TArray<FFortItemEntry>& LootToDrops = Stack->StepCompiledInRef<TArray<FFortItemEntry>>(&OutLootToDrop);
        Stack->StepCompiledIn(&TierGroupName);
        Stack->StepCompiledIn(&WorldLevel);
        Stack->StepCompiledIn(&NamedWeightsMap);
        Stack->StepCompiledIn(&ForcedLootTier);
        Stack->StepCompiledIn(&OptionalLootTags);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = false;
        return *Ret;
    }

    FFortItemEntry GiveItemEntryFullAmmoHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, FFortItemEntry* Ret)
    {
        UObject* WorldContextObject;
        FFortItemEntry InItemEntry;

        Stack->StepCompiledIn(&WorldContextObject);
        Stack->StepCompiledIn(&InItemEntry);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        FFortItemEntry ItemEntry;
        ItemEntry.CreateItemEntry();

        *Ret = ItemEntry;
        return *Ret;
    }

    UFortWorldItem* GiveItemToInventoryOwnerHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, UFortWorldItem** Ret)
    {
        TScriptInterface<IFortInventoryOwnerInterface> InventoryOwner;
        UFortWorldItemDefinition* ItemDefinition;
        FGuid ItemVariantGuid;
        int32 NumberToGive;
        bool bNotifyPlayer;
        int32 ItemLevel;
        int32 PickupInstigatorHandle;
        bool bUseItemPickupAnalyticEvent;
        int32 WeaponAmmoOverride;

        Stack->StepCompiledIn(&InventoryOwner);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&ItemVariantGuid);
        Stack->StepCompiledIn(&NumberToGive);
        Stack->StepCompiledIn(&bNotifyPlayer);
        Stack->StepCompiledIn(&ItemLevel);
        Stack->StepCompiledIn(&PickupInstigatorHandle);
        Stack->StepCompiledIn(&bUseItemPickupAnalyticEvent);
        Stack->StepCompiledIn(&WeaponAmmoOverride);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = nullptr;
        return *Ret;
    }

    UFortWorldItem* GiveItemToInventoryOwnerWithParamsHook(UFortKismetLibrary* KismetLibrary, FFrame* Stack, UFortWorldItem** Ret)
    {
        TScriptInterface<IFortInventoryOwnerInterface> InventoryOwner;
        UFortWorldItemDefinition* ItemDefinition;
        FGuid ItemVariantGuid;
        int32 NumberToGive;
        FGiveInventoryItemParams Params_0;

        Stack->StepCompiledIn(&InventoryOwner);
        Stack->StepCompiledIn(&ItemDefinition);
        Stack->StepCompiledIn(&ItemVariantGuid);
        Stack->StepCompiledIn(&NumberToGive);
        Stack->StepCompiledIn(&Params_0);

        Stack->Code += Stack->Code != nullptr;

        HLog(__FUNCTION__);

        *Ret = nullptr;
        return *Ret;
    }

    bool InitDetour()
    {
        UFortKismetLibrary* FortKismetLibraryDefault = UFortKismetLibrary::GetDefaultObj();
        UClass* FortKismetLibraryClass = UFortKismetLibrary::StaticClass();

        UFunction* K2_GiveBuildingResourceFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_GiveBuildingResource");
        HookFunctionExec(K2_GiveBuildingResourceFunc, K2_GiveBuildingResourceHook, nullptr);

        UFunction* K2_GiveItemToPlayerFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_GiveItemToPlayer");
        HookFunctionExec(K2_GiveItemToPlayerFunc, K2_GiveItemToPlayerHook, nullptr);

        UFunction* K2_RemoveItemFromPlayerFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_RemoveItemFromPlayer");
        HookFunctionExec(K2_RemoveItemFromPlayerFunc, K2_RemoveItemFromPlayerHook, nullptr);

        UFunction* K2_GiveItemToAllPlayersFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_GiveItemToAllPlayers");
        HookFunctionExec(K2_GiveItemToAllPlayersFunc, K2_GiveItemToAllPlayersHook, nullptr);

        UFunction* K2_RemoveFortItemFromPlayerFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_RemoveFortItemFromPlayer");
        HookFunctionExec(K2_RemoveFortItemFromPlayerFunc, K2_RemoveFortItemFromPlayerHook, nullptr);

        UFunction* K2_RemoveItemFromAllPlayersFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_RemoveItemFromAllPlayers");
        HookFunctionExec(K2_RemoveItemFromAllPlayersFunc, K2_RemoveItemFromAllPlayersHook, nullptr);

        UFunction* K2_RemoveItemFromPlayerByGuidFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_RemoveItemFromPlayerByGuid");
        HookFunctionExec(K2_RemoveItemFromPlayerByGuidFunc, K2_RemoveItemFromPlayerByGuidHook, nullptr);

        UFunction* K2_RemoveItemsFromPlayerByIntStateValueFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_RemoveItemsFromPlayerByIntStateValue");
        HookFunctionExec(K2_RemoveItemsFromPlayerByIntStateValueFunc, K2_RemoveItemsFromPlayerByIntStateValueHook, nullptr);

        UFunction* K2_RemoveItemsFromPlayerByNameStateValueFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_RemoveItemsFromPlayerByNameStateValue");
        HookFunctionExec(K2_RemoveItemsFromPlayerByNameStateValueFunc, K2_RemoveItemsFromPlayerByNameStateValueHook, nullptr);

        UFunction* K2_SpawnPickupInWorldFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_SpawnPickupInWorld");
        HookFunctionExec(K2_SpawnPickupInWorldFunc, K2_SpawnPickupInWorldHook, nullptr);

        UFunction* K2_SpawnPickupInWorldWithClassFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_SpawnPickupInWorldWithClass");
        HookFunctionExec(K2_SpawnPickupInWorldWithClassFunc, K2_SpawnPickupInWorldWithClassHook, nullptr);

        UFunction* K2_SpawnPickupInWorldWithClassAndItemEntryFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_SpawnPickupInWorldWithClassAndItemEntry");
        HookFunctionExec(K2_SpawnPickupInWorldWithClassAndItemEntryFunc, K2_SpawnPickupInWorldWithClassAndItemEntryHook, nullptr);

        UFunction* K2_SpawnPickupInWorldWithClassAndLevelFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_SpawnPickupInWorldWithClassAndLevel");
        HookFunctionExec(K2_SpawnPickupInWorldWithClassAndLevelFunc, K2_SpawnPickupInWorldWithClassAndLevelHook, nullptr);

        UFunction* K2_SpawnPickupInWorldWithLootTierFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_SpawnPickupInWorldWithLootTier");
        HookFunctionExec(K2_SpawnPickupInWorldWithLootTierFunc, K2_SpawnPickupInWorldWithLootTierHook, nullptr);

        UFunction* SpawnInstancedPickupInWorldFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "SpawnInstancedPickupInWorld");
        HookFunctionExec(SpawnInstancedPickupInWorldFunc, SpawnInstancedPickupInWorldHook, nullptr);

        UFunction* SpawnItemVariantPickupInWorldFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "SpawnItemVariantPickupInWorld");
        HookFunctionExec(SpawnItemVariantPickupInWorldFunc, SpawnItemVariantPickupInWorldHook, nullptr);

        UFunction* K2_SpawnPickupInWorldWithLevelFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "K2_SpawnPickupInWorldWithLevel");
        HookFunctionExec(K2_SpawnPickupInWorldWithLevelFunc, K2_SpawnPickupInWorldWithLevelHook, nullptr);

        UFunction* PickLootDropsFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "PickLootDrops");
        HookFunctionExec(PickLootDropsFunc, PickLootDropsHook, nullptr);

        UFunction* PickLootDropsWithNamedWeightsFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "PickLootDropsWithNamedWeights");
        HookFunctionExec(PickLootDropsWithNamedWeightsFunc, PickLootDropsWithNamedWeightsHook, nullptr);

        UFunction* GiveItemEntryFullAmmoFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "GiveItemEntryFullAmmo");
        HookFunctionExec(GiveItemEntryFullAmmoFunc, GiveItemEntryFullAmmoHook, nullptr);

        UFunction* GiveItemToInventoryOwnerFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "GiveItemToInventoryOwner");
        HookFunctionExec(GiveItemToInventoryOwnerFunc, GiveItemToInventoryOwnerHook, nullptr);

        UFunction* GiveItemToInventoryOwnerWithParamsFunc = FortKismetLibraryClass->GetFunction("FortKismetLibrary", "GiveItemToInventoryOwnerWithParams");
        HookFunctionExec(GiveItemToInventoryOwnerWithParamsFunc, GiveItemToInventoryOwnerWithParamsHook, nullptr);

        return true;
    }
}