"""CPU replay of the captured panel rate and failures under the opt-in policy."""
from pathlib import Path
import subprocess, tempfile
r=Path(__file__).resolve().parents[1]
fixture=(r/'tests/display13_wm.cpp').read_text().split('int main() {')[0]
tests=r'''
#include "gu603_wm_clock.hpp"
int main() {
    for(unsigned bad=0;bad<10;++bad) {
        bool requested=true,captured=true,adopted=true,panelBoot=true,valid=true;
        unsigned selected=1,panel=1,pipe=0,ddi=0;
        switch(bad) {
        case 1:requested=false;break;case 2:captured=false;break;
        case 3:adopted=false;break;case 4:panelBoot=false;break;
        case 5:valid=false;break;case 6:selected=3;break;
        case 7:panel=0;break;case 8:pipe=1;break;case 9:ddi=1;break;
        }
        assert(GU603WMClock::eligible(requested,captured,adopted,panelBoot,
            valid,selected,panel,pipe,ddi)==(bad==0));
    }
    const auto latency=decodeLatency(0x63503300,0x00909090,30);
    auto p=plan(1920,2080,446210000,984,latency,true,true,1200);
    auto c=plan(64,2080,446210000,40,latency,true);
    assert(p.status==Status::Complete && c.status==Status::Complete);
    LatchIO io(p,&c);auto s=io.snapshot(p,&c);
    assert(!clockFits(s,614400000)); // Trial8's actual rejected rate.
    s.twoPixelsPerClock=true;
    assert(clockFits(s,614400000) && clockFits(s,446210000));
    assert(!clockFits(s,446209999) && !clockFits(s,0));
    auto invalid=s;invalid.pipeLineTime=37;assert(!clockFits(invalid,614400000));
    auto faster=p;faster.pixelHz=614400001;invalid=s;invalid.primary=&faster;
    assert(!clockFits(invalid,614400000));
    faster=c;invalid=s;invalid.cursor=&faster;faster.pixelHz=614400001;
    assert(!clockFits(invalid,614400000));
    invalid=s;invalid.pipePixelHz=614400001;
    invalid.pipeLineTime=pipeLineTime(2080,invalid.pipePixelHz);
    assert(!clockFits(invalid,614400000));
    for(unsigned bad=0;bad<13;++bad) {
        LatchIO live(p,&c);
        live.regs[0x60000]=0x081F077F;live.regs[0x6000C]=0x051304AF;
        live.regs[0x70190]=0x04AF077F;live.regs[0x70180]=0x94001008;
        live.regs[0x70080]=0x10000027;
        live.regs[0x7027C]=0x03FF0028;live.regs[0x7017C]=0x00270000;
        auto q=live.snapshot(p,&c);q.twoPixelsPerClock=true;
        switch(bad) {
        case 1:live.frozen=true;break;
        case 2:live.oldSurface=true;break;
        case 3:live.regs[0x70240]^=1;break;
        case 4:live.regs[0x7017C]^=1;break;
        case 5:live.regs[0x7027C]^=1;break;
        case 6:live.regs[0x70190]^=1;break;
        case 7:live.regs[0x70B08]=0x80000000;break;
        case 8:live.changeClock=true;break;
        case 9:live.losePower=true;break;
        case 10:live.regs[0x70180]|=0x8000;break; // CCS still rejected.
        case 11:live.regs[0x70280]=0x80000000;break;
        case 12:live.regs[0x60000]^=0x10000;break;
        }
        const auto result=latch(live,q);
        if((result.status==Status::Complete)!=(bad==0))fprintf(stderr,"case=%u status=%u check=%u reg=%x expected=%x observed=%x\n",bad,unsigned(result.status),unsigned(result.check),result.lastReg,result.expected,result.observed);
        assert((result.status==Status::Complete)==(bad==0));
        if(!bad)assert(result.frameAfter>result.frameBefore);
        assert(live.writes.empty());
    }
    puts("PASS: captured 446.21 MHz panel at 307.2 MHz CDCLK; opt-in scope, boundaries and 12 real latch failures retained");
}
'''
with tempfile.TemporaryDirectory(prefix='gu603-wmclock-') as d:
    p=Path(d);(p/'test.cpp').write_text(fixture+tests)
    subprocess.run(['clang++','-std=c++17','-O1','-g','-fsanitize=address,undefined',
        '-fno-sanitize-recover=all','-I'+str(r/'WhateverXe'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
