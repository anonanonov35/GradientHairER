#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <atomic>
#include "MinHook.h"
#include "settings.h"
#include "native_guards.h"
#include "hair_bounds.h"
#ifdef HAIR_GRADIENT_COMPACT
#include "overlay_guards.h"
#endif

using namespace hair_gradient;
using GetFace=int(*)(void*,int);
using SetFace=void(*)(void*,int,int);
using Apply=void(*)(void*,int,void*,void*);
using Color=void*(*)(void*,void*,unsigned);
using Key=void*(*)(unsigned);
using ShaderSet=void(*)(void*,int,void*,void*,float);
static GetFace get_original;
static SetFace set_original;
static Apply apply_original;
static Color color;
static Key key;
static ShaderSet shader_set;
static HMODULE module;
static FILE* logfile;
static SRWLOCK log_lock=SRWLOCK_INIT;
static std::atomic<unsigned> applies{0},get_observations{0};
#ifdef HAIR_GRADIENT_COMPACT
using RegisterShader=void*(*)(void*,const wchar_t*,const void*,size_t,bool);
static RegisterShader register_shader_original;
struct ShaderPayload {void* data; unsigned size;};
static ShaderPayload shader_payloads[2]{};
static std::atomic<unsigned> shader_replacements{0};
#endif

