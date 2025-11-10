#pragma once


struct FReceivedPacketView
{

};

struct FPacketBufferView
{
};

// Pickup
void (*SetPickupTarget)(AFortPickup* Pickup, AFortPlayerPawn* Pawn, float InFlyTime, const FVector& InStartDirection, bool bPlayPickupSound);

inline void (*ProcessEventO)(UObject* object, UFunction* func, void* Parameters);
inline AActor* (*SpawnActorO)(UWorld* World, UClass* Class, void* Position, void* Rotation, void* SpawnParameters);

inline char (*PickupDelay)(AFortPickup* Pickup);

inline void (*AddNetworkActor)(UWorld* World, AActor* Actor);


inline void (*ChangingGameSessionId)();
inline void (*InternalGetNetMode)();
inline void (*ActorInternalGetNetMode)();


inline bool (*Local_SpawnPlayActor)(ULocalPlayer* Player, const FString& URL, FString& OutError, UWorld* World);


void (*ApplyCharacterCustomization)(AFortPlayerState* PlayerState, AFortPawn* Pawn);
double (*__fastcall sub_7FF61DF28260)(ULandscapeComponent* a1, class FArchive& Ar);



bool (*IsNetworkCompatible)(const uint32 LocalNetworkVersion, const uint32 RemoteNetworkVersion);
void (*BroadcastNetFailure)(UNetDriver* Driver, ENetworkFailure FailureType, const FString& ErrorStr);

void (*TickDispatch)(UIpNetDriver* Driver, float DeltaTime);
UNetConnection* (*ProcessConnectionlessPacket)(UIpNetDriver* Driver, FReceivedPacketView& PacketRef, const FPacketBufferView& WorkingBuffer);

bool (*IsEncryptionRequired)(UNetDriver* Driver);


inline bool (*InitHost)(AOnlineBeacon* Beacon);
inline bool (*InitListen)(UNetDriver* Driver, void* InNotify, FURL& LocalURL, bool bReuseAddressAndPort, FString& Error);
inline void (*SetWorld)(UNetDriver* NetDriver, UWorld* World);

inline FGameplayAbilitySpecHandle* (*GiveAbility0)(UObject* _this, FGameplayAbilitySpecHandle* outHandle, FGameplayAbilitySpec* inSpec);


bool (*CantBuild)(UWorld* World, TSubclassOf<ABuildingActor> BuildingClass, FVector_NetQuantize10& BuildLoc, FRotator& BuildRot, bool bMirrored, TArray<ABuildingActor*>* BuildingsToBeDestroyed, __int64* a7);
int32 (*PayRepairCosts)(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToRepair);
int32 (*PayBuildingCosts)(AFortPlayerController* PlayerController, FBuildingClassData BuildingClassData);



ABuildingSMActor* (*BuildingSMActorReplaceBuildingActor)(ABuildingSMActor*, __int64, UClass*, int, int, uint8_t, AFortPlayerController*);

inline FGameplayAbilitySpecHandle* (*GiveAbilityAndActivateOnce)(UAbilitySystemComponent* ABS, FGameplayAbilitySpec& Spec, const FGameplayEventData* GameplayEventData);


inline bool (*InternalTryActivateAbility)(UAbilitySystemComponent* comp, FGameplayAbilitySpecHandle Handle, FPredictionKey  InPredictionKey, UGameplayAbility** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData);


inline bool (*CanActivateAbility)();



struct FFunctionStorage
{
	void* HeapAllocation;
#if TFUNCTION_USES_INLINE_STORAGE
	// Inline storage for an owned object
	TAlignedBytes<TFUNCTION_INLINE_SIZE, TFUNCTION_INLINE_ALIGNMENT> InlineAllocation;
#endif
};

template <bool bUnique>
struct TFunctionStorage : FFunctionStorage
{

};
template <typename StorageType, typename Ret, typename... ParamTypes>
struct TFunctionRefBase
{
	// Ret(*Callable)(void*, ParamTypes&...) = nullptr;
	void* Callable;
	StorageType Storage;
};
template<typename FuncType>
class TFunction : public TFunctionRefBase<TFunctionStorage<false>, FuncType>
{

};

