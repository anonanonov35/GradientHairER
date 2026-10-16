#pragma once
#include <string>
#include <string_view>
#include <cctype>

namespace hair_gradient {
// A bounded lexer for ModEngine's external_dlls string array. This only detects
// a configured module, not a replacement TOML parser or a configuration writer.
inline void skip_config_space(std::string_view text,size_t& at) {
    for(;;) {
        while(at<text.size()&&std::isspace(static_cast<unsigned char>(text[at])))at++;
        if(at>=text.size()||text[at]!='#')return;
        while(at<text.size()&&text[at]!='\n')at++;
    }
}
inline bool config_string(std::string_view text,size_t& at,std::string& value) {
    if(at>=text.size()||(text[at]!='"'&&text[at]!='\''))return false;
    char quote=text[at++];value.clear();
    while(at<text.size()) {
        char c=text[at++];
        if(c==quote)return true;
        if(c=='\n'||c=='\r')return false;
        if(c=='\\'&&quote=='"') {
            if(at>=text.size())return false;
            c=text[at++];
        }
        value.push_back(c);
    }
    return false;
}
inline bool config_requests_uma(std::string_view text) {
    size_t at=text.substr(0,3)=="\xef\xbb\xbf"?3:0;bool modengine=false;
    while(at<text.size()) {
        skip_config_space(text,at);
        if(at==text.size())break;
        if(text[at]=='[') {
            size_t end=text.find(']',at+1);
            if(end==std::string_view::npos)return false;
            auto table=text.substr(at+1,end-at-1);
            while(!table.empty()&&std::isspace(static_cast<unsigned char>(table.front())))table.remove_prefix(1);
            while(!table.empty()&&std::isspace(static_cast<unsigned char>(table.back())))table.remove_suffix(1);
            modengine=table=="modengine"||table=="\"modengine\""||table=="'modengine'";at=end+1;continue;
        }
        std::string key,value;
        if(text[at]=='"'||text[at]=='\'') {
            if(!config_string(text,at,key))return false;
        } else {
            size_t start=at;
            while(at<text.size()&&(std::isalnum(static_cast<unsigned char>(text[at]))||text[at]=='_'||text[at]=='-'))at++;
            key=std::string(text.substr(start,at-start));
        }
        skip_config_space(text,at);
        if(modengine&&key=="external_dlls"&&at<text.size()&&text[at++]=='=') {
            skip_config_space(text,at);
            if(at==text.size()||text[at++]!='[')return false;
            for(;;) {
                skip_config_space(text,at);
                if(at==text.size()||text[at]==']')return false;
                if(!config_string(text,at,value))return false;
                auto sep=value.find_last_of("/\\");
                std::string leaf=value.substr(sep==std::string::npos?0:sep+1);
                for(auto& c:leaf)c=char(std::tolower(static_cast<unsigned char>(c)));
                if(leaf=="uma_runtime.dll")return true;
                skip_config_space(text,at);
                if(at<text.size()&&text[at]==',')at++;
                else return false;
            }
        }
        // Skip unrelated values without confusing their quoted '#', key names
        // or bracket characters with comments/tables.
        while(at<text.size()&&text[at]!='\n') {
            if(text[at]=='"'||text[at]=='\'') {
                if(!config_string(text,at,value))break;
            } else at++;
        }
    }
    return false;
}
} // namespace hair_gradient
