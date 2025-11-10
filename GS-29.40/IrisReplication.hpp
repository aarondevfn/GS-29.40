#pragma once

enum class EReplicationSystemSendPass : unsigned
{
	Invalid,
	PostTickDispatch,
	TickFlush,
};

struct FSendUpdateParams
{
	// Type of SendPass we want to do @see EReplicationSystemSendPass
	EReplicationSystemSendPass SendPass = EReplicationSystemSendPass::TickFlush;

	// DeltaTime, only relevant for EReplicationSystemSendPass::TickFlush
	float DeltaSeconds = 0.f;
};

void (*UpdateReplicationViews)(UNetDriver* NetDriver);
void (*PreSendUpdate)(UObject* ReplicationSystem, const FSendUpdateParams& Params);

void SendClientMoveAdjustments(UNetDriver* NetDriver)
{
	for (int i = 0; i < NetDriver->ClientConnections.Num();i++)
	{
		UNetConnection* Connection = NetDriver->ClientConnections[i];

		if (Connection == nullptr || Connection->ViewTarget == nullptr)
		{
			continue;
		}

		if (APlayerController* PC = Connection->PlayerController)
		{
			bool (*SendClientAdjustment2)(APlayerController * Controller) = decltype(SendClientAdjustment2)(PC->VTable[0x1EB]);
			SendClientAdjustment2(PC);
		}

		for (int j = 0; j < Connection->Children.Num();j++)
		{
			UNetConnection* ChildConnection = Connection->Children[j];

			if (ChildConnection == nullptr)
			{
				continue;
			}

			if (APlayerController* PC = ChildConnection->PlayerController)
			{
				bool (*SendClientAdjustment2)(APlayerController * Controller) = decltype(SendClientAdjustment2)(PC->VTable[0x1EB]);
				SendClientAdjustment2(PC);
			}
		}
	}
}