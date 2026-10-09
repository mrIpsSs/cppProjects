#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <chrono>
#include <thread>
#include <psapi.h>
#include <processthreadsapi.h>
#include <math.h>
#include <vector>
#include <sysinfoapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#include <comdef.h>
#include <Wbemidl.h>

typedef struct {
    std::string name;
    float cpuUsage;
    float memoryUsage;
    float gpuUsage;
    float PID;
} MyProcess;

typedef struct {
    std::vector<MyProcess> processes;
    int processCount;
} ProcessMemory;

typedef struct {
    std::vector<ULARGE_INTEGER> KernelUserSums;
} SumsVector;

typedef struct {
    float PID;
    float Usage;
} CpuUsage;

typedef struct {
    std::vector<CpuUsage> Usage;
} UsageVector;

typedef struct {
    std::vector<HCOUNTER> counters;
    float PID;
    float Usage;
} GpuUsage;

typedef struct {
    std::vector<GpuUsage> Usage;
} GpuVector;

typedef struct {
    float PID;
    float Usage;
} GpuProcessUsage;

typedef struct {
    std::vector<GpuProcessUsage> Usage;
} GpuUsageVector;

typedef struct {
    unsigned char level;
    float mb;
} Cache;

typedef struct {
    char name[256];
    unsigned char logical_processors;
    unsigned char cores;
    unsigned char sockets;
    float base_speed;
    bool virtualization;
    std::vector<Cache> cache;
} CpuInfo;

typedef struct {
    std::string name;
} GpuInfo;

typedef struct {
    int totalmem;
    int speed;
    char ddr;
} RamInfo;


RamInfo raminfo;
GpuInfo gpuinfo;
CpuInfo cpuinfo;
UsageVector Usage;
GpuUsageVector GpuProcessUsages;
SYSTEM_INFO sysinfo;
DWORD AmountOfCores;

