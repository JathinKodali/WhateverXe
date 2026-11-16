#ifndef GU603_MEDIA_GUARD_HPP
#define GU603_MEDIA_GUARD_HPP
#include <stdint.h>
namespace GU603Media {
inline bool scope(bool requested,bool captured,uint32_t identity,uint8_t revision) {
    return requested && captured && identity==0xA7A88086U && revision==4;
}
inline bool reject(bool scoped,bool ready,bool caller,bool parameters,bool owner,
                   uint8_t options,uint64_t engines) {
    return scoped && ready && caller && parameters && owner && options==2 &&
        !(engines&(1ULL<<3)) && (engines&(1ULL<<4));
}
// Video-only registry admission, informed by WhateverXe issue #23. No Metal,
// GL, 2D plug-in, framebuffer, scheduler or physical engine capability edits.
constexpr const char *VideoKeys[]={"IOGVAScaler","IOGVACodec","IOGVABGRAEnc",
    "IOGVAHEVCDecodeCapabilities","IOGVAHEVCEncodeCapabilities",
    "IODVDBundleName","IOVARendererID","IOGVAHEVCDecode"};
template<class Service>void withdraw(Service *service) {
    for(auto key:VideoKeys)service->removeProperty(key);
}
// Exact donor call site, preset, null-result return and panic instruction.
constexpr uint32_t ProcessOffset=0x40C3C,CallOffset=0x40C6F,CallerOffset=0x40C83;
constexpr uint32_t ParamsOffset=0xBE8A0,FailureOffset=0x40D49,BindOffset=0x389F8,FaultOffset=0x3A210;
constexpr uint8_t Params[]={3,0,8,0,1,0,0,0,0,0,0,0};
constexpr uint8_t Call[]={0x48,0x8D,0x35,0x2A,0xDC,7,0,0x48,0x89,0xC7,0xBA,2,0,0,0,0xE8,0xF3,0xAC,3,0};
constexpr uint8_t Return[]={0x48,0x89,0x83,0x70,0x13,0,0,0x48,0x85,0xC0,0x0F,0x84,0xB6,0,0,0};
constexpr uint8_t Failure[]={0xB8,0,0,0,0x80,0xEB,0xF0};
constexpr uint8_t Bind[]={0x40,0x0F,0xB6,0xC6,0x48,0x8B,0xBC,0xC7,0xA0,0x0C,0,0,0x89,0xD6,0xBA,1,0,0,0,0xE8,0,0x18,0,0,0xB0,1,0x5D,0xC3};
constexpr uint8_t Fault[]={0x55,0x48,0x89,0xE5,0x48,0x63,0xCE,0x8B,0x84,0x8F,0x80,2,0,0};
}
#endif
