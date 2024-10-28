/*
Compile with g++ -shared -o api.dll api.cpp -Ofast -fPIC -shared
*/

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <tlhelp32.h>

using namespace std;
namespace fs = std::filesystem;

extern "C"
{
    bool running = false;
    int killed = 0;

    __declspec(dllexport) void start_monitoring(const char* folder_path);
    __declspec(dllexport) void stop_monitoring();
    __declspec(dllexport) int get_killed_count();
    __declspec(dllexport) const char* get_executables_in_folder(const char* folder_path);
}

void monitor_executables(const string& folder_path);

void close_application_by_exe(const string& exe_name)
{
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (hProcessSnap == INVALID_HANDLE_VALUE) return;

    if (Process32First(hProcessSnap, &pe32))
    {
        do
        {
            if (_stricmp(pe32.szExeFile, exe_name.c_str()) == 0)
            {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess)
                {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    killed++;
                }
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);
}

void monitor_executables(const string& folder_path)
{
    while (running)
    {
        for (const auto& entry : fs::directory_iterator(folder_path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".exe")
            {
                close_application_by_exe(entry.path().filename().string());
            }
        }

        int interval;
        std::ifstream interval_file("interval.txt");

        if (interval_file.is_open())
        {
            interval_file >> interval;

            if (interval_file.fail())
            {
                interval = 0;
            }

            this_thread::sleep_for(chrono::seconds(interval));
            interval_file.close();
        }
    }
}

void start_monitoring(const char* folder_path)
{
    if (!running)
    {
        running = true;
        thread(monitor_executables, string(folder_path)).detach();
    }
}

void stop_monitoring()
{
    running = false;
}

int get_killed_count()
{
    return killed;
}

const char* get_executables_in_folder(const char* folder_path)
{
    static string result;
    result.clear();

    for (const auto& entry : fs::directory_iterator(folder_path))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".exe")
        {
            result += entry.path().filename().string() + "\n";
        }
    }
    return result.c_str();
}