void GetGeneralSysInfo()
{
    GetSystemInfo(&sysinfo);
    AmountOfCores = sysinfo.dwNumberOfProcessors;
    cpuinfo.logical_processors = sysinfo.dwNumberOfProcessors;

    DWORD length = 0;
    HKEY hkey;

    LONG result = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0,
        KEY_READ,
        &hkey
    );

    if (result == ERROR_SUCCESS)
    {
        char cpuName[256];
        DWORD bufSize = sizeof(cpuName);
        DWORD type = REG_SZ;

        result = RegQueryValueExA(
            hkey,
            "ProcessorNameString",
            NULL,
            &type,
            (LPBYTE)cpuName,
            &bufSize
        );

        if (result == ERROR_SUCCESS)
        {
            strcpy(cpuinfo.name, cpuName);
        }
        DWORD mhz = 0;
        DWORD mhzSize = sizeof(mhz);
        DWORD mhzType = REG_DWORD;

        result = RegQueryValueExA(
            hkey,
            "~MHz",
            NULL,
            &mhzType,
            (LPBYTE)&mhz,
            &mhzSize
        );

        if (result == ERROR_SUCCESS)
        {
            cpuinfo.base_speed = std::round(((float)mhz / 1000.0f) * 10) / 10;
        }
        RegCloseKey(hkey);
    }

    DWORD corelength = 0;

    GetLogicalProcessorInformationEx(
        RelationProcessorCore,
        NULL,
        &corelength
    );

    std::vector<BYTE> corebuffer(corelength);

    GetLogicalProcessorInformationEx(
        RelationProcessorCore,
        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(corebuffer.data()),
        &corelength
    );

    BYTE* currentcores = corebuffer.data();

    unsigned char coreCount = 0;

    while (currentcores < corebuffer.data() + corelength)
    {
        auto* infocores = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(currentcores);

        if (infocores->Relationship == RelationProcessorCore)
        {
            coreCount++;
        }
        currentcores += infocores->Size;
    }

    cpuinfo.cores = coreCount;

    GetLogicalProcessorInformationEx(
        RelationCache,
        NULL,
        &length
    );

    std::vector<BYTE> buffer(length);

    GetLogicalProcessorInformationEx(
        RelationCache,
        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()),
        &length
    );

    BYTE* current = buffer.data();

    while (current < buffer.data() + length)
    {
        auto* info =
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(current);

        if (info->Relationship == RelationCache)
        {
            Cache newCache;
            newCache.level = (unsigned char)info->Cache.Level;
            newCache.mb = ((float)info->Cache.CacheSize / 1048576);
            cpuinfo.cache.push_back(newCache);
        }

        current += info->Size;
    }


    float lvl1sum = 0.0f, lvl2sum = 0.0f, lvl3sum = 0.0f;

    for (int c = 0; c < (int)cpuinfo.cache.size(); c++)
    {
        switch (cpuinfo.cache.at(c).level)
        {
            case 1: lvl1sum += cpuinfo.cache.at(c).mb; break;
            case 2: lvl2sum += cpuinfo.cache.at(c).mb; break;
            case 3: lvl3sum += cpuinfo.cache.at(c).mb; break;
        }
    }

    cpuinfo.cache.clear();
    Cache lvl1, lvl2, lvl3;

    lvl1.level = 1;
    lvl1.mb = std::round(lvl1sum * 10) / 10;

    lvl2.level = 2;
    lvl2.mb = lvl2sum;

    lvl3.level = 3;
    lvl3.mb = lvl3sum;

    cpuinfo.cache.push_back(lvl1);
    cpuinfo.cache.push_back(lvl2);
    cpuinfo.cache.push_back(lvl3);

    cpuinfo.virtualization = IsProcessorFeaturePresent(PF_VIRT_FIRMWARE_ENABLED);

    IDXGIFactory* factory = nullptr;
    CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);

    IDXGIAdapter* adapter = nullptr;
    std::string gpuName = "Unknown";

    if (factory->EnumAdapters(0, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        // desc.Description is a wide string (WCHAR[128]), convert to std::string
        char converted[128];
        size_t convertedChars = 0;
        wcstombs_s(&convertedChars, converted, desc.Description, sizeof(converted));

        gpuName = converted;

        adapter->Release();
    }

    factory->Release();

    gpuinfo.name = gpuName;

    MEMORYSTATUSEX meminfo;
    meminfo.dwLength = sizeof(meminfo);
    GlobalMemoryStatusEx(&meminfo);

    DWORDLONG allram = meminfo.ullTotalPhys / (1024 * 1024);
    raminfo.totalmem = allram;

    CoInitializeEx(0, COINIT_MULTITHREADED);
    CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );

    IWbemLocator* pLoc = NULL;
    CoCreateInstance(
        CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc
    );

    IWbemServices* pSvc = NULL;
    pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc
    );

    CoSetProxyBlanket(
        pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE
    );

    IEnumWbemClassObject* pEnumerator = NULL;
    pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT Speed, SMBIOSMemoryType FROM Win32_PhysicalMemory"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );

    if (pEnumerator)
    {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;

        if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK && uReturn != 0)
        {
            VARIANT vtSpeed, vtType;

            pclsObj->Get(L"Speed", 0, &vtSpeed, 0, 0);
            pclsObj->Get(L"SMBIOSMemoryType", 0, &vtType, 0, 0);

            raminfo.speed = vtSpeed.uintVal;

            switch (vtType.uintVal)
            {
                case 20: raminfo.ddr = 1; break; // DDR
                case 21: raminfo.ddr = 2; break; // DDR2
                case 24: raminfo.ddr = 3; break; // DDR3
                case 26: raminfo.ddr = 4; break; // DDR4
                case 34: raminfo.ddr = 5; break; // DDR5
                default: raminfo.ddr = 0; break; // Unknown
            }

            VariantClear(&vtSpeed);
            VariantClear(&vtType);
            pclsObj->Release();
        }

        pEnumerator->Release();
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
}

