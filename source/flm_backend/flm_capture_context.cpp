//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file flm_capture_context.cpp
/// @brief  capture backend context
//=============================================================================

#include "flm_capture_context.h"
#include "flm_utils.h"

#ifdef _WIN32
#include "wingdi.h"
#endif

#include <fstream>
#include "shlobj.h"

#define CONTEXT_DEBUG_PRINT_STACK()  // printf(__FUNCTION__ "\n");

FLM_STATUS FLM_Capture_Context::LoadUserSettings()
{
    CSimpleIniA ini;
    SI_Error    rc = ini.LoadFile("flm.ini");
    if (rc < 0)
    {
        FlmPrintError("flm.ini not found");
        return FLM_STATUS::INIT_FAILED;
    }

    try
    {
        const char* section           = "CAPTURE";
        m_iOutputAdapter              = m_iUserSetOutputAdapter;

        m_setting.fStartX             = std::clamp((float)ini.GetDoubleValue(section, "StartX", m_setting.fStartX), 0.0f, 1.0f);
        m_setting.fStartY             = std::clamp((float)ini.GetDoubleValue(section, "StartY", m_setting.fStartY), 0.0f, 1.0f);

        // Must be non-zero for init, set min to 1% of buffer size
        m_setting.fCaptureWidth       = std::clamp((float)ini.GetDoubleValue(section, "CaptureWidth", m_setting.fCaptureWidth),   0.01f, 1.0f);
        m_setting.fCaptureHeight      = std::clamp((float)ini.GetDoubleValue(section, "CaptureHeight", m_setting.fCaptureHeight), 0.01f, 1.0f);

        m_setting.captureFileName     = ini.GetValue(section, "CaptureFile", m_setting.captureFileName.c_str());
        m_setting.iAVGFilterFrames    = std::clamp((int)ini.GetLongValue(section, "AVGFilterFrames", m_setting.iAVGFilterFrames), 1, 99999);
        m_setting.iFilmGrainThreshold = std::clamp((int)ini.GetLongValue(section, "FilmGrainThreshold", m_setting.iFilmGrainThreshold), 0,255);

        m_fAVGFilterAlpha = CalculateFilterAlpha(m_setting.iAVGFilterFrames);
    }
    catch (...)
    {
        FlmPrintError("Error reading flm.ini file");
        return FLM_STATUS::FAILED;
    }

    return FLM_STATUS::OK;
}

FLM_STATUS FLM_Capture_Context::SaveUserSettings()
{
    return FLM_STATUS::OK;
}

void FLM_Capture_Context::InitSettings()
{
    LoadUserSettings();
}

void FLM_Capture_Context::TextDC(int x, int y, const char* Format, ...)
{
    if (m_iOutputAdapter != 0)
        return;

    // define a pointer to save argument list
    va_list args;
    char    buff[1024];
    int     sizeBuff = 0;

    // process the arguments into our debug buffer
    va_start(args, Format);
#ifdef _WIN32
    sizeBuff = vsprintf_s(buff, Format, args);
#else
    sizeBuff = vsprint(buff, 1024, Format, args);
#endif
    va_end(args);

    m_screenHDC = ::GetDC(0);
    ::TextOut(m_screenHDC, x, y, buff, sizeBuff);
    ::ReleaseDC(0, m_screenHDC);
}

void FLM_Capture_Context::ShowCaptureRegion(COLORREF color)
{
    if (m_iOutputAdapter == 0)
    {
        // Get the windows device context
        m_screenHDC = ::GetDC(0);  // 0 is default is primary window
    }
    else
    {
        if (m_displayName.size() == 0)
        {
            m_displayName = "\\\\.\\DISPLAY";
            m_displayName.append(std::to_string(m_iOutputAdapter));
        }
        m_screenHDC = ::CreateDC(m_displayName.c_str(), NULL, NULL, NULL);
    }

    // Draw 1 pixel outside of capture region so that the box is not included in any capture data.
    RECT rect = {m_iCaptureOriginX, m_iCaptureOriginY, m_iCaptureOriginX + m_iCaptureWidth, m_iCaptureOriginY + m_iCaptureHeight};

    HBRUSH brush = CreateSolidBrush(color);
    ::FillRect(m_screenHDC, &rect, brush);

    if (m_iOutputAdapter == 0)
        ::ReleaseDC(0, m_screenHDC);
    else
        ::DeleteDC(m_screenHDC);
}

