#ifndef GU603_PAVP_GUARD_HPP
#define GU603_PAVP_GUARD_HPP
#include <stdint.h>
namespace GU603PAVP {
// This donor creates its PAVP context only for engine bit 3 (VCS0). The
// captured device instead retains bit 4 (VCS2). Discovery command 3 then
// dereferences the absent context. Report the absent feature, never success.
inline bool unavailable(bool requested,bool captured,uint32_t identity,uint8_t revision,
                        uint64_t engines,uintptr_t context,int32_t command) {
    return requested && captured && identity==0xA7A88086U && revision==4 &&
        command>=0 && command<=5 && !context && !(engines&(1ULL<<3));
}
constexpr uint32_t EntryOffset=0x29C50,DiscoveryReturn=0x25D12;
constexpr uint8_t Entry[] {0x55,0x48,0x89,0xE5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xEC,0x88,0,0,0};
constexpr uint32_t CallOffset=0x25CFE;
constexpr uint8_t Call[] {0x4C,0x89,0xEF,0xBE,0x03,0,0,0,0x31,0xD2,0x31,0xC9,0x45,0x31,0xC0,0xE8,0x3E,0x3F,0,0};
constexpr uint32_t FaultOffset=0x29DC0;
constexpr uint8_t Fault[] {0x49,0x8B,0x87,0x78,0x12,0,0,0x48,0x8B,0x80,0xB8,0,0,0};
constexpr uint32_t CreationOffset=0x25C8E;
constexpr uint8_t Creation[] {0x41,0xF6,0x85,0,0x13,0,0,0x08,0x74,0x48,0x49,0x83,0xBD,0x78,0x12,0,0,0,0x75,0x3E};
}
#endif
