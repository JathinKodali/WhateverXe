#include "gu603_wake_scope.hpp"
#include <cassert>
#include <cstdio>
#include <map>
#include <vector>
struct IO {
    std::map<unsigned,unsigned> r {{0xA188,1},{0x130044,1},{0xA278,1},{0xD84,1},
        {0x9138,1},{0x913C,15},{0x9134,0},{0x9118,0x44},{0x9140,0xE00FB}};
    std::vector<unsigned> writes;
    bool timeout=false,dropGTOnDelay=false,foreignGTOnDelay=false,lateRenderAck=false;
    unsigned delays=0;
    unsigned read(unsigned reg){return r.at(reg);}
    void write(unsigned reg,unsigned value){
        assert((reg==0xA188 || reg==0xA278) && value==0x10001);
        writes.push_back(reg);r[reg]|=1;
        if(!timeout && !(lateRenderAck && reg==0xA278))r[reg==0xA188?0x130044:0xD84]|=1;
    }
    void delay(){
        ++delays;
        if(dropGTOnDelay)r[0xA188]=r[0x130044]=0;
        if(foreignGTOnDelay)r[0xA188]|=2;
        if(lateRenderAck)r[0xD84]|=1;
    }
};
int main(){
    using namespace GU603WakeScope;const unsigned baseline[2] {};
    {IO io;auto r=ensure(io,true,baseline);assert(r.status==Status::Complete && !r.restoredMask && io.writes.empty());}
    for(unsigned mask=1;mask<4;++mask){
        IO io;if(mask&1)io.r[0xA188]=io.r[0x130044]=0;
        if(mask&2)io.r[0xA278]=io.r[0xD84]=0;
        auto r=ensure(io,true,baseline);assert(r.status==Status::Complete && r.restoredMask==mask);
        assert(io.writes.size()==unsigned(__builtin_popcount(mask)));
    }
    {IO io;io.r[0xA188]=io.r[0x130044]=0;io.r[0xA278]=3;
        assert(ensure(io,true,baseline).status==Status::ForeignOwner && io.writes.empty());}
    for(unsigned state=1;state<3;++state){IO io;io.r[0xA188]=state&1;io.r[0x130044]=(state>>1)&1;
        assert(ensure(io,true,baseline).status==Status::Transition && io.writes.empty());}
    {IO io;assert(ensure(io,false,baseline).status==Status::Unqualified && io.writes.empty());}
    {IO io;assert(ensure(io,true,nullptr).status==Status::Unqualified && io.writes.empty());}
    {IO io;io.r[0xA188]=io.r[0x130044]=0;io.timeout=true;
        auto r=ensure(io,true,baseline);assert(r.status==Status::Timeout && io.delays==500 && io.writes.size()==1 && !r.restoredMask);}
    {IO io;io.r[0x9140]=0xE00FE;assert(ensure(io,true,baseline).status==Status::FuseMismatch && io.writes.empty());}
    {IO io;io.r[0xA278]=~0U;assert(ensure(io,true,baseline).status==Status::Unavailable && io.writes.empty());}
    {IO io;const unsigned foreign[2]={2,4};io.r[0xA188]=3;io.r[0xA278]=5;
        assert(ensure(io,true,foreign).status==Status::Complete && io.writes.empty());}
    {IO io;io.r[0xA278]=io.r[0xD84]=0;io.lateRenderAck=true;io.dropGTOnDelay=true;
        assert(ensure(io,true,baseline).status==Status::Transition);}
    {IO io;io.r[0xA278]=io.r[0xD84]=0;io.lateRenderAck=true;io.foreignGTOnDelay=true;
        assert(ensure(io,true,baseline).status==Status::ForeignOwner);}
    puts("PASS: intact holds, each/both lost, foreign ownership, transitions, qualification, timeout, exact fuses, masked writes");
}
