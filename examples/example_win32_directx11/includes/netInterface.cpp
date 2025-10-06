#include "netInterface.h"

#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <codecvt>
#include <iostream>

using namespace std;
#include <sstream>
#include <regex>
#include "globals.h"

namespace netInterface {
    static std::wstring install();
    static std::wstring getAdapterGuidFromInstanceId(const std::wstring& instanceId);
    static std::wstring getNameFromGuid(const std::wstring& guid);
    static bool setAdapterIp(const std::wstring& adapterName);
    static bool isIpFree(DWORD ipNetworkOrder);

    vector<string> listInterfaces() {
        vector<string> result;

        result.push_back("0.0.0.0");

        ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
        ULONG family = AF_INET;
        PIP_ADAPTER_ADDRESSES adapters = nullptr;
        ULONG buflen = 0;

        if (GetAdaptersAddresses(family, flags, nullptr, adapters, &buflen) == ERROR_BUFFER_OVERFLOW) {
            adapters = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
        }
        if (!adapters) {
            cerr << "[Net Interface] Not found adapters" << endl;
            return result;
        }

        if (GetAdaptersAddresses(family, flags, nullptr, adapters, &buflen) != NO_ERROR) {
            free(adapters);
            return result;
        }

        for (PIP_ADAPTER_ADDRESSES cur = adapters; cur; cur = cur->Next) {
            for (PIP_ADAPTER_UNICAST_ADDRESS ua = cur->FirstUnicastAddress; ua; ua = ua->Next) {
                SOCKADDR* addr = ua->Address.lpSockaddr;
                if (addr->sa_family == AF_INET) {
                    char ipbuf[INET_ADDRSTRLEN] = { 0 };
                    sockaddr_in* sa_in = (sockaddr_in*)addr;
                    inet_ntop(AF_INET, &sa_in->sin_addr, ipbuf, sizeof(ipbuf));
                    result.push_back(string(ipbuf));
                }
            }
        }

        free(adapters);
        return result;
    }
    
    bool create() {
        if (!isRunningAsAdmin()) {
            reLaunchAsAdmin(L"install-net-adapter");
            return false;
        }

        wstring instanceId = install();
        if (instanceId.empty()) {
            cout << "[Net Interface] Install adapter fail" << endl;
            return false;
        }

        Sleep(3000);

        wstring adapterGuid = getAdapterGuidFromInstanceId(instanceId);
        if (adapterGuid.empty()) {
            cout << "[Net Interface] Get adapter net fail" << endl;
            return false;
        }

        wstring adapterName = getNameFromGuid(adapterGuid);

        if (!setAdapterIp(adapterName)) {
            cout << "[Net Interface] Set adapter IP fail" << endl;
            return false;
        }

        return true;
    }

    //Private functions
    inline wstring install() {
        HDEVINFO hDevInfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_NET, nullptr);
        if (hDevInfo == INVALID_HANDLE_VALUE) {
            cerr << "[Net Interface] INVALID_HANDLE_VALUE: " << GetLastError() << endl;
            return L"";
        }

        SP_DEVINFO_DATA devInfo = { sizeof(devInfo) };

        if (!SetupDiCreateDeviceInfo(
            hDevInfo,
            TEXT("Microsoft KM-TEST Loopback Adapter"),
            &GUID_DEVCLASS_NET,
            nullptr,
            nullptr,
            DICD_GENERATE_ID,
            &devInfo)) {
            cerr << "[Net Interface] SetupDiCreateDeviceInfo: " << GetLastError() << endl;
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return L"";
        }

        const wchar_t* hwId = L"*MSLOOP\0\0";
        if (!SetupDiSetDeviceRegistryProperty(
            hDevInfo,
            &devInfo,
            SPDRP_HARDWAREID,
            reinterpret_cast<const BYTE*>(hwId),
            (DWORD)((wcslen(hwId) + 2) * sizeof(wchar_t)))) {
            cerr << "[Net Interface] SetupDiSetDeviceRegistryProperty: " << GetLastError() << endl;
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return L"";
        }

