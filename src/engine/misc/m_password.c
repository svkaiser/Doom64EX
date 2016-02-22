// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1997 Midway Home Entertainment, Inc
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//        Password system
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "d_englsh.h"
#include "i_swap.h"
#include "con_console.h"
#include "m_password.h"

//
// PASSWORD STUFF
//

byte passwordData[16];
dboolean doPassword = false;
const char *passwordChar = "bcdfghjklmnpqrstvwxyz0123456789?";
static const int passwordTable[10] = { 1, 8, 9, 5, 6, 2, 7, 0, 4, 3 };

CVAR_EXTERNAL(p_features);

//
// M_EncodePassItem
//

static void M_EncodePassItem(int *bit, int value, int maxvalue) {
    value = (value << 3);
    *bit = value / maxvalue;

    if((value % maxvalue) != 0) {
        *bit = *bit + 1;
    }
}

//
// M_CheckPassCode
//

static dboolean M_CheckPassCode(int *bit1, int *bit2, int *bit3, byte *encode) {
    byte checkByte = 0;
    if(*bit1 < 0) {
        *bit2 = (*bit1 + 7) >> 3;
    }
    else {
        *bit2 = (*bit1 >> 3);
    }

    *bit3 = (*bit1 & 7);

    if(*bit1 < 0) {
        if(*bit3 != 0) {
            *bit3 -= 7;
        }
    }

    checkByte = *(byte*)(encode + *bit2);

    if((checkByte & (0x80 >> *bit3)) != 0) {
        return true;
    }

    return false;
}

//
// M_EncodePassword
//

void M_EncodePassword(void) {
    byte encode[10];
    int i;
    int bit = 0;
    short decodebit[3];
    int passBit = 0;
    int xbit1 = 8;
    int xbit2 = 0;
    int xbit3 = 0;
    player_t *player;

    dmemset(encode, 0, 10);
    dmemset(passwordData, 0, 16);
    player = &players[consoleplayer];

    // map and skill

    encode[0] = ((((nextmap & 0x3f) << 2) & 0xff) | (gameskill & 3));

    // weapons

    for(i = 0; i < NUMWEAPONS; i++) {
        if(i != wp_fist && i != wp_pistol) {
            if(player->weaponowned[i]) {
                encode[1] |= (1 << bit);
                encode[1] = encode[1] & 0xff;
            }

            bit++;
        }
    }

    // backpack

    if(player->backpack) {
        encode[5] |= 0x80;
    }

    bit = 0;

    // clip

    M_EncodePassItem(&bit, player->ammo[am_clip], player->maxammo[am_clip]);
    encode[2] = (bit << 4) & 0xff;

    // shell

    M_EncodePassItem(&bit, player->ammo[am_shell], player->maxammo[am_shell]);
    encode[2] |= bit & 0xff;

    // cell

    M_EncodePassItem(&bit, player->ammo[am_cell], player->maxammo[am_cell]);
    encode[3] = (bit << 4) & 0xff;

    // rocket

    M_EncodePassItem(&bit, player->ammo[am_misl], player->maxammo[am_misl]);
    encode[3] |= bit & 0xff;

    // health

    M_EncodePassItem(&bit, player->health, 200);
    encode[4] = (bit << 4);

    // armor

    M_EncodePassItem(&bit, player->armorpoints, 200);
    encode[4] |= bit;

    // armortype

    encode[5] |= player->armortype;

    // artifacts

    encode[5] |= (player->artifacts << 2);

    decodebit[0] = I_SwapBE16(*(short*)&encode[0]);
    decodebit[1] = I_SwapBE16(*(short*)&encode[2]);
    decodebit[2] = I_SwapBE16(*(short*)&encode[4]);

    *(short*)&encode[6] = I_SwapBE16(~(decodebit[0] + decodebit[1] + decodebit[2]));
    *(short*)&encode[8] = I_SwapBE16(~(decodebit[0] ^ decodebit[1] ^ decodebit[2]));

    for(i = 0; i < 10; i++) {
        bit = *(byte*)(encode + passwordTable[i]);
        encode[i] = (encode[i] ^ bit);
    }

    bit = 0;

    while(bit < 0x50) {
        passBit = 0;

        if(M_CheckPassCode(&bit, &xbit2, &xbit3, encode)) {
            passBit = 16;
        }

        xbit1 = 8;
        bit++;

        for(i = 0; i < 4; i++) {
            if(M_CheckPassCode(&bit, &xbit2, &xbit3, encode)) {
                passBit |= xbit1;
            }

            xbit1 >>= 1;
            bit++;
        }

        *(byte*)(passwordData + ((bit - 1) / 5)) = passBit;
    }
}

//
// M_DecodePassItem
//

static int M_DecodePassItem(int bytecode, int maxvalue) {
    int value;
    int bitsum = (bytecode * maxvalue);

    if(bitsum >= 0) {
        value = (bitsum >> 3);
    }
    else {
        value = ((bitsum + 7) >> 3);
    }

    return value;
}

