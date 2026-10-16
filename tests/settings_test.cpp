#include "../src/settings.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>

using namespace hair_gradient;
int main() {
    uint8_t original[face_size];
    for(unsigned i=0;i<face_size;i++)original[i]=uint8_t((i*79+13)&255);
    std::memcpy(original,"FACE",4);
    uint32_t version=4,size=face_size;
    std::memcpy(original+4,&version,4);std::memcpy(original+8,&size,4);
    uint8_t primary[3]={13,71,239};
    auto defaults=decode(original,primary);
    assert(valid_face(original)&&!defaults.saved&&!std::memcmp(defaults.rgb,primary,3));
    assert(defaults.position==128&&defaults.softness==96);
    assert(blend(1.5f,1.3f,1.8f,defaults)==0);
    for(int id=second_color_id;id<=position_id;id++)for(int value=-1;value<=256;value++) {
        uint8_t face[face_size];std::memcpy(face,original,sizeof face);
        auto s=defaults;assert(change(s,id,value));
        encode(face+storage_at,s);
        auto restored=decode(face,primary);
        assert(restored.saved&&restored.position==s.position&&restored.softness==s.softness);
        assert(!std::memcmp(restored.rgb,s.rgb,3));
        for(unsigned i=0;i<face_size;i++)if(i<storage_at||i>=storage_at+storage_size)assert(face[i]==original[i]);
        // Existing Uma Ears payload and marker are outside this exact span.
        assert(!std::memcmp(face+0x110,original+0x110,3));
        assert(!std::memcmp(face+0x11c,original+0x11c,4));
    }
    assert(!change(defaults,3071,10));
    auto invalid=original[4];original[4]=3;assert(!valid_face(original));original[4]=invalid;
    for(unsigned smooth=0;smooth<256;smooth++)for(unsigned pos=0;pos<256;pos++) {
        Settings s;s.saved=true;s.softness=uint8_t(smooth);s.position=uint8_t(pos);
        auto payload=unsigned(controls_payload(s));
        assert((payload&255)==pos&&((payload>>8)&255)==smooth&&(payload&65536));
        float last=-1;
        for(int n=0;n<=64;n++) {
            float y=1.8f-.5f*n/64.f;
            float b=blend(y,1.3f,1.8f,s);
            assert(std::isfinite(b)&&b>=0&&b<=1&&b>=last-.000001f);last=b;
        }
        float middle=1.8f-.5f*pos/255.f;
        assert(std::abs(blend(middle,1.3f,1.8f,s)-.5f)<.0005f);
    }
    for(int n=0;n<10000;n++) {
        float y=-1.f+4.f*n/10000.f;
        auto encoded=encode_bound(y);
        float decoded=float(encoded)*4.f/1023.f-1.f;
        assert(std::abs(decoded-y)<=2.f/1023.f+.000001f);
        auto packed=unsigned(bounds_payload(y,2.5f));
        assert((packed&1023)==encoded&&(packed>>10)==encode_bound(2.5f));
    }
    Settings sharp;sharp.saved=true;sharp.softness=0;sharp.position=128;
    Settings soft=sharp;soft.softness=255;
    assert(blend(1.65f,1.3f,1.8f,sharp)==0);
    assert(blend(1.45f,1.3f,1.8f,sharp)==1);
    assert(blend(1.65f,1.3f,1.8f,soft)>0&&blend(1.45f,1.3f,1.8f,soft)<1);
    std::puts("PASS: FACE storage, existing ear bytes, clamping, 65536 control payloads, gradient monotonicity/midpoints/endpoints, per-style bounds, 24-byte native variant");
}