int AllProcessesGpuUsagePerProcess()
{
    HQUERY query;
    HCOUNTER counter;
    PDH_STATUS statusopen;
    PDH_STATUS statusadd;
    PDH_STATUS statuscollect;
    PDH_STATUS statusformat;

    DWORD bufferSize = 0;
    DWORD itemCount = 0;
    PDH_FMT_COUNTERVALUE_ITEM_A* itemBuffer = NULL;

    std::vector<GpuProcessUsage> combined;

    statusopen = PdhOpenQuery(NULL, 0, &query);

    statusadd = PdhAddEnglishCounterA(
        query,
        "\\GPU Engine(*)\\Utilization Percentage",
        0,
        &counter
    );

    statuscollect = PdhCollectQueryData(query);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    statuscollect = PdhCollectQueryData(query);

    statusformat = PdhGetFormattedCounterArrayA(
        counter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        NULL
    );

    if (statusformat == PDH_MORE_DATA)
    {
        itemBuffer = (PDH_FMT_COUNTERVALUE_ITEM_A*)malloc(bufferSize);

        statusformat = PdhGetFormattedCounterArrayA(
            counter,
            PDH_FMT_DOUBLE,
            &bufferSize,
            &itemCount,
            itemBuffer
        );
    }

    if (itemBuffer != NULL)
    {
        for (DWORD i = 0; i < itemCount; i++)
        {
            std::string instanceName = itemBuffer[i].szName;

            size_t pidPos = instanceName.find("pid_");

            if (pidPos != std::string::npos)
            {
                size_t pidStart = pidPos + 4;
                size_t pidEnd = instanceName.find("_", pidStart);

                std::string pidString = instanceName.substr(
                    pidStart,
                    pidEnd - pidStart
                );

                float pid = (float)std::stoi(pidString);
                float usage = (float)itemBuffer[i].FmtValue.doubleValue;

                bool found = false;

                for (int j = 0; j < (int)combined.size(); j++)
                {
                    if (combined.at(j).PID == pid)
                    {
                        combined.at(j).Usage += usage;
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    GpuProcessUsage newGpuUsage;

                    newGpuUsage.PID = pid;
                    newGpuUsage.Usage = std::round(usage * 100) / 100;

                    combined.push_back(newGpuUsage);
                }
                else
                {
                    for (int j = 0; j < (int)combined.size(); j++)
                    {
                        if (combined.at(j).PID == pid)
                        {
                            combined.at(j).Usage =
                                std::round(combined.at(j).Usage * 100) / 100;

                            break;
                        }
                    }
                }
            }
        }

        free(itemBuffer);
    }

    GpuProcessUsages.Usage = combined;

    PdhCloseQuery(query);

    return (int)combined.size();
}

int AllProcessesCpuUsage()
{
    PROCESSENTRY32 process_struct;
    LPPROCESSENTRY32 process_pointer = &process_struct;

    bool success = true;
    bool processSuccess;

    HANDLE opprocess;

    FILETIME creation_time;
    FILETIME exit_time;
    FILETIME kernel_time;
    FILETIME user_time;

    PFILETIME creationtime_pointer = &creation_time;
    PFILETIME exittime_pointer = &exit_time;
    PFILETIME kerneltime_pointer = &kernel_time;
    PFILETIME usertime_pointer = &user_time;

    ULARGE_INTEGER KernelSum;
    ULARGE_INTEGER UserSum;
    ULARGE_INTEGER OverallSum;

    SumsVector OldSums;
    SumsVector NewSums;

    float TempUsage;

    int oldSums = 0;
    int newSums = 0;
    int FinalNumber = 0;

    HANDLE snapshot;

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    process_struct.dwSize = sizeof(process_struct);

    success = Process32First(snapshot, process_pointer);

    while (success)
    {
        success = Process32Next(snapshot, process_pointer);

        if (!success)
        {
            break;
        }

        opprocess = OpenProcess(
            PROCESS_QUERY_INFORMATION,
            false,
            process_pointer->th32ProcessID
        );

        processSuccess = GetProcessTimes(
            opprocess,
            creationtime_pointer,
            exittime_pointer,
            kerneltime_pointer,
            usertime_pointer
        );

        KernelSum.QuadPart =
            (uint64_t)kerneltime_pointer->dwLowDateTime |
            (uint64_t)kerneltime_pointer->dwHighDateTime << 32;

        UserSum.QuadPart =
            (uint64_t)usertime_pointer->dwLowDateTime |
            (uint64_t)usertime_pointer->dwHighDateTime << 32;

        OverallSum.QuadPart =
            KernelSum.QuadPart + UserSum.QuadPart;

        OldSums.KernelUserSums.push_back(OverallSum);

        oldSums++;
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(1500)
    );

    success = Process32First(snapshot, process_pointer);

    while (success)
    {
        success = Process32Next(snapshot, process_pointer);

        if (!success)
        {
            break;
        }

        opprocess = OpenProcess(
            PROCESS_QUERY_INFORMATION,
            false,
            process_pointer->th32ProcessID
        );

        processSuccess = GetProcessTimes(
            opprocess,
            creationtime_pointer,
            exittime_pointer,
            kerneltime_pointer,
            usertime_pointer
        );

        KernelSum.QuadPart =
            (uint64_t)kerneltime_pointer->dwLowDateTime |
            (uint64_t)kerneltime_pointer->dwHighDateTime << 32;

        UserSum.QuadPart =
            (uint64_t)usertime_pointer->dwLowDateTime |
            (uint64_t)usertime_pointer->dwHighDateTime << 32;

        OverallSum.QuadPart =
            KernelSum.QuadPart + UserSum.QuadPart;

        NewSums.KernelUserSums.push_back(OverallSum);

        newSums++;
    }

    if (oldSums > newSums)
    {
        FinalNumber = oldSums;
    }
    else if (newSums > oldSums)
    {
        FinalNumber = newSums;
    }
    else
    {
        FinalNumber = newSums;
    }

    Process32First(snapshot, process_pointer);

    for (int i = 0; i < FinalNumber; i++)
    {
        success = Process32Next(snapshot, process_pointer);

        if (success)
        {
            ULARGE_INTEGER difference;

            difference.QuadPart =
                NewSums.KernelUserSums.at(i).QuadPart -
                OldSums.KernelUserSums.at(i).QuadPart;

            TempUsage =
                float(difference.QuadPart) /
                (15'000'000.0f * AmountOfCores) * 100;

            TempUsage =
                std::round(TempUsage * 100) / 100;

            CpuUsage newCpuUsage;

            newCpuUsage.Usage = TempUsage;
            newCpuUsage.PID = process_pointer->th32ProcessID;

            Usage.Usage.push_back(newCpuUsage);
        }
    }

    return FinalNumber;
}


ProcessMemory PullProcesses()
{
    HANDLE snapshot;

    PROCESSENTRY32 process_struct;
    LPPROCESSENTRY32 process_pointer = &process_struct;

    PROCESS_MEMORY_COUNTERS memory_struct;
    PROCESS_MEMORY_COUNTERS* memory_pointer = &memory_struct;

    HANDLE opprocess;

    ProcessMemory everything;

    int i = 0;
    bool success = true;

    int bytes_to_mb;
    float overallprocessorload = 0;

    snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
    );

    while (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "Invalid snapshot" << std::endl;

        snapshot = CreateToolhelp32Snapshot(
            TH32CS_SNAPPROCESS,
            0
        );
    }

    process_struct.dwSize = sizeof(process_struct);

    Process32First(snapshot, process_pointer);

    while (success == true)
    {
        success = Process32Next(snapshot, process_pointer);

        if (!success)
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                int AmountOfUsages = AllProcessesCpuUsage();

                int ProcessPIDcount = 0;
                int NotFound = 0;

                while (ProcessPIDcount < AmountOfUsages)
                {
                    for (i = 0; i < AmountOfUsages; i++)
                    {
                        if (
                            everything.processes.at(ProcessPIDcount).PID
                            != Usage.Usage.at(i).PID
                        )
                        {
                            NotFound++;

                            if (NotFound == AmountOfUsages)
                            {
                                everything.processes.at(
                                    ProcessPIDcount
                                ).cpuUsage = 0.0f;

                                ProcessPIDcount++;
                                NotFound = 0;

                                break;
                            }
                        }
                        else if (
                            everything.processes.at(ProcessPIDcount).PID
                            == Usage.Usage.at(i).PID
                        )
                        {
                            everything.processes.at(
                                ProcessPIDcount
                            ).cpuUsage = Usage.Usage.at(i).Usage;

                            ProcessPIDcount++;

                            break;
                        }
                    }
                }

                int AmountOfGpuUsages = AllProcessesGpuUsagePerProcess();

                int GpuProcessPIDcount = 0;
                int GpuNotFound = 0;

                while (GpuProcessPIDcount < everything.processCount)
                {
                    if (AmountOfGpuUsages == 0)
                    {
                        everything.processes.at(
                            GpuProcessPIDcount
                        ).gpuUsage = 0.0f;

                        GpuProcessPIDcount++;

                        continue;
                    }

                    for (i = 0; i < AmountOfGpuUsages; i++)
                    {
                        if (
                            everything.processes.at(GpuProcessPIDcount).PID
                            != GpuProcessUsages.Usage.at(i).PID
                        )
                        {
                            GpuNotFound++;

                            if (GpuNotFound == AmountOfGpuUsages)
                            {
                                everything.processes.at(
                                    GpuProcessPIDcount
                                ).gpuUsage = 0.0f;

                                GpuProcessPIDcount++;
                                GpuNotFound = 0;

                                break;
                            }
                        }
                        else if (
                            everything.processes.at(GpuProcessPIDcount).PID
                            == GpuProcessUsages.Usage.at(i).PID
                        )
                        {
                            everything.processes.at(
                                GpuProcessPIDcount
                            ).gpuUsage = GpuProcessUsages.Usage.at(i).Usage;

                            GpuProcessPIDcount++;

                            break;
                        }
                    }
                }

                CloseHandle(opprocess);

                return everything;
            }
        }
        else if (process_pointer->th32ProcessID != 0)
        {
            i++;

            MyProcess newProcess;

            opprocess = OpenProcess(
                PROCESS_QUERY_INFORMATION,
                true,
                process_pointer->th32ProcessID
            );

            bool meminfo = GetProcessMemoryInfo(
                opprocess,
                memory_pointer,
                sizeof(memory_struct)
            );

            if (meminfo && opprocess != 0)
            {
                bytes_to_mb =
                    memory_struct.WorkingSetSize / 1'000'000.0;
            }
            else
            {
                bytes_to_mb = 0;
            }

            newProcess.name = process_pointer->szExeFile;
            newProcess.memoryUsage = bytes_to_mb;
            newProcess.PID = process_pointer->th32ProcessID;

            everything.processCount = i;

            everything.processes.push_back(newProcess);
        }
    }

    ProcessMemory empty_process_memory;

    return empty_process_memory;
}


