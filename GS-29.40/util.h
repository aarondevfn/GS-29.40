#pragma once

#include <consoleapi.h>
#include <cstdint>
#include <cstdio>
#include <libloaderapi.h>
#include <vector>
#include <winnt.h>

#define HLog(string) std::cout << "LOG : " << string << std::endl;


#define START_DETOUR         \
    DetourTransactionBegin(); \
    DetourUpdateThread(GetCurrentThread());

#define DetourAttach(Target, Detour) DetourAttach(&(void*&)Target, Detour);

#define DetourDetach(Target, Detour) DetourDetach(reinterpret_cast<void**>(&Target), Detour);

#define END_DETOUR DetourTransactionCommit();


#define RESULT_PARAM Z_Param__Result
#define RESULT_DECL void*const RESULT_PARAM

struct FFrame;

typedef void (*FNativeFuncPtr)(UObject* Context, FFrame* TheStack, RESULT_DECL);

#pragma pack(push, 0x1)
class alignas(0x08) FOutputDevice
{
public:
    char Pad[0x10];
};
#pragma pack(pop)

struct FFrame : public FOutputDevice
{
public:
    UFunction* Node;
    UObject* Object;
    uint8* Code;
    uint8* Locals;

    FProperty* MostRecentProperty;
    uint8* MostRecentPropertyAddress; 

    uint8 Pad_01[0x58];

public:
    void Step(UObject* Context, RESULT_DECL)
    {
        void (*Step)(FFrame * Frame, UObject * Context, RESULT_DECL) = decltype(Step)(0x164d454 + uintptr_t(GetModuleHandle(0)));
        Step(this, Context, RESULT_PARAM);
    }

    void StepExplicitProperty(void* const Result, void* Property)
    {
        void (*StepExplicitProperty)(FFrame * Frame, void* const Result, void* Property) = decltype(StepExplicitProperty)(0x144932c + uintptr_t(GetModuleHandle(0)));
        StepExplicitProperty(this, Result, Property);
    }

    void StepCompiledIn(void* const Result)
    {
        if (Code)
        {
            Step(Object, Result);
        }
        else
        {
            FProperty* Property = (FProperty*)(*(FField**)(__int64(this) + 0x88));
            *(FField**)(__int64(this) + 0x88) = Property->Next;

            StepExplicitProperty(Result, Property);
        }
    }

    template<typename TNativeType>
    TNativeType& StepCompiledInRef(void* const TemporaryBuffer)
    {
        MostRecentPropertyAddress = NULL;

        if (Code)
        {
            Step(Object, TemporaryBuffer);
        }
        else
        {
            UProperty* Property = (UProperty*)(*(UField**)(__int64(this) + 0x88));
            *(UField**)(__int64(this) + 0x88) = Property->Next;

            StepExplicitProperty(TemporaryBuffer, Property);
        }

        return (MostRecentPropertyAddress != NULL) ? *(TNativeType*)(MostRecentPropertyAddress) : *(TNativeType*)TemporaryBuffer;
    }
};

class Util
{
public:
    static __forceinline VOID InitConsole()
    {
        FILE* fDummy;
        AllocConsole();
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
    }

    static __forceinline uintptr_t BaseAddress()
    {
        return reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    }

    static __forceinline uintptr_t FindPattern(const char* signature, bool bRelative = false, uint32_t offset = 0)
    {
        uintptr_t base_address = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
        static auto patternToByte = [](const char* pattern)
        {
            auto bytes = std::vector<int> {};
            const auto start = const_cast<char*>(pattern);
            const auto end = const_cast<char*>(pattern) + strlen(pattern);

            for (auto current = start; current < end; ++current)
            {
                if (*current == '?')
                {
                    ++current;
                    if (*current == '?')
                        ++current;
                    bytes.push_back(-1);
                }
                else
                {
                    bytes.push_back(strtoul(current, &current, 16));
                }
            }
            return bytes;
        };

        const auto dosHeader = (PIMAGE_DOS_HEADER)base_address;
        const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)base_address + dosHeader->e_lfanew);

        const auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        auto patternBytes = patternToByte(signature);
        const auto scanBytes = reinterpret_cast<std::uint8_t*>(base_address);

        const auto s = patternBytes.size();
        const auto d = patternBytes.data();

        for (auto i = 0ul; i < sizeOfImage - s; ++i)
        {
            bool found = true;
            for (auto j = 0ul; j < s; ++j)
            {
                if (scanBytes[i + j] != d[j] && d[j] != -1)
                {
                    found = false;
                    break;
                }
            }
            if (found)
            {
                uintptr_t address = reinterpret_cast<uintptr_t>(&scanBytes[i]);
                if (bRelative)
                {
                    address = ((address + offset + 4) + *(int32_t*)(address + offset));
                    return address;
                }
                return address;
            }
        }
        return NULL;
    }
};