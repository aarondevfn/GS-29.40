#pragma once

namespace GameMode
{
	EFortTeam (*PickTeamOG)(AFortGameModeAthena* GameModeAthena, EFortTeam PreferredTeam, AFortPlayerControllerAthena* PlayerControllerAthena);
	EFortTeam PickTeamHook(AFortGameModeAthena* GameModeAthena, EFortTeam PreferredTeam, AFortPlayerControllerAthena* PlayerControllerAthena)
	{
		AFortGameStateAthena* GameStateAthena = Cast<AFortGameStateAthena>(GameModeAthena->GameState);
		if (!GameStateAthena) return PickTeamOG(GameModeAthena, PreferredTeam, PlayerControllerAthena);

		UFortPlaylistAthena* PlaylistAthena = GameStateAthena->CurrentPlaylistInfo.BasePlaylist;
		if (!PlaylistAthena) return PickTeamOG(GameModeAthena, PreferredTeam, PlayerControllerAthena);

		ULocalPlayer* FirstLocalPlayer = UWorld::GetWorld()->OwningGameInstance->LocalPlayers[0];

		if (FirstLocalPlayer)
		{
			if (FirstLocalPlayer->PlayerController == PlayerControllerAthena)
				return EFortTeam::MAX;
		}

		int32 MaxTeamCount = PlaylistAthena->MaxTeamCount;
		int32 MaxTeamSize = PlaylistAthena->MaxTeamSize;

		EFortTeam ChooseTeam = EFortTeam::MAX;

		if (PlaylistAthena->bIsLargeTeamGame || PlaylistAthena->bAllowTeamSwitching)
		{
			int32 MinimumPlayers = INT32_MAX;

			for (int32 i = 0; i < GameStateAthena->Teams.Num(); i++)
			{
				AFortTeamInfo* TeamInfo = GameStateAthena->Teams[i];
				if (!TeamInfo) continue;

				if (i >= MaxTeamCount) break;

				int32 TeamMembersSize = TeamInfo->TeamMembers.Num();

				if (TeamMembersSize < MinimumPlayers && TeamMembersSize < MaxTeamSize)
				{
					MinimumPlayers = TeamMembersSize;
					ChooseTeam = EFortTeam(TeamInfo->Team);
				}
			}
		}
		else
		{
			for (int32 i = 0; i < GameStateAthena->Teams.Num(); i++)
			{
				AFortTeamInfo* TeamInfo = GameStateAthena->Teams[i];
				if (!TeamInfo) continue;

				if (i > MaxTeamCount)
					break;

				int32 TeamMembersSize = TeamInfo->TeamMembers.Num();

				if (TeamMembersSize >= MaxTeamSize)
					continue;

				ChooseTeam = EFortTeam(TeamInfo->Team);
				break;
			}
		}

		return ChooseTeam;
	}

	bool InitGameMode()
	{
		auto BaseAdress = (uintptr_t)GetModuleHandle(0);

		PickTeamOG = decltype(PickTeamOG)(BaseAdress + 0x8e91a64);

		START_DETOUR;
		DetourAttach(PickTeamOG, PickTeamHook);
		END_DETOUR;

		return true;
	}
}