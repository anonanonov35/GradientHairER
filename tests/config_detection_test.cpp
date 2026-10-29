#include "../src/config_detection.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
using hair_gradient::config_requests_uma;
int main(int argc,char** argv) {
    assert(config_requests_uma("[modengine]\nexternal_dlls = [\"mod/HairGradient/hair_gradient.dll\",\"mod/SeamlessCoop/ersc.dll\",\"mod/Freecam.dll\",\"mod/UmaEars/uma_runtime.dll\"]"));
    assert(config_requests_uma("[ modengine ]\r\nexternal_dlls=[\n# note\n'X:\\dir\\UMA_RUNTIME.DLL', # enabled\n]\n"));
    assert(config_requests_uma("[modengine]\n\"external_dlls\" = [\"C:\\\\mods\\\\UmaEars\\\\uma_runtime.dll\"]"));
    assert(config_requests_uma("\xef\xbb\xbf[\"modengine\"]\nexternal_dlls=['uma_runtime.dll']"));
    assert(!config_requests_uma("[modengine]\nexternal_dlls=[\"mod/HairGradient/hair_gradient.dll\"]\n# \"mod/UmaEars/uma_runtime.dll\""));
    assert(!config_requests_uma("[modengine]\n# external_dlls=[\"uma_runtime.dll\"]\nexternal_dlls=[]"));
    assert(!config_requests_uma("[extension.mod_loader]\nexternal_dlls=[\"uma_runtime.dll\"]"));
    assert(!config_requests_uma("[modengine]\nname='external_dlls=[\"uma_runtime.dll\"]'\nexternal_dlls=[]"));
    assert(!config_requests_uma("[modengine]\nexternal_dlls=[\n# 'uma_runtime.dll'\n'not_uma_runtime.dll',\n'uma_runtime.dll.disabled'\n]"));
    assert(!config_requests_uma("[modengine]\nexternal_dlls=['uma_runtime.dll"));
    assert(!config_requests_uma(""));
    if(argc==3) {
        auto f=std::fopen(argv[1],"rb");assert(f);
        std::string text;char buf[4096];size_t n;
        while((n=std::fread(buf,1,sizeof buf,f)))text.append(buf,n);
        std::fclose(f);
        assert(config_requests_uma(text)==(argv[2][0]=='1'));
        std::puts("PASS: actual configuration file matches expected UmaEars mode");
    }
    std::puts("PASS: standalone and delayed Uma configuration detection, literal/basic strings, comments, case, Windows paths, unrelated keys and malformed input");
}
