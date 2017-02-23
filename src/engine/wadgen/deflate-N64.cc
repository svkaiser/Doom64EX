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
#ifdef RCSID
static const char rcsid[] =
    "$Id: DeflateN64.c 922 2011-08-13 00:49:06Z svkaiser $";
#endif

#include "wadgen.h"

typedef struct {
	int var0;
	int var1;
	int var2;
	int var3;
	byte *write;
	byte *writePos;
	byte *read;
	byte *readPos;
	int null1;
	int null2;
} decoder_t;

static decoder_t decoder;

#define OVERFLOWCHECK		0x7FFFFFFF

#define TABLESIZE	1280
byte DecodeTable[TABLESIZE * 4];

byte array01[0xFFFFFF];		// 0x800B3660
byte array05[0xFFFFFF];		// 0x8005D8A0
byte tableVar01[0xFFFFFF];	// 0x800B2250

int allocPtr[0xFFFFFF];

//**************************************************************
//**************************************************************
//      Deflate_InitDecodeTable
//**************************************************************
//**************************************************************

void Deflate_InitDecodeTable(void)
{
	int v[2];
	int a[4];
	byte *a0p;
	byte *v1p;
	byte *a1p;
	byte *a2p;
	byte *a3p;
	byte *v0p;

	*(signed short *)array05 = 0x04;
	*(signed short *)(array05 + 2) = 0x06;
	*(signed short *)(array05 + 4) = 0x08;
	*(signed short *)(array05 + 6) = 0x0A;
	*(signed short *)(array05 + 8) = 0x0C;
	*(signed short *)(array05 + 10) = 0x0E;

	*(signed short *)(tableVar01 + 0x34) = 0x558F;

	*(int *)(tableVar01 + 0x3C) = 3;
	*(int *)(tableVar01 + 0x40) = 0;
	*(int *)(tableVar01 + 0x44) = 0;

	decoder.var0 = 0;
	decoder.var1 = 0;
	decoder.var2 = 0;
	decoder.var3 = 0;

	a0p = (array01 + 4);
	v1p = (byte *) (DecodeTable + 0x9E4);

	v[0] = 2;

	do {
		if (v[0] < 0)
			*(signed short *)v1p = (signed short)((v[0] + 1) << 1);
		else
			*(signed short *)v1p = (signed short)(v[0] >> 1);

		*(signed short *)a0p = 1;

		v1p += 2;
		a0p += 2;

	} while (++v[0] < 1258);

	a1p = (byte *) (DecodeTable + 0x4F2);
	a0p = (byte *) (DecodeTable + 2);

	v[1] = 2;
	a[2] = 3;

	do {
		*(signed short *)a1p = a[2];
		a[2] += 2;

		*(signed short *)a0p = v[1];
		v[1] += 2;

		a0p += 2;
		a1p += 2;

	} while (a[2] < 1259);

	*(int *)tableVar01 = 0;
	v[1] = (1 << *(signed short *)(array05));
	*(int *)(tableVar01 + 0x18) = (v[1] - 1);

	*(int *)(tableVar01 + 4) = v[1];
	v[1] += (1 << *(signed short *)(array05 + 2));
	*(int *)(tableVar01 + 0x1C) = (v[1] - 1);

	v[0] = 2;
	a2p = (array05 + (v[0] << 1));

	a[0] = (v[0] << 2);
	a1p = (tableVar01 + a[0]);

	*(signed short *)a1p = v[1];

	v[1] += (1 << *(signed short *)a2p);
	*(int *)(a1p + 4) = v[1];

	v[1] += (1 << *(signed short *)(a2p + 2));
	*(int *)(a1p + 8) = v[1];

#ifdef _MSC_VER
	(int *)a3p = (int *)((byte *) (tableVar01 + 0x18) + a[0]);
#else
	a3p = ((byte *) (tableVar01 + 0x18) + a[0]);
#endif
	*(int *)a3p = (v[1] - 1);

	v[1] += (1 << *(signed short *)(a2p + 4));
	*(int *)(a1p + 12) = v[1];

	v[1] += (1 << *(signed short *)(a2p + 6));
	*(int *)(a3p + 4) = (v[1] - 1);
	*(int *)(a3p + 8) = (v[1] - 1);
	*(int *)(a3p + 0xc) = (v[1] - 1);

	v0p = (byte *) (tableVar01 + 0x30);

	*(int *)v0p = (v[1] - 1);
	*(int *)(v0p + 4) = ((v[1] - 1) + 64);
}