void FLM_Capture_Context::RedrawMainScreen()
{
    ::InvalidateRect(NULL, NULL, false);
}

void FLM_Capture_Context::ClearCaptureRegion()
{
    ::InvalidateRect(NULL, NULL, false);
}

float FLM_Capture_Context::CalculateFilterAlpha(int iNumIterations)
{
    const float fFraction = 0.01f;  // For simplicity sake assuming we always want to get to 1% of the steady-state value

    // Calculates the parameter "a" in the iterative equation:
    // Val_av = Val_av * a + (1-a) * Val
    //
    // such that Val_av will reach it's final position within a fraction fFraction,
    // after iNumIteration iterations.
    //
    // Or in other words, what "a" do you need, such that the result of the
    // iterative equation { Val = 1.0; for(iNumIterations) {Val = Val * a;} }
    // will become "fFraction" after "iNumIterations" iterations.

    float fAlpha = expf(logf(fFraction) / iNumIterations);
    // float fAlpha = pow( log10f(fFraction) / iNumIterations ), 10 ); // also works
    // float fAlpha = pow(             -2.0f / iNumIterations ), 10 ); // ...when fFraction == 0.01f

    // Example values for fFraction = 1% ( 0.01f )
    // iNumIterations | fAlpha
    // ----------------------------------
    //             44 | 0.9
    //             90 | 0.95
    //            100 | 0.955
    //            200 | 0.9772
    //            400 | 0.98855
    //            800 | 0.988
    //            999 | 0.9954

    return fAlpha;
}

int FLM_Capture_Context::GetThresholdedSAD(int64_t frameIdx, int iSAD, float fThresholdMultiplierCoeff)
{
    // 1. Calculate the thresholded SAD
    // First - calculate the thresh hold value
    int iThreshold      = (int)(m_fBackgroundSAD * fThresholdMultiplierCoeff);
    int iThresholdedSAD = std::max<int>(0, iSAD - iThreshold);

    // 2. Printout SAD values for each frame - very useful as a sanity check
    if( frameIdx != 0 ) // It will be non-zero only for FLM_PRINT_LEVEL::PRINT_DEBUG
        if( KEY_DOWN(VK_LMENU) )
            FlmPrint( frameIdx % 32 == 0 ? "%i \n" : "%i ", iSAD);
    
    // 3. Estimate the "background SAD" - these will be unrelated to mouse click/movement, and are usually due
    //    to in-game animations and/or film grain noise effect happening in the monitored region.
    {
        static int iPrevSAD         = 0;
        static int iPrevPrevSAD     = 0;
        static int iPrevPrevPrevSAD = 0;

        // A workaround for situations where the SAD for even frames is significantly different from SAD for odd frames.
        // For example, a pathological framegen case where every frame is duplicated, therefore every other SAD is zero.
        iSAD =  iSAD + iPrevSAD / 4; // Note: the way this works is not straightforward to understand...................

        // iSAD needs to be at least 1 to avoid quantization problems
        iSAD = std::max<int>(1, iSAD);

        // Second - update statistics. Filter out the large SADs caused by the mouse move
        if ((iSAD <= iPrevSAD         * fThresholdMultiplierCoeff) &&
            (iSAD <= iPrevPrevSAD     * fThresholdMultiplierCoeff) &&
            (iSAD <= iPrevPrevPrevSAD * fThresholdMultiplierCoeff))
        {
            m_fBackgroundSAD = m_fBackgroundSAD * m_fAVGFilterAlpha + (1 - m_fAVGFilterAlpha) * iSAD;
        }

        // Advance history
        iPrevPrevPrevSAD = iPrevPrevSAD;
        iPrevPrevSAD     = iPrevSAD;
        iPrevSAD         = iSAD;
    }


    // Return the thresh hold result
    return iThresholdedSAD;
}

