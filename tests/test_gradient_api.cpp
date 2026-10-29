#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <vector>
#include <cstring>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "appearance_api.h"
#include "hook_route.h"
#include "native_guards.h"
struct AppearanceTargets {uint8_t* get;uint8_t* set;uint8_t* apply;bool uma;};
static bool read(const void* p,void* out,size_t size){SIZE_T n=0;return p&&ReadProcessMemory(GetCurrentProcess(),p,out,size,&n)&&n==size;}
static void log(const char*,void*,int,int){}
static volatile LONG ready=0;
static int get(void*,int value){return value+11;}
static void set(void*,int,int){}
static void apply(void*,int,void*,void*){}
static uma_cosmetics::AppearanceApiV1 api{40,1,get,set,apply,&ready};
extern "C" __declspec(dllexport) const uma_cosmetics::AppearanceApiV1* UmaAppearanceInterfaceV1(){return &api;}
static DWORD now=0,ready_at=DWORD(-1);
static DWORD fake_tick(){return now;}
static void fake_sleep(DWORD ms){now+=ms;if(now>=ready_at)InterlockedExchange(&ready,1);}
#define GetTickCount fake_tick
#define Sleep fake_sleep
#include "../src/appearance_v1.h"
#undef GetTickCount
#undef Sleep
int main(){
    auto module=reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(module);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(module+dos->e_lfanew);auto size=nt->OptionalHeader.SizeOfImage;
    assert(GetProcAddress(reinterpret_cast<HMODULE>(module),"UmaAppearanceInterfaceV1"));
    std::vector<uint8_t> game(0x280000);unsigned native[]={get_rva,set_rva,apply_rva};uintptr_t targets[]={reinterpret_cast<uintptr_t>(get),reinterpret_cast<uintptr_t>(set),reinterpret_cast<uintptr_t>(apply)};
    for(int i=0;i<3;i++){
        auto entry=game.data()+native[i],relay=game.data()+256+i*32;entry[0]=0xe9;int32_t offset=int32_t(relay-entry-5);std::memcpy(entry+1,&offset,4);
        relay[0]=0xff;relay[1]=0x25;std::memcpy(relay+6,&targets[i],8);
    }
    AppearanceTargets chosen{};ready_at=60;assert(choose_uma_v1(game.data(),module,size,chosen)&&now==60&&chosen.uma);
    assert(chosen.get==reinterpret_cast<uint8_t*>(get)&&chosen.set==reinterpret_cast<uint8_t*>(set)&&chosen.apply==reinterpret_cast<uint8_t*>(apply));
    now=0;ready_at=DWORD(-1);ready=0;chosen={};assert(!choose_uma_v1(game.data(),module,size,chosen)&&now==30000&&!chosen.get);
    ready=1;api.version=2;now=0;assert(!choose_uma_v1(game.data(),module,size,chosen)&&now==0);api.version=1;
    api.get=nullptr;assert(!choose_uma_v1(game.data(),module,size,chosen));api.get=get;
    api.ready=nullptr;assert(!choose_uma_v1(game.data(),module,size,chosen));api.ready=&ready;
    auto byte=game[get_rva];game[get_rva]=0x90;now=0;assert(!choose_uma_v1(game.data(),module,size,chosen)&&now==30000);game[get_rva]=byte;
    now=0;assert(choose_uma_v1(game.data(),module,size,chosen));
    std::puts("PASS: real exported API discovery, image/executable checks, version/pointer refusal, delayed readiness, native route verification and timeout");
}
