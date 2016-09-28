//
//  instructions.hpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef instructions_hpp
#define instructions_hpp

#include <stdio.h>
#include "Z80.hpp"

void Step(Z80& proc);
uint8_t cb_prefix_instr(Z80& proc);

uint8_t ld_nn_n(Z80& proc, uint8_t b1);
uint8_t ld_n_nn(Z80& proc, uint8_t b1);
uint8_t ld_a_n(Z80& proc, uint8_t b1);
uint8_t ld_n_a(Z80& proc, uint8_t b1);
uint8_t ld_offs_c_a(Z80& proc);
uint8_t ld_offs_n_a(Z80& proc);

uint8_t ld_hl_dec_a(Z80& proc, uint8_t b1);
uint8_t ld_hl_plus_a(Z80& proc);
uint8_t ldh_a_n(Z80& proc);

uint8_t xor_n(Z80& proc, uint8_t b1);
uint8_t rl_n(Z80& proc, uint8_t b1);
uint8_t rla(Z80& proc);
uint8_t cp_n(Z80& proc, uint8_t b1);

uint8_t inc_n(Z80& proc, uint8_t b1);
uint8_t inc_nn(Z80& proc, uint8_t b1);
uint8_t dec_n(Z80& proc, uint8_t b1);
uint8_t sub_n(Z80& proc, uint8_t b1);

uint8_t bit_b_r(Z80& proc, uint8_t b1);
uint8_t bit_b_hl(Z80& proc, uint8_t b1);

uint8_t jr_cc_n(Z80& proc, uint8_t b1);
uint8_t call_nn(Z80& proc);
uint8_t ret(Z80& proc);
uint8_t jr_n(Z80& proc);

uint8_t push_nn(Z80& proc, uint8_t b1);
uint8_t pop_nn(Z80& proc, uint8_t b1);

#endif /* instructions_hpp */
