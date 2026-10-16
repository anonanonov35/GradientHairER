#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "MinHook.h"
#include "../src/hook_route.h"

static bool safe_read(const void* p,void* out,size_t size) {
    SIZE_T n=0;return p&&ReadProcessMemory(GetCurrentProcess(),p,out,size,&n)&&n==size;
}
using Get=int(*)(void*,int);
using Set=void(*)(void*,int,int);
using Apply=void(*)(void*,int,void*,void*);
static Get uma_get_original,gradient_get_original;
static Set uma_set_original,gradient_set_original;
static Apply uma_apply_original,gradient_apply_original;
static volatile int native_get_calls,uma_get_calls,gradient_get_calls;
static int ear_state,gradient_state,trace;
// Keep the 64-byte comparison windows from overlapping an adjacent mock handler.
#define TEST_FUNCTION __declspec(noinline) __attribute__((aligned(128)))
TEST_FUNCTION static int game_get(void*,int id) {native_get_calls++;return id+1;}
TEST_FUNCTION static int uma_get(void* face,int id) {uma_get_calls++;return id==60000?321:uma_get_original(face,id);}
TEST_FUNCTION static int gradient_get(void* face,int id) {gradient_get_calls++;return id==61000?654:gradient_get_original(face,id);}
TEST_FUNCTION static void game_set(void* face,int,int value) {*static_cast<int*>(face)=value;}
TEST_FUNCTION static void uma_set(void* face,int id,int value) {if(id==60020)ear_state=value;else uma_set_original(face,id,value);}
TEST_FUNCTION static void gradient_set(void* face,int id,int value) {if(id==61003)gradient_state=value;else gradient_set_original(face,id,value);}
TEST_FUNCTION static void game_apply(void*,int kind,void*,void*) {trace=trace*10+kind;}
TEST_FUNCTION static void uma_apply(void* face,int kind,void* part,void* over) {uma_apply_original(face,kind,part,over);trace=trace*10+2;}
TEST_FUNCTION static void gradient_apply(void* face,int kind,void* part,void* over) {gradient_apply_original(face,kind,part,over);trace=trace*10+3;}

int main() {
    uint8_t code[128]{};
    code[0]=0xe9;int32_t rel=32-5;std::memcpy(code+1,&rel,4);
    code[32]=0xff;code[33]=0x25;
    uintptr_t dest=reinterpret_cast<uintptr_t>(code+80);std::memcpy(code+38,&dest,8);
    code[80]=0xc3;
    assert(hair_gradient::redirected_to(reinterpret_cast<uintptr_t>(code),safe_read)==dest);
    rel=-5;std::memcpy(code+1,&rel,4);
    assert(!hair_gradient::redirected_to(reinterpret_cast<uintptr_t>(code),safe_read));
    assert(!hair_gradient::redirected_to(1,safe_read));
    assert(!hair_gradient::redirected_to(dest,safe_read));
    assert(MH_Initialize()==MH_OK);
    assert(MH_CreateHook(reinterpret_cast<void*>(game_get),reinterpret_cast<void*>(uma_get),reinterpret_cast<void**>(&uma_get_original))==MH_OK);
    assert(MH_CreateHook(reinterpret_cast<void*>(game_set),reinterpret_cast<void*>(uma_set),reinterpret_cast<void**>(&uma_set_original))==MH_OK);
    assert(MH_CreateHook(reinterpret_cast<void*>(game_apply),reinterpret_cast<void*>(uma_apply),reinterpret_cast<void**>(&uma_apply_original))==MH_OK);
    assert(MH_EnableHook(MH_ALL_HOOKS)==MH_OK);
    uint8_t get_before[64],set_before[64],apply_before[64];
    assert(safe_read(reinterpret_cast<void*>(game_get),get_before,64));
    assert(safe_read(reinterpret_cast<void*>(game_set),set_before,64));
    assert(safe_read(reinterpret_cast<void*>(game_apply),apply_before,64));
    assert(hair_gradient::redirected_to(reinterpret_cast<uintptr_t>(game_get),safe_read)==reinterpret_cast<uintptr_t>(uma_get));
    assert(MH_CreateHook(reinterpret_cast<void*>(uma_get),reinterpret_cast<void*>(gradient_get),reinterpret_cast<void**>(&gradient_get_original))==MH_OK);
    assert(MH_CreateHook(reinterpret_cast<void*>(uma_set),reinterpret_cast<void*>(gradient_set),reinterpret_cast<void**>(&gradient_set_original))==MH_OK);
    assert(MH_CreateHook(reinterpret_cast<void*>(uma_apply),reinterpret_cast<void*>(gradient_apply),reinterpret_cast<void**>(&gradient_apply_original))==MH_OK);
    assert(MH_EnableHook(MH_ALL_HOOKS)==MH_OK);
    Get volatile call_get=game_get;Set volatile call_set=game_set;Apply volatile call_apply=game_apply;
    assert(call_get(nullptr,60000)==321);
    assert(call_get(nullptr,61000)==654);
    assert(call_get(nullptr,3068)==3069);
    assert(gradient_get_calls==3&&uma_get_calls==2&&native_get_calls==1);
    int native_state=0;
    call_set(&native_state,60020,17);call_set(&native_state,61003,91);call_set(&native_state,3071,45);
    assert(native_state==45&&ear_state==17&&gradient_state==91);
    call_apply(nullptr,1,nullptr,nullptr);assert(trace==123);
    assert(!std::memcmp(reinterpret_cast<void*>(game_get),get_before,64));
    assert(!std::memcmp(reinterpret_cast<void*>(game_set),set_before,64));
    assert(!std::memcmp(reinterpret_cast<void*>(game_apply),apply_before,64));
    assert(MH_DisableHook(MH_ALL_HOOKS)==MH_OK&&MH_Uninitialize()==MH_OK);
    std::puts("PASS: real MinHook chain preserves native/ear/gradient IDs, setter delegation, appearance order and existing engine hooks; bounded relay resolver rejects loops/invalid reads");
}
