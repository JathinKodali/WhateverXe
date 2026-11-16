// Opt-in internal-panel isolation informed by diaayousef2-cyber's report:
// https://github.com/b00t0x/WhateverXe/issues/22 (fix 4).
// This selects routing only; it grants no native power/clock/GT admission.
#ifndef GU603_INTERNAL_PANEL_HPP
#define GU603_INTERNAL_PANEL_HPP
#include "kern_display13_output_wiring.hpp"
namespace GU603InternalPanel {
inline bool select(bool requested,uint32_t identity,uint32_t revision,
                   bool explicitWiring,Display13OutputWiring &board) {
    if(!requested || explicitWiring || identity!=0xA7A88086U || revision!=4 ||
       !board.described() || board.panel!=1 || board.dp!=0x18 ||
       board.hdmi || board.dsi || board.selected()!=0x19)return false;
    Display13OutputWiring panel;
    panel.valid=true;panel.panel=1;
    if(!panel.described())return false;
    board=panel;
    return true;
}
}
#endif