bool FLM_Capture_Context::InitCapture(FLM_Timer_AMF& m_timer)
{
    InitSettings();

    if (m_hEventFrameReady == 0)
        m_hEventFrameReady = CreateEvent(NULL, TRUE, FALSE, NULL);

    FLM_STATUS res = InitCaptureDevice(m_iUserSetOutputAdapter, &m_timer);

    return (res == FLM_STATUS::OK);
}

void FLM_Capture_Context::ResetState()
{
    m_fCumulativeFrameTimesMS        = 0.0f;
    m_iCumulativeFrameTimeSamples    = 0;
    m_fMovingAverageFrameTimeMS      = 0;
    m_fMovingAverageOddFramesTimeMS  = 0;
    m_fMovingAverageEvenFramesTimeMS = 0;
    m_bFrameLocked                   = false;
}

bool FLM_Capture_Context::AcquireFrameAndDownscaleToHost(int64_t* pTimeStamp, int64_t* pFrameIdx)
{
    //FLM_Profile_Timer profile_timer(__FUNCTION__);

    if (m_bExitCaptureThread || m_bDoCaptureFrames == false)
        return false;

#ifdef CAPTURE_FRAMES_ON_SEPARATE_THREAD
    DWORD state = WaitForSingleObject(m_hEventFrameReady, 1000);  // Don't wait for more than 1 second
    if (state == WAIT_OBJECT_0)
        ResetEvent(m_hEventFrameReady);
    else
    {
        // Frame acquire timed out.: reset
        ResetState();
        ResetEvent(m_hEventFrameReady);
        return false;
    }
#else
    GetFrame();
#endif

    CONTEXT_DEBUG_PRINT_STACK()

    if (m_bNeedToRebuildPipeline)  // This is updated in GetFrame();
        return false;

    return GetConverterOutput(pTimeStamp, pFrameIdx);
}

void FLM_Capture_Context::SaveAsBitmap(const char* filename, FLM_PIXEL_DATA pixelData, bool vertFlip)
{
    if (pixelData.pixelSizeInBytes == 0)
        return;

    BITMAPFILEHEADER bmfHeader = {0};
    BITMAPINFOHEADER bi        = {0};

    bi.biSize        = sizeof(BITMAPINFOHEADER);
    bi.biWidth       = pixelData.width;                 // RGBA channels
    bi.biHeight      = pixelData.height;                // Make the size negative if the image is upside down.
    bi.biPlanes      = 1;                               // There is only one plane in RGB color space where as 3 planes in YUV.
    bi.biBitCount    = pixelData.pixelSizeInBytes * 8;  // RGB depth for each of R, G, B and alpha.
    bi.biCompression = BI_RGB;                          // We are not compressing the image.
    bi.biSizeImage   = 0;                               // Set to zero for BI_RGB bitmaps.

    DWORD dwSizeofImage = pixelData.pitchH * pixelData.height;  // rowPitch = the size of the row in bytes.

    // Add the size of the headers to the size of the bitmap to get the total file size
    DWORD dwSizeofDIB = dwSizeofImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    //Offset to where the actual bitmap bits start.
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize    = dwSizeofDIB;  //Size of the file
    bmfHeader.bfType    = 0x4D42;       //Type must always be BM for Bitmaps

    FILE* file = NULL;
    fopen_s(&file, filename, "wb");
    if (file != NULL)
    {
        fwrite(&bmfHeader, sizeof(BITMAPFILEHEADER), 1, file);
        fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, file);

        unsigned char padding[3]   = {0, 0, 0};
        int           widthInBytes = pixelData.width * pixelData.pixelSizeInBytes;
        int           paddingSize  = (pixelData.pixelSizeInBytes - (widthInBytes) % pixelData.pixelSizeInBytes) % pixelData.pixelSizeInBytes;

        for (int i = 0; i < pixelData.height; i++)
        {
            fwrite(vertFlip ? pixelData.data + pixelData.pitchH * (pixelData.height - i - 1) : pixelData.data + i * pixelData.pitchH,
                   pixelData.pixelSizeInBytes,
                   pixelData.width,
                   file);
            fwrite(padding, 1, paddingSize, file);
        }

        fclose(file);
    }
}

