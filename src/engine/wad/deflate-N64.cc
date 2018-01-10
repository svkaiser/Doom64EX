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

constexpr int16 root_node = 1;
std::array<int16, 0x275 * 2> freqs;
std::array<int16, 0x275 * 2> parent_nodes;
std::array<int16, 0x275> left_child, right_child;
std::array<byte, 0x558f> dictionary;

//**************************************************************
//**************************************************************
//      Deflate_InitDecodeTable
//**************************************************************
//**************************************************************

const std::array<int, 12> tableVar01 {{
        0, 0, 16, 0, 80, 0, 336, 0, 1360, 0, 5456
}};

void Deflate_InitDecodeTable(void)
{
	decoder.var0 = 0;
	decoder.var1 = 0;

  std::fill(std::begin(freqs), std::end(freqs), 1);

	for (size_t i = 0; i < 0x275 * 2; ++i) {
      parent_nodes[i] = i >> 1;
	}

  for (size_t i = 0; i < 0x275; ++i){
      left_child[i] = 2 * i;
      right_child[i] = 2 * i + 1;
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

void Deflate_CheckTable(int node, int sibling)
{
    int parent;
    while (node != root_node) {
        parent = parent_nodes[node];

        freqs[parent] = freqs[sibling] + freqs[node];

        if (parent != root_node) {
            auto grandparent = parent_nodes[parent];

            sibling = left_child[grandparent];

            // parent was actually left child, so select right child as the sibling
            if (parent == sibling)
                sibling = right_child[grandparent];
        }

        node = parent;
    }

    if (freqs[root_node] != 2000)
        return;

    for (auto& x : freqs)
        x >>= 1;
}

//**************************************************************
//**************************************************************
//      Deflate_DecodeByte
//**************************************************************
//**************************************************************

void Deflate_DecodeByte(int node)
{
	freqs[node]++;

  // If code is the root node, we don't need to update anything.
	if (parent_nodes[node] == root_node)
		return;

  auto parent = parent_nodes[node];

	if (node == left_child[parent]) {
		Deflate_CheckTable(node, right_child[parent]);
	} else {
		Deflate_CheckTable(node, left_child[parent]);
	}

  while (parent_nodes[node] != root_node) {
    auto grandparent = parent_nodes[parent];

		auto& left_grandsibling = left_child[grandparent];
    auto& right_grandsibling = right_child[grandparent];
		auto grandsibling = left_grandsibling;

		if (parent_nodes[node] == left_grandsibling) {
        grandsibling = right_grandsibling;
		}

    // Balance the tree
		if (freqs[grandsibling] < freqs[node]) {
			if (parent_nodes[node] == left_grandsibling)
          right_grandsibling = node;
			else
          left_grandsibling = node;

      auto sibling = left_child[parent];
			if (node == sibling) {
          sibling = right_child[parent];
          left_child[parent] = grandsibling;
			} else {
          right_child[parent] = grandsibling;
			}

			parent_nodes[grandsibling] = parent_nodes[node];
			parent_nodes[node] = parent_nodes[parent];

			Deflate_CheckTable(grandsibling, sibling);
			node = grandsibling;
		}

		node = parent_nodes[node];
		parent = parent_nodes[node];
	}
}

//**************************************************************
//**************************************************************
//      Deflate_StartDecodeByte
//**************************************************************
//**************************************************************

int Deflate_StartDecodeByte(void)
{
    int node = 1; // root node

    while (node < 0x275) {
        node = (Deflate_DecodeScan() == 0) ? left_child[node] : right_child[node];
    }

    Deflate_DecodeByte(node);

    return node - 0x275;
}

//**************************************************************
//**************************************************************
//      Deflate_RescanByte
//**************************************************************
//**************************************************************

int Deflate_RescanByte(int count)
{
	int bit = 1;		// $s0
	int data = 0;	// $s2

	if (count <= 0)
		return data;

  for (int i {}; i < count; ++i) {
		if (!(Deflate_DecodeScan() == 0))
			data |= bit;

		bit <<= 1;
	}

	return data;
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
	int div;
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
        code -= 257;
        div = code / 31 & ~1;	// (62)

        auto length = code - 31 * div + 3;	// move    $s3, $fp

        auto offset = tableVar01[div] + Deflate_RescanByte(div + 4);

        auto dict_src = dict_top - offset - length;

      // Loop around
      if (dict_src < 0)
          dict_src += dictionary.size();

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
