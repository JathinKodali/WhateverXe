#include "gu603_edp_keep.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
 using namespace GU603EDP;
 Live live{};live.func=0x8A000106;live.conf=0xC0000000;
 live.source=0x077F04AF;live.timing[0]=0x081F077F;live.timing[3]=0x04D304AF;
 Wanted want{};want.func=live.func;want.source=live.source;
 memcpy(want.timing,live.timing,sizeof(want.timing));
 assert(compare(live,want,4)==0);
 auto changed=want;changed.source=0x063F04AF;assert(compare(live,changed,4)&Image);
 changed=want;changed.timing[0]^=1;assert(compare(live,changed,4)&Image);
 changed=want;changed.timing[3]^=1;assert(compare(live,changed,4)&Image);
 changed=want;changed.func^=0x00100000;assert(compare(live,changed,4)&Format);
 changed=want;changed.func^=1U<<27;assert(compare(live,changed,4)&Format);
 changed=want;changed.func^=1U<<24;assert(compare(live,changed,4)&Format);
 changed=want;changed.func^=1U<<31;assert(compare(live,changed,4)&Format);
 changed=want;changed.func^=1U<<12;assert(compare(live,changed,4)&Format);
 changed=want;changed.func=(changed.func&~0xEU)|2;assert(compare(live,changed,4)&Format);
 assert(compare(live,want,2)&Format);assert(compare(live,want,0)&Format);assert(compare(live,want,5)&Format);
 auto off=live;off.conf=0x80000000;assert(compare(off,want,4)&PipeOff);
 changed=want;changed.timing[0]^=1U<<16;
 assert(compare(live,changed,4)==Blanking && !(compare(live,changed,4)&Blocking));
 // Unqualified owner/path/source/readout/attachment is never allowed to keep.
 constexpr Reason blocking[] {Target,Adoption,Readout,Source,PipeOff,Image,Format,Attachment};
 for(auto reason:blocking)assert(reason&Blocking);
 uint8_t raw[0x30] {};memcpy(raw+4,&want.func,4);memcpy(raw+0x10,want.timing,24);memcpy(raw+0x28,&want.source,4);
 const auto parsed=wanted(raw);assert(parsed.func==want.func && parsed.source==want.source && !memcmp(parsed.timing,want.timing,24));
 puts("PASS: four-lane matching image kept; wrong image/format, disabled pipe and unqualified state rejected");
}
