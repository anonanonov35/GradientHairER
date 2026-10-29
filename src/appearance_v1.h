#pragma once
#include "appearance_api.h"

// Used by the private GradientHair compatibility build. The engine entrypoints
// still belong to Uma; Gradient wraps its advertised callbacks, then forwards.
static bool choose_uma_v1(uint8_t* game,uint8_t* uma,unsigned uma_size,AppearanceTargets& targets) {
    auto inside=[&](const void* p,size_t n){auto address=reinterpret_cast<uintptr_t>(p),start=reinterpret_cast<uintptr_t>(uma);return address>=start&&n<=uma_size&&address-start<=uma_size-n;};
    auto executable=[&](const void* p){MEMORY_BASIC_INFORMATION info{};return inside(p,1)&&VirtualQuery(p,&info,sizeof info)&&info.State==MEM_COMMIT&&(info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))&&!(info.Protect&PAGE_GUARD);};
    auto getter=GetProcAddress(reinterpret_cast<HMODULE>(uma),"UmaAppearanceInterfaceV1");
    if(!getter||!executable(reinterpret_cast<void*>(getter)))return false;
    auto api=reinterpret_cast<uma_cosmetics::GetAppearanceApiV1>(getter)();
    uma_cosmetics::AppearanceApiV1 value{};
    if(!inside(api,sizeof value)||!read(api,&value,sizeof value)||value.version!=1||value.size<sizeof value||
       !executable(reinterpret_cast<void*>(value.get))||!executable(reinterpret_cast<void*>(value.set))||!executable(reinterpret_cast<void*>(value.apply))||!inside(const_cast<const LONG*>(value.ready),sizeof(LONG))) {
        log("incompatible-uma-api-v1",uma,0,0);return false;
    }
    log("uma-waiting-for-api-v1",uma,0,0);DWORD started=GetTickCount();
    do {
        LONG ready=0;
        bool routed=read(const_cast<const LONG*>(value.ready),&ready,sizeof ready)&&ready==1&&
            hair_gradient::redirected_to(reinterpret_cast<uintptr_t>(game+get_rva),read)==reinterpret_cast<uintptr_t>(value.get)&&
            hair_gradient::redirected_to(reinterpret_cast<uintptr_t>(game+set_rva),read)==reinterpret_cast<uintptr_t>(value.set)&&
            hair_gradient::redirected_to(reinterpret_cast<uintptr_t>(game+apply_rva),read)==reinterpret_cast<uintptr_t>(value.apply);
        if(routed) {
            targets={reinterpret_cast<uint8_t*>(value.get),reinterpret_cast<uint8_t*>(value.set),reinterpret_cast<uint8_t*>(value.apply),true};
            log("appearance-route-uma-api-v1",uma,1,int(value.size));return true;
        }
        Sleep(20);
    } while(GetTickCount()-started<30000);
    log("uma-api-v1-not-ready",uma,0,0);return false;
}
