#include <algorithm>
#include <fstream>
#include <imp/App>
#include <imp/Prelude>
#include <imp/util/Endian>

#include <system/Rom.hh>

#if 0
#define ASSERT_SIZEOF(_Struct, _Sizeof)                                 \
    static_assert(sizeof(_Struct) == _Sizeof, #_Struct " must be " #_Sizeof " bytes")

namespace {
  constexpr auto datasize = 0x1716d0;
  constexpr auto snd_title = 0x34364e53U;
  constexpr auto seq_title = 0x51455353U;

  struct Sn64Header {
      char id[4];
      uint32 game_id; //< Must be 2
      uint32 _pad0;
      uint32 _version_id; //< Always 100. Not read or used in game.
      uint32 _pad1;
      uint32 _pad2;
      uint32 len1; //< Length of file minus the header size
      uint32 _pad3;
      uint32 num_inst; //< Number of instruments (31)
      uint16 num_patches; //< Number of patches
      uint16 patch_size; //< sizeof patch struct
      uint16 num_subpatches; //< Number of subpatches
      uint16 subpatch_size; //< sizeof subpatch struct
      uint16 num_sounds; //< Number of sounds
      uint16 sound_size; //< sizeof sound struct
      uint32 _pad4;
      uint32 _pad5;
  };

  struct PatchHeader {
      uint16 length; //< Length (little endian)
      uint16 offset; //< Offset in SN64 file
  };

  struct SubpatchHeader {
      uint8 unity_pitch; //< Unknown; used by N64 library
      uint8 attenuation;
      uint8 pan; //< Left/Right panning
      uint8 instrument; //< (Boolean) Treat this as an instrument?
      uint8 root_key; //< Instrument's root key
      uint8 detune; //< Detune key (20120327 villsa - identified)
      uint8 min_note; //< Use this subpatch if note > min_note
      uint8 max_note; //< Use this subpatch if note < max_note
      uint8 pitch_wheel_range_low; //< (20120326 villsa - identified)
      uint8 pitch_wheel_range_high; //< (20120326 villsa - identified)
      uint16 id; //< Sound ID
      int16 attack_time; //< Attack time (fade in)
      uint8 _unknown0;
      uint8 _unknown1;
      int16 decay_time; //< Decay time (fade out)
      uint8 _volume0; //< Volume (unknown purpose)
      uint8 _volume1; //< Volume (unused?)
  };

  struct WaveTable {
      uint16 start; //< Start of ROM offset
      uint16 size; //< Size of sound
      uint16 _pad0;
      uint16 pitch; //< Pitch correction
      uint16 loop_id; //< Index ID for the loop table
      uint16 _pad1;
  };

  struct LoopInfo {
      uint16 num_sounds; //< Sound count (124)
      uint16 _pad0;
      uint16 num_loops; //< Loop data count (23)
      uint16 _num_sounds2; //< Sound count again? (124)
  };

  struct LoopTable {
      uint32 loop_start;
      uint32 loop_end;
      char _pad0[40]; //< Garbage in rom, but changed/set in game
  };

  struct PredictorTable {
      uint32 order; //< Order ID
      uint32 num_predictors; //< Number of predictors
      uint16 predictors[128];
  };

  struct SseqHeader {
      char id[4];
      uint32 game_id; //< Must be 2
      uint32 _pad0;
      uint32 num_entries; //< Number of entries
      uint32 _pad1;
      uint32 _pad2;
      uint32 entry_size; //< sizeof(entry) * num_entries
      uint32 _pad3;
  };

  struct EntryHeader {
      uint16 num_tracks; //< Number of tracks
      uint16 _pad0;
      uint32 length; //< Length of entry
      uint32 offset; //< Offset in SSeq file
      uint32 _pad1;
  };

  struct TrackHeader {
      uint16 flag; //< Usually0 on sounds, 0x100 on music
      uint16 id; //< Subpatch ID
      uint16 _pad0;
      uint8 volume; //< Default volume
      uint8 pan; //< Default pan
      uint16 _pad1;
      uint16 bpm; //< Beats per minute
      uint16 timediv; //< Time division
      uint16 loop; //< (Boolean) 0 if no loop, 1 if yes
      uint16 _pad2;
      uint16 size; //< Size of MIDI data
  };

  struct MidiTrackHeader {
      char id[4];
      int length;
  };

  struct MidiHeader {
      char id[4];
      int32 chunk_size;
      int16 type;
      uint16 num_tracks;
      uint16 delta;
      uint32 size;
  };

  struct RomIwad {
      std::size_t position;
      char country_id;
      char version;
  };

  RomIwad rom_iwad_[4] {
      { 0x6df60, 'P', 0 },
      { 0x64580, 'J', 0 },
      { 0x63d10, 'E', 0 },
      { 0x63dc0, 'E', 1 }
  };

  ASSERT_SIZEOF(Sn64Header,     56);
  ASSERT_SIZEOF(PatchHeader,    4);
  ASSERT_SIZEOF(SubpatchHeader, 20);
  ASSERT_SIZEOF(WaveTable,      12);
  ASSERT_SIZEOF(LoopInfo,       8);
  ASSERT_SIZEOF(LoopTable,      48);

  void load_sn64_();
}

void load_sn64_()
{
    auto s = rom::sn64();

    /* read header */
    Sn64Header sn64;
    s.read(reinterpret_cast<char*>(&sn64), sizeof(sn64));
    if (std::memcmp(sn64.id, "SN64", 4))
        fatal("Invalid SN64 id.");

    set_big_endian(sn64.game_id);
    set_big_endian(sn64.num_inst);
    set_big_endian(sn64.len1);
    set_big_endian(sn64.num_patches);
    set_big_endian(sn64.patch_size);
    set_big_endian(sn64.num_subpatches);
    set_big_endian(sn64.subpatch_size);
    set_big_endian(sn64.num_sounds);
    set_big_endian(sn64.sound_size);

    /* read patches */
    auto patches = std::make_unique<PatchHeader[]>(sn64.num_patches);
    s.read(reinterpret_cast<char*>(patches.get()), sizeof(PatchHeader) * sn64.num_patches);
    for (size_t i {}; i < sn64.num_patches; ++i)
        set_big_endian(patches[i].offset);

    /* read subpatches */
    auto subpatches = std::make_unique<SubpatchHeader[]>(sn64.num_subpatches);
    s.read(reinterpret_cast<char*>(subpatches.get()), sizeof(SubpatchHeader) * sn64.num_subpatches);
    for (size_t i {}; i < sn64.num_subpatches; ++i) {
        set_big_endian(subpatches[i].id);
        set_big_endian(subpatches[i].attack_time);
        set_big_endian(subpatches[i].decay_time);
    }

    /* TODO: find where non-instruments begin */

    /* read waves */
    auto waves = std::make_unique<WaveTable[]>(sn64.num_sounds);
    s.read(reinterpret_cast<char*>(waves.get()), sizeof(WaveTable) * sn64.num_sounds);
    for (size_t i {}; i < sn64.num_sounds; ++i) {
        set_big_endian(waves[i].size);
        set_big_endian(waves[i].start);
        set_big_endian(waves[i].loop_id);
        set_big_endian(waves[i].pitch);
        // waves[i].ptrindex = i;
    }

    /* read loop info */
    LoopInfo loop_info;
    s.read(reinterpret_cast<char*>(&loop_info), sizeof(loop_info));
    set_big_endian(loop_info.num_sounds);
    set_big_endian(loop_info.num_loops);

    /* read loop table */
    auto loop_table = std::make_unique<LoopTable[]>(loop_info.num_loops);
    s.read(reinterpret_cast<char*>(loop_table.get()), sizeof(LoopTable) * loop_info.num_loops);
    for (size_t i {}; i < loop_info.num_loops; ++i) {
        set_big_endian(loop_table[i].loop_start);
        set_big_endian(loop_table[i].loop_end);
    }

    /* read predictors */
    std::vector<PredictorTable> predictors { sn64.num_sounds };
    for (auto& t : predictors) {
        s.read(reinterpret_cast<char*>(&t), sizeof(t));
        set_big_endian(t.num_predictors);
        set_big_endian(t.order);

        for (auto& p : t.predictors)
            set_big_endian(p);
    }

    /* read pcm */
}

void load_soundfont()
{
    auto s = rom::sseq();

    /* process header */
    constexpr auto sseq_magic = "SSEQ";

    SseqHeader sseq;
    s.read(reinterpret_cast<char*>(&sseq), sizeof(sseq));

    if (std::memcmp(sseq_magic, sseq.id, 4) != 0)
        fatal("Invalid SSEQ id");

    set_big_endian(sseq.entry_size);
    set_big_endian(sseq.num_entries);
    set_big_endian(sseq.game_id);

    /* process entries */
    constexpr auto midi_header_size = 14;
    auto track_pos = static_cast<size_t>(s.tellg()) + sizeof(EntryHeader) * sseq.num_entries;
    for (size_t i {}; i < sseq.num_entries; ++i) {
        EntryHeader entry;
        s.read(reinterpret_cast<char*>(&entry), sizeof(entry));
        auto entry_pos = s.tellg();

        set_big_endian(entry.length);
        set_big_endian(entry.num_tracks);
        set_big_endian(entry.offset);

        MidiHeader midi;
        std::copy_n("MThd", 4, midi.id);
        midi.chunk_size = swap_bytes(6_i32);
        midi.type       = swap_bytes(1_i16);
        midi.num_tracks = swap_bytes(entry.num_tracks);
        midi.size       = midi_header_size;

        size_t track_offset {};
        for (size_t j {}; j < entry.num_tracks; ++j) {
            TrackHeader track;
            s.seekg(track_pos);
            s.read(reinterpret_cast<char*>(&track), sizeof(track));

            if (track.loop) {
                int loop_id;
                s.read(reinterpret_cast<char*>(&loop_id), sizeof(loop_id));
                /* TODO: Read track data */
            } else {
                /* TODO: Read track data */
            }

            set_big_endian(track.flag);
            set_big_endian(track.id);
            set_big_endian(track.bpm);
            set_big_endian(track.timediv);
            set_big_endian(track.loop);
            set_big_endian(track.size);

            if (!(track.flag & 0x100) && entry.num_tracks > 1)
                fatal("Bad track {} offset [entry {:03d}]", j, i);

            /* TODO: Sound_CreateMidiTrack */

            track_offset += sizeof(TrackHeader) + track.size;

            if (track.loop) {
                track_offset += 4;
            }
        }

        s.seekg(entry_pos);
    }

    std::vector<MidiHeader> midis;

    std::exit(1);
}
#endif

void load_soundfont() {}
