#include "f4se/PluginAPI.h"
#include "f4se/GameAPI.h"
#include <shlobj.h>

#include "f4se_common/f4se_version.h"
#include "f4se_common/SafeWrite.h"

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformCallbacks.h"

#include "Config.h"
#include "rva/RVA.h"

bool ReadMemory(uintptr_t addr, void* data, size_t len);

IDebugLog               gLog;
PluginHandle            g_pluginHandle = kPluginHandle_Invalid;
F4SEScaleformInterface* g_scaleform = NULL;

extern std::string commandString;
extern std::string commandString_safetyCheck;

//-----------------------
// Addresses [2]
//-----------------------

typedef void (*_ExecuteCommand)(const char* str);
RVA <_ExecuteCommand> ExecuteCommand({
    { RUNTIME_VERSION_1_10_75, 0x0125B2E0 },
    { RUNTIME_VERSION_1_10_64, 0x0125B320 },
    { RUNTIME_VERSION_1_10_40, 0x0125AE40 },
    { RUNTIME_VERSION_1_10_26, 0x012594F0 },
}, "40 53 55 56 57 41 54 48 81 EC ? ? ? ? 8B 15 ? ? ? ?");

RVA <uintptr_t> CPrint_Call({
    { RUNTIME_VERSION_1_10_75, 0x0125B356 },
    { RUNTIME_VERSION_1_10_64, 0x0125B396 },
    { RUNTIME_VERSION_1_10_40, 0x0125AEB6 },
    { RUNTIME_VERSION_1_10_26, 0x01259566 },
}, "E8 ? ? ? ? 48 8D 94 24 ? ? ? ? 41 B8 ? ? ? ? 48 8B CD");

//-----------------------
// Scaleform Functions
//-----------------------

class F4SEScaleform_OnVertibirdJump : public GFxFunctionHandler
{
public:
    virtual void F4SEScaleform_OnVertibirdJump::Invoke(Args* args)
    {
        // shh... keep this quiet.
        SInt32 rel32 = 0;
        ReadMemory(CPrint_Call.GetUIntPtr() + 1, &rel32, sizeof(SInt32));
        SafeWrite8(CPrint_Call.GetUIntPtr() + 5 + rel32, 0xC3);	// RETN
        ExecuteCommand(commandString.c_str());
        SafeWrite8(CPrint_Call.GetUIntPtr() + 5 + rel32, 0x48);	// ORG
    }
};

bool RegisterScaleform(GFxMovieView * view, GFxValue * f4se_root)
{
    GFxMovieRoot *movieRoot = view->movieRoot;

    GFxValue currentSWFPath;
    std::string currentSWFPathString = "";
    if (movieRoot->GetVariable(&currentSWFPath, "root.loaderInfo.url")) {
        currentSWFPathString = currentSWFPath.GetString();
    } else {
        _MESSAGE("WARNING: Scaleform registration failed.");
    }

    if (currentSWFPathString.find("VertibirdMenu.swf") != std::string::npos) {
        // Create BGSCodeObj.Jump();
        GFxValue codeObj;
        movieRoot->GetVariable(&codeObj, "root.BGSCodeObj");

        // Enable Jump button
        movieRoot->SetVariable("root.buttonHint_Jump.ButtonEnabled", &GFxValue(true));
        
        if (!codeObj.IsUndefined()) {
            RegisterFunction<F4SEScaleform_OnVertibirdJump>(&codeObj, movieRoot, "Jump");
        }
    }

    return true;
}

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
{
    gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\vertibird_jump.log");

    _MESSAGE("%s v%s", PLUGIN_NAME_SHORT, PLUGIN_VERSION_STRING);
    _MESSAGE("%s query", PLUGIN_NAME_SHORT);

    // populate info structure
    info->infoVersion = PluginInfo::kInfoVersion;
    info->name    = PLUGIN_NAME_SHORT;
    info->version = PLUGIN_VERSION;

    // store plugin handle so we can identify ourselves later
    g_pluginHandle = f4se->GetPluginHandle();

    // Check game version
    if (!COMPATIBLE(f4se->runtimeVersion)) {
        char str[512];
        sprintf_s(str, sizeof(str), "Your game version: v%d.%d.%d.%d\nExpected version: v%d.%d.%d.%d\n%s will be disabled.",
            GET_EXE_VERSION_MAJOR(f4se->runtimeVersion),
            GET_EXE_VERSION_MINOR(f4se->runtimeVersion),
            GET_EXE_VERSION_BUILD(f4se->runtimeVersion),
            GET_EXE_VERSION_SUB(f4se->runtimeVersion),
            GET_EXE_VERSION_MAJOR(SUPPORTED_RUNTIME_VERSION),
            GET_EXE_VERSION_MINOR(SUPPORTED_RUNTIME_VERSION),
            GET_EXE_VERSION_BUILD(SUPPORTED_RUNTIME_VERSION),
            GET_EXE_VERSION_SUB(SUPPORTED_RUNTIME_VERSION),
            PLUGIN_NAME_LONG
        );

        MessageBox(NULL, str, PLUGIN_NAME_LONG, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    if (f4se->runtimeVersion > SUPPORTED_RUNTIME_VERSION) {
        _MESSAGE("INFO: Newer game version (%08X) than target (%08X).", f4se->runtimeVersion, SUPPORTED_RUNTIME_VERSION);
    }

    // get the scaleform interface and query its version
    g_scaleform = (F4SEScaleformInterface *)f4se->QueryInterface(kInterface_Scaleform);
    if(!g_scaleform)
    {
        _MESSAGE("couldn't get scaleform interface");
        return false;
    }

    return true;
}

bool F4SEPlugin_Load(const F4SEInterface *f4se)
{
    _MESSAGE("vertibird_jump load");

    RVAManager::UpdateAddresses(f4se->runtimeVersion);

    // register scaleform callbacks
    g_scaleform->Register("vertibird_jump", RegisterScaleform);

    // Read plugin settings from INI
    int bSafetyCheck = GetPrivateProfileInt("VertibirdJump", "bSafetyCheck", 0, "./Data/F4SE/Plugins/vertibird_jump.ini");
    if (bSafetyCheck == 1) {
        _MESSAGE("bSafetyCheck: %d", bSafetyCheck);
        commandString = commandString_safetyCheck;
    }

    return true;
}

};

//-----------------
// Utilities
//-----------------

bool ReadMemory(uintptr_t addr, void* data, size_t len) {
    UInt32 oldProtect;
    if (VirtualProtect((void *)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(data, (void*)addr, len);
        if (VirtualProtect((void *)addr, len, oldProtect, &oldProtect))
            return true;
    }
    return false;
}

//-----------------
// Constants
//-----------------

std::string commandString = "player.pa ActionInteractionExit; hidemenu VertibirdMenu; setstage vft 100";
std::string commandString_safetyCheck = "if \"14\".WornHasKeyword dn_HasMisc_JetPack || \"14\".HasPerk 1F8A9 || \"14\".getav ba43d >= 100; player.pa ActionInteractionExit; hidemenu VertibirdMenu; setstage vft 100; else; cgf \"Debug.Notification\" \"You must be in Power Armor to jump from a Vertibird.\"; endif";