#ifndef GU603_WM_CLOCK_HPP
#define GU603_WM_CLOCK_HPP
namespace GU603WMClock {
// captured includes physical A7A8/rev4 and the exact 64-EU/6-L3 fuse profile.
// Keep the throughput correction opt-in and confined to the adopted panel.
inline bool eligible(bool requested,bool captured,bool adopted,bool bootPanel,
                     bool wiringValid,unsigned selected,unsigned panel,
                     unsigned pipe,unsigned ddi) {
    return requested && captured && adopted && bootPanel && wiringValid &&
        selected==1 && panel==1 && pipe==0 && ddi==0;
}
}
#endif
