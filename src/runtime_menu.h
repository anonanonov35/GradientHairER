#pragma once
#include "menu_patch.h"

// The native range constructor (0xd36810) fills fields at +8,+16,+24.
// Its three verified callers ignore the return register and keep this object.
struct MenuSpan { void* vtable; int32_t first,padding; hair_gradient::MenuRow* rows; int32_t count,tail; };
static_assert(sizeof(MenuSpan)==32&&offsetof(MenuSpan,rows)==16&&offsetof(MenuSpan,count)==24);
using GetMenuSpan=void(*)(MenuSpan*,int);
using LookupMessage=const wchar_t*(*)(void*,unsigned,unsigned,int);
static GetMenuSpan menu_span_original;
static LookupMessage message_original;
static SRWLOCK menu_lock=SRWLOCK_INIT;
static std::atomic<unsigned> menu_observations{0},message_observations{0};
static std::atomic<bool> gradient_menu_available{false};
struct MenuCopy {
    MenuCopy* next;
    const hair_gradient::MenuRow* source;
    size_t count;
    uint64_t hash;
    hair_gradient::MenuRow* rows;
};
static MenuCopy* menu_copies;

static uint64_t menu_hash(const void* p,size_t size) {
    uint64_t h=14695981039346656037ull;
    auto bytes=static_cast<const uint8_t*>(p);
    for(size_t i=0;i<size;i++)h=(h^bytes[i])*1099511628211ull;
    return h;
}
static void menu_span(MenuSpan* span,int first) {
    menu_span_original(span,first);
    MenuSpan original{};
    if(!read(span,&original,sizeof original)||!original.rows||original.count<=0||original.count>65532)return;
    size_t count=size_t(original.count),bytes=count*sizeof(hair_gradient::MenuRow);
    // Snapshot on the requesting thread after the native range constructor.
    // Do not race a worker thread against PARAM loading or replace game pointers.
    auto snapshot=static_cast<hair_gradient::MenuRow*>(HeapAlloc(GetProcessHeap(),0,bytes));
    if(!snapshot)return;
    if(!read(original.rows,snapshot,bytes)) {HeapFree(GetProcessHeap(),0,snapshot);return;}
    auto plan=hair_gradient::plan_menu(snapshot,count);
    if(plan.status!=hair_gradient::MenuStatus::ready) {
        if(plan.status==hair_gradient::MenuStatus::existing)gradient_menu_available.store(true);
        if(menu_observations.fetch_add(1)<32)log("runtime-menu-original",original.rows,first,int(plan.status));
        HeapFree(GetProcessHeap(),0,snapshot);return;
    }
    auto digest=menu_hash(snapshot,bytes);
    AcquireSRWLockExclusive(&menu_lock);
    MenuCopy* copy=menu_copies;
    for(;copy;copy=copy->next)if(copy->source==original.rows&&copy->count==count&&copy->hash==digest)break;
    if(!copy) {
        auto output=static_cast<hair_gradient::MenuRow*>(VirtualAlloc(nullptr,(count+3)*sizeof(hair_gradient::MenuRow),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        auto node=static_cast<MenuCopy*>(HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(MenuCopy)));
        bool okay=output&&node&&hair_gradient::extend_menu(snapshot,count,output,count+3);
        DWORD previous=0;
        if(okay)okay=VirtualProtect(output,(count+3)*sizeof(hair_gradient::MenuRow),PAGE_READONLY,&previous)!=0;
        if(okay) {
            *node={menu_copies,original.rows,count,digest,output};
            menu_copies=node;copy=node;
            log("runtime-menu-extended",output,int(count),int(count+3));
            log("runtime-menu-source",original.rows,first,int(plan.color));
        } else {
            if(output)VirtualFree(output,0,MEM_RELEASE);
            if(node)HeapFree(GetProcessHeap(),0,node);
            log("runtime-menu-copy-failed",original.rows,first,GetLastError());
        }
    }
    if(copy) {
        // Keep every copy until process exit: menus/cache objects retain row pointers.
        span->rows=copy->rows;span->count=int32_t(count+3);
        gradient_menu_available.store(true);
    }
    ReleaseSRWLockExclusive(&menu_lock);
    HeapFree(GetProcessHeap(),0,snapshot);
}

static bool russian_menu(void* repository,unsigned language) {
    auto label=message_original(repository,language,200,293101);
    if(!label)return false;
    wchar_t text[64]{};
    // Read only accessible UTF-16 code units, stopping at the terminator.
    for(size_t i=0;i<63;i++) {
        if(!read(label+i,text+i,sizeof(wchar_t))||!text[i])break;
        if(text[i]>=0x400&&text[i]<=0x4ff)return true;
    }
    return false;
}
static const wchar_t* lookup_message(void* repository,unsigned language,unsigned bundle,int id) {
    using namespace hair_gradient;
    // Only our IDs in the two menu banks. All other strings use native lookup.
    if(gradient_menu_available.load()&&(bundle==200||bundle==201)&&id>=gradient_text_id&&id<gradient_text_id+3) {
        static const wchar_t* const labels[2][3]={
            {L"Second Hair Color",L"Gradient Softness",L"Gradient Position"},
            {L"Второй цвет волос",L"Плавность градиента",L"Высота границы"}
        };
        static const wchar_t* const help[2][3]={
            {L"Choose the color toward the tips. The primary hair color remains at the roots.",
             L"Transition softness: 0 for a sharp boundary, 255 for a smooth blend.",
             L"Transition position: 0 near the roots, 255 near the tips."},
            {L"Выберите цвет кончиков волос. Основной цвет остаётся у корней.",
             L"Плавность перехода: 0 — чёткая граница, 255 — мягкий переход.",
             L"Положение перехода: 0 — выше, у корней; 255 — ниже, у кончиков."}
        };
        bool ru=russian_menu(repository,language);
        if(message_observations.fetch_add(1)<24)log("runtime-menu-text",repository,int(bundle),id);
        return bundle==200?labels[ru][id-gradient_text_id]:help[ru][id-gradient_text_id];
    }
    return message_original(repository,language,bundle,id);
}
