// Optional CPU speed limit (Windows PDH) and CPU temperature (Core Temp).
// Neither needs administrator rights or a driver in this process.
#pragma once
#include <stddef.h>
#include <string.h>

// "% Performance Limit": the performance the processor guarantees, as a share of
// its nominal (base) performance; Windows lowers it for power policy, power
// budget or heat. No PDH startup import.
struct LimitReader {
    HMODULE library; HANDLE query,counter;
    LONG (WINAPI *open)(LPCWSTR,DWORD_PTR,HANDLE*);
    LONG (WINAPI *add)(HANDLE,LPCWSTR,DWORD_PTR,HANDLE*);
    LONG (WINAPI *collect)(HANDLE);
    LONG (WINAPI *value)(HANDLE,DWORD,DWORD*,GpuPdhValue*);
    LONG (WINAPI *close)(HANDLE);
    void stop() {
        if(query && close) close(query);
        if(library) FreeLibrary(library);
        library=0; query=0; counter=0;
    }
    bool start() {
        stop();
        wchar_t path[MAX_PATH]; UINT length=GetSystemDirectoryW(path,MAX_PATH);
        if(!length || length+9>=MAX_PATH) return false;
        lstrcatW(path,L"\\pdh.dll"); library=LoadLibraryW(path);
        if(!library) return false;
        open=reinterpret_cast<decltype(open)>(GetProcAddress(library,"PdhOpenQueryW"));
        add=reinterpret_cast<decltype(add)>(GetProcAddress(library,"PdhAddEnglishCounterW"));
        collect=reinterpret_cast<decltype(collect)>(GetProcAddress(library,"PdhCollectQueryData"));
        value=reinterpret_cast<decltype(value)>(GetProcAddress(library,"PdhGetFormattedCounterValue"));
        close=reinterpret_cast<decltype(close)>(GetProcAddress(library,"PdhCloseQuery"));
        if(!open || !add || !collect || !value || !close || open(0,0,&query)!=0 ||
           add(query,L"\\Processor Information(_Total)\\% Performance Limit",0,&counter)!=0) { stop(); return false; }
        return true;
    }
    bool sample(double& percent) {
        if(!query || collect(query)!=0) return false;
        GpuPdhValue result={}; DWORD type=0;
        if(value(counter,0x200,&type,&result)!=0 || result.status>1) return false;
        if(!(result.value>=0 && result.value<=100)) return false;
        percent=result.value; return true;
    }
};

// Core Temp's documented shared-memory block, 4-byte aligned. Only the
// original fields are read; the Ex block starts with the same layout.
struct CoreTempData {
    unsigned int load[256],tjMax[128],coreCount,cpuCount;
    float temperature[256],vid,speed,fsb,multiplier;
    char name[100]; unsigned char fahrenheit,deltaToTjMax;
};
static_assert(offsetof(CoreTempData,temperature)==1544 && offsetof(CoreTempData,deltaToTjMax)==2685,"Core Temp ABI");
const size_t kCoreTempBytes=offsetof(CoreTempData,deltaToTjMax)+1;

// Hottest core in Celsius; out-of-range and NaN readings are skipped.
inline bool coreTempHottest(const CoreTempData& data,double& celsius) {
    unsigned cpus=data.cpuCount,cores=data.coreCount;
    if(!cpus || !cores || cpus>128 || cores>256 || cpus*cores>256) return false;
    bool found=false; double hottest=0;
    for(unsigned i=0;i<cpus*cores;i++) {
        double reading=data.temperature[i];
        if(data.deltaToTjMax) reading=double(data.tjMax[i/cores])-reading;
        if(data.fahrenheit) reading=(reading-32)*5/9;
        if(!(reading>0 && reading<150)) continue;
        if(!found || reading>hottest) { hottest=reading; found=true; }
    }
    if(found) celsius=hottest;
    return found;
}
// Open and close on every sample: a held handle would keep a stale block
// alive after Core Temp exits.
inline bool readCoreTemp(double& celsius) {
    const wchar_t* names[]={L"CoreTempMappingObjectEx",L"CoreTempMappingObject"};
    for(int i=0;i<2;i++) {
        HANDLE mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,names[i]);
        if(!mapping) continue;
        void* view=MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);
        bool found=false;
        if(view) {
            MEMORY_BASIC_INFORMATION region={};
            if(VirtualQuery(view,&region,sizeof(region)) && region.RegionSize>=kCoreTempBytes) {
                CoreTempData data; memcpy(&data,view,kCoreTempBytes);
                found=coreTempHottest(data,celsius);
            }
            UnmapViewOfFile(view);
        }
        CloseHandle(mapping);
        if(found) return true;
    }
    return false;
}
