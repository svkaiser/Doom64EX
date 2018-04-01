// -*- mode: c++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2010 GhostlyDeath
// Copyright(C) 2011 svkaiser
// Copyright(C) 2018 dotfloat
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//-----------------------------------------------------------------------------

#include <algorithm>
#include <array>

#include "rom_private.hh"

namespace {
  class Deflate {
      std::istream &stream;
      std::string output;

      /* Balanced binary tree for the Huffman codes */
      static constexpr short root_node = 1;
      std::array<short, 0x275 * 2> subtree_size {};
      std::array<short, 0x275 * 2> parent_nodes {};
      std::array<short, 0x275> left_child       {};
      std::array<short, 0x275> right_child      {};

      short& sibling_of(int node);
      void update_node(int node);
      void update_node_size(int node, int sibling);

      int next_code();

      /* Bit reading variables */
      int bits_left  {};
      int bit_buffer {};

      int read_bits(int count);
      bool next_bit();

      /* Delete all default initialisators */
      Deflate()                          = delete;
      Deflate(const Deflate&)            = delete;
      Deflate(Deflate&&)                 = delete;
      Deflate& operator=(const Deflate&) = delete;
      Deflate& operator=(Deflate&&)      = delete;

  public:
      Deflate(std::istream& s);

      std::string deflate();
  };
}

Deflate::Deflate(std::istream& s):
    stream(s)
{
    std::fill(subtree_size.begin(), subtree_size.end(), 1);

    for (size_t i {}; i < left_child.size(); ++i)
        left_child[i] = 2 * i;

    for (size_t i {}; i < right_child.size(); ++i)
        right_child[i] = 2 * i + 1;

    for (size_t i {}; i < parent_nodes.size(); ++i)
        parent_nodes[i] = i / 2;
}

short& Deflate::sibling_of(int node)
{
    auto p = parent_nodes[node];
    return (left_child[p] == node) ? right_child[p] : left_child[p];
}

int Deflate::next_code()
{
    int node { root_node };

    while (node < 0x275) {
        node = !next_bit() ? left_child[node] : right_child[node];
    }

    update_node(node);

    return node - 0x275;
}

void Deflate::update_node(int node)
{
    subtree_size[node]++;

    // If code is the root node, we don't need to update anything.
    if (parent_nodes[node] == root_node)
        return;

    auto parent = parent_nodes[node];

    if (node == left_child[parent]) {
        update_node_size(node, right_child[parent]);
    } else {
        update_node_size(node, left_child[parent]);
    }

    while (parent_nodes[node] != root_node) {
        auto grandsibling = sibling_of(parent);

        // Balance the tree
        if (subtree_size[grandsibling] < subtree_size[node]) {
            sibling_of(parent) = node;

            auto sibling = sibling_of(node);
            sibling_of(sibling) = grandsibling;

            parent_nodes[grandsibling] = parent_nodes[node];
            parent_nodes[node] = parent_nodes[parent];

            update_node_size(grandsibling, sibling);
            node = grandsibling;
        }

        node = parent_nodes[node];
        parent = parent_nodes[node];
    }
}

void Deflate::update_node_size(int node, int sibling)
{
    while (node != root_node) {
        auto parent = parent_nodes[node];

        subtree_size[parent] = subtree_size[sibling] + subtree_size[node];

        if (parent != root_node) {
            sibling = sibling_of(parent);
        }

        node = parent;
    }

    if (subtree_size[root_node] != 2000)
        return;

    for (auto& x : subtree_size)
        x >>= 1;
}

bool Deflate::next_bit()
{
    if (!bits_left) {
        bit_buffer = stream.get();
        bits_left  = 8;
    }

    /* Check if most signifact bit is set */
    bool bit = bit_buffer & 0x80;

    bit_buffer <<= 1;
    bits_left--;

    return bit;
}

int Deflate::read_bits(int count)
{
    int bits {};

    for (int i {}; i < count; ++i) {
        if (next_bit()) {
            bits |= 1 << i;
        }
    }

    return bits;
}

std::string Deflate::deflate()
{
    for (;;) {
        auto code = next_code();

        /* If the code is 256 we're done */
        if (code == 256)
            break;

        /* If the code is greater than 256 then it's a dictionary pointer */
        if (code >= 257) {
            constexpr std::array<int, 6> offset_table {{ 0, 16, 80, 336, 1360, 5456 }};

            code -= 257;

            auto len = code % 62 + 3;
            auto off = offset_table[code / 62] + read_bits(code / 62 * 2 + 4);

            auto str = output.substr(output.size() - off - len, len);
            output.append(str);
        } else {
            /* Otherwise it's a char literal which we just output back */
            output.push_back(code);
        }
    }

    return output;
}

std::string wad::rom::deflate(std::istream& stream)
{
    return Deflate { stream }.deflate();
}
