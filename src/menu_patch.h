#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hair_gradient {
// CHARMAKEMENUTOP_PARAM_ST, verified against the installed PARAM and its consumer.
struct MenuRow {
    int32_t command, caption, face, table, condition;
    int8_t preview;
    uint8_t padding[3];
    int32_t table2, reference_face, reference_text, help;
    uint32_t unlock;
    uint8_t tail[4];
};
static_assert(sizeof(MenuRow)==48);
constexpr int gradient_text_id=9100000;
enum class MenuStatus { ready, absent, existing, collision, ambiguous, missing_slider, invalid };
struct MenuPlan { MenuStatus status; size_t color=0, slider=0; };

inline MenuPlan plan_menu(const MenuRow* rows,size_t count) {
    if(!rows||!count||count>65532)return {MenuStatus::invalid};
    size_t found=0,color=0;
    unsigned existing=0;
    bool collision=false;
    for(size_t i=0;i<count;i++) {
        const auto& r=rows[i];
        if(r.command==2&&r.face==3068) {color=i;found++;}
        if(r.face>=61000&&r.face<=61004) {
            unsigned bit=r.face==61000?1:r.face==61003?2:r.face==61004?4:0;
            int index=r.face==61000?0:r.face==61003?1:2;
            if(bit&&(existing&bit)==0&&r.caption==gradient_text_id+index&&
               r.help==gradient_text_id+index&&r.command==(index==0?2:1))existing|=bit;
            else collision=true;
        }
        if(r.caption>=gradient_text_id&&r.caption<gradient_text_id+3&&
           r.face!=61000&&r.face!=61003&&r.face!=61004)collision=true;
    }
    if(collision||(existing&&existing!=7))return {MenuStatus::collision};
    if(existing==7)return {MenuStatus::existing};
    if(!found)return {MenuStatus::absent};
    if(found!=1)return {MenuStatus::ambiguous};
    for(size_t i=color+1;i<count;i++) {
        if(rows[i].command==10||rows[i].command==11)break;
        if(rows[i].command==1&&rows[i].face==3071&&
           rows[i].preview==rows[color].preview&&rows[i].condition==rows[color].condition)
            return {MenuStatus::ready,color,i};
    }
    return {MenuStatus::missing_slider};
}

inline bool extend_menu(const MenuRow* rows,size_t count,MenuRow* output,size_t capacity) {
    auto plan=plan_menu(rows,count);
    if(plan.status!=MenuStatus::ready||!output||capacity<count+3)return false;
    // The game receives a presentation copy; its PARAM file, row IDs and wrapper
    // remain untouched. No spacer rows are consumed or stock row count assumed.
    size_t prefix=plan.color+1;
    std::memcpy(output,rows,prefix*sizeof(MenuRow));
    for(int i=0;i<3;i++) {
        auto& added=output[prefix+i];
        added=rows[i==0?plan.color:plan.slider];
        added.caption=added.help=gradient_text_id+i;
        added.face=i==0?61000:i==1?61003:61004;
        added.reference_face=-1;
        added.reference_text=-1;
    }
    std::memcpy(output+prefix+3,rows+prefix,(count-prefix)*sizeof(MenuRow));
    return true;
}
} // namespace hair_gradient
