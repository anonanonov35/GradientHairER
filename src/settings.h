#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace hair_gradient {
constexpr int second_color_id=61000, softness_id=61003, position_id=61004;
constexpr unsigned face_size=288, storage_at=0x113, storage_size=9;
struct Settings {
    uint8_t rgb[3]{};
    uint8_t softness=96, position=128;
    bool saved=false;
};
inline bool valid_face(const uint8_t* face) {
    uint32_t version=0,size=0;
    std::memcpy(&version,face+4,4);std::memcpy(&size,face+8,4);
    return !std::memcmp(face,"FACE",4)&&version==4&&size==face_size;
}
inline Settings decode(const uint8_t* face,const uint8_t* primary) {
    Settings s;std::memcpy(s.rgb,primary,3);
    if(valid_face(face)&&!std::memcmp(face+storage_at,"HG1\0",4)) {
        std::memcpy(s.rgb,face+storage_at+4,3);
        s.softness=face[storage_at+7];s.position=face[storage_at+8];s.saved=true;
    }
    return s;
}
inline void encode(uint8_t* nine_bytes,const Settings& s) {
    std::memcpy(nine_bytes,"HG1\0",4);std::memcpy(nine_bytes+4,s.rgb,3);
    nine_bytes[7]=s.softness;nine_bytes[8]=s.position;
}
inline bool owns_id(int id) {return id>=second_color_id&&id<=position_id;}
inline bool change(Settings& s,int id,int value) {
    if(!owns_id(id))return false;
    value=std::clamp(value,0,255);
    if(id<=second_color_id+2)s.rgb[id-second_color_id]=uint8_t(value);
    else if(id==softness_id)s.softness=uint8_t(value);
    else s.position=uint8_t(value);
    s.saved=true;return true;
}
inline unsigned encode_bound(float value) {
    return unsigned(std::lround(std::clamp((value+1.f)*255.75f,0.f,1023.f)));
}
inline float bounds_payload(float low,float high) {
    return float(encode_bound(low)|(encode_bound(high)<<10));
}
inline float controls_payload(const Settings& s) {
    return s.saved?float(65536u+unsigned(s.position)+(unsigned(s.softness)<<8)):0.f;
}
inline float blend(float y,float low,float high,const Settings& s) {
    if(!s.saved)return 0.f;
    float h=(high-y)/std::max(high-low,.01f);
    float width=std::max(s.softness/255.f,1.f/1020.f);
    float x=std::clamp((h-s.position/255.f+width*.5f)/width,0.f,1.f);
    return x*x*(3.f-2.f*x);
}
// Engine ParamValue is a 24-byte tagged variant, not a float[4].
struct ParamValue { uint32_t type=1;float component[5]{}; };
static_assert(sizeof(ParamValue)==24);
static_assert(storage_at>=0x113&&storage_at+storage_size==0x11c);
}
