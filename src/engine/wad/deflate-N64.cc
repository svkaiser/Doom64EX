// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: DeflateN64.c 922 2011-08-13 00:49:06Z svkaiser $
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Author: svkaiser $
// $Revision: 922 $
// $Date: 2011-08-12 17:49:06 -0700 (Fri, 12 Aug 2011) $
//
// DESCRIPTION: Deflate decompression in Doom64. Needs A LOT of cleanup and proper variable naming
//
//-----------------------------------------------------------------------------

#include <cassert>
#include <fmt/format.h>
using byte = unsigned char;

typedef struct {
	int var0;
	int var1;
	byte *writePos;
	byte *readPos;
} decoder_t;

static decoder_t decoder;

using int16 = signed short;
#define OVERFLOWCHECK		0x7FFFFFFF

#define TABLESIZE	1280
int16 DecodeTable[TABLESIZE * 4];

int16 array01[0x275*2];		// 0x800B3660

byte dictionary[0xFFFFFF];

//**************************************************************
//**************************************************************
//      Deflate_InitDecodeTable
//**************************************************************
//**************************************************************

const int tableVar01[20] = {
	0, 16, 80, 336, 1360, 5456,
	15, 79, 0,
	21839, 21839, 21839, 21839, 21903
};

int16 DecodeTable_0x9e0[1258];

void Deflate_InitDecodeTable(void)
{
	decoder.var0 = 0;
	decoder.var1 = 0;

  std::fill(std::begin(array01), std::end(array01), 1);

	for (int v0 = 0; v0 < 1258; ++v0) {
      DecodeTable_0x9e0[v0] = v0 >> 1;
	}

  for (size_t i = 0; i < 0x275; ++i){
      DecodeTable[i + 0x275] = 2 * i + 1;
      DecodeTable[i] = 2 * i;
	}
}

//**************************************************************
//**************************************************************
//      Deflate_GetDecodeByte
//**************************************************************
//**************************************************************

byte Deflate_GetDecodeByte(void)
{
	return *decoder.readPos++;
}

//**************************************************************
//**************************************************************
//      Deflate_DecodeScan
//**************************************************************
//**************************************************************

int Deflate_DecodeScan(void)
{
	int resultbyte;

	resultbyte = decoder.var0;

	decoder.var0 = (resultbyte - 1);
	if ((resultbyte < 1)) {
      resultbyte = Deflate_GetDecodeByte();

		decoder.var1 = resultbyte;
		decoder.var0 = 7;
	}

	resultbyte = (0 < (decoder.var1 & 0x80));
	decoder.var1 = (decoder.var1 << 1);

	return resultbyte;
}

//**************************************************************
//**************************************************************
//      Deflate_CheckTable
//**************************************************************
//**************************************************************

void Deflate_CheckTable(int a0, int a1)
{
    int idByte1 = a0;
    int idByte2;

    do {
        idByte2 = DecodeTable_0x9e0[idByte1];

        array01[idByte2] = array01[a1] + array01[idByte1];

        a0 = idByte2;

        if (idByte2 != 1) {
            idByte1 = DecodeTable_0x9e0[idByte2];
            idByte2 = DecodeTable[idByte1];

            a1 = idByte2;

            if (a0 == idByte2)
                a1 = DecodeTable[idByte1 + 0x275];
        }

        idByte1 = a0;

    } while (a0 != 1);

    if (array01[1] != 0x7D0)
        return;

    array01[1] >>= 1;

    for (size_t i = 0; i < 1256; ++i) {
        array01[i] >>= 1;
    }
}

//**************************************************************
//**************************************************************
//      Deflate_DecodeByte
//**************************************************************
//**************************************************************

void Deflate_DecodeByte(int a0)
{
	int v[2];
	int a[4];
	int s[10];
	int16 *s3p;

  a0 += 0x275;

	array01[a0]++;

	if (DecodeTable_0x9e0[a0] == 1)
		return;

  auto& var0 = DecodeTable_0x9e0[a0];

	if (a0 == DecodeTable[var0]) {
		Deflate_CheckTable(a0, DecodeTable[var0 + 0x275]);
	} else {
		Deflate_CheckTable(a0, DecodeTable[var0]);
	}

	s3p = DecodeTable + 0x275;

  auto pos0 = a0;

	v[1] = a0;
	a[1] = DecodeTable[var0];
	a[2] = a0;
	a[3] = var0;
	do {
      a[0] = DecodeTable_0x9e0[a[3]];

		auto& var1 =DecodeTable[a[0]];
		s[0] = var1;

		if (DecodeTable_0x9e0[pos0] == var1) {
        s[0] = s3p[a[0]];
		}

		auto& var2 = DecodeTable[a[3]];

		if (array01[s[0]] < array01[v[1]]) {
			if (DecodeTable_0x9e0[pos0] == var1) {
          s3p[a[0]] = a[2];
			} else
				var1 = a[2];

			a[1] = var2;
			if (a[2] == a[1]) {
          a[2] = s3p[a[3]];
				var2 = s[0];
			} else {
          s3p[a[3]] = s[0];
				a[2] = a[1];
			}

			DecodeTable_0x9e0[s[0]] = DecodeTable_0x9e0[pos0];
			DecodeTable_0x9e0[pos0] = DecodeTable_0x9e0[a[3]];
			a[0] = s[0];
			a[1] = a[2];

			Deflate_CheckTable(a[0], a[1]);
			pos0 = s[0];
		}

		a[2] = DecodeTable_0x9e0[pos0];
		v[1] = a[2];

		pos0 = a[2];
		a[3] = DecodeTable_0x9e0[pos0];

	} while (DecodeTable_0x9e0[pos0] != 1);
}

