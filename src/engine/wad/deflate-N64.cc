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
#include <array>
using byte = unsigned char;

typedef struct {
	int var0;
	int var1;
	byte *writePos;
	byte *readPos;
} decoder_t;

static decoder_t decoder;

using int16 = signed short;

std::array<int16, 0x275 * 2> array01;
std::array<int16, 0x275 * 2> DecodeTable_0x9e0;
std::array<int16, 0x275> table1, table2;
std::array<byte, 0x558f> dictionary;

//**************************************************************
//**************************************************************
//      Deflate_InitDecodeTable
//**************************************************************
//**************************************************************

const std::array<int, 6> tableVar01 {{
    0, 16, 80, 336, 1360, 5456
}};

void Deflate_InitDecodeTable(void)
{
	decoder.var0 = 0;
	decoder.var1 = 0;

  std::fill(std::begin(array01), std::end(array01), 1);

	for (size_t i = 0; i < 0x275 * 2; ++i) {
      DecodeTable_0x9e0[i] = i >> 1;
	}

  for (size_t i = 0; i < 0x275; ++i){
      table1[i] = 2 * i;
      table2[i] = 2 * i + 1;
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
        assert(idByte2 < 0x275);

        array01[idByte2] = array01[a1] + array01[idByte1];

        a0 = idByte2;

        if (idByte2 != 1) {
            idByte1 = DecodeTable_0x9e0[idByte2];
            assert(idByte1 < 0x275);

            idByte2 = table1[idByte1];
            a1 = idByte2;

            if (a0 == idByte2)
                a1 = table2[idByte1];
        }

        idByte1 = a0;

    } while (a0 != 1);

    if (array01[1] != 2000)
        return;

    for (auto& x : array01)
        x >>= 1;
}

//**************************************************************
//**************************************************************
//      Deflate_DecodeByte
//**************************************************************
//**************************************************************

void Deflate_DecodeByte(int a0)
{

	array01[a0]++;

	if (DecodeTable_0x9e0[a0] == 1)
		return;

  auto& var0 = DecodeTable_0x9e0[a0];
  assert(var0 < 0x275);

	if (a0 == table1[var0]) {
		Deflate_CheckTable(a0, table2[var0]);
	} else {
		Deflate_CheckTable(a0, table1[var0]);
	}

  auto pos1 = var0;
  auto pos3 = a0;

	do {
    auto tmp0 = DecodeTable_0x9e0[pos1];

    assert(tmp0 < 0x275);
		auto& var1 =table1[tmp0];
    auto& var1a = table2[tmp0];
		auto tmp = var1;

		if (DecodeTable_0x9e0[pos3] == var1) {
        tmp = var1a;
		}

    assert(pos1 < 0x275);
		auto& var2 = table1[pos1];
    auto& var2a = table2[pos1];

		if (array01[tmp] < array01[pos3]) {
			if (DecodeTable_0x9e0[pos3] == var1) {
          var1a = pos3;
			} else
          var1 = pos3;

      int new_pos;
			if (pos3 == var2) {
          new_pos = var2a;
          var2 = tmp;
			} else {
          new_pos = var2;
          var2a = tmp;
			}

			DecodeTable_0x9e0[tmp] = DecodeTable_0x9e0[pos3];
			DecodeTable_0x9e0[pos3] = DecodeTable_0x9e0[pos1];

			Deflate_CheckTable(tmp, new_pos);
			pos3 = tmp;
		}

		pos3 = DecodeTable_0x9e0[pos3];
		pos1 = DecodeTable_0x9e0[pos3];

	} while (DecodeTable_0x9e0[pos3] != 1);
}

//**************************************************************
//**************************************************************
//      Deflate_StartDecodeByte
//**************************************************************
//**************************************************************

int Deflate_StartDecodeByte(void)
{
	int lookup = 1;		// $s0

	while (lookup < 0x275) {
		if (Deflate_DecodeScan() == 0) {
			lookup = table1[lookup];
		}
		else
			lookup = table2[lookup];
	}

	Deflate_DecodeByte(lookup);

	return lookup - 0x275;
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
	int t[10];
	int div;
	int mul;
	int dict_top {};

	Deflate_InitDecodeTable();

	decoder.readPos = input;
	decoder.writePos = output;

	// GhostlyDeath <May 14, 2010> -- loc_8002E058 is part of a while loop
    for (;;) {
		auto code = Deflate_StartDecodeByte();

    // If code == 256 then we're done
		if (code == 256)
			break;

    // If code < 256 then it's a char literal
		if (code < 256) {
			Deflate_WriteOutput(code);

			dictionary[dict_top] = code;
      if (++dict_top == dictionary.size()) dict_top = 0;
		}
    // Otherwise code > 256 and it's a dictionary pointer
		else {
			t[2] = code - 257;
			div = t[2] / 62;	// (62)

			t[4] = (t[2] / 31) & 0xfffe;

			mul = div * 62;

			auto length = (code - mul + 0xff02) & 0xffff;	// move    $s3, $fp

			auto offset = tableVar01[div] + Deflate_RescanByte(t[4] + 4);

      auto dict_src = dict_top - offset - length;

      // Loop around
      if (dict_src < 0)
          dict_src += dictionary.size();

			if (length > 0) {
          // Copy [length] characters from the dictionary to the output
				for (int i {}; i < length; ++i) {
					Deflate_WriteOutput(dictionary[dict_src]);

					dictionary[dict_top] = dictionary[dict_src];

          // wrap around
					if (++dict_src == dictionary.size()) dict_src = 0;
					if (++dict_top == dictionary.size()) dict_top = 0;
				}
      }
		}
	}
}