void (*SetTimerForFunction)(FTimerHandle& InOutHandle, TFunction<void(void)>&& Callback, float InRate, bool InbLoop, float InFirstDelay);

struct FActorSpawnParameters
{
	FName Name;
	UObject* Template; // AActor*
	UObject* Owner; // AActor*
	UObject* Instigator; // APawn*
	UObject* OverrideLevel; // ULevel*
	UObject* OverrideParentComponent;
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;
	// ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;
	uint8_t TransformScaleMethod;
	uint16_t	bRemoteOwned : 1;
	uint16_t	bNoFail : 1;
	uint16_t	bDeferConstruction : 1;
	uint16_t	bAllowDuringConstructionScript : 1;

	enum class ESpawnActorNameMode : uint8_t
	{
		Required_Fatal,
		Required_ErrorAndReturnNull,
		Required_ReturnNull,
		Requested
	};

	ESpawnActorNameMode NameMode;
	EObjectFlags ObjectFlags;

	TFunction<void(UObject*)> CustomPreSpawnInitalization;
};


inline AActor* SpawnActor(UWorld* World, UClass* Class, FVector Location = FVector(), FRotator Rotation = FRotator(), AActor* Owner = nullptr)
{
	FActorSpawnParameters SpawnParameters{};
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParameters.Owner = Owner;


	return SpawnActorO(World, Class, &Location, &Rotation, &SpawnParameters);
}




#include "Patterns.h"

namespace Inits
{
	