//**************************************************************
//**************************************************************
//      Deflate_StartDecodeByte
//**************************************************************
//**************************************************************

int Deflate_StartDecodeByte(void)
{
	int lookup = 1;		// $s0
	auto tablePtr1 = DecodeTable;	// $s2
	auto tablePtr2 = DecodeTable + 0x275;	// $s1

	while (lookup < 0x275) {
		if (Deflate_DecodeScan() == 0) {
			lookup = tablePtr1[lookup];
		}
		else
			lookup = tablePtr2[lookup];
	}

	lookup = (lookup + (signed short)0xFD8B);
	Deflate_DecodeByte(lookup);

	return lookup;
}

//**************************************************************
//**************************************************************
//      Deflate_RescanByte
//**************************************************************
//**************************************************************

int Deflate_RescanByte(int byte)
{
	int i = 0;		// $s1
	int shift = 1;		// $s0
	int resultbyte = 0;	// $s2

	if (byte <= 0)
		return resultbyte;

	do {
		if (!(Deflate_DecodeScan() == 0))
			resultbyte |= shift;

		i++;
		shift = (shift << 1);
	} while (i != byte);

	return resultbyte;
}

//**************************************************************
//**************************************************************
//      Deflate_WriteOutput
//**************************************************************
//**************************************************************

void Deflate_WriteOutput(byte outByte)
{
	*decoder.writePos++ = outByte;
}

//**************************************************************
//**************************************************************
//      Deflate_Decompress
//**************************************************************
//**************************************************************

void Deflate_Decompress(byte * input, byte * output)
{
	int a[2];
	int s[10];
	int t[10];
	int div;
	int mul;
	int incrBit;

	Deflate_InitDecodeTable();
	incrBit = 0;

	decoder.readPos = input;
	decoder.writePos = output;

	// GhostlyDeath <May 14, 2010> -- loc_8002E058 is part of a while loop
    for (;;) {
		auto c = Deflate_StartDecodeByte();

		if (c == 256)
			break;

		// GhostlyDeath <May 15, 2010> -- loc_8002E094 is an if statement
		if (c < 256) {
			Deflate_WriteOutput(c);

			dictionary[incrBit] = c;

			incrBit += 1;
			if (incrBit == 0x558f)
				incrBit = 0;
		}
		// GhostlyDeath <May 15, 2010> -- Since then old shots point to loc_8002E19C the remainder of
		// loc_8002E094 until loc_8002E19C is an else.
		else {
			t[2] = (c + 0xfeff) & 0xffff;
			div = t[2] / 62;	// (62)

			s[2] = 0;
			s[5] = div;
			t[4] = (t[2] / 31) & 0xfffe;

			mul = s[5] * 62;

			s[3] = (c - mul + 0xff02) & 0xffff;	// move    $s3, $fp

			c = Deflate_RescanByte(t[4] + 4);

			t[7] = tableVar01[div] + c;

			a[0] = (incrBit - (t[7] + s[3]));	// subu input, incrBit, $v1
			s[0] = a[0];	// move $s0, input

			// GhostlyDeath <May 15, 2010> -- loc_8002E124 is an if
			if (a[0] < 0)	// bgez input, loc_8002E124
			{
				t[8] = 0x558f;
				s[0] = (a[0] + t[8]);
			}
			// GhostlyDeath <May 15, 2010> -- loc_8002E184 is an if
			if (s[3] > 0)
				// GhostlyDeath <May 15, 2010> -- loc_8002E12C is a while loop (jump back from end)
                s[1] = incrBit;
				for (int s2 {}; s2 < s[3]; ++s2) {
					Deflate_WriteOutput(dictionary[s[0]]);

					s[2] += 1;

					dictionary[s[1]] = dictionary[s[0]];

					if (++s[0] == 0x558f) s[0] = 0;
					if (++s[1] == 0x558f) s[1] = 0;
				}

			incrBit += s[3];

			// GhostlyDeath <May 15, 2010> -- loc_8002E19C is the end of a while
			if (incrBit >= 0x558f)
				incrBit -= 0x558f;
		}
	}
}
