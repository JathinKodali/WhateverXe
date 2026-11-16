// Panel-only clock selection informed by Thatisjigen/WhateverXe,
// pr/edp-handoff-0.7.0 (BSD-3-Clause), upstream issue #38 / issue #35.
// This opt-in is limited to the separately captured GU603 A7A8/rev4 profile.
#ifndef GU603_PANEL_CLOCK_HPP
#define GU603_PANEL_CLOCK_HPP
#include <stdint.h>
namespace GU603PanelClock {
struct State {
    bool requested {}, captured {}, multi {}, owner {}, adopted {};
    uint32_t panel {}, bootPort {}, framebuffers {}, destroying {};
};
inline bool eligible(const State &s) {
    return s.requested && s.captured && s.multi && s.owner && s.adopted &&
        s.bootPort<2 && (s.panel&(1U<<s.bootPort)) && s.framebuffers==1 && !s.destroying;
}
inline bool revalidated(const State &s,bool sameFramebuffer,bool sameMutex,uint32_t active) {
    return eligible(s) && sameFramebuffer && sameMutex && !(active&~1U);
}
}
#endif
