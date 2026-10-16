#include "../src/menu_patch.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <vector>
#include <algorithm>
using namespace hair_gradient;

static MenuRow row(int command,int face=0) {
    MenuRow r{};r.command=command;r.face=face;r.caption=face+20000;
    r.table=command==2?1201:296118;r.preview=2;
    r.table2=r.reference_face=r.reference_text=r.help=-1;
    return r;
}
int main(int argc,char** argv) {
    if(argc==3) {
        auto f=std::fopen(argv[1],"rb");assert(f);
        std::fseek(f,0,SEEK_END);auto size=std::ftell(f);std::rewind(f);
        assert(size>0&&size%sizeof(MenuRow)==0);
        std::vector<MenuRow> input(size/sizeof(MenuRow)),output(input.size()+3);
        assert(std::fread(input.data(),1,size_t(size),f)==size_t(size));std::fclose(f);
        assert(extend_menu(input.data(),input.size(),output.data(),output.size()));
        f=std::fopen(argv[2],"wb");assert(f);
        assert(std::fwrite(output.data(),sizeof(MenuRow),output.size(),f)==output.size());std::fclose(f);
        std::printf("Extended %zu -> %zu rows\n",input.size(),output.size());
        return 0;
    }
    // Different menu layouts, extra mod rows, no reserved blanks, changed templates.
    for(size_t prefix: {size_t(0),size_t(47),size_t(151)}) {
        for(size_t suffix: {size_t(1),size_t(211),size_t(500)}) {
            std::vector<MenuRow> source(prefix,row(1,123));
            source.push_back(row(2,3068));
            source.back().table=98765;source.back().condition=3;
            source.push_back(row(1,3071));source.back().condition=3;
            source.push_back(row(11));
            for(size_t i=0;i<suffix;i++)source.push_back(row(1,300+int(i)));
            auto original=source;
            std::vector<MenuRow> output(source.size()+3);
            auto plan=plan_menu(source.data(),source.size());
            assert(plan.status==MenuStatus::ready&&plan.color==prefix);
            assert(extend_menu(source.data(),source.size(),output.data(),output.size()));
            assert(std::memcmp(source.data(),original.data(),source.size()*sizeof(MenuRow))==0);
            assert(std::memcmp(output.data(),source.data(),(prefix+1)*sizeof(MenuRow))==0);
            assert(std::memcmp(output.data()+prefix+4,source.data()+prefix+1,(source.size()-prefix-1)*sizeof(MenuRow))==0);
            assert(output[prefix+1].face==61000&&output[prefix+1].table==98765);
            assert(output[prefix+2].face==61003&&output[prefix+3].face==61004);
            assert(output[prefix+1].condition==3&&output[prefix+2].condition==3);
            assert(plan_menu(output.data(),output.size()).status==MenuStatus::existing);
            assert(!extend_menu(output.data(),output.size(),output.data(),output.size()));
            assert(!extend_menu(source.data(),source.size(),output.data(),source.size()+2));
            source.back().face=61002;
            assert(plan_menu(source.data(),source.size()).status==MenuStatus::collision);
            source=original;source.back().caption=9100002;
            assert(plan_menu(source.data(),source.size()).status==MenuStatus::collision);
            source=original;source[prefix+1].face=3072;
            assert(plan_menu(source.data(),source.size()).status==MenuStatus::missing_slider);
            source=original;source.insert(source.begin()+prefix+1,row(11));
            assert(plan_menu(source.data(),source.size()).status==MenuStatus::missing_slider);
            source=original;source.push_back(row(2,3068));
            assert(plan_menu(source.data(),source.size()).status==MenuStatus::ambiguous);
        }
    }
    MenuRow unrelated=row(1,100);
    assert(plan_menu(&unrelated,1).status==MenuStatus::absent);
    assert(plan_menu(nullptr,10).status==MenuStatus::invalid);
    assert(plan_menu(&unrelated,65533).status==MenuStatus::invalid);
    std::puts("PASS: arbitrary layouts, no spare rows, source preservation, native templates, idempotence and conflict refusal");
}
