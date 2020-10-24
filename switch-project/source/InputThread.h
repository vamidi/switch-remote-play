#ifndef __INPUTTHREAD_H__
#define __INPUTTHREAD_H__

#include <switch.h>
#include <iostream>
#include <thread>
#include <atomic>
#include "VideoStream.h"
#include "CommandSender.h"
#include <sys/socket.h>
#include "FFMPEGConfigUI.h"

void RunStopCommandThread(std::string ip, uint16_t port)
{
    int commandSocket = -1;
    if(ConnectTo(ip, 20001, commandSocket))
    {
        if(SendCode(commandSocket, Command::CLOSE_SERVER))
            std::cout << "Sent Close server command payload" << std::endl;
        else
            std::cout << "Error sending close server command payload" << std::endl;
    }
    close(commandSocket);
}

void CommandStopThreadStart(std::string ip, uint16_t port)
{
    auto t = std::thread(RunStopCommandThread, ip, port);
    t.join();
}

void RunStartPresetStreamCommand(std::string ip, uint16_t port, STREAM_MODE setting)
{
    int commandSocket = -1;
    if(ConnectTo(ip, 20001, commandSocket))
    {
        auto streamCommand = Command::START_STREAM_LOW_LATENCY_30FPS;
        switch(setting)
        {
            default:
            case STREAM_MODE::LOW_LATENCY_30_FPS:
                streamCommand = Command::START_STREAM_LOW_LATENCY_30FPS;
                break;

            case STREAM_MODE::OK_LATENCY_60_FPS:
                streamCommand = Command::START_STREAM_OK_LATENCY_60FPS;
                break;

            case STREAM_MODE::LOW_LATENCY_V_FPS:
                streamCommand = Command::START_STREAM_LOW_LATENCY_VFPS;
                break;
        }

        if(SendCode(commandSocket, streamCommand))
            std::cout << "Sent start preset command" << std::endl;
        else
            std::cout << "Error sending start preset command" << std::endl;
    }
    close(commandSocket);
    std::cout << "Closed command socket" << std::endl;
}

void RunStartConfiguredStreamCommand(std::string ip, uint16_t port, FFMPEG_Config const config)
{
    int commandSocket = -1;
    if(ConnectTo(ip, 20001, commandSocket))
    {
        auto streamCommand = Command::START_STREAM;
        auto const payload = CommandPayload{
            .configData = config,
            .commandCode = streamCommand
        };
        
        if(SendCommandPayload(commandSocket, payload))
            std::cout << "Sent start command payload with configuration" << std::endl;
        else
            std::cout << "Error sending start payload with configuration" << std::endl;
    }
    close(commandSocket);
    std::cout << "Closed command socket" << std::endl;
}

bool ProcessInactiveStreamInput(u32 kDown,
                                std::atomic_bool& streamRequested,
                                std::atomic_bool& streamOn, 
                                std::atomic_bool& quitApp,
                                FFMPEGConfigUI& configRender)
{
    if (kDown & KEY_PLUS) 
    {
        if(!streamOn)
        {
            quitApp = true; // break in order to return to hbmenu
            return false;
        }
    }
    if(kDown & KEY_DUP)
    {
        configRender.SelectPrevious();
    }
    else if(kDown & KEY_DDOWN)
    {
        configRender.SelectNext();
    }
    else if(kDown & KEY_R)
        streamRequested = true;

    if(kDown & KEY_DRIGHT)
    {
        configRender.IncreaseSetting();
    }
    else if(kDown & KEY_DLEFT)
    {
        configRender.DecreaseSetting();
    }

    return true;
}

void RunInactiveStreamInput(std::atomic_bool& streamRequested,
                            std::atomic_bool& streamOn, 
                            std::atomic_bool& quitApp,
                            FFMPEGConfigUI& configRender)
{
    auto const sleepDuration = std::chrono::duration<int, std::milli>(16); //poll input 60fps
    while(appletMainLoop())
    {
        if(!streamOn && !streamRequested)
        {
            hidScanInput();

            //different buttons pressed compared to previous time
            u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
            if(!ProcessInactiveStreamInput(kDown, streamRequested,
                                            streamOn, quitApp, configRender))
                break;
        }

        std::this_thread::sleep_for(sleepDuration);
    }
}

struct GamepadDataPayload
{
    u32 keys;
    s32 ljx, ljy;
    s32 rjx, rjy;
    char padding[32 - (sizeof(u32) + sizeof(s32) * 4)];
};

void RunGamepadThread(std::string ip, uint16_t port)
{
    int padSocket;
    if(ConnectTo(ip, port, padSocket))
    {
        JoystickPosition lJoy;
        JoystickPosition rJoy;
        const int dataSize = sizeof(GamepadDataPayload);
        auto inputData = GamepadDataPayload();
        inputData.keys = 0;
        inputData.ljx = inputData.ljy = 0;
        inputData.rjx = inputData.rjy = 0;
        for(auto& c : inputData.padding)
            c = 0;

        const double quitTimer = 3.0;
        const double NANO_TO_SECONDS = 1000000000.0;
        auto now = armTicksToNs(armGetSystemTick());
        auto last = now;
        auto delta = (now - last)/NANO_TO_SECONDS;
        auto quitHeldTime = 0.0;

        auto const sleepDuration = std::chrono::duration<int, std::milli>(16);

        while(appletMainLoop())
        {
            std::this_thread::sleep_for(sleepDuration); // sleep a tiny bit between inputs
            hidScanInput();
            now = armTicksToNs(armGetSystemTick());
            delta = (now - last)/NANO_TO_SECONDS;
            last = now;
            //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
            //u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
            u32 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
            if(kHeld & KEY_PLUS)
            {
                quitHeldTime += delta;
                if(quitHeldTime > quitTimer)
                {
                    u32 exit = 0xFFFF;
                    inputData.keys = exit;
                    send(padSocket, (char*)&inputData, dataSize, 0);
                    break;
                }
            }
            else
            {
                quitHeldTime = 0;
            }
            

            inputData.keys = kHeld;

            hidJoystickRead(&lJoy, CONTROLLER_P1_AUTO, JOYSTICK_LEFT);
            hidJoystickRead(&rJoy, CONTROLLER_P1_AUTO, JOYSTICK_RIGHT);
            inputData.ljx = lJoy.dx;
            inputData.ljy = lJoy.dy;
            inputData.rjx = rJoy.dx;
            inputData.rjy = rJoy.dy;

            auto result = send(padSocket, (char*)&inputData, dataSize, 0);
            if(result < 0)
            {
                std::cout << "Error sending pad data" << std::endl;
            }

            // reset the input data
            inputData.keys = 0;
            inputData.ljx = inputData.ljy = 0;
            inputData.rjx = inputData.rjy = 0;
        }
    }

    close(padSocket);
}

#endif