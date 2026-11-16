#include "gu603_pavp_guard.hpp"
#include <cassert>
#include <cstdio>
int main() {
    const uint32_t id=0xA7A88086U;
    const uint64_t vcs2=1ULL<<4,vcs0=1ULL<<3;
    // The captured startup fault: discovery 3, missing protected context, VCS2.
    assert(GU603PAVP::unavailable(true,true,id,4,vcs2,0,3));
    // Subsequent protected operations also report the absent feature.
    for(int command=0;command<=5;++command)
        assert(GU603PAVP::unavailable(true,true,id,4,vcs2|7,0,command));
    // Valid contexts and other hardware/engine configurations retain native handling.
    assert(!GU603PAVP::unavailable(true,true,id,4,vcs2,0x1000,3));
    assert(!GU603PAVP::unavailable(true,true,id,4,vcs2|vcs0,0,3));
    // Inactive media engines cannot provide a protected context either.
    assert(GU603PAVP::unavailable(true,true,id,4,0,0,3));
    assert(!GU603PAVP::unavailable(false,true,id,4,vcs2,0,3));
    assert(!GU603PAVP::unavailable(true,false,id,4,vcs2,0,3));
    assert(!GU603PAVP::unavailable(true,true,0x46A68086,4,vcs2,0,3));
    assert(!GU603PAVP::unavailable(true,true,id,3,vcs2,0,3));
    // Command 6 has a separate non-context path. Unknown commands stay native.
    constexpr int nativeCommands[] {-1,6,7,100};
    for(int command:nativeCommands)
        assert(!GU603PAVP::unavailable(true,true,id,4,vcs2,0,command));
    puts("PASS: captured PAVP fault guarded; valid contexts, command 6 and other profiles stay native");
}