static void log(const char* message,void* face,int a,int b) {
    AcquireSRWLockExclusive(&log_lock);
    if(logfile) {std::fprintf(logfile,"%s face=%p a=%d b=%d\n",message,face,a,b);std::fflush(logfile);}
    ReleaseSRWLockExclusive(&log_lock);
}
static bool read(const void* address,void* output,size_t size) {
    SIZE_T n=0;return address&&ReadProcessMemory(GetCurrentProcess(),address,output,size,&n)&&n==size;
}
static uintptr_t pointer(uintptr_t address) {
    uintptr_t p=0;read(reinterpret_cast<void*>(address),&p,sizeof(p));return p;
}
#ifdef HAIR_GRADIENT_RUNTIME_MENU
#include "menu_guards.h"
#include "runtime_menu.h"
#endif
#ifdef HAIR_GRADIENT_COMPACT
static bool load_shader_payload(const wchar_t* path,ShaderPayload& payload) {
    auto file=_wfopen(path,L"rb");if(!file)return false;
    uint32_t header[3]{};
    bool valid=std::fread(header,sizeof header,1,file)==1&&header[0]==0x315a4748&&
        header[1]>64&&header[1]<4*1024*1024&&header[2]>0&&header[2]<=header[1];
    if(!valid) {std::fclose(file);return false;}
    auto compressed=static_cast<uint8_t*>(VirtualAlloc(nullptr,header[2],MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    auto unpacked=static_cast<uint8_t*>(VirtualAlloc(nullptr,header[1],MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    valid=compressed&&unpacked&&std::fread(compressed,1,header[2],file)==header[2]&&std::fgetc(file)==EOF;
    std::fclose(file);
    using Decompress=LONG(WINAPI*)(USHORT,PUCHAR,ULONG,PUCHAR,ULONG,PULONG);
    auto decompress=reinterpret_cast<Decompress>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlDecompressBuffer"));
    ULONG size=0;
    valid=valid&&decompress&&decompress(2,unpacked,header[1],compressed,header[2],&size)>=0&&
        size==header[1]&&std::memcmp(unpacked,"BND4",4)==0;
    if(compressed)VirtualFree(compressed,0,MEM_RELEASE);
    if(!valid) {if(unpacked)VirtualFree(unpacked,0,MEM_RELEASE);return false;}
    DWORD previous=0;
    if(!VirtualProtect(unpacked,size,PAGE_READONLY,&previous)) {VirtualFree(unpacked,0,MEM_RELEASE);return false;}
    // Shader repository retains pointers into bundle storage for its full lifetime.
    // Keep these two small immutable buffers until the process exits.
    payload={unpacked,size};return true;
}
static void* register_shader(void* repository,const wchar_t* path,const void* data,size_t size,bool compressed) {
    const wchar_t* name=path;
    if(path)for(auto p=path;*p;p++)if(*p==L'/'||*p==L'\\')name=p+1;
    int index=-1;
    // Native filename helper 0x120f80 removes directory and extension before
    // registration; repository keys are the exact shader family stems.
    if(name&&!_wcsicmp(name,L"P[ChrCustomize][Hair]"))index=0;
    if(name&&!_wcsicmp(name,L"P[ChrCustomize][Hair]_cloth"))index=1;
    if(index>=0&&shader_payloads[index].data) {
        // Replace the initial registration. A later sparse archive loses to the
        // repository's first-registration-wins rule at this exact interface.
        data=shader_payloads[index].data;size=shader_payloads[index].size;compressed=false;
        shader_replacements.fetch_add(1);
        log("compact-shader-replaced",const_cast<void*>(data),index,int(size));
    }
    return register_shader_original(repository,path,data,size,compressed);
}
#endif
static bool snapshot(void* face,uint8_t(&bytes)[face_size]) {
    return read(static_cast<uint8_t*>(face)+8,bytes,face_size)&&valid_face(bytes);
}
static Settings settings(void* face,uint8_t(&bytes)[face_size]) {
    uint8_t primary[3]{};
    if(snapshot(face,bytes)) {std::memcpy(primary,bytes+0xf6,3);return decode(bytes,primary);}
    return {};
}
static int get(void* face,int id) {
    if(!owns_id(id))return get_original(face,id);
    uint8_t bytes[face_size];auto s=settings(face,bytes);
    int result=id<=second_color_id+2?s.rgb[id-second_color_id]:id==softness_id?s.softness:s.position;
    if(get_observations.fetch_add(1)<24)log("gradient-get",face,id,result);
    return result;
}
static void set(void* face,int id,int value) {
    if(!owns_id(id)) {set_original(face,id,value);return;}
    uint8_t bytes[face_size];
    if(!snapshot(face,bytes)) {log("invalid-FACE",face,id,0);return;}
    auto s=decode(bytes,bytes+0xf6);change(s,id,value);
    uint8_t storage[storage_size];encode(storage,s);
    SIZE_T n=0;
    if(!WriteProcessMemory(GetCurrentProcess(),static_cast<uint8_t*>(face)+8+storage_at,storage,sizeof storage,&n)||n!=sizeof storage) {
        log("gradient-store-failed",face,id,GetLastError());return;
    }
    // Request the native appearance refresh while retaining its exact value.
    set_original(face,3071,get_original(face,3071));
    log("gradient-set",face,id,std::clamp(value,0,255));
}
static int hair_model(void* part) {
    auto item=pointer(reinterpret_cast<uintptr_t>(part)+0x10);
    if(!item||pointer(item+0x6d8)!=9)return -1;
    wchar_t name[10]{};
    if(!read(reinterpret_cast<void*>(pointer(item+0x6c8)),name,18)||wcsncmp(name,L"HR_A_",5))return -1;
    int id=0;
    for(int i=5;i<9;i++) {if(name[i]<L'0'||name[i]>L'9')return -1;id=id*10+name[i]-L'0';}
    return id;
}
static void apply(void* face,int kind,void* part,void* override_material) {
    apply_original(face,kind,part,override_material);
    if(kind!=1||!part)return;
    int model=hair_model(part);
    if(model<0)return;
    uint8_t bytes[face_size];
    if(!snapshot(face,bytes))return;
    auto s=decode(bytes,bytes+0xf6);
    ParamValue secondary{};
    if(s.saved) {
        // Native conversion supplies appearance-preset rules and color range.
        // It may write all 24 bytes of ParamValue.
        alignas(16) uint8_t copy[0x160]{};
        if(!read(face,copy,sizeof copy))return;
        std::memcpy(copy+8+0xf6,s.rgb,3);
        std::memset(copy+0x128,0,8);copy[0x154]&=~2;
        color(copy,&secondary,37);
        secondary.component[3]=controls_payload(s);
        secondary.component[4]=1.f;
        ParamValue primary{};color(face,&primary,37);
        float low=1.25f,high=1.8f;
        for(auto b:hair_bounds)if(b.model==model) {low=b.low;high=b.high;break;}
        primary.component[3]=bounds_payload(low,high);
        primary.component[4]=1.f;
        // Filter 0 is the original hair material bank; other parts are untouched.
        shader_set(part,0,key(800040),&primary,1.f);
    }
    // The added material parameter borrows the existing Accessories_Color key.
    // Its alpha is an exact integer payload; RGB uses native sRGB conversion.
    shader_set(part,0,key(800050),&secondary,1.f);
    if(applies.fetch_add(1)<16) {
#ifdef HAIR_GRADIENT_COMPACT
        log("compact-shader-status",nullptr,int(shader_replacements.load()),model);
#endif
        log("gradient-hair-apply",face,model,s.saved);
        log("gradient-controls",face,s.softness,s.position);
        log("gradient-second-rg",face,s.rgb[0],s.rgb[1]);
        log("gradient-second-b",face,s.rgb[2],int(secondary.type));
    }
}
static unsigned hash(const uint8_t* p) {
    unsigned h=2166136261;for(int i=0;i<64;i++)h=(h^p[i])*16777619;return h;
}
static bool matches_interface(const uint8_t* base,unsigned image_size,unsigned rva,unsigned expected) {
    uint8_t bytes[64];
    return rva<=image_size&&image_size-rva>=sizeof bytes&&read(base+rva,bytes,sizeof bytes)&&hash(bytes)==expected;
}
#ifdef HAIR_GRADIENT_UMA_COMPAT
#include "uma_compat.h"
#endif
static DWORD WINAPI start(void*) {
    wchar_t path[32768];DWORD length=GetModuleFileNameW(module,path,32768);
    if(!length||length>=32768)return 0;
    auto slash=wcsrchr(path,L'\\');if(!slash)return 0;
    wcscpy(slash+1,L"hair-gradient.log");logfile=_wfopen(path,L"w");
    auto base=reinterpret_cast<uint8_t*>(GetModuleHandleW(L"eldenring.exe"));
    if(!base||GetModuleHandleW(L"EasyAntiCheat_EOS.dll")||GetModuleHandleW(L"EasyAntiCheat.dll")) {
        log("refused-process",nullptr,0,0);return 0;
    }
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return 0;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
       nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        log("invalid-game-image",nullptr,0,0);return 0;
    }
    // Executable version metadata does not determine compatibility. Check only
    // the actual interfaces used by this DLL, bounded by the current image.
    auto image_size=nt->OptionalHeader.SizeOfImage;
#ifdef HAIR_GRADIENT_RUNTIME_MENU
    if(!matches_interface(base,image_size,menu_span_rva,menu_span_guard)||
       !matches_interface(base,image_size,message_lookup_rva,message_lookup_guard)) {
        log("incompatible-menu-interfaces",nullptr,0,0);return 0;
    }
#endif
    if(
#ifndef HAIR_GRADIENT_UMA_COMPAT
       !matches_interface(base,image_size,get_rva,get_guard)||
       !matches_interface(base,image_size,set_rva,set_guard)||!matches_interface(base,image_size,apply_rva,apply_guard)||
#endif
       !matches_interface(base,image_size,color_rva,color_guard)||!matches_interface(base,image_size,key_rva,key_guard)||
       !matches_interface(base,image_size,shader_set_rva,shader_set_guard)) {
        log("incompatible-appearance-interfaces",nullptr,0,0);return 0;
    }
    color=reinterpret_cast<Color>(base+color_rva);key=reinterpret_cast<Key>(base+key_rva);
    shader_set=reinterpret_cast<ShaderSet>(base+shader_set_rva);
#ifdef HAIR_GRADIENT_COMPACT
    if(!matches_interface(base,image_size,shader_register_rva,shader_register_guard)) {
        log("incompatible-shader-interface",nullptr,0,0);return 0;
    }
    const wchar_t* files[]={L"shaders\\hair.hgz",L"shaders\\hair_cloth.hgz"};
    for(int i=0;i<2;i++) {
        wcscpy(slash+1,files[i]);
        if(!load_shader_payload(path,shader_payloads[i])) {log("compact-payload-invalid",nullptr,i,0);return 0;}
        log("compact-payload-ready",shader_payloads[i].data,i,int(shader_payloads[i].size));
    }
#endif
    if(MH_Initialize()!=MH_OK) {log("MinHook-init-failed",nullptr,0,0);return 0;}
#ifdef HAIR_GRADIENT_RUNTIME_MENU
    auto menu_status=MH_CreateHook(base+menu_span_rva,reinterpret_cast<void*>(menu_span),reinterpret_cast<void**>(&menu_span_original));
    auto message_status=MH_CreateHook(base+message_lookup_rva,reinterpret_cast<void*>(lookup_message),reinterpret_cast<void**>(&message_original));
    if(menu_status!=MH_OK||message_status!=MH_OK) {
        log("runtime-menu-hook-failed",nullptr,menu_status,message_status);MH_Uninitialize();return 0;
    }
#endif
#ifndef HAIR_GRADIENT_UMA_COMPAT
    auto a=MH_CreateHook(base+get_rva,reinterpret_cast<void*>(get),reinterpret_cast<void**>(&get_original));
    auto b=MH_CreateHook(base+set_rva,reinterpret_cast<void*>(set),reinterpret_cast<void**>(&set_original));
    auto c=MH_CreateHook(base+apply_rva,reinterpret_cast<void*>(apply),reinterpret_cast<void**>(&apply_original));
#endif
#ifdef HAIR_GRADIENT_COMPACT
    auto d=MH_CreateHook(base+shader_register_rva,reinterpret_cast<void*>(register_shader),reinterpret_cast<void**>(&register_shader_original));
    if(d!=MH_OK) {log("overlay-hook-create-failed",nullptr,d,0);MH_Uninitialize();return 0;}
#endif
#ifndef HAIR_GRADIENT_UMA_COMPAT
    if(a!=MH_OK||b!=MH_OK||c!=MH_OK) {log("hook-create-failed",nullptr,a,b);log("apply-hook",nullptr,c,0);MH_Uninitialize();return 0;}
    MH_QueueEnableHook(base+get_rva);MH_QueueEnableHook(base+set_rva);MH_QueueEnableHook(base+apply_rva);
#endif
#ifdef HAIR_GRADIENT_RUNTIME_MENU
    MH_QueueEnableHook(base+menu_span_rva);MH_QueueEnableHook(base+message_lookup_rva);
#endif
#ifdef HAIR_GRADIENT_COMPACT
    MH_QueueEnableHook(base+shader_register_rva);
#endif
    auto enabled=MH_ApplyQueued();
#ifdef HAIR_GRADIENT_UMA_COMPAT
    log("early-shader-menu-hooks",nullptr,enabled,0);
    if(enabled!=MH_OK)return 0;
    AppearanceTargets targets{};
    if(!choose_appearance_targets(base,image_size,targets))return 0;
    auto a=MH_CreateHook(targets.get,reinterpret_cast<void*>(get),reinterpret_cast<void**>(&get_original));
    auto b=MH_CreateHook(targets.set,reinterpret_cast<void*>(set),reinterpret_cast<void**>(&set_original));
    auto c=MH_CreateHook(targets.apply,reinterpret_cast<void*>(apply),reinterpret_cast<void**>(&apply_original));
    if(a!=MH_OK||b!=MH_OK||c!=MH_OK) {log("appearance-chain-create-failed",nullptr,a,b);log("apply-chain",nullptr,c,0);return 0;}
    MH_QueueEnableHook(targets.get);MH_QueueEnableHook(targets.set);MH_QueueEnableHook(targets.apply);
    enabled=MH_ApplyQueued();
    log("appearance-hooks",nullptr,enabled,targets.uma?1:0);
    if(targets.uma)log("uma-appearance-chain-ready",nullptr,enabled,3);
#else
    log("appearance-hooks",nullptr,enabled,0);
#endif
#ifdef HAIR_GRADIENT_RUNTIME_MENU
    log("runtime-menu-hooks",nullptr,enabled,1);
#endif
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        module=instance;DisableThreadLibraryCalls(instance);
        auto thread=CreateThread(nullptr,0,start,nullptr,0,nullptr);if(thread)CloseHandle(thread);
    }
    return TRUE;
}
