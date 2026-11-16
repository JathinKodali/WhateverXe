#include <cassert>
#include <cstring>
#include <initializer_list>
#include "gu603_internal_panel.hpp"
#include "kern_display13_fb_topology.hpp"
int main() {
    Display13OutputWiring original;
    original.valid=true;original.panel=1;original.dp=0x18;original.typeC=0x18;
    assert(original.described());
    assert(Display13FBTopology::multiOutputWiring(original));
    auto w=original;
    assert(GU603InternalPanel::select(true,0xA7A88086,4,false,w));
    assert(w.described() && w.selected()==1 && w.panel==1 && !w.dp && !w.typeC);
    assert(Display13FBTopology::supportedWiring(w,true,false));
    assert(Display13FBTopology::singleOutputWiring(w));
    // Preserve real eDP port and boot-pipe qualification. External or multiple
    // live GOP scanouts are not silently accepted by the narrowed routing.
    assert(Display13FBTopology::bootstrapDDI(w,true,1,1,0x8A000106,true)==0);
    assert(Display13FBTopology::bootstrapDDI(w,true,3,3,0x8A000106,true)==UINT32_MAX);
    assert(Display13FBTopology::bootstrapDDI(w,true,1,1,0xA2000106,true)==UINT32_MAX);
    assert(Display13FBTopology::bootstrapDDI(w,false,1,1,0x8A000106,true)==UINT32_MAX);
    for(unsigned scenario=0;scenario<12;++scenario) {
        w=original;bool requested=true,explicitWiring=false;uint32_t id=0xA7A88086,rev=4;
        switch(scenario) {
        case 0:requested=false;break;case 1:explicitWiring=true;break;
        case 2:id=0x46A68086;break;case 3:rev=0;break;case 4:w.valid=false;break;
        case 5:w.panel=2;break;case 6:w.panel=0;break;case 7:w.dp=0x78;break;
        case 8:w.dsi=1;break;case 9:w.hdmi=2;w.ddc[1]=2;break;
        case 10:w.typeC=0x20;break;case 11:w.ddc[0]=1;break;
        }
        auto before=w;
        assert(!GU603InternalPanel::select(requested,id,rev,explicitWiring,w));
        assert(w.selected()==before.selected() && w.panel==before.panel &&
            w.dp==before.dp && w.typeC==before.typeC && w.valid==before.valid);
        for(unsigned i=0;i<w.Ports;++i)assert(w.ddc[i]==before.ddc[i]);
    }
    // Already narrowed or unrelated single-panel boards are left untouched.
    w=original;w.dp=w.typeC=0;
    assert(!GU603InternalPanel::select(true,0xA7A88086,4,false,w));
}
