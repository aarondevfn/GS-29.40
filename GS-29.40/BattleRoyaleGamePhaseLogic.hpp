#pragma once

namespace BattleRoyaleGamePhaseLogic
{
	AFortAthenaAircraft* Word_SpawnAirCraft(UWorld* World, int32 AircraftIndex)
	{
		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(World->GameState);
		if (!GameStateAthena) return nullptr;

		AFortAthenaMapInfo* MapInfo = GameStateAthena->MapInfo;
		if (!MapInfo) return nullptr;

		TSubclassOf<AFortAthenaAircraft> AircraftClass = MapInfo->AircraftClass;

		if (AircraftClass)
		{
			FAircraftFlightInfo* FlightInfo = &MapInfo->FlightInfos[AircraftIndex];
			if (!FlightInfo) return nullptr;

			AFortAthenaAircraft* AthenaAircraft = AFortAthenaAircraft::SpawnAircraft(World, AircraftClass, *FlightInfo);

			if (AthenaAircraft)
				AthenaAircraft->AircraftIndex = AircraftIndex;

			return AthenaAircraft;
		}

		return nullptr;
	}

	bool StartAircraftPhase(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic)
	{
		UWorld* World = UWorld::GetWorld();
		if (!World) return false;

		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(GamePhaseLogic->GetOwner());
		if (!GameStateAthena) return false;

		AFortGameModeAthena* GameModeAthena = Cast<AFortGameModeAthena>(GameStateAthena->AuthorityGameMode);
		if (!GameModeAthena)  return false;

		// bool bBlockingNewPlayers;
		if (!(*(bool*)(__int64(GameModeAthena) + 0x1010)))
		{
			// RemoveMissingPlayersFromSession(GameModeAthena);
			// sub_7FF66F258F40(GameModeAthena);

			*(bool*)(__int64(GameModeAthena) + 0x1010) = true;
		}

		GamePhaseLogic->bGameModeWillSkipAircraft = false;

		if (GamePhaseLogic->GamePhase > EAthenaGamePhase::Aircraft)
			return false;

		if (GamePhaseLogic->GamePhase == EAthenaGamePhase::Aircraft)
		{
			HLog("UFortGameStateComponent_BattleRoyaleGamePhaseLogic::StartAircraftPhase: Initiating aircraft phase");
			return true;
		}

		UFortPlaylistAthena* PlaylistAthena = GameStateAthena->CurrentPlaylistInfo.BasePlaylist;

		if (!PlaylistAthena)
		{
			HLog("UFortGameStateComponent_BattleRoyaleGamePhaseLogic::StartAircraftPhase: Playlist is Null!");
			return false;
		}

		int32 TeamCount = 1;

		if (PlaylistAthena->AirCraftBehavior == EAirCraftBehavior::OpposingAirCraftForEachTeam)
			TeamCount = GameStateAthena->TeamCount;

		if (TeamCount > 0)
		{
			TArray<AFortAthenaAircraft*> InAircrafts;

			for (int32 i = 0; i < TeamCount; i++)
			{
				AFortAthenaAircraft* AthenaAircraft = Word_SpawnAirCraft(World, i);
				if (!AthenaAircraft) break;

				TWeakObjectPtr<AFortAthenaAircraft> WeakAircraft{};
				WeakAircraft.ObjectIndex = AthenaAircraft->Index;
				WeakAircraft.ObjectSerialNumber = 0;

				GamePhaseLogic->Aircrafts_GameState.Add(WeakAircraft);
				GamePhaseLogic->Aircrafts_GameMode.Add(WeakAircraft);

				const double TimeSeconds = UGameplayStatics::GetTimeSeconds(Globals::World);
				const float StartTime = GameStateAthena->ServerWorldTimeSecondsDelta + TimeSeconds;

				AthenaAircraft->StartFlightPath(StartTime);

				FAircraftFlightInfo* FlightPath = &GameStateAthena->MapInfo->FlightInfos[i];
				if (!FlightPath) break;

				// OnAircraftEnteredDropZone
				{
					int32 Interval = FlightPath->TimeTillDropStart * 1000;

					auto callbackOnAircraftEnteredDropZone = [GamePhaseLogic, AthenaAircraft]() {
						GamePhaseLogic->OnAircraftEnteredDropZone(AthenaAircraft);
						};

					std::thread timerThreadOnAircraftEnteredDropZon(TimerFunction, Interval, callbackOnAircraftEnteredDropZone);
					timerThreadOnAircraftEnteredDropZon.detach();
				}

				// OnAircraftExitedDropZone
				{
					int32 Interval = FlightPath->TimeTillDropEnd * 1000;

					auto callbackOnAircraftExitedDropZone = [GamePhaseLogic, AthenaAircraft]() {
						GamePhaseLogic->OnAircraftExitedDropZone(AthenaAircraft);
						};

					std::thread timerThreadOnAircraftExitedDropZone(TimerFunction, Interval, callbackOnAircraftExitedDropZone);
					timerThreadOnAircraftExitedDropZone.detach();
				}

				// OnAircraftFlightEnded
				{
					int32 Interval = FlightPath->TimeTillFlightEnd * 1000;

					auto callbackOnAircraftFlightEnded = [GamePhaseLogic, AthenaAircraft]() {
						GamePhaseLogic->OnAircraftFlightEnded(AthenaAircraft);
						};
					std::thread timerThreadOnAircraftFlightEnded(TimerFunction, Interval, callbackOnAircraftFlightEnded);
					timerThreadOnAircraftFlightEnded.detach();
				}

				InAircrafts.Add(AthenaAircraft);
			}

			if (InAircrafts.Num() <= 0)
			{
				HLog("UFortGameStateComponent_BattleRoyaleGamePhaseLogic::StartAircraftPhase: Unable to spawn aircraft.");
				return false;
			}

			GamePhaseLogic->SetAircrafts(InAircrafts);

			void (*SetAircraftLock)(UFortGameStateComponent_BattleRoyaleGamePhaseLogic * GamePhaseLogic, bool bLockAicraft) = decltype(SetAircraftLock)(0xa5842e8 + uintptr_t(GetModuleHandle(0)));
			SetAircraftLock(GamePhaseLogic, true);
		}

		void (*SetGamePhase)(UFortGameStateComponent_BattleRoyaleGamePhaseLogic * GamePhaseLogic, EAthenaGamePhase GamePhase) = decltype(SetGamePhase)(0xa584478 + uintptr_t(GetModuleHandle(0)));
		SetGamePhase(GamePhaseLogic, EAthenaGamePhase::Aircraft);

		GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::BusLocked;
		GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::BusLocked);

