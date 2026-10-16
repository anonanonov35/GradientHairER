#pragma once
#include <cstdint>
#include <cstring>

namespace hair_gradient {
using SafeRead=bool(*)(const void*,void*,size_t);
// Resolve only the two jump forms used by MinHook x64. Do not treat arbitrary
// code or a failed read as a hook, and never scan unrelated process memory.
inline uintptr_t redirected_to(uintptr_t address,SafeRead read) {
    uintptr_t visited[4]{};
    for(unsigned depth=0;depth<4;depth++) {
        for(unsigned i=0;i<depth;i++)if(visited[i]==address)return 0;
        visited[depth]=address;
        uint8_t bytes[6]{};
        if(!address||!read(reinterpret_cast<void*>(address),bytes,sizeof bytes))return 0;
        if(bytes[0]==0xe9) {
            int32_t relative=0;std::memcpy(&relative,bytes+1,sizeof relative);
            address=address+5+intptr_t(relative);
        } else if(bytes[0]==0xff&&bytes[1]==0x25) {
            int32_t relative=0;std::memcpy(&relative,bytes+2,sizeof relative);
            uintptr_t destination=0;
            if(!read(reinterpret_cast<void*>(address+6+intptr_t(relative)),&destination,sizeof destination))return 0;
            address=destination;
        } else return depth?address:0;
    }
    return 0;
}
} // namespace hair_gradient
