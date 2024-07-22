//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file main.cpp
/// @brief  cli app
//=============================================================================

#include "Windows.h"
#include <conio.h>
#include <stdlib.h>
#include "stdio.h"
#include "string"

#include "main.h"

#define FLM_CONSOLE_WIDTH 1480

FLM_Context* g_flame = NULL;

void FlameSetConsoleCursorPos(SHORT XPos, SHORT YPos)
{
    COORD Coord = {XPos, YPos};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Coord);
}

void FlameGetConsoleCursorPosition(SHORT& XPos, SHORT& YPos)
{
    CONSOLE_SCREEN_BUFFER_INFO cbsi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cbsi))
    {
        XPos = cbsi.dwCursorPosition.X;
        YPos = cbsi.dwCursorPosition.Y;
    }
    else
    {
        XPos = 0;
        YPos = 0;
    }
}

void UserProcessMessages(FLM_PROCESS_MESSAGE_TYPE messagetype, const char* message)
{
    if (messagetype == FLM_PROCESS_MESSAGE_TYPE::PRINT)
        printf("%s", message);
    else if (messagetype == FLM_PROCESS_MESSAGE_TYPE::STATUS)
    {
        SHORT XPos, YPos;
        FlameGetConsoleCursorPosition(XPos, YPos);

        // print repeating status line
        printf("%s", message);

        // restore cursor pos
        FlameSetConsoleCursorPos(XPos, YPos);
    }
    else
    {
        printf("Error: %s\n", message);
    }
}

void printHelpOptions()
{
    printf("%s v%s command line options\n\n", APP_NAME_STRING, VERSION_TEXT);
    for (int i = 0; i < flm_help.size(); i++)
        printf("%s\n", flm_help[i].c_str());
}

char* vendorName(FLM_GPU_VENDOR_TYPE vendor)
{
    if (vendor == FLM_GPU_VENDOR_TYPE::AMD)
        return ("AMD");
    if (vendor == FLM_GPU_VENDOR_TYPE::NVIDIA)
        return ("NVIDIA");
    if (vendor == FLM_GPU_VENDOR_TYPE::INTEL)
        return ("INTEL");
    return ("Not supported");
}

char* captureCodec(FLM_CAPTURE_CODEC_TYPE codec)
{
    if (codec == FLM_CAPTURE_CODEC_TYPE::AMF)
        return ("AMF");
    else if (codec == FLM_CAPTURE_CODEC_TYPE::DXGI)
        return ("DXGI");
    return ("UNKNOWN");
}

bool ParseCommandLine(int argCount, char* args[], FLM_CLI_OPTIONS& cliOptions)
{
    // Options
    int i = 1;
    std::string cmd_arg;
    for (; i < argCount; ++i)
    {
        cmd_arg = args[i];
        std::transform(cmd_arg.begin(), cmd_arg.end(), cmd_arg.begin(), ::tolower); 
        if ((cmd_arg.compare("--help") == 0) || (cmd_arg.compare("-help") == 0) || (cmd_arg.compare("help") == 0) || (cmd_arg.compare("-?") == 0) ||
            (cmd_arg.compare("?") == 0))
        {
            cliOptions.showHelp = true;
            return true;
        }

        if (cmd_arg.compare("-amf") == 0)
            cliOptions.captureUsing = FLM_CAPTURE_CODEC_TYPE::AMF;
        else
        if (cmd_arg.compare("-dxgi") == 0)
            cliOptions.captureUsing = FLM_CAPTURE_CODEC_TYPE::DXGI;
        else
        if (cmd_arg.compare("-fg") == 0)
            cliOptions.enableFG = true;
        else
            break;
    }

    if (i < argCount)
    {
        printf("Unknown command: %s\n", args[i]);
        return false;
    }
    return true;
}

void SetAppWindowTopMost(HWND hWnd, bool topmost, bool extendConsole)
{
    RECT r;
    GetWindowRect(hWnd, &r);
    const LONG wndWidthOrig  = r.right - r.left;
    const LONG wndHeightOrig = r.bottom - r.top;

    LONG wndWidth  = wndWidthOrig;
    LONG wndHeight = wndHeightOrig; // make it less tall;

    if (extendConsole)
    {
        wndHeight = wndHeight * 3 / 4; // make it less tall;

        // Make it wider to fit longer lines
        if (wndWidth < FLM_CONSOLE_WIDTH)
        {
            wndWidth = wndWidth * 11 / 8;

            // still too small
            if (wndWidth < FLM_CONSOLE_WIDTH)
                wndWidth = FLM_CONSOLE_WIDTH;
        }

        // Animate
        bool bAnimate = true;
        if( bAnimate )
        {
            int steps = 10;
            for( int i = 0; i < steps; i++ )
            {
                LONG w = wndWidthOrig  * (steps - i) + wndWidth  * i;
                LONG h = wndHeightOrig * (steps - i) + wndHeight * i;
                Sleep(10);
                SetWindowPos(hWnd, topmost ? HWND_TOPMOST : HWND_TOP, 0, 0, w/steps, h/steps, SWP_NOREPOSITION | SWP_NOMOVE);
            }
        }
        else
            SetWindowPos(hWnd, topmost ? HWND_TOPMOST : HWND_TOP, 0, 0, wndWidth, wndHeight, SWP_NOREPOSITION | SWP_NOMOVE);
    }
    else
        SetWindowPos(hWnd, topmost ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOREPOSITION | SWP_NOMOVE | SWP_NOSIZE);
}

void ClearKeyboard()
{
    while (_kbhit())
    {
        int key = _getch();
    }
}

