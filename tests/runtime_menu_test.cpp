#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <atomic>
#include <cstdio>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <vector>
#include <thread>
static bool read(const void* address,void* output,size_t size) {
    SIZE_T n=0;
    return address&&ReadProcessMemory(GetCurrentProcess(),address,output,size,&n)&&n==size;
}
static void log(const char*,void*,int,int) {}
#include "../src/runtime_menu.h"
using namespace hair_gradient;
static MenuRow rows[5]{};
static void native_span(MenuSpan* span,int first) {
    span->first=first;span->rows=rows;span->count=5;
    if(first==200) {span->rows=rows+3;span->count=2;}
}
static const wchar_t* native_message(void*,unsigned lang,unsigned bank,int id) {
    if(bank==200&&id==293101)return lang==1?L"Цвет волос":L"Hair Color";
    return L"Unmodified native string";
}
int main() {
    rows[0].command=10;
    rows[1].command=2;rows[1].face=3068;rows[1].table=1201;
    rows[2].command=1;rows[2].face=3071;rows[2].table=296118;
    rows[3].command=11;rows[4].command=99;rows[4].caption=291050;
    MenuRow before[5];std::memcpy(before,rows,sizeof rows);
    menu_span_original=native_span;message_original=native_message;
    auto baseline=native_message(nullptr,0,200,17);
    assert(lookup_message(nullptr,0,200,9100000)==baseline); // Not installed yet.
    MenuSpan a{};menu_span(&a,1);
    assert(a.first==1&&a.count==8&&a.rows!=rows);
    assert(!std::memcmp(before,rows,sizeof rows));
    assert(a.rows[2].face==61000&&a.rows[3].face==61003&&a.rows[4].face==61004);
    assert(a.rows[7].command==99&&a.rows[7].caption==291050);
    MEMORY_BASIC_INFORMATION info{};
    assert(VirtualQuery(a.rows,&info,sizeof info)==sizeof info&&info.Protect==PAGE_READONLY);
    MenuSpan b{};menu_span(&b,1);assert(a.rows==b.rows&&a.count==b.count);
    menu_span(&b,200);assert(b.first==200&&b.count==2&&b.rows==rows+3);
    assert(lookup_message(nullptr,0,200,123)==baseline);
    assert(lookup_message(nullptr,0,10,9100000)==baseline);
    assert(!std::wcscmp(lookup_message(nullptr,0,200,9100000),L"Second Hair Color"));
    assert(!std::wcscmp(lookup_message(nullptr,1,200,9100000),L"Второй цвет волос"));
    assert(std::wcsstr(lookup_message(nullptr,1,201,9100002),L"Положение"));
    // Concurrent constructor requests share a fully built immutable copy.
    std::vector<std::thread> threads;
    for(int i=0;i<8;i++)threads.emplace_back([&] {
        for(int j=0;j<100;j++) {MenuSpan c{};menu_span(&c,1);assert(c.rows==a.rows&&c.count==8);}
    });
    for(auto& t:threads)t.join();
    // A changed loaded table produces a new copy, keeping cached menu pointers valid.
    rows[2].table=123456;
    menu_span(&b,1);
    assert(b.rows!=a.rows&&b.rows[3].table==123456&&a.rows[3].table==296118);
    std::puts("PASS: native span boundary, original pointers/data, final command, immutable lifetime, concurrency, reload and RU/EN message routing");
}
