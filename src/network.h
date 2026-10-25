// Optional network traffic from the physical adapters' own byte counters
// (IP Helper). TaskVitals sends nothing. No IP Helper startup import.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// MIB_IF_ROW2 is 1352 bytes on x86 and x64 (no pointers, 8-byte aligned).
// Older MinGW SDKs lack netioapi.h, so only the fields used are read by offset.
const size_t kIfRowBytes=1352, kIfTableRows=8, kIfLuid=0, kIfAlias=28, kIfType=1128,
    kIfFlags=1152, kIfOperStatus=1156, kIfTransmitSpeed=1192, kIfReceiveSpeed=1200,
    kIfInOctets=1208, kIfOutOctets=1280;
const DWORD kIfLoopback=24, kIfUp=1;
const unsigned char kIfHardware=1, kIfFilter=2;

struct NetworkCounter { uint64_t adapter, received, sent; };
// The connected physical adapter's link rate to the router or switch, in bits
// per second. It is not the internet speed.
struct NetworkLink { uint64_t down, up; wchar_t name[32]; };
// Sum each adapter's growth since the previous sample. An adapter that
// appears, disappears or resets its counters adds nothing to that sample.
inline bool networkDelta(const NetworkCounter* before,int beforeCount,const NetworkCounter* after,int afterCount,
                         uint64_t& received,uint64_t& sent) {
    received=0; sent=0; bool matched=false;
    for(int i=0;i<afterCount;i++)
        for(int j=0;j<beforeCount;j++) {
            if(after[i].adapter!=before[j].adapter) continue;
            matched=true;
            if(after[i].received>=before[j].received && after[i].sent>=before[j].sent) {
                received+=after[i].received-before[j].received; sent+=after[i].sent-before[j].sent;
            }
            break;
        }
    return matched;
}
// Physical adapters only: virtual, VPN and filter interfaces repeat the same
// traffic, and loopback never leaves the computer. The link shown is the
// fastest connected adapter's.
inline int networkCounters(const unsigned char* table,NetworkCounter* list,int capacity,NetworkLink& link) {
    DWORD rows; memcpy(&rows,table,sizeof(rows));
    int count=0; link.down=0; link.up=0; link.name[0]=0;
    for(DWORD i=0;i<rows && count<capacity;i++) {
        const unsigned char* row=table+kIfTableRows+i*kIfRowBytes;
        DWORD type; memcpy(&type,row+kIfType,sizeof(type));
        unsigned char flags=row[kIfFlags];
        if(type==kIfLoopback || !(flags&kIfHardware) || (flags&kIfFilter)) continue;
        DWORD status; memcpy(&status,row+kIfOperStatus,sizeof(status));
        uint64_t down,up; memcpy(&down,row+kIfReceiveSpeed,8); memcpy(&up,row+kIfTransmitSpeed,8);
        // An unknown speed is reported as all ones.
        if(status==kIfUp && down>link.down && down!=~uint64_t(0) && up!=~uint64_t(0)) {
            link.down=down; link.up=up;
            memcpy(link.name,row+kIfAlias,31*sizeof(wchar_t)); link.name[31]=0;
        }
        memcpy(&list[count].adapter,row+kIfLuid,8);
        memcpy(&list[count].received,row+kIfInOctets,8);
        memcpy(&list[count].sent,row+kIfOutOctets,8);
        ++count;
    }
    return count;
}
struct NetworkReader {
    HMODULE library; NetworkCounter previous[32]; int count; LARGE_INTEGER previousTime; bool primed;
    NetworkLink link;
    DWORD (WINAPI *table)(void**);
    void (WINAPI *release)(void*);
    void stop() {
        if(library) FreeLibrary(library);
        library=0; count=0; primed=false; link.down=0; link.up=0; link.name[0]=0;
    }
    bool read(NetworkCounter* list,int& found,LARGE_INTEGER& time) {
        void* rows=0;
        if(table(&rows)!=0 || !rows) { link.down=0; link.up=0; link.name[0]=0; return false; }
        found=networkCounters(static_cast<unsigned char*>(rows),list,32,link);
        release(rows);
        return QueryPerformanceCounter(&time)!=FALSE;
    }
    bool start() {
        stop();
        wchar_t path[MAX_PATH]; UINT length=GetSystemDirectoryW(path,MAX_PATH);
        if(!length || length+14>=MAX_PATH) return false;
        lstrcatW(path,L"\\iphlpapi.dll"); library=LoadLibraryW(path);
        if(!library) return false;
        table=procAddress<decltype(table)>(library,"GetIfTable2");
        release=procAddress<decltype(release)>(library,"FreeMibTable");
        if(!table || !release) { stop(); return false; }
        primed=read(previous,count,previousTime);
        return true;
    }
    // Bytes per second, averaged since the previous sample.
    bool sample(double& down,double& up) {
        if(!library) return false;
        NetworkCounter now[32]; int found=0; LARGE_INTEGER time,frequency;
        bool previousValid=primed;
        if(!read(now,found,time) || !QueryPerformanceFrequency(&frequency)) { primed=false; return false; }
        uint64_t received=0,sent=0;
        bool matched=networkDelta(previous,count,now,found,received,sent);
        double seconds=double(time.QuadPart-previousTime.QuadPart)/double(frequency.QuadPart);
        memcpy(previous,now,sizeof(now)); count=found; previousTime=time; primed=true;
        if(!previousValid || !matched || !(seconds>0.1)) return false;
        down=double(received)/seconds; up=double(sent)/seconds; return true;
    }
};
