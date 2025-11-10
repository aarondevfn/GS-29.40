#include <Windows.h>
#include <iostream>
#include <thread>
#include <detours.h>
#include <string>

#include "SDK.hpp"
#include "Util.h"
#include "Globals.h"
#include <fstream>
#include "GameMode.hpp"
#include "Replication.hpp"
#include "IrisReplication.hpp"
#include "Inventory.hpp"
#include "InitFunc.h"
#include "Ability.hpp"
#include "BattleRoyaleGamePhaseLogic.hpp"
#include "FortKismetLibrary.hpp"
#include "DecoTool.hpp"
#include "Hooks.h"


DWORD WINAPI MainThread(LPVOID)
{
    Util::InitConsole();
    if (Inits::InitializeAll())
    {
        HLog("Init Success !");
        Hooks::InitDetour();
        FortKismetLibrary::InitDetour();
        BattleRoyaleGamePhaseLogic::InitDetour();
        Inventory::InitInventory();
        GameMode::InitGameMode();
        DecoTool::InitDecoTool();
    }
    else
    {
        HLog("Init Failed !")
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID res)
{
    if (reason == 1)
        CreateThread(0, 0, MainThread, mod, 0, 0);

    return TRUE;
}