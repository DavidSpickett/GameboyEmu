//
//  Z80.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "Z80.hpp"

uint8_t Z80::fetch_byte() {
    uint8_t ret = mem.read8(pc.read());
    pc.inc(1);
    return ret;
}

std::vector<uint8_t> Z80::fetch_bytes(uint16_t num)
{
    std::vector<uint8_t> ret = mem.read_bytes(pc.read(), num);
    pc.inc(num);
    return ret;
}
