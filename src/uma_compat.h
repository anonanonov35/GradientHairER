#pragma once
#include "hook_route.h"
#include "uma_compat_guards.h"
#include "config_detection.h"

struct AppearanceTargets { uint8_t* get; uint8_t* set; uint8_t* apply; bool uma; };

#include "appearance_v1.h"

static bool configured_uma() {
    // The official ModEngine launcher passes its selected config in this
    // environment variable before loading external DLLs. Read it, never edit it.
    wchar_t path[32768]{};
    DWORD length=GetEnvironmentVariableW(L"MODENGINE_CONFIG",path,32768);
    if(!length||length>=32768)return false;
    auto file=_wfopen(path,L"rb");if(!file)return false;
    if(std::fseek(file,0,SEEK_END)!=0) {std::fclose(file);return false;}
    long size=std::ftell(file);
    if(size<=0||size>1024*1024) {std::fclose(file);return false;}
    std::rewind(file);
    std::string text(size,'\0');
    bool okay=std::fread(text.data(),1,text.size(),file)==text.size();
    std::fclose(file);
    return okay&&hair_gradient::config_requests_uma(text);
}

static unsigned image_size_of(uint8_t* base) {
    IMAGE_DOS_HEADER dos{};
    if(!read(base,&dos,sizeof dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0||dos.e_lfanew>0x100000)return 0;
    IMAGE_NT_HEADERS64 nt{};
    if(!read(base+dos.e_lfanew,&nt,sizeof nt)||nt.Signature!=IMAGE_NT_SIGNATURE||
       nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return 0;
    return nt.OptionalHeader.SizeOfImage;
}
static bool choose_appearance_targets(uint8_t* base,unsigned image_size,AppearanceTargets& targets) {
    wchar_t path[32768]{};
    DWORD n=GetModuleFileNameW(module,path,32768);
    if(!n||n>=32768)return false;
    auto slash=wcsrchr(path,L'\\');if(!slash)return false;
    wcscpy(slash+1,L"hair-gradient.ini");
    // -1: automatic detection; 1: this test profile explicitly requires UmaEars.
    int required=GetPrivateProfileIntW(L"compatibility",L"uma_ears",-1,path);
    if(required==-1&&configured_uma()) {
        required=1;
        log("uma-requested-by-modengine-config",nullptr,0,0);
    }
    HMODULE uma=nullptr;
    DWORD module_wait=required==1?30000:3000;
    DWORD started=GetTickCount();
    do {
        uma=GetModuleHandleW(L"uma_runtime.dll");
        if(uma)break;
        Sleep(20);
    } while(GetTickCount()-started<module_wait);
    if(!uma) {
        if(required==1) {log("uma-module-missing",nullptr,0,0);return false;}
        if(!matches_interface(base,image_size,get_rva,get_guard)||
           !matches_interface(base,image_size,set_rva,set_guard)||
           !matches_interface(base,image_size,apply_rva,apply_guard)) {
            log("incompatible-appearance-interfaces",nullptr,0,0);return false;
        }
        targets={base+get_rva,base+set_rva,base+apply_rva,false};
        log("appearance-route-native",nullptr,0,0);return true;
    }
    auto uma_base=reinterpret_cast<uint8_t*>(uma);
    auto uma_size=image_size_of(uma_base);
    if(GetProcAddress(uma,"UmaAppearanceInterfaceV1"))
        return choose_uma_v1(base,uma_base,uma_size,targets);
    if(!matches_interface(uma_base,uma_size,uma_get_rva,uma_get_guard)||
       !matches_interface(uma_base,uma_size,uma_set_rva,uma_set_guard)||
       !matches_interface(uma_base,uma_size,uma_apply_rva,uma_apply_guard)) {
        log("incompatible-uma-handlers",uma,0,0);return false;
    }
    // Let UmaEars validate the pristine engine and finish installing its hooks.
    // Hair shaders/menu hooks have already been installed independently.
    log("uma-waiting-for-appearance-hooks",uma,0,0);
    started=GetTickCount();
    do {
        bool ready=hair_gradient::redirected_to(reinterpret_cast<uintptr_t>(base+get_rva),read)==reinterpret_cast<uintptr_t>(uma_base+uma_get_rva)&&
            hair_gradient::redirected_to(reinterpret_cast<uintptr_t>(base+set_rva),read)==reinterpret_cast<uintptr_t>(uma_base+uma_set_rva)&&
            hair_gradient::redirected_to(reinterpret_cast<uintptr_t>(base+apply_rva),read)==reinterpret_cast<uintptr_t>(uma_base+uma_apply_rva);
        if(ready) {
            // Hook the verified Uma handler bodies, leaving the three engine
            // entrypoints owned by UmaEars. Our trampolines preserve Uma logic.
            if(!matches_interface(uma_base,uma_size,uma_get_rva,uma_get_guard)||
               !matches_interface(uma_base,uma_size,uma_set_rva,uma_set_guard)||
               !matches_interface(uma_base,uma_size,uma_apply_rva,uma_apply_guard))return false;
            targets={uma_base+uma_get_rva,uma_base+uma_set_rva,uma_base+uma_apply_rva,true};
            log("appearance-route-uma-handlers",uma,uma_get_rva,uma_set_rva);
            log("appearance-route-uma-apply",targets.apply,apply_rva,uma_apply_rva);
            return true;
        }
        Sleep(20);
    } while(GetTickCount()-started<30000);
    log("uma-appearance-hooks-not-ready",uma,0,0);return false;
}