void FLM_Capture_Context::DisplayThreadFunction()
{
    m_bExitCaptureThread = false;
    for (; m_bTerminateCaptureThread == false;)
    {
        // If the pipeline was just rebuilt - throw out 1 remnant frame from the previous pipeline
        {
            static bool prev_bNeedToRebuildPipeline = true;

            if (m_bDoCaptureFrames == true)
                if ((m_bNeedToRebuildPipeline == false) && (prev_bNeedToRebuildPipeline == true))
                    GetFrame(); // Throw out 1 remnant frame from the previous pipeline

            prev_bNeedToRebuildPipeline = m_bNeedToRebuildPipeline;
        }

        // Capture a new frame
        if ((m_bDoCaptureFrames == true) && (m_bNeedToRebuildPipeline == false))
        {
            if (GetFrame() == FLM_STATUS::CAPTURE_PROCESS_FRAME)
                SetEvent(m_hEventFrameReady);
        }
        else
            Sleep(1);
    }
    m_bExitCaptureThread = true;
}

void FLM_Capture_Context::UpdateAverageFrameTime(int64_t iiTimeStamp, int64_t iiFrameIdx)
{
    static int64_t iiPrevTimeStamp = 0;
    static int64_t iiPrevFrameIdx  = 0;

    // Update m_fMovingAverageFrameTimeMS
    if (iiFrameIdx != iiPrevFrameIdx)                              // Not a repeating frame
        if ((iiFrameIdx - iiPrevFrameIdx) <= 2)                    // Not more than 1 frame skipped
            if ((iiTimeStamp - iiPrevTimeStamp) < AMF_SECOND / 2)  // Not more than half a second had passed
            {
                int64_t iiDeltaTime = (iiTimeStamp - iiPrevTimeStamp) / (iiFrameIdx - iiPrevFrameIdx);
                float fFrameTimeMS  = iiDeltaTime / float(AMF_MILLISECOND);

                m_fCumulativeFrameTimesMS += fFrameTimeMS;
                m_iCumulativeFrameTimeSamples++;

                if (m_fMovingAverageFrameTimeMS != 0.0f)
                    m_fMovingAverageFrameTimeMS = m_fMovingAverageFrameTimeMS * m_fAVGFilterAlpha + (1 - m_fAVGFilterAlpha) * fFrameTimeMS;
                else
                    m_fMovingAverageFrameTimeMS = 1.0f * iiDeltaTime / AMF_MILLISECOND;

                // Sanity limiting
                m_fMovingAverageFrameTimeMS = std::min<float>(m_fMovingAverageFrameTimeMS, 250.0f);  // Less than a 1/4 second
                m_fMovingAverageFrameTimeMS = std::max<float>(m_fMovingAverageFrameTimeMS, 0.1f);    // More than 0.1 milliseconds
            }

    // Update m_fMovingAverageOddFramesTimeMS and m_fMovingAverageEvenFramesTimeMS
    if (iiFrameIdx - iiPrevFrameIdx == 1)                      // Needs to be exactly 1 frame
        if ((iiTimeStamp - iiPrevTimeStamp) < AMF_SECOND / 2)  // Not more than a half second had passed
        {
            float& fMovingAverageParityFrameTimeMS = (iiFrameIdx & 1) ? m_fMovingAverageOddFramesTimeMS : m_fMovingAverageEvenFramesTimeMS;

            int64_t iiDeltaTime = iiTimeStamp - iiPrevTimeStamp;
            if (fMovingAverageParityFrameTimeMS != 0.0f)
                fMovingAverageParityFrameTimeMS = fMovingAverageParityFrameTimeMS * m_fAVGFilterAlpha + (1 - m_fAVGFilterAlpha) * iiDeltaTime / AMF_MILLISECOND;
            else
                fMovingAverageParityFrameTimeMS = 1.0f * iiDeltaTime / AMF_MILLISECOND;

            // Sanity limiting
            fMovingAverageParityFrameTimeMS = std::min<float>(fMovingAverageParityFrameTimeMS, 250.0f);  // Less than a 1/4 second
            fMovingAverageParityFrameTimeMS = std::max<float>(fMovingAverageParityFrameTimeMS, 0.1f);    // More than 0.1 milliseconds
        }

    iiPrevFrameIdx  = iiFrameIdx;
    iiPrevTimeStamp = iiTimeStamp;
}