		HLog("UFortGameStateComponent_BattleRoyaleGamePhaseLogic: Set AthenaGamePhase to [Aircraft].");

		TArray<AFortPlayerController*> AllFortPlayerControllers = UFortKismetLibrary::GetAllFortPlayerControllers(GameModeAthena, true, true);

		for (int32 i = 0; i < AllFortPlayerControllers.Num(); i++)
		{
			AFortPlayerController* PlayerController = AllFortPlayerControllers[i];
			if (!PlayerController) continue;

			AFortPlayerPawn* PlayerPawn = PlayerController->MyFortPawn;

			if (PlayerPawn)
				PlayerPawn->K2_DestroyActor();

			AFortPlayerState* PlayerState = Cast<AFortPlayerState>(PlayerController->PlayerState);
			if (!PlayerState) continue;

			UFortAbilitySystemComponent* AbilitySystemComponent = PlayerState->AbilitySystemComponent;

			if (AbilitySystemComponent)
				AbilitySystemComponent->ClearAllAbilities();

			AFortInventory* WorldInventory = Cast<AFortInventory>(PlayerController->WorldInventory.GetObjectRef());
			if (!WorldInventory) continue;

			if (WorldInventory->Inventory.ItemInstances.IsValid())
				WorldInventory->Inventory.ItemInstances.Free();

			if (WorldInventory->Inventory.ReplicatedEntries.IsValid())
				WorldInventory->Inventory.ReplicatedEntries.Free();

			WorldInventory->Inventory.MarkArrayDirty();
			WorldInventory->HandleInventoryLocalUpdate();
		}

		GamePhaseLogic->OnRep_Aircrafts();

		HLog("UFortGameStateComponent_BattleRoyaleGamePhaseLogic::StartAircraftPhase called!!");

		return true;
	}

	bool StartWarmupPhase(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic)
	{
		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(GamePhaseLogic->GetOwner());
		if (!GameStateAthena) return false;

		AFortGameModeAthena* GameModeAthena = Cast<AFortGameModeAthena>(GameStateAthena->AuthorityGameMode);
		if (!GameModeAthena)  return false;

		const double TimeSeconds = UGameplayStatics::GetTimeSeconds(GameStateAthena);
		const float ServerWorldTimeSeconds = GameStateAthena->ServerWorldTimeSecondsDelta + TimeSeconds;

		GamePhaseLogic->WarmupCountdownDuration = 900;
		GamePhaseLogic->WarmupEarlyCountdownDuration = 20;
		GamePhaseLogic->WarmupCountdownStartTime = ServerWorldTimeSeconds;
		GamePhaseLogic->WarmupCountdownEndTime = GamePhaseLogic->WarmupCountdownStartTime + GamePhaseLogic->WarmupCountdownDuration;

		GamePhaseLogic->OnRep_WarmupCountdownEndTime();

		void (*SetGamePhase)(UFortGameStateComponent_BattleRoyaleGamePhaseLogic * GamePhaseLogic, EAthenaGamePhase GamePhase) = decltype(SetGamePhase)(0xa584478 + uintptr_t(GetModuleHandle(0)));
		SetGamePhase(GamePhaseLogic, EAthenaGamePhase::Warmup);

		GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::Warmup;
		GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::Warmup);

		HLog("FortGameStateComponent_BattleRoyaleGamePhaseLogic: Set AthenaGamePhase to [Warmup].");

		return true;
	}

	void UpdateWarmupPhase(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic)
	{
		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(GamePhaseLogic->GetOwner());
		if (!GameStateAthena) return;

		const double TimeSeconds = UGameplayStatics::GetTimeSeconds(GameStateAthena);
		const float ServerWorldTimeSeconds = GameStateAthena->ServerWorldTimeSecondsDelta + TimeSeconds;

		if (GamePhaseLogic->WarmupCountdownEndTime > 0) 
		{
			if (ServerWorldTimeSeconds >= GamePhaseLogic->WarmupCountdownEndTime) 
				StartAircraftPhase(GamePhaseLogic);
		}
	}

	void Tick(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic, float DeltaSeconds)
	{
		EAthenaGamePhase GamePhase = GamePhaseLogic->GamePhase;

		if (GamePhase == EAthenaGamePhase::Warmup)
		{
			UpdateWarmupPhase(GamePhaseLogic);
		}
		else if (GamePhase == EAthenaGamePhase::SafeZones)
		{

		}
	}

	bool InitDetour()
	{
		return true;
	}
}