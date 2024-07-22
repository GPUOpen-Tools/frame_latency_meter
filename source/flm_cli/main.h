//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file main.h
/// @brief  cli main header
//=============================================================================

#ifndef MAIN_H
#define MAIN_H

#include "flm.h"
#include "flm_help.h"
#include "flm_user_interface.h"


typedef struct FLM_CLI_OPTIONS_TYPE
{
    bool                   showHelp        = false;
    bool                   enableFG        = false;
    FLM_GPU_VENDOR_TYPE    vendor          = FLM_GPU_VENDOR_TYPE::UNKNOWN;
    FLM_CAPTURE_CODEC_TYPE captureUsing    = (FLM_CAPTURE_CODEC_TYPE)(-1);
} FLM_CLI_OPTIONS;

#endif