//
// M_DecodePassword
//

dboolean M_DecodePassword(dboolean checkOnly) {
    byte data[16];
    byte decode[10];
    int bit = 0;
    int i = 0;
    short xbit1 = 0;
    short xbit2 = 0;
    short xbit3 = 0;
    short x, y;
    player_t *player;

    dmemcpy(data, passwordData, 16);
    dmemset(decode, 0, 10);
    player = &players[consoleplayer];

    //
    // decode password
    //
    while(bit < 0x50) {
        int passBit = 0;
        int decodeBit = 0x80;
        byte checkByte = 0;

        i = 0;

        while(i != 8) {
            int j = 0;

            i += 4;

            for(j = 0; j < 4; j++) {
                checkByte = *(byte*)(data + (bit / 5));
                if((checkByte & (16 >> (bit % 5)))) {
                    passBit |= decodeBit;
                }

                bit++;
                decodeBit >>= 1;
            }
        }

        if((bit - 1) >= 0) {
            checkByte = ((bit - 1) >> 3);
        }
        else {
            checkByte = (((bit - 1) + 7) >> 3);
        }

        *(byte*)(decode + checkByte) = passBit;
    }

    for(i = 9; i >= 0; i--) {
        bit = *(byte*)(decode + passwordTable[i]);
        decode[i] = (decode[i] ^ bit);
    }

    //
    // verify decoded password
    //
    xbit1 = I_SwapBE16(*(short*)&decode[0]);
    xbit2 = I_SwapBE16(*(short*)&decode[2]);
    xbit3 = I_SwapBE16(*(short*)&decode[4]);

    x = ((~((xbit1 + xbit2) + xbit3) << 16) >> 16);
    y = I_SwapBE16(*(short*)&decode[6]);

    if(x != y) {
        return false;
    }

    x = ((~(xbit1 ^ (xbit2 ^ xbit3)) << 16) >> 16);
    y = I_SwapBE16(*(short*)&decode[8]);

    if(x != y) {
        return false;
    }

    //
    // verify map
    //
    if((decode[0] >> 2) >= 33) {
        return false;
    }

    //
    // verify skill
    //
    if((decode[0] & 3) > sk_nightmare) {
        return false;
    }

    //
    // verify ammo?
    //
    if((decode[2] & 0xf) >= 9 || (decode[2] >> 4) >= 9) {
        return false;
    }

    //
    // verify ammo?
    //
    if((decode[3] & 0xf) >= 9 || (decode[3] >> 4) >= 9) {
        return false;
    }

    //
    // verify health/armor?
    //
    if((decode[4] & 0xf) >= 9 || (decode[4] >> 4) >= 9) {
        return false;
    }

    //
    // verify armortype
    //
    if((decode[5] & 3) >= 3) {
        return false;
    }

    if(checkOnly) {
        return true;
    }

    bit = 0;

    //
    // get map
    //
    gamemap = decode[0] >> 2;

    //
    // get skill
    //
    gameskill = decode[0] & 3;

    //
    // get weapons
    //
    for(i = 0; i < NUMWEAPONS; i++) {
        if(i != wp_fist && i != wp_pistol) {
            if(decode[1] & (1 << bit)) {
                player->weaponowned[i] = true;
            }

            bit++;
        }
    }

    //
    // get backpack
    //
    if(decode[5] & 0x80) {
        player->backpack = true;
        player->maxammo[am_clip]    = (maxammo[am_clip] << 1);
        player->maxammo[am_shell]   = (maxammo[am_shell] << 1);
        player->maxammo[am_cell]    = (maxammo[am_cell] << 1);
        player->maxammo[am_misl]    = (maxammo[am_misl] << 1);
    }

    //
    // get ammo
    //
    player->ammo[am_clip]   = M_DecodePassItem(decode[2] >> 4, player->maxammo[am_clip]);
    player->ammo[am_shell]  = M_DecodePassItem(decode[2] & 0xf, player->maxammo[am_shell]);
    player->ammo[am_cell]   = M_DecodePassItem(decode[3] >> 4, player->maxammo[am_cell]);
    player->ammo[am_misl]   = M_DecodePassItem(decode[3] & 0xf, player->maxammo[am_misl]);

    //
    // get health
    //
    player->health = M_DecodePassItem(decode[4] >> 4, 200);

    if(player->mo) {
        player->mo->health = player->health;
    }

    //
    // getarmor
    //
    player->armorpoints = M_DecodePassItem(decode[4] & 0xf, 200);

    //
    // get armor type
    //
    player->armortype = (decode[5] & 3);

    //
    // get artifacts
    //
    player->artifacts = ((decode[5] >> 2) & 7);

    //
    // set cheat menu if password leads to map 01
    //
    CON_CvarSetValue(p_features.name, ((decode[0] >> 2) == 1) ? 1.0f : 0.0f);

    return true;
}
