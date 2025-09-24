#include "interface.h"

#include "ftd2xx.h"
#include "artnet.h"
#include "globals.h"

#include <thread>
#include <chrono>
#include <mutex>

static FT_HANDLE ftHandle = nullptr;
static uint8_t bufferDMX[513];

static thread dmxThread;
static atomic<bool> running = false;

static int refreshRate = 25;    
static uint8_t sharedBuffer[512];
static mutex bufferMutex;

namespace Interface {
    void dmxLoop();
    const char* FTStatusToString(FT_STATUS status)
    {
        switch (status)
        {
        case FT_OK:                   return "FT_OK";
        case FT_INVALID_HANDLE:       return "FT_INVALID_HANDLE";
        case FT_DEVICE_NOT_FOUND:     return "FT_DEVICE_NOT_FOUND";
        case FT_DEVICE_NOT_OPENED:    return "FT_DEVICE_NOT_OPENED";
        case FT_IO_ERROR:             return "FT_IO_ERROR";
        case FT_INSUFFICIENT_RESOURCES: return "FT_INSUFFICIENT_RESOURCES";
        case FT_INVALID_PARAMETER:    return "FT_INVALID_PARAMETER";
        case FT_INVALID_BAUD_RATE:    return "FT_INVALID_BAUD_RATE";
        case FT_DEVICE_NOT_OPENED_FOR_ERASE: return "FT_DEVICE_NOT_OPENED_FOR_ERASE";
        case FT_DEVICE_NOT_OPENED_FOR_WRITE: return "FT_DEVICE_NOT_OPENED_FOR_WRITE";
        case FT_FAILED_TO_WRITE_DEVICE:      return "FT_FAILED_TO_WRITE_DEVICE";
        case FT_EEPROM_READ_FAILED:          return "FT_EEPROM_READ_FAILED";
        case FT_EEPROM_WRITE_FAILED:         return "FT_EEPROM_WRITE_FAILED";
        case FT_EEPROM_ERASE_FAILED:         return "FT_EEPROM_ERASE_FAILED";
        case FT_EEPROM_NOT_PRESENT:          return "FT_EEPROM_NOT_PRESENT";
        case FT_EEPROM_NOT_PROGRAMMED:       return "FT_EEPROM_NOT_PROGRAMMED";
        case FT_INVALID_ARGS:                return "FT_INVALID_ARGS";
        case FT_NOT_SUPPORTED:               return "FT_NOT_SUPPORTED";
        case FT_OTHER_ERROR:                 return "FT_OTHER_ERROR";
        case FT_DEVICE_LIST_NOT_READY:       return "FT_DEVICE_LIST_NOT_READY";
        default:                             return "UNKNOWN_ERROR";
        }
    }

    /*DWORD CountFTDI() {
        DWORD numDevs = 0;
        FT_STATUS st = FT_CreateDeviceInfoList(&numDevs);
        return (st == FT_OK) ? numDevs : 0;
    }*/

    int connect(int index = 0) {
        if (ftHandle)
        {
            DWORD rxBytes = 0, txBytes = 0, eventDWord = 0;
            FT_STATUS st = FT_GetStatus(ftHandle, &rxBytes, &txBytes, &eventDWord);
            if (st == FT_DEVICE_NOT_FOUND || st == FT_INVALID_HANDLE || st == 4) {
                disconnect();
                return false;
            }

            return true;
        }

        FT_STATUS status = FT_Open(index, &ftHandle);

        if (status != FT_OK && status != FT_DEVICE_NOT_FOUND) {
           
            cerr << "[Interface] Init interfaces fail. Code: " << FTStatusToString(status) << "\n";
            return 2;
        }
        else if (status == FT_DEVICE_NOT_FOUND) {
            return false;
        }

        FT_ResetDevice(ftHandle);
        FT_SetDivisor(ftHandle, 12);
        FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_2, FT_PARITY_NONE);
        FT_SetFlowControl(ftHandle, FT_FLOW_NONE, 0, 0);
        FT_ClrRts(ftHandle);
        FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
        FT_SetTimeouts(ftHandle, 1, 1);

        if (ArtNet::isReceiving())
        {
            start();
        }

        return true;
    }

    void disconnect() {
        if (!ftHandle) return;

        stop();

        FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
        FT_Close(ftHandle);
        ftHandle = nullptr;
    }

    void start() {
        if (!ftHandle) return;

        running.store(true);
        lock_guard<mutex> lock(bufferMutex);
        memset(sharedBuffer, 0, sizeof(sharedBuffer));
        dmxThread = thread(dmxLoop);
    }

    void stop() {
        if (!ftHandle) return;

        setDMX({});

        thread([]() {
            this_thread::sleep_for(chrono::seconds(1));
            if (!running.load()) return;
            running.store(false);
            if (dmxThread.joinable())
                dmxThread.join();
        }).detach();
    }

    void setRefreshRate(int value) {
        refreshRate = value;
    }

    void setDMX(const vector<uint8_t>& dmxData) {
        lock_guard<mutex> lock(bufferMutex);
        size_t len = min<size_t>(512, dmxData.size());
        memcpy(sharedBuffer, dmxData.data(), len);
        if (len < 512) {
            memset(sharedBuffer + len, 0, 512 - len);
        }
    }
    
    // Private
    static void dmxLoop() {
        using clock = chrono::steady_clock;
        auto next = clock::now();
        uint8_t localBuffer[513];

        while (running.load()) {
            {
                std::lock_guard<std::mutex> lock(bufferMutex);
                localBuffer[0] = 0;
                memcpy(&localBuffer[1], sharedBuffer, 512);
            }

            FT_SetBreakOn(ftHandle);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            FT_SetBreakOff(ftHandle);
            std::this_thread::sleep_for(std::chrono::microseconds(20));

            DWORD written = 0;
            FT_STATUS status = FT_Write(ftHandle, localBuffer, 513, &written);
            if (status != FT_OK && written != 513) {
                cerr << "[Interface] Write interfaces fail. Code: " << FTStatusToString(status) << "\n";
            }

            next += std::chrono::milliseconds(refreshRate);
            std::this_thread::sleep_until(next);
        }

    }
}
