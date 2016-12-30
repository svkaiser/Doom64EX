// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
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
//    Fixed point implementation.
//
//-----------------------------------------------------------------------------

#include "stdlib.h"

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"

#include "m_fixed.h"

// Fixme. __USE_C_FIXED__ or something.

fixed_t
FixedMul
(fixed_t    a,
 fixed_t    b) {
#ifdef USE_ASM
    fixed_t    c;
    _asm {
        mov eax, [a]
        mov ecx, [b]
        imul ecx
        shr eax, 16
        shl edx, 16
        or eax, edx
        mov [c], eax
    }
    return(c);
#else
    return (fixed_t)(((int64) a * (int64) b) >> FRACBITS);
#endif
}



//
// FixedDiv, C version.
//

fixed_t
FixedDiv
(fixed_t    a,
 fixed_t    b) {
    if((D_abs(a)>>14) >= D_abs(b)) {
        return (a^b)<0 ? D_MININT : D_MAXINT;
    }
    return FixedDiv2(a,b);

}



fixed_t
FixedDiv2
(fixed_t    a,
 fixed_t    b) {
    return (fixed_t)((((int64)a)<<FRACBITS)/b);
}

//
// FixedDot
//

fixed_t FixedDot(fixed_t a1, fixed_t b1,
                 fixed_t c1, fixed_t a2,
                 fixed_t b2, fixed_t c2) {
    return
        FixedMul(a1, a2) +
        FixedMul(b1, b2) +
        FixedMul(c1, c2);
}