        if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hDevInfo, &devInfo)) {
            cerr << "[Net Interface] SetupDiCallClassInstaller: " << GetLastError() << endl;
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return L"";
        }

        if (!SetupDiBuildDriverInfoList(hDevInfo, &devInfo, SPDIT_COMPATDRIVER)) {
            DWORD err = GetLastError();
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return L"";
        }

        SP_DRVINFO_DATA drvInfo = { sizeof(drvInfo) };
        if (!SetupDiEnumDriverInfo(
            hDevInfo,
            &devInfo,
            SPDIT_COMPATDRIVER,
            0,
            &drvInfo)) {
            cerr << "[Net Interface] SetupDiEnumDriverInfo: " << GetLastError() << endl;
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return L"";
        }

        if (!SetupDiSetSelectedDriver(hDevInfo, &devInfo, &drvInfo)) {
            cerr << "[Net Interface] SetupDiSetSelectedDriver: " << GetLastError() << endl;
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return L"";
        }

        if (!SetupDiCallClassInstaller(DIF_SELECTDEVICE, hDevInfo, &devInfo)) {
            cerr << "[Net Interface] SetupDiCallClassInstaller: " << GetLastError() << endl;
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return L"";
        }

        if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICE, hDevInfo, &devInfo)) {
            cerr << "[Net Interface] SetupDiCallClassInstaller: " << GetLastError() << endl;
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return L"";
        }

        wchar_t instanceId[MAX_DEVICE_ID_LEN];
        if (CM_Get_Device_ID(devInfo.DevInst, instanceId, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) {
            cerr << "[Net Interface] CM_Get_Device_ID: " << GetLastError() << endl;
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return L"";
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
        return instanceId;
    }

    wstring getAdapterGuidFromInstanceId(const wstring& instanceId) {
        HDEVINFO devInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_NET, nullptr, nullptr, DIGCF_PRESENT);
        SP_DEVINFO_DATA devData = { sizeof(SP_DEVINFO_DATA) };

        for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devData); i++) {
            wchar_t buffer[512];
            if (SetupDiGetDeviceInstanceId(devInfo, &devData, buffer, 512, nullptr)) {
                if (instanceId == buffer) {
                    HKEY hKey = SetupDiOpenDevRegKey(devInfo, &devData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
                    if (hKey != INVALID_HANDLE_VALUE) {
                        wchar_t guid[128];
                        DWORD size = sizeof(guid);
                        if (RegQueryValueExW(hKey, L"NetCfgInstanceId", nullptr, nullptr, (LPBYTE)guid, &size) == ERROR_SUCCESS) {
                            RegCloseKey(hKey);
                            SetupDiDestroyDeviceInfoList(devInfo);
                            return guid;
                        }
                        RegCloseKey(hKey);
                    }
                }
            }
        }

        SetupDiDestroyDeviceInfoList(devInfo);
        return L"";
    }

    std::wstring getNameFromGuid(const std::wstring& guid) {
        ULONG bufLen = 15000;
        std::vector<BYTE> buffer(bufLen);
        PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

        if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, addresses, &bufLen) != NO_ERROR)
            return L"";

        for (PIP_ADAPTER_ADDRESSES addr = addresses; addr; addr = addr->Next) {
            if (addr->AdapterName) {
                std::wstring adapterGuid(addr->AdapterName, addr->AdapterName + strlen(addr->AdapterName));
                if (adapterGuid.find(guid) != std::wstring::npos)
                    return addr->FriendlyName;
            }
        }

        return L"";
    }

    bool isIpFree(DWORD ip) {
        ULONG mac[2];
        ULONG len = 6;
        return SendARP(ip, 0, mac, &len) != NO_ERROR;
    }

    bool setAdapterIp(const wstring& adapterName) {
        std::string adapterNameStr(adapterName.begin(), adapterName.end());
        if (adapterNameStr.empty()) return false;

        ULONG bufLen = 15000;
        std::vector<BYTE> buffer(bufLen);
        PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

        std::regex artNetRegex(R"(Art-Net(?: (\d+))?)", std::regex_constants::icase);
        int maxNum = 0;

        if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, addresses, &bufLen) != NO_ERROR)
            return false;

        for (PIP_ADAPTER_ADDRESSES addr = addresses; addr; addr = addr->Next) {
            std::wstring wname = addr->FriendlyName;
            std::string name(wname.begin(), wname.end());

            std::smatch match;
            if (std::regex_search(name, match, artNetRegex)) {
                if (match[1].matched && !match[1].str().empty()) {
                    int num = std::stoi(match[1].str());
                    if (num > maxNum) maxNum = num;
                }
                else {
                    maxNum = 1;
                }
            }
        }

        string newName = "Art-Net";
        if (maxNum != 0) newName += " " + to_string(maxNum + 1);

        for (int i = 10; i < 255; i++) {
            std::string ipStr = "2.0.0." + to_string(i);
            in_addr ipAddrStruct{};
            inet_pton(AF_INET, ipStr.c_str(), &ipAddrStruct);
            DWORD ip = ipAddrStruct.S_un.S_addr;

            if (isIpFree(ip)) {
                std::ostringstream cmd;
                cmd << "netsh interface ipv4 set address \""
                    << adapterNameStr
                    << "\" static "
                    << ipStr
                    << " 255.0.0.0 && netsh interface set interface name=\""
                    << adapterNameStr
                    << "\" newname=\""
                    << newName
                    << "\"";

                std::string netshCmd = cmd.str();

                int result = system(netshCmd.c_str());
                if (result != 0) return false;

                return true;
            }
        }
        
        return false;
    }
}
