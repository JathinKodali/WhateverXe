#ifndef GU603_WAKE_SCOPE_HPP
#define GU603_WAKE_SCOPE_HPP
#include <stdint.h>
namespace GU603WakeScope {
enum class Status : uint32_t {
    Unqualified, Complete, Unavailable, ForeignOwner, Transition, Timeout, FuseMismatch
};
struct Receipt {
    uint32_t version {1};
    Status status {Status::Unqualified};
    uint32_t restoredMask {},request[2] {},ack[2] {},fuses[5] {};
};
// The enclosing configure scope already owns both requests and is the only
// cleanup owner. This routine cannot acquire a new lifetime or release bits.
template<class IO> Receipt ensure(IO &io,bool qualified,const uint32_t baseline[2]) {
    Receipt r;
    if(!qualified || !baseline)return r;
    constexpr uint32_t req[] {0xA188,0xA278},ack[] {0x130044,0xD84};
    uint32_t missing=0;
    for(unsigned d=0;d<2;++d) {
        r.request[d]=io.read(req[d]);r.ack[d]=io.read(ack[d]);
        if(r.request[d]==UINT32_MAX || r.ack[d]==UINT32_MAX) {
            r.status=Status::Unavailable;return r;
        }
        if((r.request[d]&0xFFFEU)!=(baseline[d]&0xFFFEU)) {
            r.status=Status::ForeignOwner;return r;
        }
        if((r.request[d]&1)!=(r.ack[d]&1)) {
            r.status=Status::Transition;return r;
        }
        if(!(r.request[d]&1))missing|=1U<<d;
    }
    // Qualify every domain before the first masked write. In particular, do
    // not repair one domain and only later discover foreign state in another.
    for(unsigned d=0;d<2;++d)if(missing&(1U<<d)) {
        io.write(req[d],0x00010001);
        bool ready=false;
        for(unsigned n=0;n<500;++n) {
            r.ack[d]=io.read(ack[d]);
            if(r.ack[d]==UINT32_MAX)break;
            if(r.ack[d]&1){ready=true;break;}
            io.delay();
        }
        r.request[d]=io.read(req[d]);
        if(!ready || r.request[d]==UINT32_MAX || !(r.request[d]&1) ||
           (r.request[d]&0xFFFEU)!=(baseline[d]&0xFFFEU)) {
            r.status=Status::Timeout;return r;
        }
        r.restoredMask|=1U<<d;
    }
    constexpr uint32_t reg[] {0x9138,0x913C,0x9134,0x9118,0x9140};
    constexpr uint32_t expected[] {1,0xF,0,0x44,0x000E00FB};
    bool valid=true;
    for(unsigned i=0;i<5;++i){r.fuses[i]=io.read(reg[i]);valid=valid && r.fuses[i]==expected[i];}
    // Another callback may have touched a domain while its peer was polling.
    // A successful initial sample is not proof that both holds still exist.
    for(unsigned d=0;d<2;++d) {
        r.request[d]=io.read(req[d]);r.ack[d]=io.read(ack[d]);
        if(r.request[d]==UINT32_MAX || r.ack[d]==UINT32_MAX) {
            r.status=Status::Unavailable;return r;
        }
        if((r.request[d]&0xFFFEU)!=(baseline[d]&0xFFFEU)) {
            r.status=Status::ForeignOwner;return r;
        }
        if(!(r.request[d]&1) || !(r.ack[d]&1)) {
            r.status=Status::Transition;return r;
        }
    }
    r.status=valid?Status::Complete:Status::FuseMismatch;
    return r;
}
}
#endif
