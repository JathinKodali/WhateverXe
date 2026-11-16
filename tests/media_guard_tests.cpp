#include <cassert>
#include <map>
#include <string>
#include "gu603_media_guard.hpp"
struct Service {
    std::map<std::string,std::string> properties;
    void removeProperty(const char *key){properties.erase(key);}
};
int main() {
    using namespace GU603Media;
    assert(scope(true,true,0xA7A88086,4));
    assert(!scope(false,true,0xA7A88086,4));assert(!scope(true,false,0xA7A88086,4));
    assert(!scope(true,true,0x46A68086,4));assert(!scope(true,true,0xA7A88086,5));
    const uint64_t captured=(1ULL<<0)|(1ULL<<1)|(1ULL<<2)|(1ULL<<4)|(1ULL<<5);
    for(unsigned predicates=0;predicates<32;++predicates)
        assert(reject(predicates&1,predicates&2,predicates&4,predicates&8,predicates&16,2,captured)==(predicates==31));
    for(unsigned options=0;options<256;++options)
        assert(reject(true,true,true,true,true,options,captured)==(options==2));
    assert(!reject(true,true,true,true,true,2,captured|(1ULL<<3)));
    assert(!reject(true,true,true,true,true,2,0));
    assert(!reject(true,true,true,true,true,2,captured&~(1ULL<<4)));
    Service s;
    for(auto key:VideoKeys)s.properties[key]="video";
    s.properties["MetalPluginName"]="AppleIntelTGLGraphicsMTLDriver";
    s.properties["IOGLBundleName"]="AppleIntelTGLGraphicsGLDriver";
    s.properties["IOCFPlugInTypes"]="IOAccelerator2D.plugin";
    s.properties["device-id"]="real-hardware";
    s.properties["IOAccelerator"]="attached";
    withdraw(&s);withdraw(&s);
    assert(s.properties.size()==5);
    assert(s.properties.at("MetalPluginName")=="AppleIntelTGLGraphicsMTLDriver");
    assert(s.properties.at("IOGLBundleName")=="AppleIntelTGLGraphicsGLDriver");
    assert(s.properties.at("IOCFPlugInTypes")=="IOAccelerator2D.plugin");
    assert(s.properties.at("device-id")=="real-hardware");
    assert(s.properties.at("IOAccelerator")=="attached");
}
