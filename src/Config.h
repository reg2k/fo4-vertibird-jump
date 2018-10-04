#pragma once
#include "f4se_common/f4se_version.h"

//-----------------------
// Plugin Information
//-----------------------
#define PLUGIN_VERSION              15
#define PLUGIN_VERSION_STRING       "1.9.6"
#define PLUGIN_NAME_SHORT           "vertibird_jump"
#define PLUGIN_NAME_LONG            "Vertibird Jump"
#define SUPPORTED_RUNTIME_VERSION   CURRENT_RELEASE_RUNTIME
#define MINIMUM_RUNTIME_VERSION     RUNTIME_VERSION_1_10_26
#define COMPATIBLE(runtimeVersion)  (runtimeVersion == SUPPORTED_RUNTIME_VERSION)