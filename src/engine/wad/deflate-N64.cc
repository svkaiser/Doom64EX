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
byte DecodeTable[TABLESIZE * 4];

int16 array01[0xFFFFF];		// 0x800B3660

byte allocPtr[0xFFFFFF];

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
	int a[4];
	byte *a0p;
	byte *a1p;

	decoder.var0 = 0;
	decoder.var1 = 0;

	a0p = (byte*) (array01);

	for (int v0 = 0; v0 <= 1258; ++v0) {
		DecodeTable_0x9e0[v0] = v0 >> 1;

		*(signed short *)a0p = 1;

		a0p += 2;

	}

	a1p = (byte *) (DecodeTable + 0x4F2);
	a0p = (byte *) (DecodeTable + 2);

	a[2] = 3;

    for (int v1 = 2; a[2] < 1259; v1 += 2){
		*(signed short *)a1p = a[2];
		a[2] += 2;

		*(signed short *)a0p = v1;

		a0p += 2;
		a1p += 2;

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

void Deflate_CheckTable(int a0, int a1, int a2)
{
	int i = 0;
	byte *v0p;
	int idByte1;
	int idByte2;

	idByte1 = (a0 << 1);

	do {
		idByte2 = *(DecodeTable_0x9e0 + (idByte1 >> 1));

		array01[idByte2] = array01[a1] + array01[idByte1 >> 1];

		a0 = idByte2;

		if (idByte2 != 1) {
			idByte1 = *(DecodeTable_0x9e0 + idByte2);
			idByte2 =
			    *(signed short *)(DecodeTable + (idByte1 << 1));

			a1 = idByte2;

			if (a0 == idByte2)
				a1 = *(signed short *)((DecodeTable + 0x4F0) +
						       (idByte1 << 1));
		}

		idByte1 = (a0 << 1);

	} while (a0 != 1);

	if (array01[1] != 0x7D0)
		return;

    array01[1] >>= 1;

	v0p = (byte *) (array01 + 2);

	do {
		*(signed short *)(v0p + 6) >>= 1;
		*(signed short *)(v0p + 4) >>= 1;
		*(signed short *)(v0p + 2) >>= 1;
		*(signed short *)(v0p) >>= 1;

		v0p += 8;
		i += 8;

	} while (i != 2512);
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
	byte *s2p;
	byte *v1p;
	byte *s1p;
	byte *s6p;
	byte *s3p;
	byte *a1p;


	s2p = (byte *) (DecodeTable_0x9e0);

	v1p = (byte*) (array01 + a0);
	s[5] = 1;

	a[2] = (a0 + 0x275);
	(*(signed short *)(v1p + 0x4EA))++;

	if (s[5] == *(signed short *)((s2p + a0 * 2) + 0x4EA))
		return;

	v[1] = (a[2] << 1);

	s1p = (s2p + v[1]);

	s6p = (byte *) DecodeTable;

	a[3] = (*(signed short *)s1p << 1);
	a[1] = *(signed short *)(s6p + a[3]);
	s3p = (byte *) (DecodeTable + 0x4F0);

	if (a[2] == a[1]) {
		a[1] = *(signed short *)(s3p + a[3]);
		a[0] = a[2];
		Deflate_CheckTable(a[0], a[1], a[2]);
		a[3] = (*(signed short *)s1p << 1);
	} else {
		a[0] = a[2];
		Deflate_CheckTable(a[0], a[1], a[2]);
		s3p = (byte *) (DecodeTable + 0x4F0);
		a[3] = (*(signed short *)s1p << 1);
	}

	do {
		a[0] = (*(signed short *)(s2p + a[3]) << 1);

		a1p = (s6p + a[0]);
		v[0] = *(signed short *)a1p;
		s[0] = v[0];

		if (*(signed short *)s1p == v[0]) {
			s[0] = *(signed short *)(s3p + a[0]);
		}

		v1p = (s6p + a[3]);

		if (array01[s[0]] < array01[v[1] / 2]) {
			if (*(signed short *)s1p == v[0]) {
				*(signed short *)(s3p + a[0]) = a[2];
			} else
				*(signed short *)a1p = a[2];

			a[1] = *(signed short *)v1p;
			if (a[2] == a[1]) {
				a[2] = *(signed short *)(s3p + a[3]);
				*(signed short *)v1p = s[0];
			} else {
				*(signed short *)(s3p + a[3]) = s[0];
				a[2] = a[1];
			}

			*(signed short *)(s2p + (s[0] << 1)) =
			    *(signed short *)s1p;
			*(signed short *)s1p = *(signed short *)(s2p + a[3]);
			a[0] = s[0];
			a[1] = a[2];

			Deflate_CheckTable(a[0], a[1], a[2]);
			s1p = (s2p + (s[0] << 1));
		}

		a[2] = *(signed short *)s1p;
		v[1] = (a[2] << 1);

		s1p = (s2p + v[1]);
		a[3] = (*(signed short *)s1p << 1);

	} while (*(signed short *)s1p != s[5]);
}

//**************************************************************
//**************************************************************
//      Deflate_StartDecodeByte
//**************************************************************
//**************************************************************

int Deflate_StartDecodeByte(void)
{
	int lookup = 1;		// $s0
	byte *tablePtr1 = DecodeTable;	// $s2
	byte *tablePtr2 = (byte *) (DecodeTable + 0x4F0);	// $s1

	while (lookup < 0x275) {
		if (Deflate_DecodeScan() == 0) {
			lookup = *(signed short *) (tablePtr1 + (lookup << 1));
		}
		else
			lookup = *(signed short *)(tablePtr2 + (lookup << 1));
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

	byte *t1p;
	byte *v0p;
	byte *t2p;
	byte *t4p;

	Deflate_InitDecodeTable();
	incrBit = 0;

	decoder.readPos = input;
	decoder.writePos = output;

	auto c = Deflate_StartDecodeByte();

	s[0] = c;

	// GhostlyDeath <May 14, 2010> -- loc_8002E058 is part of a while loop
	while (c != 256) {
		// GhostlyDeath <May 15, 2010> -- loc_8002E094 is an if statement
		if (c < 256) {
			Deflate_WriteOutput(c);

			allocPtr[incrBit] = c;

			incrBit += 1;
			if (incrBit == 0x558f)
				incrBit = 0;
		}
		// GhostlyDeath <May 15, 2010> -- Since then old shots point to loc_8002E19C the remainder of
		// loc_8002E094 until loc_8002E19C is an else.
		else {
			t[2] = (c + 0xfeff) & 0xffff;
			div = t[2] / 62;	// (62)

			assert(c < 1024);

			s[2] = 0;
			s[5] = div;
			t[4] = (t[2] / 31) & 0xfffe;

			mul = s[5] * 62;

			s[3] = (c - mul + 0xff02) & 0xffff;	// move    $s3, $fp

			c = Deflate_RescanByte(t[4] + 4);

			t[6] = tableVar01[div];
			s[1] = incrBit;

			t[7] = (t[6] + c);

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
				while (s[2] != s[3]) {
					Deflate_WriteOutput(allocPtr[s[0]]);

					v0p = allocPtr;
					s[2] += 1;

					t2p = (v0p + s[0]);	// addu $t2, $s0, $v0
					t[3] = *(byte *) t2p;

					t4p = (v0p + s[1]);
					*(byte *) t4p = t[3];

					s[1]++;
					s[0]++;

					// GhostlyDeath <May 15, 2010> -- loc_8002E170 is an if
					if (s[1] == 0x558f)
						s[1] = 0;

					// GhostlyDeath <May 15, 2010> -- loc_8002E17C is an if 
					if (s[0] == 0x558f)
						s[0] = 0;
				}

			incrBit += s[3];

			// GhostlyDeath <May 15, 2010> -- loc_8002E19C is the end of a while
			if (incrBit >= 0x558f)
				incrBit -= 0x558f;
		}

		c = Deflate_StartDecodeByte();

		s[0] = c;
	}

	a[1] = *(int *)allocPtr;
}
