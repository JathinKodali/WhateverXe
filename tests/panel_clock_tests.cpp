#include "gu603_panel_clock.hpp"
#include <cassert>
#include <initializer_list>
int main() {
    using namespace GU603PanelClock;
    const State panel{true,true,true,true,true,1,0,1,0};
    assert(eligible(panel));
    assert(revalidated(panel,true,true,0)); // Cold modeset, all pipes off.
    assert(revalidated(panel,true,true,1)); // Existing panel WM checks still required by caller.
    for(unsigned active=2;active<8;++active)assert(!revalidated(panel,true,true,active));
    for(unsigned mask=0;mask<8;++mask) {auto s=panel;s.framebuffers=mask;assert(eligible(s)==(mask==1));}
    for(unsigned mask=1;mask<8;++mask) {auto s=panel;s.destroying=mask;assert(!eligible(s));}
    for(unsigned port: {2U,3U,6U,31U,32U,0xffffffffU}) {auto s=panel;s.bootPort=port;assert(!eligible(s));}
    auto b=panel;b.bootPort=1;b.panel=2;assert(eligible(b));b.panel=1;assert(!eligible(b));
    for(unsigned i=0;i<5;++i) {
        auto s=panel;
        switch(i){case 0:s.requested=false;break;case 1:s.captured=false;break;
        case 2:s.multi=false;break;case 3:s.owner=false;break;case 4:s.adopted=false;break;}
        assert(!eligible(s));
    }
    // Recheck after dropping c.lock to acquire the real framebuffer mutex:
    // a new peer, a replaced owner/mutex, or changed panel must not be admitted.
    auto changed=panel;changed.framebuffers=3;assert(!revalidated(changed,true,true,0));
    changed=panel;changed.panel=0;assert(!revalidated(changed,true,true,0));
    assert(!revalidated(panel,false,true,0));assert(!revalidated(panel,true,false,0));
}
