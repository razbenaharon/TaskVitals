// Optional native Windows performance-counter reader. No PDH startup import.
#pragma once
#include <stddef.h>

// PDH uses the Windows 8-byte packing ABI, including in 32-bit builds.
// These minimal declarations also support older MinGW SDKs without pdh.h.
struct alignas(8) GpuPdhValue { DWORD status; union { double value; LONGLONG large; }; };
struct alignas(8) GpuPdhItem { wchar_t* name; GpuPdhValue formatted; };
static_assert(offsetof(GpuPdhValue,value)==8 && sizeof(GpuPdhValue)==16,"PDH value ABI");
static_assert(offsetof(GpuPdhItem,formatted)==8 && sizeof(GpuPdhItem)==24,"PDH item ABI");

struct GpuEngine { wchar_t key[96]; double usage; };
inline bool gpuEngineKey(const wchar_t* name,wchar_t* key) {
    const wchar_t* begin=wcsstr(name,L"_luid_");
    const wchar_t* end=begin?wcsstr(begin,L"_engtype_"):0;
    if(!begin || !end || !wcsstr(begin,L"_phys_") || !wcsstr(begin,L"_eng_")) return false;
    size_t length=size_t(end-begin);
    if(length>=96) return false;
    wmemcpy(key,begin,length); key[length]=0; return true;
}
inline bool gpuAggregate(const GpuPdhItem* items,DWORD count,double& result) {
    GpuEngine engines[256]; int used=0;
    for(DWORD i=0;i<count;i++) {
        double value=items[i].formatted.value;
        if(items[i].formatted.status>1 || !items[i].name || !(value>=0 && value<=100)) continue;
        wchar_t key[96]; if(!gpuEngineKey(items[i].name,key)) continue;
        int index=0; while(index<used && wcscmp(engines[index].key,key)!=0) ++index;
        if(index==used) {
            if(used==256) return false;
            wcscpy(engines[used].key,key); engines[used++].usage=0;
        }
        engines[index].usage+=value;
    }
    if(!used) return false;
    result=0;
    for(int i=0;i<used;i++) if(engines[i].usage>result) result=engines[i].usage;
    if(result>100) result=100;
    return true;
}
struct GpuReader {
    HMODULE library; HANDLE query,counter; void* buffer; DWORD capacity;
    bool primed;
    LONG (WINAPI *open)(LPCWSTR,DWORD_PTR,HANDLE*);
    LONG (WINAPI *add)(HANDLE,LPCWSTR,DWORD_PTR,HANDLE*);
    LONG (WINAPI *collect)(HANDLE);
    LONG (WINAPI *array)(HANDLE,DWORD,DWORD*,DWORD*,void*);
    LONG (WINAPI *close)(HANDLE);
    void stop() {
        if(query && close) close(query);
        if(buffer) HeapFree(GetProcessHeap(),0,buffer);
        if(library) FreeLibrary(library);
        library=0; query=0; counter=0; buffer=0; capacity=0; primed=false;
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
        array=reinterpret_cast<decltype(array)>(GetProcAddress(library,"PdhGetFormattedCounterArrayW"));
        close=reinterpret_cast<decltype(close)>(GetProcAddress(library,"PdhCloseQuery"));
        if(!open || !add || !collect || !array || !close || open(0,0,&query)!=0 ||
           add(query,L"\\GPU Engine(*)\\Utilization Percentage",0,&counter)!=0) { stop(); return false; }
        primed=collect(query)==0;
        return true;
    }
    bool sample(double& value) {
        if(!query) return false;
        bool previous=primed; primed=collect(query)==0;
        if(!previous || !primed) return false;
        DWORD bytes=0,count=0;
        const LONG moreData=LONG(0x800007d2u);
        if(array(counter,0x200,&bytes,&count,0)!=moreData || !bytes || bytes>4*1024*1024) return false;
        if(bytes>capacity) {
            void* next=HeapAlloc(GetProcessHeap(),0,bytes);
            if(!next) return false;
            if(buffer) HeapFree(GetProcessHeap(),0,buffer);
            buffer=next; capacity=bytes;
        }
        DWORD available=capacity;
        if(array(counter,0x200,&available,&count,buffer)!=0 || count>capacity/sizeof(GpuPdhItem)) return false;
        return gpuAggregate(static_cast<GpuPdhItem*>(buffer),count,value);
    }
};