	bool InitializeAll()
	{
		auto BaseAdress = (uintptr_t)GetModuleHandle(0);

		// GameMode
		//static auto HandlePostSafeZonePhaseChangedPattern = Util::FindPattern(Patterns::HandlePostSafeZonePhaseChanged);

		//GameMode::HandlePostSafeZonePhaseChanged = decltype(GameMode::HandlePostSafeZonePhaseChanged)(HandlePostSafeZonePhaseChangedPattern);

		// Replication
		static auto CallPreReplicationPattern = Util::FindPattern(Patterns::CallPreReplication);
		static auto ReplicateActorPattern = Util::FindPattern(Patterns::ReplicateActor);
		static auto SetChannelActorPattern = Util::FindPattern(Patterns::SetChannelActor);
		static auto CreateChannelByNamePattern = Util::FindPattern(Patterns::CreateChannelByName);
		static auto SendClientAdjustmentPattern = Util::FindPattern(Patterns::SendClientAdjustment);
		//static auto IsLevelInitializedForActorPattern = Util::FindPattern(Patterns::IsLevelInitializedForActor);

		CallPreReplication = decltype(CallPreReplication)(CallPreReplicationPattern);
		ReplicateActor = decltype(ReplicateActor)(ReplicateActorPattern);
		SetChannelActor = decltype(SetChannelActor)(SetChannelActorPattern);
		CreateChannelByName = decltype(CreateChannelByName)(CreateChannelByNamePattern);



		

		//IsLevelInitializedForActor = decltype(IsLevelInitializedForActor)(IsLevelInitializedForActorPattern);

		// Beacon
		static auto InitHostPattern = Util::FindPattern(Patterns::InitHost);
		static auto InitListenPattern = Util::FindPattern(Patterns::InitListen);

		InitHost = decltype(InitHost)(InitHostPattern);
		InitListen = decltype(InitListen)(InitListenPattern);


		// Inventory
		static auto CreatePickupDataPattern = Util::FindPattern(Patterns::CreatePickupData);
		static auto CreatePickupFromDataPattern = Util::FindPattern(Patterns::CreatePickupFromData);
		//
		static auto SimpleCreatePickupPattern = Util::FindPattern(Patterns::SimpleCreatePickup);
		static auto CreateItemEntryPattern = Util::FindPattern(Patterns::CreateItemEntry);
		static auto CreateDefaultItemEntryPattern = Util::FindPattern(Patterns::CreateDefaultItemEntry);
		static auto CopyItemEntryPattern = Util::FindPattern(Patterns::CopyItemEntry);
		static auto FreeItemEntryPattern = Util::FindPattern(Patterns::FreeItemEntry);



		// Pickup
		static auto SetPickupTargetPattern = Util::FindPattern(Patterns::SetPickupTarget);
		SetPickupTarget = decltype(SetPickupTarget)(SetPickupTargetPattern);
		CantBuild = decltype(CantBuild)(Util::FindPattern(Patterns::CantBuild));




		auto StaticFindObjectPattern = Util::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 4C 8B E9 48 8D 4D E8 45 8A E1 48 83 FA FF 0F 84 ? ? ? ? 48 89 55 E0 49 8B D0"); ///
		auto SpawnActorPattern = Util::FindPattern("48 89 5C 24 ? 55 56 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 2F 0F 28 0D ? ? ? ? 48 8B FA 0F 28 15 ? ? ? ? 48 8B D9");


		auto DispatchRequestPattern = Util::FindPattern(Patterns::DispatchRequest);
		auto InternalGetNetModePattern = Util::FindPattern(Patterns::InternalGetNetMode);
		//auto ActorInternalGetNetModePattern = Util::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B B1 ? ? ? ? 33 DB 48 85 F6 74 36");





		auto FreeMemoryPattern = Util::FindPattern("48 85 C9 0F 84 ? ? ? ? 57 48 83 EC 70 4C 89 74 24 ? 48 8B F9 4C 8B 35 ? ? ? ? 4D 85 F6 0F 84 ? ? ? ? 49 8B 06 4C 8B 40 48");
		auto ReallocPattern = Util::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F1 41 8B D8 48 8B 0D ? ? ? ? 48 8B FA");



		auto GiveAbilityPattern = Util::FindPattern(Patterns::GiveAbility);
		auto InternalTryActivateAbilityPattern = Util::FindPattern(Patterns::InternalTryActivateAbility);
	

		auto ReplaceBuildingActorPattern = Util::FindPattern(Patterns::ReplaceBuildingActor);
		BuildingSMActorReplaceBuildingActor = decltype(BuildingSMActorReplaceBuildingActor)(ReplaceBuildingActorPattern);



		InternalTryActivateAbility = decltype(InternalTryActivateAbility)(InternalTryActivateAbilityPattern);
		GiveAbility0 = decltype(GiveAbility0)(GiveAbilityPattern);


		FMemory::Realloc = decltype(FMemory::Realloc)(ReallocPattern);
		FMemory::Free = decltype(FMemory::Free)(FreeMemoryPattern);

		auto ApplyCharacterCustomizationPattern = Util::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 80 B9 ? ? ? ? ? 48 8B C2 48 89 54 24 ? 4C 8B F9 48 89 4C 24 ? 0F 85 ? ? ? ? 45 33 E4 48 85 C0 74 10 48 8B CA E8 ? ? ? ? 84 C0");
		ApplyCharacterCustomization = decltype(ApplyCharacterCustomization)(ApplyCharacterCustomizationPattern);
		//ChangingGameSessionId = decltype(ChangingGameSessionId)(ChangingGameSessionIdPattern);
		InternalGetNetMode = decltype(InternalGetNetMode)(InternalGetNetModePattern);
		//ActorInternalGetNetMode = decltype(ActorInternalGetNetMode)(ActorInternalGetNetModePattern);


		ProcessEventO = decltype(ProcessEventO)(BaseAdress + Offsets::ProcessEvent);
		StaticFindObjectO = decltype(StaticFindObjectO)(StaticFindObjectPattern);
		SpawnActorO = decltype(SpawnActorO)(SpawnActorPattern);


		Globals::FortEngine = UObject::FindObjectSlow<UFortEngine>("FortEngine_");
		Globals::GPS = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
		Globals::KismetStringLibrary = (UKismetStringLibrary*)UKismetStringLibrary::StaticClass()->DefaultObject;


		auto GameViewport = Globals::FortEngine->GameViewport;
		auto ConsoleObject = Globals::GPS->SpawnObject(UConsole::StaticClass(), GameViewport);
		GameViewport->ViewportConsole = (UConsole*)ConsoleObject;
		Globals::World = GameViewport->World;



		HLog("All Patterns found!");

		return true;
	}
}
