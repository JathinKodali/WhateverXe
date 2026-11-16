// Mode comparison adapted from nickk19/WhateverXe display13-edp-keep
// (7aef3d04f2f98f60df630799424d019c40d5c8db), under this project's BSD license.
// Preserve the captured panel's actual link format; no receiver-cap/EDID edits.
#ifndef GU603_EDP_KEEP_HPP
#define GU603_EDP_KEEP_HPP
#include <stdint.h>
namespace GU603EDP {
enum Reason : uint32_t {
    Target=1U,Adoption=2U,Readout=4U,Source=8U,PipeOff=16U,
    Image=32U,Format=64U,Blanking=128U,Attachment=256U
};
constexpr uint32_t Blocking=~uint32_t(Blanking);
constexpr uint32_t Registers[] {0x60400,0x70008,0x60000,0x60004,0x60008,0x6000C,0x60010,0x60014,0x6001C};
struct Live {uint32_t func,conf,timing[6],source;};
struct Wanted {uint32_t func,timing[6],source;};
inline uint32_t field(const void *base,uint32_t offset) {
    uint32_t v;__builtin_memcpy(&v,static_cast<const uint8_t *>(base)+offset,4);return v;
}
inline Wanted wanted(const void *params) {
    Wanted w{};w.func=field(params,4);w.source=field(params,0x28);
    for(unsigned i=0;i<6;++i)w.timing[i]=field(params,0x10+4*i);
    return w;
}
inline uint32_t compare(const Live &l,const Wanted &w,uint32_t portLanes) {
    uint32_t reasons=0;
    if((l.conf&0xC0000000U)!=0xC0000000U)reasons|=PipeOff;
    if((l.timing[0]&0x3FFFU)!=(w.timing[0]&0x3FFFU) ||
       (l.timing[3]&0x1FFFU)!=(w.timing[3]&0x1FFFU) || l.source!=w.source)reasons|=Image;
    const auto lanes=((l.func>>1)&7)+1;
    if(!portLanes || portLanes>4 || lanes!=portLanes || ((w.func>>1)&7)+1!=lanes ||
       ((l.func^w.func)&0xFF707000U))reasons|=Format;
    // Like the upstream keep path, retain the firmware's refresh for the
    // same active image. The native partial modeset never reprograms its link.
    for(unsigned i=0;i<6;++i)if(l.timing[i]!=w.timing[i])reasons|=Blanking;
    return reasons;
}
constexpr uint32_t EntryOffset=0x4F044,CallOffset=0x4FCA1,SetModeOffset=0x4F9D4;
constexpr uint8_t Entry[] {0x55,0x48,0x89,0xE5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xEC,0x18};
constexpr uint8_t Call[] {0xE8,0x9E,0xF3,0xFF,0xFF,0x84,0xC0,0x74,0x3D};
}
#endif