//**************************************************************
//**************************************************************
//      Deflate_GetDecodeByte
//**************************************************************
//**************************************************************

byte Deflate_GetDecodeByte(void)
{
	if (!((decoder.readPos - decoder.read) < OVERFLOWCHECK))
		return -1;

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
	byte *t7p;
	byte *v0p;
	int idByte1;
	int idByte2;
	byte *tablePtr = (byte *) (DecodeTable + 0x9E0);

	idByte1 = (a0 << 1);

	do {
		idByte2 = *(signed short *)(tablePtr + idByte1);

		t7p = (array01 + (idByte2 << 1));
		*(signed short *)t7p =
		    (*(signed short *)(array01 + (a1 << 1)) +
		     *(signed short *)(array01 + idByte1));

		a0 = idByte2;

		if (idByte2 != 1) {
			idByte1 = *(signed short *)(tablePtr + (idByte2 << 1));
			idByte2 =
			    *(signed short *)(DecodeTable + (idByte1 << 1));

			a1 = idByte2;

			if (a0 == idByte2)
				a1 = *(signed short *)((DecodeTable + 0x4F0) +
						       (idByte1 << 1));
		}

		idByte1 = (a0 << 1);

	} while (a0 != 1);

	if (*(signed short *)(array01 + 2) != 0x7D0)
		return;

	*(signed short *)(array01 + 2) >>= 1;

	v0p = (byte *) (array01 + 4);

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
	byte *s4p;
	byte *s2p;
	byte *v1p;
	byte *s1p;
	byte *s6p;
	byte *s3p;
	byte *a1p;

	s4p = array01;
	v[0] = (a0 << 1);

	s2p = (byte *) (DecodeTable + 0x9E0);

	v1p = (s4p + v[0]);
	s[5] = 1;

	a[2] = (a0 + 0x275);
	*(signed short *)(v1p + 0x4EA) = (*(signed short *)(v1p + 0x4EA) + 1);

	if (s[5] == *(signed short *)((s2p + v[0]) + 0x4EA))
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

		if (*(signed short *)(s4p + (s[0] << 1)) <
		    *(signed short *)(s4p + v[1])) {
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
		if (Deflate_DecodeScan() == 0)
			lookup = *(signed short *)(tablePtr1 + (lookup << 1));
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
	if (!((decoder.writePos - decoder.write) < OVERFLOWCHECK)) {
		//I_Error("Overflowed output buffer");
		WGen_Complain("Overflowed output buffer");
		return;
	}

	*decoder.writePos++ = outByte;
}

//**************************************************************
//**************************************************************
//      Deflate_Decompress
//**************************************************************
//**************************************************************

void Deflate_Decompress(byte * input, byte * output)
{
	int v[2];
	int a[4];
	int s[10];
	int t[10];
	int at;
	int div;
	int mul;
	int incrBit;

	byte *tablePtr1;
//	byte *a1p;
	byte *s4p;
	byte *t8p;
	byte *t9p;
	byte *t1p;
	byte *v0p;
	byte *t2p;
	byte *t4p;

	Deflate_InitDecodeTable();
	incrBit = 0;

	decoder.read = input;
	decoder.readPos = input;

	decoder.null1 = 0x7FFFFFFF;

	decoder.write = output;
	decoder.writePos = output;

	tablePtr1 = (byte *) (tableVar01 + 0x34);

	decoder.null2 = 0x7FFFFFFF;

//	a1p = tablePtr1;
	a[2] = 1;
	a[3] = 0;
	// Z_Alloc(a[0], a1p, a[2], a[3]);

	s4p = (byte *) allocPtr;

	v[0] = Deflate_StartDecodeByte();

	at = 256;
	s[0] = v[0];

	// GhostlyDeath <May 14, 2010> -- loc_8002E058 is part of a while loop
	while (v[0] != at) {
		at = (v[0] < 256);
		v[0] = 62;

		// GhostlyDeath <May 15, 2010> -- loc_8002E094 is an if statement
		if (at != 0) {
			a[0] = (s[0] & 0xff);
			Deflate_WriteOutput((byte) a[0]);

			t8p = s4p;
			t9p = (t8p + incrBit);
			*t9p = s[0];

			t[1] = *(int *)tablePtr1;

			incrBit += 1;
			if (incrBit == t[1])
				incrBit = 0;
		}
		// GhostlyDeath <May 15, 2010> -- Since then old shots point to loc_8002E19C the remainder of
		// loc_8002E094 until loc_8002E19C is an else.
		else {
			t[2] = (s[0] + (signed short)0xFEFF);
			div = t[2] / v[0];	// (62)

			// GhostlyDeath <May 15, 2010> -- loc_8002E0AC is an adjacent jump (wastes cpu cycles for fun!)
			at = -1;

			// GhostlyDeath <May 15, 2010> -- loc_8002E0C4 is an if
			if (v[0] == at)
				at = 0x8000;

			s[2] = 0;
			s[5] = div;
			t[4] = (s[5] << 1);

			mul = s[5] * v[0];

			a[0] = *(signed short *)(array05 + t[4]);

			t[3] = mul;

			s[8] = (s[0] - t[3]);	// subu    $fp, $s0, $t3
			s[8] += (signed short)0xFF02;	// addiu   $fp, 0xFF02
			s[3] = s[8];	// move    $s3, $fp

			v[0] = Deflate_RescanByte(a[0]);

			t[5] = (s[5] << 2);
			t[6] = *(int *)(tableVar01 + t[5]);
			s[1] = incrBit;

			t[7] = (t[6] + v[0]);

			v[1] = (t[7] + s[8]);	// addu $v1, $t7, $fp
			a[0] = (incrBit - v[1]);	// subu input, incrBit, $v1
			s[0] = a[0];	// move $s0, input

			// GhostlyDeath <May 15, 2010> -- loc_8002E124 is an if
			if (a[0] < 0)	// bgez input, loc_8002E124
			{
				t[8] = *(int *)tablePtr1;
				s[0] = (a[0] + t[8]);
			}
			// GhostlyDeath <May 15, 2010> -- loc_8002E184 is an if
			if (s[8] > 0)
				// GhostlyDeath <May 15, 2010> -- loc_8002E12C is a while loop (jump back from end)
				while (s[2] != s[3]) {
					t9p = s4p;
					t1p = (t9p + s[0]);
					a[0] = *(byte *) t1p;	// lbu  input, 0($t1)
					Deflate_WriteOutput((byte) a[0]);

					v0p = s4p;
					s[2] += 1;

					t2p = (v0p + s[0]);	// addu $t2, $s0, $v0
					t[3] = *(byte *) t2p;

					t4p = (v0p + s[1]);
					*(byte *) t4p = t[3];

					v[1] = *(int *)tablePtr1;

					s[1]++;
					s[0]++;

					// GhostlyDeath <May 15, 2010> -- loc_8002E170 is an if
					if (s[1] == v[1])
						s[1] = 0;

					// GhostlyDeath <May 15, 2010> -- loc_8002E17C is an if 
					if (s[0] == v[1])
						s[0] = 0;
				}

			v[1] = *(int *)tablePtr1;
			incrBit += s[8];
			at = (incrBit < v[1]);

			// GhostlyDeath <May 15, 2010> -- loc_8002E19C is the end of a while
			if (at == 0)
				incrBit -= v[1];
		}

		v[0] = Deflate_StartDecodeByte();

		at = 256;
		s[0] = v[0];
	}

	a[1] = *(int *)s4p;
	// Z_Free();
}
