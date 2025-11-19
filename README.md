![download](https://img.shields.io/github/downloads/GPUOpen-Tools/frame_latency_meter/total.svg)

# frame latency meter
The performance of a gaming system is critical to the overall gaming experience. One of the key factors that can impact the performance of a gaming system is the latency of the mouse response time. Measuring the latency of the mouse response time is important to ensure that the gaming system is performing optimally. However, traditional methods of measuring the latency of the mouse response time can be expensive and require additional hardware. 

To address this issue, a software-based latency meter has been developed that is designed to measure the system mouse response times in games. This software-based solution is free and easy to use, making it a highly disruptive alternative to expensive hardware-based solutions.

![FLM](./documentation/media/FLM_2.PNG)

Key Features:

The software-based latency meter offers a range of key features that make it an ideal solution for measuring the latency of the mouse response time in games. These features include:

- Measurement of the entire latency of the mouse response time, from the moment the mouse is moved to the moment the frame is displayed on the screen.
- Includes options to use : Advanced Media Framework (AMF) capture codec, optimized for AMD GPU or Desktop Duplication (DXGI) screen capture codec for use on any GPU.
- Runs on Windows platforms, making it accessible to a wide range of users.
- Provides detailed statistics for latency and per frame measurements, which can be exported to a csv file for further analysis.
- Allows users to configure hotkeys for enabling measurements, making it easy to use and customize.
- Provides users to configure the screen capture region, making it easy to capture the relevant area of the screen.
- User configurable keys to do sequenced frame captures to BMP files for capture codec diagnostics and validation

The software-based latency meter offers a range of key features for measuring the latency of the mouse response time in games. By using AMF or DXGI desktop duplication for screen capture, the software-based latency meter is compatible with a wide range of gaming systems. It provides detailed statistics for per frame and latency measurements, which can be exported to a csv file for further analysis. With user-configurable hotkeys and screen capture regions, the software-based latency meter is easy to use and customize. Overall, the software-based latency meter is an ideal solution for users who want to measure the latency of the mouse response time in games without incurring additional costs.

## Vulkan Base Applications
When using the AMF option to test latency on a Vulkan based application, set the FLM.ini to use InitAMFUsingDX12 = 1

## Quick Start

Requirements Windows 10 or higher with DX11 and DX12 support, games should run in windowed mode for DXGI capture codec to work. AMF can run in full screen mode and high frame rates.

**Step 1:** Configure your primary monitor to run the game on.

Set the monitor to use free sync or have it set to an appropriate refresh rate try starting at 60Hz first.

**Step 2:** Run flm.exe

To see the capture region bounding box, press right Alt key.<br>
Note: The bounding box will only show if the game is running in window mode.<br>
<br>
Adjust the game scene placement so that the FLM capture region is situated in an area.<br>
where the scene transitions from dark to bright when the mouse is moved horizontally.<br>

**Step 3:** Run the game.

Adjust the game scene placement so that the FLM capture region is situated in an area where the scene transitions from dark to bright when the mouse is moved horizontally.

**Step 4:** Select start measurements key sequence (default is ALT+T).

* Wait for the capture process to start, it may take a few seconds to start as FLM process data, you should see the mouse move left and right in rapid concession while the application records latency measurements. Some games may require you to press down on the mouse keys to move the mouse left and right. If the mouse movements are too small, adjust the scene location or change the mouse steps in flm.ini using "MouseHorizontalStep".

* If you do not see any measurements or the measurements are slow, stop the measurements by pressing Alt+T and try another scene location with better contrast between left and right capture regions, if that fails review the adjust setting section for addition details on how to change the default settings.

* By default, FLM starts in "RUN" work mode, in which the output shows latency[ms] and latency[frames] averages of 16 consecutive measurements. The current work mode can be changed for the current session via the settings dialog, or a new default work mode can be set by modifying the INI file

   fps = xxx.x | ................ | latency = xxx.x | frames = x.xxx

* A running series of measurements will indicate the rate at which these latency measurements are occurring. If it is not moving or is slow - then stop the measurements (Step 5), change the game scene's location and retry. If you change the scene while the measurement is still running the average latency value will not be correct until it reaches a steady state value.

**Step 5**:  Select stop measurements and review the output.

   Start measurements.<br>
   fps = xxx.x | ................ | latency = xxx.x | frames = x.xxx<br>
   Stop measurements.<br>


## To Build Project Solution Files

### Requirements 
- VS2022 installed
- CMake v3.10 or higher

### Run this batch file once for setup in the build folder

- vsbuild

A build/win folder containing FLM.sln should have been generated<br>
Open the solution file and build the project files.

### Run this batch file to remove build/win and bin folders  
- vsclean

### Adding your own capture codec
The FLM backend code is designed to add additional capture codecs, look at the capture entry code flm_capture_context and use the samples flm_capture_amf and flm_capture_dxgi as guides to developing your own specialized capture codec.


## Known Issues and Limitations

### Desktop Capture codec (DXGI) performance 
* Desktop capture is too slow when monitors refresh rate is set too high > 144 Hz.
 Try setting the monitor refresh rate to 60 Hz while you get accustomed to how FLM works, then try higher rates.
* Certain games may limit the utilization of desktop duplication screen capture, making the option to use the DXGI codec inoperable.
* DXGI capture codec must be running on the primary display, else frame capture will be too slow.
* The current capture codecs are processing SAD measurements using CPU and may be too slow on some PC.

### No result shown 
* The default settings may not trigger latency measurements. Try adjustments to settings for ThresholdCoefficient and AVGFilter in the flm file.
* FLM will not work using remote desktop connections.

### Latency results shown are too high

* When games are running at high frame rates FLM measurements show higher values then those measured by hardware devices.




