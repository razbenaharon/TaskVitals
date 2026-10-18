#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0601
#define WINVER 0x0601
#define _WIN32_IE 0x0600
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <imm.h>
#include <commdlg.h>
#include <ole2.h>
#include <oleacc.h>
#include <shellapi.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include "gpu.h"
#include "thermal.h"

namespace {
GpuReader gpu = {};
LimitReader limit = {};
const wchar_t* kClass = L"TaskVitals.Window.1";
const wchar_t* kPrefs = L"Software\\TaskVitals";
const wchar_t* kRun = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const COLORREF kBackground=RGB(22,25,31);
const UINT kAppBarMessage=WM_APP+1;
struct Palette { COLORREF cpu,ram,value; };
const Palette kPalettes[]={
    {RGB(91,206,182),RGB(190,176,255),RGB(244,246,250)},
    {RGB(245,245,245),RGB(245,245,245),RGB(245,245,245)},
    {RGB(110,190,255),RGB(80,220,210),RGB(230,244,255)},
    {RGB(255,198,91),RGB(255,149,128),RGB(255,240,220)},
    {RGB(38,96,130),RGB(98,65,150),RGB(33,37,45)}
};
enum { Startup=102, Quit, TaskbarMode, Refresh1=201, Refresh2, Refresh5, Refresh10,
       TextSmall=301, TextNormal, TextLarge, PositionRight=401, PositionCenter,
       PositionLeft, PositionCustom, MoveLeft, MoveRight,
       ColorsDefault=501, ColorsWhite, ColorsOcean, ColorsWarm, ColorsDark,
       PickCpuColor, PickRamColor, PickValueColor, ToggleGpu=601, ToggleLimit, ToggleTemperature };
struct CpuTimes { uint64_t idle, kernel, user; };
struct State {
    HWND window, tooltip; HFONT regular, number, labelFont; int dpi;
    bool ready, haveCpu, haveMemory, compact, dragging, gpuEnabled, haveGpu;
    bool limitEnabled, haveLimit, temperatureEnabled, haveTemperature;
    bool appBar, shellFullscreen, hidden; HWINEVENTHOOK foregroundHook;
    double gpuUsage, limitPercent, temperature;
    CpuTimes previous; double cpu; MEMORYSTATUSEX memory;
    int x, y;
    UINT refreshMs;
    UINT textScale, anchor; int taskbarOffset;
    SYSTEMTIME sampledAt; wchar_t tooltipText[1024];
    Palette colors;
} s = {};
struct Span { int left,right; };
struct TaskbarLayout {
    HWND bar; HWINEVENTHOOK hook; RECT bounds;
    Span occupied[128]; int count; bool valid,dirty,pending;
} layout = {};

// Find a full-width gap, never clamp an overlay into an occupied button.
bool freePosition(int left,int right,int width,int desired,const Span* spans,int count,int& x) {
    bool found=false; int best=0;
    for(int i=0;i<=count;i++) {
        int end=i<count?spans[i].left:right;
        if(end>right) end=right;
        if(end-left>=width) {
            int candidate=desired;
            if(candidate<left) candidate=left;
            if(candidate>end-width) candidate=end-width;
            int distance=abs(candidate-desired);
            if(!found || distance<best) { found=true; best=distance; x=candidate; }
        }
        if(i<count && spans[i].right>left) left=spans[i].right;
    }
    return found;
}

void accessibleBounds(IAccessible* object,VARIANT child,int& budget,int depth) {
    if(!layout.valid) return;
    if(--budget<0 || depth>16) { layout.valid=false; return; }
    VARIANT role,state; VariantInit(&role); VariantInit(&state);
    HRESULT roleResult=object->get_accRole(child,&role);
    HRESULT stateResult=object->get_accState(child,&state);
    bool hidden=SUCCEEDED(stateResult) && state.vt==VT_I4 &&
        (state.lVal&(STATE_SYSTEM_INVISIBLE|STATE_SYSTEM_OFFSCREEN));
    bool control=SUCCEEDED(roleResult) && role.vt==VT_I4 &&
        (role.lVal==ROLE_SYSTEM_PUSHBUTTON || role.lVal==ROLE_SYSTEM_CHECKBUTTON ||
         role.lVal==ROLE_SYSTEM_RADIOBUTTON || role.lVal==ROLE_SYSTEM_BUTTONDROPDOWN ||
         role.lVal==ROLE_SYSTEM_BUTTONMENU || role.lVal==ROLE_SYSTEM_PAGETAB ||
         role.lVal==ROLE_SYSTEM_COMBOBOX || role.lVal==ROLE_SYSTEM_TEXT);
    VariantClear(&role); VariantClear(&state);
    if(control) {
        if(hidden) return;
        long x=0,y=0,w=0,h=0;
        if(SUCCEEDED(object->accLocation(&x,&y,&w,&h,child)) && w>0 && h>0 &&
           y<layout.bounds.bottom && y+h>layout.bounds.top &&
           x<layout.bounds.right && x+w>layout.bounds.left) {
            if(layout.count==128) { layout.valid=false; return; }
            int gap=MulDiv(6,s.dpi,96);
            layout.occupied[layout.count++]={int(x)-gap,int(x+w)+gap};
            return; // A button's descendants cannot occupy more than the button.
        }
    }
    if(child.lVal!=CHILDID_SELF) return;
    long count=0;
    if(FAILED(object->get_accChildCount(&count))) { layout.valid=false; return; }
    if(!count) return;
    if(count>128) { layout.valid=false; return; }
    VARIANT children[128]; long obtained=0;
    if(FAILED(AccessibleChildren(object,0,count,children,&obtained))) { layout.valid=false; return; }
    for(long i=0;i<obtained;i++) {
        if(children[i].vt==VT_DISPATCH && children[i].pdispVal) {
            IAccessible* next=0;
            if(SUCCEEDED(children[i].pdispVal->QueryInterface(IID_IAccessible,reinterpret_cast<void**>(&next)))) {
                VARIANT self; VariantInit(&self); self.vt=VT_I4; self.lVal=CHILDID_SELF;
                accessibleBounds(next,self,budget,depth+1); next->Release();
            } else layout.valid=false;
        } else if(children[i].vt==VT_I4) accessibleBounds(object,children[i],budget,depth+1);
        VariantClear(&children[i]);
    }
    if(obtained!=count) layout.valid=false;
}
void CALLBACK taskbarChanged(HWINEVENTHOOK,DWORD event,HWND window,LONG object,LONG child,DWORD,DWORD) {
    if(!s.compact || !window || GetAncestor(window,GA_ROOT)!=layout.bar) return;
    if(event==EVENT_OBJECT_STATECHANGE) {
        // Visibility state changes can remove the last app button without moving
        // its neighbours. Ignore ordinary focus/pressed/flashing state updates.
        IAccessible* accessible=0; VARIANT item,state; VariantInit(&item); VariantInit(&state);
        HRESULT result=AccessibleObjectFromEvent(window,object,child,&accessible,&item);
        bool visible=false;
        if(SUCCEEDED(result) && accessible) {
            visible=SUCCEEDED(accessible->get_accState(item,&state)) && state.vt==VT_I4 &&
                !(state.lVal&(STATE_SYSTEM_INVISIBLE|STATE_SYSTEM_OFFSCREEN));
            accessible->Release();
        }
        VariantClear(&item); VariantClear(&state);
        if(visible) return;
    }
    // Focus, selection and pressed/flashing states do not change button bounds.
    if(event!=EVENT_OBJECT_CREATE && event!=EVENT_OBJECT_DESTROY &&
       event!=EVENT_OBJECT_SHOW && event!=EVENT_OBJECT_HIDE &&
       event!=EVENT_OBJECT_REORDER && event!=EVENT_OBJECT_LOCATIONCHANGE &&
       event!=EVENT_OBJECT_STATECHANGE) return;
    layout.dirty=true;
    // Coalesce Explorer's animation events; no periodic accessibility polling.
    if(!layout.pending) layout.pending=SetTimer(s.window,2,200,0)!=0;
}
void readTaskbarLayout(HWND bar,const RECT& bounds) {
    if(layout.bar!=bar) {
        if(layout.hook) UnhookWinEvent(layout.hook);
        layout.bar=bar; DWORD process=0; GetWindowThreadProcessId(bar,&process);
        layout.hook=SetWinEventHook(EVENT_OBJECT_CREATE,EVENT_OBJECT_LOCATIONCHANGE,0,
            taskbarChanged,process,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
        layout.dirty=true;
    }
    if(!EqualRect(&layout.bounds,&bounds)) layout.dirty=true;
    if(!layout.dirty && layout.hook) return;
    layout.bounds=bounds; layout.count=0; layout.valid=true; layout.dirty=false;
    IAccessible* root=0;
    if(SUCCEEDED(AccessibleObjectFromWindow(bar,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&root)))) {
        VARIANT self; VariantInit(&self); self.vt=VT_I4; self.lVal=CHILDID_SELF;
        int budget=768; accessibleBounds(root,self,budget,0); root->Release();
    } else layout.valid=false;
    if(!layout.count) layout.valid=false;
    // Insertion sort is enough for this fixed, small collection; no heap container.
    for(int i=1;i<layout.count;i++) {
        Span value=layout.occupied[i]; int j=i;
        while(j>0 && layout.occupied[j-1].left>value.left) { layout.occupied[j]=layout.occupied[j-1]; --j; }
        layout.occupied[j]=value;
    }
}

int px(int n) { return MulDiv(n, s.dpi*int(s.textScale), 9600); }
// CPU and RAM are always shown; each optional metric adds one column.
struct Column { const wchar_t* label; bool valid, degrees; double value; COLORREF accent; };
int columns(Column* list) {
    int n=0;
    list[n++]={L"CPU",s.haveCpu,false,s.cpu,s.colors.cpu};
    list[n++]={L"RAM",s.haveMemory,false,double(s.memory.dwMemoryLoad),s.colors.ram};
    if(s.gpuEnabled) list[n++]={L"GPU",s.haveGpu,false,s.gpuUsage,s.colors.ram};
    if(s.limitEnabled) list[n++]={L"LIM",s.haveLimit,false,s.limitPercent,s.colors.cpu};
    if(s.temperatureEnabled) list[n++]={L"TEMP",s.haveTemperature,true,s.temperature,s.colors.cpu};
    return n;
}
int extraColumns() { return int(s.gpuEnabled)+int(s.limitEnabled)+int(s.temperatureEnabled); }
int compactWidth() { return 196+98*extraColumns(); }
int desktopWidth() { return 320+154*extraColumns(); }
uint64_t ticks(FILETIME t) { return (uint64_t(t.dwHighDateTime)<<32) | t.dwLowDateTime; }
bool readCpu(CpuTimes& t) {
    FILETIME i,k,u;
    if (!GetSystemTimes(&i,&k,&u)) return false;
    t = {ticks(i),ticks(k),ticks(u)}; return true;
}
bool cpuUsage(CpuTimes a, CpuTimes b, double& value) {
    if (b.idle<a.idle || b.kernel<a.kernel || b.user<a.user) return false;
    uint64_t total=(b.kernel-a.kernel)+(b.user-a.user), idle=b.idle-a.idle;
    if (!total || idle>total) return false;
    value=100.0*double(total-idle)/double(total); return true;
}
DWORD pref(const wchar_t* name, DWORD fallback) {
    DWORD data=0, size=sizeof(data), type=0; HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,kPrefs,0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS) return fallback;
    LONG result=RegQueryValueExW(key,name,0,&type,reinterpret_cast<BYTE*>(&data),&size);
    RegCloseKey(key);
    return result==ERROR_SUCCESS && type==REG_DWORD && size==sizeof(data) ? data : fallback;
}
void save() {
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,kPrefs,0,0,0,KEY_SET_VALUE,0,&key,0)!=ERROR_SUCCESS) return;
    RECT r; GetWindowRect(s.window,&r);
    if(!s.compact) { s.x=r.left; s.y=r.top; }
    DWORD values[]={DWORD(s.x),DWORD(s.y),DWORD(s.compact),DWORD(s.refreshMs),s.textScale,s.anchor,DWORD(s.taskbarOffset),s.colors.cpu,s.colors.ram,s.colors.value,DWORD(s.gpuEnabled),DWORD(s.limitEnabled),DWORD(s.temperatureEnabled)};
    const wchar_t* names[]={L"X",L"Y",L"Taskbar",L"RefreshMs",L"TextScale",L"Anchor",L"TaskbarOffset",L"CpuColor",L"RamColor",L"ValueColor",L"GpuEnabled",L"LimitEnabled",L"TemperatureEnabled"};
    for (int i=0;i<13;i++) RegSetValueExW(key,names[i],0,REG_DWORD,reinterpret_cast<BYTE*>(&values[i]),sizeof(DWORD));
    RegCloseKey(key);
}
COLORREF validColor(DWORD color,COLORREF fallback) {
    if(color>0x00ffffff) return fallback;
    // The background is the transparency key. Keep foreground pixels opaque.
    return color==kBackground ? color^1u : color;
}
void colorsChanged() { save(); InvalidateRect(s.window,0,FALSE); }
void chooseColor(COLORREF& value) {
    // Load the system dialog only on request, then release it after closing.
    wchar_t path[MAX_PATH]; UINT length=GetSystemDirectoryW(path,MAX_PATH);
    if(!length || length+13>=MAX_PATH) return;
    lstrcatW(path,L"\\comdlg32.dll");
    HMODULE library=LoadLibraryW(path);
    typedef BOOL (WINAPI *ColorDialog)(LPCHOOSECOLORW);
    ColorDialog dialog=library?reinterpret_cast<ColorDialog>(GetProcAddress(library,"ChooseColorW")):0;
    if(!dialog) {
        if(library) FreeLibrary(library);
        MessageBoxW(s.window,L"Could not open the Windows color picker.",L"TaskVitals",MB_OK|MB_ICONERROR);
        return;
    }
    static COLORREF custom[16]={};
    CHOOSECOLORW choice={}; choice.lStructSize=sizeof(choice); choice.hwndOwner=s.window;
    choice.rgbResult=value; choice.lpCustColors=custom; choice.Flags=CC_FULLOPEN|CC_RGBINIT;
    if(s.tooltip) SendMessageW(s.tooltip,TTM_POP,0,0);
    BOOL accepted=dialog(&choice);
    FreeLibrary(library);
    if(accepted) { value=validColor(choice.rgbResult,value); colorsChanged(); }
}
bool validRefresh(UINT milliseconds) {
    return milliseconds==1000 || milliseconds==2000 || milliseconds==5000 || milliseconds==10000;
}
bool setRefresh(UINT milliseconds) {
    if(!validRefresh(milliseconds) || !SetTimer(s.window,1,milliseconds,0)) return false;
    s.refreshMs=milliseconds;
    s.ready=readCpu(s.previous);
    save();
    return true;
}
bool startupEnabled() {
    HKEY key; wchar_t value[32768]; DWORD size=sizeof(value),type=0;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,kRun,0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS) return false;
    LONG result=RegQueryValueExW(key,L"TaskVitals",0,&type,reinterpret_cast<BYTE*>(value),&size);
    RegCloseKey(key);
    wchar_t path[32768]; DWORD length=GetModuleFileNameW(0,path,32768);
    if (!length || length>=32768 || result!=ERROR_SUCCESS || type!=REG_SZ || size<sizeof(wchar_t)) return false;
    DWORD end=size/sizeof(wchar_t); if(end>=32768) end=32767;
    value[end]=0;
    wchar_t expected[32768]; if (length+3>=32768) return false;
    expected[0]=L'"'; lstrcpyW(expected+1,path); lstrcatW(expected,L"\"");
    return lstrcmpiW(value,expected)==0;
}
void toggleStartup() {
    bool enabled=startupEnabled(); HKEY key;
    LONG result=RegCreateKeyExW(HKEY_CURRENT_USER,kRun,0,0,0,KEY_SET_VALUE,0,&key,0);
    if (result==ERROR_SUCCESS) {
        if (enabled) result=RegDeleteValueW(key,L"TaskVitals");
        else {
            wchar_t path[32768], command[32768]; DWORD length=GetModuleFileNameW(0,path,32768);
            if (!length || length+3>=32768) result=ERROR_BAD_PATHNAME;
            else { command[0]=L'"'; lstrcpyW(command+1,path); lstrcatW(command,L"\"");
                result=RegSetValueExW(key,L"TaskVitals",0,REG_SZ,reinterpret_cast<BYTE*>(command),DWORD((wcslen(command)+1)*sizeof(wchar_t))); }
        }
        RegCloseKey(key);
    }
    if (result!=ERROR_SUCCESS) MessageBoxW(s.window,L"Could not change startup settings.",L"TaskVitals",MB_OK|MB_ICONERROR);
}
void sample() {
    Column before[5]; int count=columns(before);
    CpuTimes now;
    if (readCpu(now)) { s.haveCpu=s.ready && cpuUsage(s.previous,now,s.cpu); s.previous=now; s.ready=true; }
    else { s.ready=false; s.haveCpu=false; }
    s.memory.dwLength=sizeof(s.memory); s.haveMemory=GlobalMemoryStatusEx(&s.memory)!=FALSE;
    if(s.gpuEnabled) s.haveGpu=gpu.sample(s.gpuUsage);
    if(s.limitEnabled) s.haveLimit=limit.sample(s.limitPercent);
    if(s.temperatureEnabled) s.haveTemperature=readCoreTemp(s.temperature);
    GetLocalTime(&s.sampledAt);
    // Repaint only when a displayed (rounded) value or its availability changes.
    Column after[5]; columns(after);
    for(int i=0;i<count;i++)
        if(before[i].valid!=after[i].valid || (after[i].valid && int(before[i].value+0.5)!=int(after[i].value+0.5))) {
            InvalidateRect(s.window,0,FALSE); break;
        }
}
void fill(HDC dc, RECT r, COLORREF color) { HBRUSH b=CreateSolidBrush(color); FillRect(dc,&r,b); DeleteObject(b); }
void text(HDC dc, const wchar_t* value, RECT r, HFONT font, COLORREF color, UINT flags=DT_LEFT) {
    SelectObject(dc,font); SetTextColor(dc,color);
    DrawTextW(dc,value,-1,&r,flags|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
}
RECT box(int l,int t,int r,int b) { RECT v={px(l),px(t),px(r),px(b)}; return v; }
void updateTooltipRect() {
    if(!s.tooltip) return;
    TOOLINFOW tool={}; tool.cbSize=sizeof(tool); tool.hwnd=s.window; tool.uId=1;
    GetClientRect(s.window,&tool.rect);
    SendMessageW(s.tooltip,TTM_NEWTOOLRECTW,0,reinterpret_cast<LPARAM>(&tool));
}
void buildTooltip() {
    wchar_t cpu[64],memory[160],gpuText[96]=L"",limitText[128]=L"",temperatureText[96]=L"";
    if(s.limitEnabled) {
        if(s.haveLimit) wsprintfW(limitText,L"CPU speed limit: %d%% of maximum (lower when slowed by heat or power saving)\r\n",int(s.limitPercent+0.5));
        else lstrcpyW(limitText,L"CPU speed limit: waiting for samples or counter unavailable\r\n");
    }
    if(s.temperatureEnabled) {
        if(s.haveTemperature) wsprintfW(temperatureText,L"CPU temperature: %d\u00B0C (hottest core, read from Core Temp)\r\n",int(s.temperature+0.5));
        else lstrcpyW(temperatureText,L"CPU temperature: start Core Temp to show it\r\n");
    }
    if(s.gpuEnabled) {
        if(s.haveGpu) wsprintfW(gpuText,L"GPU: %d%% (busiest engine across all GPUs)\r\n",int(s.gpuUsage+0.5));
        else lstrcpyW(gpuText,L"GPU: waiting for samples or counters unavailable\r\n");
    }
    if(s.haveCpu) wsprintfW(cpu,L"CPU: %d%% (sample average)",int(s.cpu+0.5));
    else lstrcpyW(cpu,L"CPU: waiting for a valid sample");
    if(s.haveMemory) {
        const uint64_t gb=uint64_t(1)<<30;
        uint64_t total=s.memory.ullTotalPhys*10/gb;
        uint64_t used=(s.memory.ullTotalPhys-s.memory.ullAvailPhys)*10/gb;
        uint64_t available=s.memory.ullAvailPhys*10/gb;
        wsprintfW(memory,L"RAM: %u.%u / %u.%u GiB used (%u%%)\r\nAvailable RAM: %u.%u GiB",
            UINT(used/10),UINT(used%10),UINT(total/10),UINT(total%10),UINT(s.memory.dwMemoryLoad),UINT(available/10),UINT(available%10));
    } else lstrcpyW(memory,L"RAM: unavailable");
    typedef ULONGLONG (WINAPI *Tick64)();
    // This callback runs only on the UI thread. Constant initialization avoids
    // pulling a C++ thread-safe-static runtime into this otherwise Win32-only app.
    static Tick64 ticks64=0;
    if(!ticks64) ticks64=reinterpret_cast<Tick64>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"GetTickCount64"));
    ULONGLONG minutes=(ticks64?ticks64():GetTickCount())/60000;
    wsprintfW(s.tooltipText,L"%s\r\n%s\r\n%s%s%sUptime: %u days, %u h %u min\r\nSampled: %02u:%02u:%02u | Refresh: %u s\r\nRight-click for options. Ctrl+drag to move.",
        cpu,memory,gpuText,limitText,temperatureText,UINT(minutes/1440),UINT((minutes/60)%24),UINT(minutes%60),UINT(s.sampledAt.wHour),UINT(s.sampledAt.wMinute),UINT(s.sampledAt.wSecond),s.refreshMs/1000);
}
void createTooltip() {
    INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_BAR_CLASSES};
    if(!InitCommonControlsEx(&controls)) return;
    s.tooltip=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,0,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,
        CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,s.window,0,GetModuleHandleW(0),0);
    if(!s.tooltip) return;
    TOOLINFOW tool={}; tool.cbSize=sizeof(tool); tool.hwnd=s.window; tool.uId=1;
    tool.uFlags=TTF_SUBCLASS; tool.lpszText=LPSTR_TEXTCALLBACKW; GetClientRect(s.window,&tool.rect);
    if(!SendMessageW(s.tooltip,TTM_ADDTOOLW,0,reinterpret_cast<LPARAM>(&tool))) { DestroyWindow(s.tooltip); s.tooltip=0; return; }
    SendMessageW(s.tooltip,TTM_SETMAXTIPWIDTH,0,MulDiv(390,s.dpi,96));
    SendMessageW(s.tooltip,TTM_SETDELAYTIME,TTDT_INITIAL,600);
    SendMessageW(s.tooltip,TTM_SETDELAYTIME,TTDT_AUTOPOP,15000);
}
void positionTaskbar() {
    if(!s.compact || s.dragging) return;
    HWND bar=FindWindowW(L"Shell_TrayWnd",0); RECT r;
    if(!bar || !GetWindowRect(bar,&r)) return;
    readTaskbarLayout(bar,r);
    int width=px(compactWidth()),height=px(32);
    int right=r.right-px(12);
    HWND tray=FindWindowExW(bar,0,L"TrayNotifyWnd",0);
    if(tray) { RECT t; GetWindowRect(tray,&t); right=t.left-px(12); }
    int left=r.left+MulDiv(12,s.dpi,96);
    int x=right-width;
    if(s.anchor==1) x=left+(right-left-width)/2;
    else if(s.anchor==2) x=left;
    else if(s.anchor==3) x=r.left+MulDiv(s.taskbarOffset,s.dpi,96);
    int desired=x;
    int y=r.top+(r.bottom-r.top-height)/2;
    if(!layout.valid || !freePosition(left,right,width,desired,layout.occupied,layout.count,x)) {
        // Stay reachable without covering shell controls when full or unreadable.
        x=desired; if(x>r.right-width) x=r.right-width; if(x<r.left) x=r.left;
        MONITORINFO monitor={}; monitor.cbSize=sizeof(monitor);
        if(GetMonitorInfoW(MonitorFromRect(&r,MONITOR_DEFAULTTONEAREST),&monitor) && r.top<=monitor.rcMonitor.top)
            y=r.bottom+MulDiv(6,s.dpi,96);
        else y=r.top-height-MulDiv(6,s.dpi,96);
    }
    RECT current; GetWindowRect(s.window,&current);
    if(current.left!=x || current.top!=y || current.right-current.left!=width || current.bottom-current.top!=height)
        SetWindowPos(s.window,HWND_TOPMOST,x,y,width,height,SWP_NOACTIVATE);
}
// Follow the shell's own full-screen decision, as the taskbar does. The shell
// reports it per session, so only hide for a window on this widget's monitor.
void updateFullscreen() {
    HWND front=GetForegroundWindow();
    bool hide=s.shellFullscreen && front && front!=s.window &&
        MonitorFromWindow(front,MONITOR_DEFAULTTONULL)==MonitorFromWindow(s.window,MONITOR_DEFAULTTONEAREST);
    if(hide==s.hidden) return;
    s.hidden=hide;
    if(hide) {
        if(s.tooltip) SendMessageW(s.tooltip,TTM_POP,0,0);
        ShowWindow(s.window,SW_HIDE);
    } else {
        positionTaskbar();
        SetWindowPos(s.window,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    }
}
// Focus moving to another monitor need not change the shell's state.
void CALLBACK foregroundChanged(HWINEVENTHOOK,DWORD,HWND,LONG,LONG,DWORD,DWORD) { updateFullscreen(); }
void rememberTaskbarOffset(int delta=0) {
    RECT bar,r; HWND taskbar=FindWindowW(L"Shell_TrayWnd",0);
    if(taskbar && GetWindowRect(taskbar,&bar) && GetWindowRect(s.window,&r)) {
        s.taskbarOffset=MulDiv(r.left-bar.left,96,s.dpi)+delta;
        if(s.taskbarOffset<0) s.taskbarOffset=0;
        s.anchor=3;
    }
}
void setMode(bool compact) {
    HWND bar=FindWindowW(L"Shell_TrayWnd",0);
    if(compact && !bar) {
        MessageBoxW(s.window,L"The taskbar is not available right now.",L"TaskVitals",MB_OK|MB_ICONINFORMATION);
        compact=false;
    }
    if(!s.compact) { RECT r; GetWindowRect(s.window,&r); s.x=r.left; s.y=r.top; }
    s.compact=compact;
    ShowWindow(s.window,SW_HIDE);
    if(compact) {
        layout.dirty=true;
        SetWindowLongW(s.window,GWL_STYLE,WS_POPUP);
        SetWindowLongW(s.window,GWL_EXSTYLE,GetWindowLongW(s.window,GWL_EXSTYLE)|WS_EX_LAYERED);
        SetLayeredWindowAttributes(s.window,kBackground,255,LWA_COLORKEY);
        SetWindowLongPtrW(s.window,GWLP_HWNDPARENT,reinterpret_cast<LONG_PTR>(bar));
        positionTaskbar();
    } else {
        SetWindowLongW(s.window,GWL_EXSTYLE,GetWindowLongW(s.window,GWL_EXSTYLE)&~WS_EX_LAYERED);
        SetWindowLongPtrW(s.window,GWLP_HWNDPARENT,0);
        SetWindowLongW(s.window,GWL_STYLE,WS_POPUP);
        SetWindowPos(s.window,HWND_TOPMOST,s.x,s.y,px(desktopWidth()),px(145),SWP_NOACTIVATE|SWP_FRAMECHANGED);
        SendMessageW(s.window,WM_DISPLAYCHANGE,0,0);
    }
    if(!s.hidden) ShowWindow(s.window,SW_SHOWNOACTIVATE);
    InvalidateRect(s.window,0,FALSE);
    save();
}
HFONT font(int size,int weight) { return CreateFontW(-px(size),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI"); }
bool setTextScale(UINT scale) {
    if(scale!=85 && scale!=100 && scale!=120) return false;
    UINT oldScale=s.textScale; s.textScale=scale;
    HFONT regular=font(14,FW_SEMIBOLD),number=font(30,FW_SEMIBOLD),labelFont=font(12,FW_NORMAL);
    if(!regular || !number || !labelFont) {
        if(regular) DeleteObject(regular);
        if(number) DeleteObject(number);
        if(labelFont) DeleteObject(labelFont);
        s.textScale=oldScale; return false;
    }
    if(s.regular) DeleteObject(s.regular);
    if(s.number) DeleteObject(s.number);
    if(s.labelFont) DeleteObject(s.labelFont);
    s.regular=regular; s.number=number; s.labelFont=labelFont;
    if(s.window) {
        if(s.compact) positionTaskbar();
        else {
            SetWindowPos(s.window,0,0,0,px(desktopWidth()),px(145),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
            SendMessageW(s.window,WM_DISPLAYCHANGE,0,0);
        }
        updateTooltipRect(); InvalidateRect(s.window,0,FALSE); save();
    }
    return true;
}
void draw(HDC dc) {
    RECT client; GetClientRect(s.window,&client); fill(dc,client,kBackground); SetBkMode(dc,TRANSPARENT);
    Column list[5]; int count=columns(list);
    if(s.compact) {
        for(int n=0;n<count;n++) {
            int left=n*98;
            if(n) fill(dc,box(left,7,left+1,25),RGB(60,66,78));
            wchar_t number[16];
            if(!list[n].valid) wcscpy(number,L"--");
            // The narrow strip has room for "63°" but not "63°C".
            else wsprintfW(number,list[n].degrees?L"%d\u00B0":L"%d%%",int(list[n].value+0.5));
            text(dc,list[n].label,box(left+8,0,left+51,31),s.labelFont,list[n].accent);
            text(dc,number,box(left+51,0,left+91,31),s.regular,s.colors.value,DT_RIGHT);
        }
        return;
    }
    text(dc,L"TaskVitals",box(14,7,192,33),s.regular,RGB(231,235,241));
    for(int n=0;n<count;n++) {
        int left=12+n*154; fill(dc,box(left,40,left+142,133),RGB(32,37,46));
        int value=int(list[n].value+0.5);
        wchar_t number[32];
        if(!list[n].valid) wcscpy(number,L"--");
        else wsprintfW(number,list[n].degrees?L"%d\u00B0C":L"%d%%",value);
        text(dc,list[n].label,box(left+12,46,left+130,68),s.labelFont,list[n].accent);
        text(dc,number,box(left+12,67,left+130,108),s.number,s.colors.value);
        RECT track=box(left+12,117,left+130,120); fill(dc,track,RGB(54,62,77));
        // Percentages fill to 100%; temperature uses a 0-100 degree scale.
        if(list[n].valid) { track.right=track.left+int((track.right-track.left)*(value>100?100:value)/100); fill(dc,track,list[n].accent); }
    }
}
void paint() {
    PAINTSTRUCT ps; HDC target=BeginPaint(s.window,&ps);
    RECT r; GetClientRect(s.window,&r); HDC buffer=CreateCompatibleDC(target);
    HBITMAP bitmap=CreateCompatibleBitmap(target,r.right,r.bottom);
    if(buffer && bitmap) { HGDIOBJ old=SelectObject(buffer,bitmap); draw(buffer); BitBlt(target,0,0,r.right,r.bottom,buffer,0,0,SRCCOPY); SelectObject(buffer,old); }
    else draw(target);
    if(bitmap) DeleteObject(bitmap);
    if(buffer) DeleteDC(buffer);
    EndPaint(s.window,&ps);
}
void columnsChanged() {
    if(s.compact) positionTaskbar();
    else { SetWindowPos(s.window,0,0,0,px(desktopWidth()),px(145),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE); SendMessageW(s.window,WM_DISPLAYCHANGE,0,0); }
    updateTooltipRect(); save(); InvalidateRect(s.window,0,FALSE);
}
void menu(POINT point) {
    HMENU m=CreatePopupMenu();
    AppendMenuW(m,MF_STRING|(s.compact?MF_CHECKED:0),TaskbarMode,L"Taskbar view");
    AppendMenuW(m,MF_STRING|(s.gpuEnabled?MF_CHECKED:0),ToggleGpu,L"Show GPU (optional)");
    AppendMenuW(m,MF_STRING|(s.limitEnabled?MF_CHECKED:0),ToggleLimit,L"Show CPU speed limit (optional)");
    AppendMenuW(m,MF_STRING|(s.temperatureEnabled?MF_CHECKED:0),ToggleTemperature,L"Show CPU temperature (needs Core Temp)");
    HMENU sizes=CreatePopupMenu();
    const UINT scales[]={85,100,120};
    const wchar_t* sizeLabels[]={L"Small",L"Normal (default)",L"Large"};
    for(int i=0;i<3;i++) AppendMenuW(sizes,MF_STRING|(s.textScale==scales[i]?MF_CHECKED:0),TextSmall+i,sizeLabels[i]);
    AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(sizes),L"Text size");
    HMENU colors=CreatePopupMenu();
    const wchar_t* colorLabels[]={L"Default (mint / lavender)",L"White",L"Ocean (blue / cyan)",L"Warm (amber / coral)",L"Dark text (light taskbar)"};
    for(int i=0;i<5;i++) {
        bool selected=s.colors.cpu==kPalettes[i].cpu && s.colors.ram==kPalettes[i].ram && s.colors.value==kPalettes[i].value;
        AppendMenuW(colors,MF_STRING|(selected?MF_CHECKED:0),ColorsDefault+i,colorLabels[i]);
    }
    AppendMenuW(colors,MF_SEPARATOR,0,0);
    AppendMenuW(colors,MF_STRING,PickCpuColor,L"CPU label and bar...");
    AppendMenuW(colors,MF_STRING,PickRamColor,L"RAM label and bar...");
    AppendMenuW(colors,MF_STRING,PickValueColor,L"Percentage text...");
    AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(colors),L"Colors");
    HMENU positions=CreatePopupMenu();
    const wchar_t* positionLabels[]={L"Right (near notification area)",L"Center of available space",L"Left (after app buttons)",L"Custom (Ctrl+drag)"};
    for(int i=0;i<4;i++) AppendMenuW(positions,MF_STRING|(s.anchor==UINT(i)?MF_CHECKED:0),PositionRight+i,positionLabels[i]);
    AppendMenuW(positions,MF_SEPARATOR,0,0);
    AppendMenuW(positions,MF_STRING,MoveLeft,L"Move left");
    AppendMenuW(positions,MF_STRING,MoveRight,L"Move right");
    AppendMenuW(m,MF_POPUP|(s.compact?0:MF_GRAYED),reinterpret_cast<UINT_PTR>(positions),L"Taskbar position");
    HMENU refresh=CreatePopupMenu();
    const UINT intervals[]={1000,2000,5000,10000};
    const wchar_t* labels[]={L"1 second",L"2 seconds",L"5 seconds (default)",L"10 seconds"};
    for(int i=0;i<4;i++) AppendMenuW(refresh,MF_STRING|(s.refreshMs==intervals[i]?MF_CHECKED:0),Refresh1+i,labels[i]);
    AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(refresh),L"Refresh interval");
    AppendMenuW(m,MF_STRING|(startupEnabled()?MF_CHECKED:0),Startup,L"Start with Windows");
    AppendMenuW(m,MF_SEPARATOR,0,0);
    AppendMenuW(m,MF_STRING,Quit,L"Exit");
    SetForegroundWindow(s.window);
    UINT command=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,s.window,0);
    DestroyMenu(m); if(command) SendMessageW(s.window,WM_COMMAND,command,0);
}
LRESULT CALLBACK windowProc(HWND w,UINT message,WPARAM wp,LPARAM lp) {
    switch(message) {
    case WM_PAINT: paint(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_SIZE: updateTooltipRect(); return 0;
    case WM_NOTIFY: {
        NMHDR* info=reinterpret_cast<NMHDR*>(lp);
        if(info && info->hwndFrom==s.tooltip && info->code==TTN_GETDISPINFOW) {
            buildTooltip(); reinterpret_cast<NMTTDISPINFOW*>(lp)->lpszText=s.tooltipText; return 0;
        }
        break;
    }
    case WM_TIMER:
        if(wp==1) { sample(); positionTaskbar(); updateFullscreen(); }
        else if(wp==2) { KillTimer(w,2); layout.pending=false; positionTaskbar(); }
        return 0;
    case WM_LBUTTONDOWN:
        if(!s.compact || (GetKeyState(VK_CONTROL)&0x8000)) {
            s.dragging=true;
            if(s.tooltip) SendMessageW(s.tooltip,TTM_POP,0,0);
            ReleaseCapture(); SendMessageW(w,WM_NCLBUTTONDOWN,HTCAPTION,0);
            s.dragging=false; positionTaskbar();
        } return 0;
    case WM_CONTEXTMENU: {
        POINT p={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        if(p.x==-1 && p.y==-1) { RECT r; GetWindowRect(w,&r); p.x=r.left+px(15); p.y=r.top+px(30); }
        menu(p); return 0;
    }
    case WM_COMMAND:
        switch(LOWORD(wp)) {
        case Startup: toggleStartup(); break;
        case TaskbarMode: setMode(!s.compact); break;
        case ToggleGpu:
            s.gpuEnabled=!s.gpuEnabled; s.haveGpu=false; s.gpuUsage=0;
            if(s.gpuEnabled) gpu.start(); else gpu.stop();
            columnsChanged(); break;
        case ToggleLimit:
            s.limitEnabled=!s.limitEnabled; s.haveLimit=false; s.limitPercent=0;
            if(s.limitEnabled) s.haveLimit=limit.start() && limit.sample(s.limitPercent); else limit.stop();
            columnsChanged(); break;
        case ToggleTemperature:
            s.temperatureEnabled=!s.temperatureEnabled; s.temperature=0;
            s.haveTemperature=s.temperatureEnabled && readCoreTemp(s.temperature);
            columnsChanged(); break;
        case ColorsDefault: case ColorsWhite: case ColorsOcean: case ColorsWarm: case ColorsDark:
            s.colors=kPalettes[LOWORD(wp)-ColorsDefault]; colorsChanged(); break;
        case PickCpuColor: chooseColor(s.colors.cpu); break;
        case PickRamColor: chooseColor(s.colors.ram); break;
        case PickValueColor: chooseColor(s.colors.value); break;
        case TextSmall: case TextNormal: case TextLarge: {
            const UINT scales[]={85,100,120};
            if(!setTextScale(scales[LOWORD(wp)-TextSmall]))
                MessageBoxW(w,L"Could not change text size.",L"TaskVitals",MB_OK|MB_ICONERROR);
            break;
        }
        case PositionRight: case PositionCenter: case PositionLeft: case PositionCustom:
            if(s.compact) {
                if(LOWORD(wp)==PositionCustom) rememberTaskbarOffset();
                else s.anchor=LOWORD(wp)-PositionRight;
                positionTaskbar(); save();
            } break;
        case MoveLeft: case MoveRight:
            if(s.compact) { rememberTaskbarOffset(LOWORD(wp)==MoveLeft?-12:12); positionTaskbar(); save(); } break;
        case Refresh1: case Refresh2: case Refresh5: case Refresh10: {
            const UINT intervals[]={1000,2000,5000,10000};
            if(!setRefresh(intervals[LOWORD(wp)-Refresh1]))
                MessageBoxW(w,L"Could not change the refresh interval.",L"TaskVitals",MB_OK|MB_ICONERROR);
            break;
        }
        case Quit: DestroyWindow(w); break;
        } return 0;
    case WM_KEYDOWN:
        if(wp==VK_ESCAPE) { DestroyWindow(w); return 0; }
        break;
    case WM_EXITSIZEMOVE:
        if(s.compact) rememberTaskbarOffset();
        save(); return 0;
    case WM_DISPLAYCHANGE: {
        if(s.compact) { positionTaskbar(); return 0; }
        RECT r; GetWindowRect(w,&r); MONITORINFO mi={}; mi.cbSize=sizeof(mi);
        if(GetMonitorInfoW(MonitorFromRect(&r,MONITOR_DEFAULTTONEAREST),&mi)) {
            int x=r.left,y=r.top;
            if(x<mi.rcWork.left) x=mi.rcWork.left;
            if(y<mi.rcWork.top) y=mi.rcWork.top;
            if(x+r.right-r.left>mi.rcWork.right) x=mi.rcWork.right-(r.right-r.left);
            if(y+r.bottom-r.top>mi.rcWork.bottom) y=mi.rcWork.bottom-(r.bottom-r.top);
            SetWindowPos(w,HWND_TOPMOST,x,y,0,0,SWP_NOSIZE|SWP_NOACTIVATE);
        } return 0;
    }
    case kAppBarMessage:
        if(wp==ABN_FULLSCREENAPP) { s.shellFullscreen=lp!=0; updateFullscreen(); }
        return 0;
    case WM_CLOSE: DestroyWindow(w); return 0;
    case WM_DESTROY:
        if(s.appBar) {
            APPBARDATA bar={}; bar.cbSize=sizeof(bar); bar.hWnd=w;
            SHAppBarMessage(ABM_REMOVE,&bar); s.appBar=false;
        }
        if(s.foregroundHook) { UnhookWinEvent(s.foregroundHook); s.foregroundHook=0; }
        gpu.stop(); limit.stop();
        KillTimer(w,1); KillTimer(w,2);
        if(layout.hook) { UnhookWinEvent(layout.hook); layout.hook=0; }
        if(s.tooltip) { DestroyWindow(s.tooltip); s.tooltip=0; }
        save(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(w,message,wp,lp);
}
int selfTest() {
    GpuPdhItem counters[]={
        {const_cast<wchar_t*>(L"pid_1_luid_0x0_0x1_phys_0_eng_0_engtype_3D"),{0,{20.0}}},
        {const_cast<wchar_t*>(L"pid_2_luid_0x0_0x1_phys_0_eng_0_engtype_3D"),{0,{30.0}}},
        {const_cast<wchar_t*>(L"pid_1_luid_0x0_0x1_phys_0_eng_1_engtype_Copy"),{0,{40.0}}},
        {const_cast<wchar_t*>(L"pid_1_luid_0x0_0x2_phys_0_eng_0_engtype_3D"),{1,{80.0}}}
    };
    double usage=0;
    if(!gpuAggregate(counters,4,usage) || usage!=80.0) return 20;
    if(!gpuAggregate(counters,3,usage) || usage!=50.0) return 21;
    counters[0].formatted.value=80;
    if(!gpuAggregate(counters,3,usage) || usage!=100.0) return 22;
    counters[0].formatted.status=0xc0000bc6;
    if(gpuAggregate(counters,1,usage) || gpuAggregate(counters,0,usage)) return 23;
    wchar_t key[96];
    if(gpuEngineKey(L"pid_1_engtype_3D",key)) return 24;
    CoreTempData core={}; double celsius=0;
    if(coreTempHottest(core,celsius)) return 30;
    core.cpuCount=1; core.coreCount=4;
    core.temperature[0]=55; core.temperature[1]=71.5f; core.temperature[2]=60; core.temperature[3]=0;
    if(!coreTempHottest(core,celsius) || celsius!=71.5) return 31;
    core.fahrenheit=1; core.temperature[1]=212;
    if(!coreTempHottest(core,celsius) || celsius!=100) return 32;
    core.fahrenheit=0; core.deltaToTjMax=1; core.tjMax[0]=100;
    core.temperature[0]=40; core.temperature[1]=25; core.temperature[2]=30; core.temperature[3]=100;
    if(!coreTempHottest(core,celsius) || celsius!=75) return 33;
    core.coreCount=257;
    if(coreTempHottest(core,celsius)) return 34;
    int x=0; Span spans[]={{0,540},{1500,1920}};
    if(!freePosition(12,1908,196,12,spans,2,x) || x!=540) return 10;
    if(!freePosition(12,1908,196,1712,spans,2,x) || x!=1304) return 11;
    Span full[]={{0,1920}};
    if(freePosition(12,1908,196,12,full,1,x)) return 12;
    Span overlap[]={{0,500},{450,800},{850,1920}};
    if(freePosition(12,1908,196,12,overlap,3,x)) return 13;
    Span centered[]={{0,150},{650,1250},{1550,1920}};
    const int widths[]={167,196,235,250,294,353};
    for(int i=0;i<6;i++) {
        if(!freePosition(12,1908,widths[i],12,centered,3,x) || x!=150) return 14;
        int expectedRight=(widths[i]<=300?1550:650)-widths[i];
        if(!freePosition(12,1908,widths[i],1712,centered,3,x) || x!=expectedRight) return 15;
        if(!freePosition(12,1908,widths[i],700,centered,3,x) || x+widths[i]>650) return 16;
    }
    Span narrow[]={{0,1200},{1450,1920}};
    if(!freePosition(12,1908,196,12,narrow,2,x) || freePosition(12,1908,294,12,narrow,2,x)) return 17;
    double value=0; CpuTimes a={100,200,100}, b={150,300,200};
    if(!cpuUsage(a,b,value) || value!=75.0) return 1;
    if(cpuUsage(a,a,value)) return 2;
    if(cpuUsage(b,a,value)) return 3;
    if(cpuUsage({0,0,0},{100,10,10},value)) return 4;
    if(!cpuUsage({0,0,0},{100,100,0},value) || value!=0) return 5;
    if(!cpuUsage({0,0,0},{0,50,50},value) || value!=100) return 6;
    CpuTimes live; MEMORYSTATUSEX memory={}; memory.dwLength=sizeof(memory);
    if(!readCpu(live) || !GlobalMemoryStatusEx(&memory) || memory.dwMemoryLoad>100) return 7;
    return 0;
}
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR arguments,int) {
    if(strcmp(arguments,"--self-test")==0) return selfTest();
    HRESULT com=CoInitializeEx(0,COINIT_APARTMENTTHREADED);
    // The widget has no free-text input; the optional RGB dialog uses numbers.
    // Opt out before creating windows, retaining ordinary key/menu input.
    ImmDisableIME(GetCurrentThreadId());
    typedef BOOL (WINAPI *DpiAware)();
    DpiAware dpiAware=reinterpret_cast<DpiAware>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetProcessDPIAware"));
    if(dpiAware) dpiAware();
    HDC screen=GetDC(0); s.dpi=GetDeviceCaps(screen,LOGPIXELSX); ReleaseDC(0,screen);
    HANDLE mutex=CreateMutexW(0,FALSE,L"Local\\TaskVitals.Personal.1");
    DWORD mutexError=GetLastError();
    if(!mutex || mutexError==ERROR_ALREADY_EXISTS) {
        HWND existing=FindWindowW(kClass,0); if(existing) { ShowWindow(existing,SW_SHOWNOACTIVATE); SetWindowPos(existing,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE); }
        if(mutex) CloseHandle(mutex);
        return 0;
    }
    s.textScale=pref(L"TextScale",100);
    if(s.textScale!=85 && s.textScale!=100 && s.textScale!=120) s.textScale=100;
    s.anchor=pref(L"Anchor",0); if(s.anchor>3) s.anchor=0;
    s.gpuEnabled=pref(L"GpuEnabled",0)==1;
    s.limitEnabled=pref(L"LimitEnabled",0)==1;
    s.temperatureEnabled=pref(L"TemperatureEnabled",0)==1;
    s.taskbarOffset=int(pref(L"TaskbarOffset",0));
    if(s.taskbarOffset<0 || s.taskbarOffset>65535) s.taskbarOffset=0;
    s.colors.cpu=validColor(pref(L"CpuColor",kPalettes[0].cpu),kPalettes[0].cpu);
    s.colors.ram=validColor(pref(L"RamColor",kPalettes[0].ram),kPalettes[0].ram);
    s.colors.value=validColor(pref(L"ValueColor",kPalettes[0].value),kPalettes[0].value);
    if(!setTextScale(s.textScale)) { CloseHandle(mutex); return 1; }
    s.refreshMs=pref(L"RefreshMs",5000);
    if(!validRefresh(s.refreshMs)) s.refreshMs=5000;
    WNDCLASSW wc={}; wc.lpfnWndProc=windowProc; wc.hInstance=instance; wc.lpszClassName=kClass;
    wc.hCursor=LoadCursorW(0,IDC_ARROW); wc.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));
    if(!RegisterClassW(&wc)) { CloseHandle(mutex); return 1; }
    RECT work; SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    s.x=int(pref(L"X",DWORD(work.right-px(340)))); s.y=int(pref(L"Y",DWORD(work.top+px(24))));
    s.window=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,kClass,L"TaskVitals",WS_POPUP,s.x,s.y,px(320),px(145),0,0,instance,0);
    if(!s.window) { CloseHandle(mutex); return 1; }
    createTooltip();
    // Register for full-screen notifications only. Without ABM_SETPOS the
    // widget reserves no screen space.
    APPBARDATA bar={}; bar.cbSize=sizeof(bar); bar.hWnd=s.window; bar.uCallbackMessage=kAppBarMessage;
    s.appBar=SHAppBarMessage(ABM_NEW,&bar)!=0;
    // The shell only notifies on changes; ask once in case a full-screen app is
    // already running. Older SDK headers lack this export, so resolve it.
    typedef HRESULT (WINAPI *NotificationState)(int*);
    NotificationState notificationState=reinterpret_cast<NotificationState>(
        GetProcAddress(GetModuleHandleW(L"shell32.dll"),"SHQueryUserNotificationState"));
    int shellState=0; // 2: busy/full-screen, 3: Direct3D full-screen
    if(s.appBar && notificationState && SUCCEEDED(notificationState(&shellState)))
        s.shellFullscreen=shellState==2 || shellState==3;
    updateFullscreen();
    if(s.appBar) s.foregroundHook=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,0,
        foregroundChanged,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
    setMode(pref(L"Taskbar",1)!=0);
    if(s.gpuEnabled) gpu.start();
    if(s.limitEnabled) limit.start();
    sample();
    if(!setRefresh(s.refreshMs)) { DestroyWindow(s.window); CloseHandle(mutex); return 1; }
    if(!s.hidden) ShowWindow(s.window,SW_SHOWNOACTIVATE);
    UpdateWindow(s.window);
    MSG msg; int result;
    while((result=GetMessageW(&msg,0,0,0))>0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    DeleteObject(s.regular); DeleteObject(s.number); DeleteObject(s.labelFont); CloseHandle(mutex);
    if(SUCCEEDED(com)) CoUninitialize();
    return result<0?1:0;
}
