#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <vector>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "native_guards.h"

static HMODULE module=GetModuleHandleW(nullptr);
static std::vector<uint8_t> game(0x280000),uma(0x10000);
static DWORD clock_now,uma_at;
static int required_mode=-1;
static bool handlers_valid=true;
static bool read(const void* p,void* out,size_t size) {
    SIZE_T n=0;return p&&ReadProcessMemory(GetCurrentProcess(),p,out,size,&n)&&n==size;
}
static void log(const char*,void*,int,int) {}
static bool matches_interface(const uint8_t*,unsigned size,unsigned rva,unsigned) {
    return handlers_valid&&rva<=size&&size-rva>=64;
}
static void install_routes();
static DWORD fake_tick() {return clock_now;}
static void fake_sleep(DWORD ms) {clock_now+=ms;if(clock_now>=uma_at)install_routes();}
static UINT fake_ini(LPCWSTR,LPCWSTR,INT,LPCWSTR) {return UINT(required_mode);}
static HMODULE fake_module(LPCWSTR) {return clock_now>=uma_at?reinterpret_cast<HMODULE>(uma.data()):nullptr;}
// These synthetic byte arrays are not PE modules managed by the loader.
// Exercise the legacy branch here; test_gradient_api covers real export lookup.
static FARPROC fake_proc(HMODULE,LPCSTR) {return nullptr;}
#define GetProcAddress fake_proc
#define GetTickCount fake_tick
#define Sleep fake_sleep
#define GetPrivateProfileIntW fake_ini
#define GetModuleHandleW fake_module
#include "../src/uma_compat.h"
#undef GetTickCount
#undef GetProcAddress
#undef Sleep
#undef GetPrivateProfileIntW
#undef GetModuleHandleW

static void install_routes() {
    unsigned native[]={get_rva,set_rva,apply_rva},other[]={uma_get_rva,uma_set_rva,uma_apply_rva};
    for(int i=0;i<3;i++) {
        auto entry=game.data()+native[i],relay=game.data()+0x100+i*32;
        entry[0]=0xe9;int32_t displacement=int32_t(relay-entry-5);std::memcpy(entry+1,&displacement,4);
        relay[0]=0xff;relay[1]=0x25;
        uintptr_t target=reinterpret_cast<uintptr_t>(uma.data()+other[i]);std::memcpy(relay+6,&target,8);
    }
}
static void setup(const char* config,DWORD available,int mode=-1) {
    clock_now=0;uma_at=available;required_mode=mode;handlers_valid=true;
    std::memset(game.data(),0,game.size());std::memset(uma.data(),0,uma.size());
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(uma.data());dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=128;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(uma.data()+128);
    nt->Signature=IMAGE_NT_SIGNATURE;nt->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;nt->OptionalHeader.SizeOfImage=unsigned(uma.size());
    auto f=std::fopen("out/test-fixtures/detection-fixture.toml","wb");assert(f);
    assert(std::fwrite(config,1,std::strlen(config),f)==std::strlen(config));std::fclose(f);
    assert(SetEnvironmentVariableW(L"MODENGINE_CONFIG",L"out/test-fixtures/detection-fixture.toml"));
    if(!available)install_routes();
}
int main() {
    const char* standalone="[modengine]\nexternal_dlls=['mod/HairGradient/hair_gradient.dll']\n";
    const char* combined="[modengine]\nexternal_dlls=['hair_gradient.dll','ersc.dll','Freecam.dll','mod/UmaEars/uma_runtime.dll']\n";
    AppearanceTargets result{};
    setup(standalone,0xffffffffu);
    assert(choose_appearance_targets(game.data(),unsigned(game.size()),result)&&!result.uma);
    assert(result.get==game.data()+get_rva&&result.set==game.data()+set_rva&&result.apply==game.data()+apply_rva);
    assert(clock_now==3000);
    setup(combined,8000);result={};
    assert(choose_appearance_targets(game.data(),unsigned(game.size()),result)&&result.uma);
    assert(clock_now==8000&&result.get==uma.data()+uma_get_rva&&result.apply==uma.data()+uma_apply_rva);
    setup(standalone,0);result={};
    assert(choose_appearance_targets(game.data(),unsigned(game.size()),result)&&result.uma);
    setup(standalone,0xffffffffu,1);result={};
    assert(!choose_appearance_targets(game.data(),unsigned(game.size()),result)&&clock_now==30000);
    setup(combined,0);handlers_valid=false;result={};
    assert(!choose_appearance_targets(game.data(),unsigned(game.size()),result)&&!result.get);
    SetEnvironmentVariableW(L"MODENGINE_CONFIG",nullptr);
    std::puts("PASS: standalone targets, delayed Uma after 8 seconds, preloaded Uma, required-module timeout and incompatible-handler refusal; no game launched");
}