void FLM_Close()
{
    g_user_interface.CloseUI();

    if (g_flame != NULL)
    {
        // Close SDK
        g_flame->Close();

        // Remove SDK context
        delete g_flame;
        g_flame = NULL;

        // clear any queued keys on app exit
        ClearKeyboard();
    }
}

BOOL WINAPI HandlerRoutine(DWORD eventCode)
{
    switch (eventCode)
    {
    case CTRL_CLOSE_EVENT:
        FLM_Close();
        return FALSE;
        break;
    }

    return TRUE;
}

DWORD WINAPI SettingsDialogBoxThreadStub(LPVOID lpParameter)
{
    g_user_interface.ThreadMain();

    return 0;
}

int main(int argCount, char* args[])
{
    FLM_CLI_OPTIONS cliOptions;
    FLM_STATUS      result                     = FLM_STATUS::OK;
    HWND            hWnd                       = GetConsoleWindow();
    HINSTANCE       hINSTANCE                  = GetModuleHandle(NULL);
    HANDLE          hUserInterfaceThread       = NULL;

    // Workaround for Win11
    while( GetParent(hWnd) )
        hWnd = GetParent(hWnd);
    g_ui.hWndConsole = hWnd;

    hUserInterfaceThread = CreateThread(0, 0, SettingsDialogBoxThreadStub, NULL, 0, NULL);

    try
    {
        if (argCount > 1)
        {
            if (ParseCommandLine(argCount, args, cliOptions) == false)
                return -1;

            if (cliOptions.showHelp)
            {
                printHelpOptions();
                return 0;
            }
        }

        // Create pipeline context, returns null ptr on failure
        g_flame = CreateFLMContext();

        if (!g_flame)
        {
            printf("Unable to create Flame Pipeline\n");
            return (int)FLM_STATUS::MEMORY_ALLOCATION_ERROR;
        }

        SetConsoleCtrlHandler(HandlerRoutine, TRUE);

        // Set message callback to get info messages
        g_flame->SetUserProcessCallback(UserProcessMessages);

        printf("\n%s v%s\n", APP_NAME_STRING, VERSION_TEXT);

        if (cliOptions.captureUsing == FLM_CAPTURE_CODEC_TYPE::DXGI)
            printf("DXGI codec does not support Exclusive FullScreen mode.\n");

        // Init SDK and run main process loop on success
        result = g_flame->Init(cliOptions.captureUsing);
        if (result == FLM_STATUS::OK)
        {
            printf("\n"); 
            printf("---------------------------------------------\n"); 
            printf("Processing %s GPU with %s capture codec.\n", vendorName(g_flame->GetGPUVendorType()), captureCodec(g_flame->GetCodec()));
            printf("---------------------------------------------\n"); 

            // command line override
            if (cliOptions.enableFG)
                g_flame->m_runtimeOptions.gameUsesFrameGeneration = true;

            // init context menu options
            g_user_interface.Init(&g_flame->m_runtimeOptions, hUserInterfaceThread);

            // Print the selected running mode
            const char* run_mode_str[(int)FLM_PRINT_LEVEL::PRINT_LEVEL_COUNT] = {"RUN", "ACCUMULATED", "OPERATIONAL", "DEBUG" };
            int iMode = std::max<int>(0, std::min<int>(_countof(run_mode_str), (int)(g_flame->m_runtimeOptions.printLevel)));
            printf("\n");
            printf("===============================\n");
            printf("Running in %s mode.\n", run_mode_str[iMode]);
            printf("===============================\n");

            // show basic user settings
            // if (g_flame->m_runtimeOptions.minimizeApp)
            //     printf("For improved performance application will be minimized\nduring measurements on primary display.");
            printf("\nMouse right click in console window for user options\n");
            printf("press %s to start and stop measurements, %s to exit\n", g_flame->GetToggleKeyNames().c_str(), g_flame->GetAppExitKeyNames().c_str());

            // Resize the window if needed
            SetAppWindowTopMost(hWnd,
                                g_flame->m_runtimeOptions.appWindowTopMost,
                                g_flame->m_runtimeOptions.printLevel >= FLM_PRINT_LEVEL::OPERATIONAL);

            FLM_PROCESS_STATUS process_status      = FLM_PROCESS_STATUS::WAIT_FOR_START;
            //FLM_PROCESS_STATUS hold_process_status = FLM_PROCESS_STATUS::WAIT_FOR_START;
            while (process_status != FLM_PROCESS_STATUS::CLOSE)
            {
                //hold_process_status = process_status;
                process_status = g_flame->Process();

                // Look for process state changes to minimize CPU usage on UI running threads
                // if ((hold_process_status == FLM_PROCESS_STATUS::WAIT_FOR_START) && (process_status == FLM_PROCESS_STATUS::PROCESSING))
                // {
                // }
                // else if ((hold_process_status == FLM_PROCESS_STATUS::PROCESSING) && (process_status == FLM_PROCESS_STATUS::WAIT_FOR_START))
                // {
                // }
            }
        }
        else
        {
            std::string err = g_flame->GetErrorStr();
            while (err.size() > 0)
            {
                printf("Error [%x]: %s\n", (int)result,err.c_str());
                err = g_flame->GetErrorStr();
            }

            printf("press any key to continue\n");
            char ch = _getch();

        }
    }
    catch (const std::exception& e)
    {
        printf("Error: Running application failed [%s]", e.what());
    }

    // flame could be closed by HandlerRoutine so check exit
    FLM_Close();

    return (int)result;
}
