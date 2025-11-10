#pragma once

inline void (*CallPreReplication)(AActor* Actor, UNetDriver* NetDriver);
inline int64_t(*ReplicateActor)(UActorChannel* ActorChannel);
inline void (*SetChannelActor)(UActorChannel* ActorChannel, UObject* InActor, int32 flag);
inline UChannel* (*CreateChannelByName)(UNetConnection* NetConnection, const FName& ChName, int32 CreateFlags, int32 ChIndex);
inline bool (*SendClientAdjustment)(APlayerController* Controller);
inline bool (*IsLevelInitializedForActor)(UNetDriver* NetDriver, AActor* InActor, UNetConnection* InConnection);

FORCEINLINE int32_t& GetReplicationFrame(UObject* Driver)
{
	return *(int32_t*)(int64_t(Driver) + 0x4C8); //1.8 : 648  // 4.1 : 816
}


int ServerReplicateActors_PrepConnections(UNetDriver* NetDriver)
{
    int ReadyConnections = 0;

    for (int ConnIdx = 0; ConnIdx < NetDriver->ClientConnections.Num(); ConnIdx++)
    {
        UNetConnection* Connection = NetDriver->ClientConnections[ConnIdx];

        if (!Connection)
            continue;

        AActor* OwningActor = Connection->OwningActor;

        if (OwningActor)
        {
            ReadyConnections++;
            AActor* DesiredViewTarget = OwningActor;

            if (Connection->PlayerController)
            {
                if (AActor* ViewTarget = Connection->PlayerController->GetViewTarget())
                {
                    DesiredViewTarget = ViewTarget;
                }
            }

            Connection->ViewTarget = DesiredViewTarget;

            for (int ChildIdx = 0; ChildIdx < Connection->Children.Num(); ++ChildIdx)
            {
                UNetConnection* ChildConnection = Connection->Children[ChildIdx];
                if (ChildConnection && ChildConnection->PlayerController && ChildConnection->ViewTarget)
                {
                    ChildConnection->ViewTarget = DesiredViewTarget;
                }
            }
        }
        else
        {
            Connection->ViewTarget = nullptr;

            for (int ChildIdx = 0; ChildIdx < Connection->Children.Num(); ++ChildIdx)
            {
                UNetConnection* ChildConnection = Connection->Children[ChildIdx];
                if (ChildConnection && ChildConnection->PlayerController && ChildConnection->ViewTarget)
                {
                    ChildConnection->ViewTarget = nullptr;
                }
            }
        }
    }

    return ReadyConnections;
}

UActorChannel* FindChannel(AActor* Actor, UNetConnection* Connection)
{
    for (int i = 0; i < Connection->OpenChannels.Num(); i++)
    {
        auto Channel = Connection->OpenChannels[i];
        if (Channel && Channel->Class)
        {
            if (Channel->Class == UActorChannel::StaticClass())
            {
                if (((UActorChannel*)Channel)->Actor == Actor)
                {
                    return ((UActorChannel*)Channel);
                }

            }
        }
    }

    return NULL;
}

void ServerReplicateActors_BuildConsiderList(UNetDriver* NetDriver,std::vector<AActor*>& OutConsiderList) {

    TArray<AActor*> Actors;
    Globals::GPS->GetAllActorsOfClass(Globals::World, AActor::StaticClass(),&Actors);



    for (int i = 0; i < Actors.Num(); ++i) {
        auto Actor = Actors[i];

        if (Actor->bActorIsBeingDestroyed)
        {
            continue;
        }

        if (Actor->RemoteRole == ENetRole::ROLE_None) {
            continue;
        }
        if (Actor->NetDormancy == ENetDormancy::DORM_Initial && Actor->BitPad_10)
        {
            continue;
        }

        if (Actor->NetDriverName != NetDriver->NetDriverName || !Actor->bActorInitialized)
            continue;

        OutConsiderList.push_back(Actor);
        CallPreReplication(Actor, NetDriver);
    }
    Actors.Free();
}

inline int32_t ServerReplicateActors(UNetDriver* NetDriver, float DeltaSeconds) {


	++GetReplicationFrame(NetDriver);

    auto NumClientsToTick = ServerReplicateActors_PrepConnections(NetDriver);

    if (NumClientsToTick == 0)
        return -1;

    UNetConnection* Connection = nullptr;

    std::vector<AActor*> OutConsiderList;
    ServerReplicateActors_BuildConsiderList(NetDriver, OutConsiderList);

    for (int i = 0; i < NetDriver->ClientConnections.Num(); i++) {

        auto Connection = NetDriver->ClientConnections[i];

        if (!Connection)
            continue;

        if (i >= NumClientsToTick)
            break;

        else if (Connection->ViewTarget){

            bool (*SendClientAdjustment2)(APlayerController * Controller) = decltype(SendClientAdjustment2)(Connection->PlayerController->VTable[0x1EB]);

            if (Connection->PlayerController)
                SendClientAdjustment2(Connection->PlayerController);

            for (auto Actor : OutConsiderList) {
                if (!Actor)
                    continue;

                if (Actor->IsA(APlayerController::StaticClass()) && Actor != Connection->PlayerController)
                {
                    continue;
                }

                auto Channel = FindChannel(Actor, Connection);

                if (!Channel)
                {
                    FName ActorName = Globals::KismetStringLibrary->Conv_StringToName(L"Actor");

                    Channel = (UActorChannel*)(CreateChannelByName(Connection, ActorName, 2, -1));
                    SetChannelActor(Channel, Actor,0);
                }

                if (Channel)
                    ReplicateActor(Channel);
            }
        }
    }
}