void PullGeneralUsage(float* ramUsage, float* cpuUsage, float* gpuUsage)
{
    HANDLE snapshot;
    HANDLE process;

    PROCESS_MEMORY_COUNTERS PMC;

    PROCESSENTRY32 tempstruct;
    LPPROCESSENTRY32 tempstructpointer = &tempstruct;
    tempstruct.dwSize = sizeof(tempstruct);

    int x = 0;
    float tempusage = 0.0f;
    bool success = true;

    FILETIME kerneltime;
    FILETIME usertime;
    FILETIME idletime;

    LPFILETIME kerneltimep = &kerneltime;
    LPFILETIME usertimep = &usertime;
    LPFILETIME idletimep = &idletime;

    ULARGE_INTEGER kernelsum, usersum, OldSum, NewSum, difference, idlesum, OldIdle, NewIdle, idledifference;

    HQUERY gpuquery;
    HCOUNTER gpucounter;
    PDH_STATUS gpustatusopen;
    PDH_STATUS gpustatusadd;
    PDH_STATUS gpustatuscollect;
    PDH_STATUS gpustatusformat;

    DWORD gpuBufferSize = 0;
    DWORD gpuItemCount = 0;
    PDH_FMT_COUNTERVALUE_ITEM_A* gpuItemBuffer = NULL;

    float gputotal = 0.0f;

    gpustatusopen = PdhOpenQuery(NULL, 0, &gpuquery);

    gpustatusadd = PdhAddEnglishCounterA(
        gpuquery,
        "\\GPU Engine(*)\\Utilization Percentage",
        0,
        &gpucounter
    );

    gpustatuscollect = PdhCollectQueryData(gpuquery);

    snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 
        0
    );

    success = Process32First(
        snapshot,
        tempstructpointer
    );

    while (success)
    {
        success = Process32Next(
            snapshot,
            tempstructpointer
        );

        x++;

        if (!success && GetLastError() == ERROR_NO_MORE_FILES)
        {
            break;
        }
    }

    success = Process32First(
        snapshot,
        tempstructpointer
    );

    for (int i = 0; i < x; i++)
    {
        success = Process32Next(
            snapshot,
            tempstructpointer
        );

        if (success)
        {
            HANDLE op = OpenProcess(
                PROCESS_QUERY_INFORMATION,
                false,
                tempstructpointer->th32ProcessID
            );
            
            if (op != NULL)
            {
                bool meminfo = GetProcessMemoryInfo(
                op,
                &PMC,
                sizeof(PMC)
                );
                
                if (meminfo)
                {
                    *ramUsage += PMC.WorkingSetSize / 1'000'000.0;
                }
            }
        }
    }
    
    bool csuccess = GetSystemTimes(
        idletimep,
        kerneltimep,
        usertimep
    );

    idlesum.QuadPart = (uint64_t)idletime.dwLowDateTime | (uint64_t)idletime.dwHighDateTime << 32;
    kernelsum.QuadPart = (uint64_t)kerneltime.dwLowDateTime | (uint64_t)kerneltime.dwHighDateTime << 32;
    usersum.QuadPart = (uint64_t)usertime.dwLowDateTime | (uint64_t)usertime.dwHighDateTime << 32;

    OldIdle.QuadPart = idlesum.QuadPart;
    OldSum.QuadPart = kernelsum.QuadPart + usersum.QuadPart;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    csuccess = GetSystemTimes(
        idletimep,
        kerneltimep,
        usertimep
    );

    idlesum.QuadPart = (uint64_t)idletime.dwLowDateTime | (uint64_t)idletime.dwHighDateTime << 32;
    kernelsum.QuadPart = (uint64_t)kerneltime.dwLowDateTime | (uint64_t)kerneltime.dwHighDateTime << 32;
    usersum.QuadPart = (uint64_t)usertime.dwLowDateTime | (uint64_t)usertime.dwHighDateTime << 32;

    NewIdle.QuadPart = idlesum.QuadPart;
    NewSum.QuadPart = kernelsum.QuadPart + usersum.QuadPart;

    idledifference.QuadPart = NewIdle.QuadPart - OldIdle.QuadPart;
    difference.QuadPart = NewSum.QuadPart - OldSum.QuadPart;

    tempusage = float(difference.QuadPart - idledifference.QuadPart) / (15'000'000.0f * AmountOfCores) * 100;
    tempusage = std::round(tempusage * 100) / 100;

    *cpuUsage = tempusage;
    std::cout << *cpuUsage << " % cpu usage" << std::endl;

    gpustatuscollect = PdhCollectQueryData(gpuquery);

    gpustatusformat = PdhGetFormattedCounterArrayA(
        gpucounter,
        PDH_FMT_DOUBLE,
        &gpuBufferSize,
        &gpuItemCount,
        NULL
    );

    if (gpustatusformat == PDH_MORE_DATA)
    {
        gpuItemBuffer = (PDH_FMT_COUNTERVALUE_ITEM_A*)malloc(gpuBufferSize);

        gpustatusformat = PdhGetFormattedCounterArrayA(
            gpucounter,
            PDH_FMT_DOUBLE,
            &gpuBufferSize,
            &gpuItemCount,
            gpuItemBuffer
        );
    }

    if (gpuItemBuffer != NULL)
    {
        for (DWORD i = 0; i < gpuItemCount; i++)
        {
            gputotal += (float)gpuItemBuffer[i].FmtValue.doubleValue;
        }

        free(gpuItemBuffer);
    }

    gputotal = std::round(gputotal * 100) / 100;

    *gpuUsage = gputotal;

    PdhCloseQuery(gpuquery);

    std::cout << *gpuUsage << " % gpu usage" << std::endl;
}