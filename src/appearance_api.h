#pragma once
#include <windows.h>
#include <cstdint>
namespace uma_cosmetics {
// Versioned cooperation contract for appearance extensions. Only callbacks in
// this DLL are exposed; the game entrypoints remain guarded and owned by Uma.
struct AppearanceApiV1 {
    uint32_t size,version;
    int (*get)(void*,int);
    void (*set)(void*,int,int);
    void (*apply)(void*,int,void*,void*);
    const volatile LONG* ready;
};
static_assert(sizeof(AppearanceApiV1)==40);
using GetAppearanceApiV1=const AppearanceApiV1*(*)();
} // namespace uma_cosmetics
