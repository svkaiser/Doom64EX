#include <algorithm>
#include <fstream>
#include <platform/app.hh>
#include <prelude.hh>
#include <utility/endian.hh>

#include <system/n64_rom.hh>
#include <fluidsynth.h>
#include <ostream>
#include <numeric>
#include "BinaryReader.hh"

namespace {
  sys::N64Rom g_rom;

  struct Sn64Header {
      // char id[4];
      uint32 game_id; //< Must be 2
      // uint32 _pad0;
      // uint32 _version_id; //< Always 100. Not read or used in game.
      // uint32 _pad1;
      // uint32 _pad2;
      uint32 len1; //< Length of file minus the header size
      // uint32 _pad3;
      uint32 num_inst; //< Number of instruments (31)
      uint16 num_patches; //< Number of patches
      uint16 patch_size; //< sizeof patch struct
      uint16 num_subpatches; //< Number of subpatches
      uint16 subpatch_size; //< sizeof subpatch struct
      uint16 num_sounds; //< Number of sounds
      uint16 sound_size; //< sizeof sound struct
      // uint32 _pad4;
      // uint32 _pad5;

      static constexpr size_t size = 56;

      static Sn64Header from_istream(std::istream& stream)
      {
          Sn64Header d {};

          BinaryReader(stream)
              .magic("SN64")
              .big_endian(d.game_id)
              .ignore_t<uint32>(4)
              .big_endian(d.len1)
              .ignore_t<uint32>()
              .big_endian(d.num_inst,
                          d.num_patches,
                          d.patch_size,
                          d.num_subpatches,
                          d.subpatch_size,
                          d.num_sounds,
                          d.sound_size)
              .ignore_t<uint32>(2)
              .assert_size(size);

          return d;
      }
  } sn64;

  struct PatchHeader {
      uint16 length; //< Length (little endian)
      uint16 offset; //< Offset in SN64 file

      static PatchHeader from_istream(std::istream& stream)
      {
          PatchHeader d {};
          BinaryReader(stream)
              .little_endian(d.length)
              .big_endian(d.offset)
              .assert_size(4);
          return d;
      }
  };

  struct SubpatchHeader {
      uint8 unity_pitch; //< Unknown; used by N64 library
      uint8 attenuation;
      uint8 pan; //< Left/Right panning
      uint8 instrument; //< (Boolean) Treat this as an instrument?
      uint8 root_key; //< Instrument's root key
      uint8 detune; //< Detune key (20120327 villsa - identified)
      uint8 note_min; //< Use this subpatch if note > min_note
      uint8 note_max; //< Use this subpatch if note < max_note
      uint8 pitch_wheel_range_low; //< (20120326 villsa - identified)
      uint8 pitch_wheel_range_high; //< (20120326 villsa - identified)
      uint16 id; //< Sound ID
      int16 attack_time; //< Attack time (fade in)
      // uint8 _unknown0;
      // uint8 _unknown1;
      int16 decay_time; //< Decay time (fade out)
      // uint8 _volume0; //< Volume (unknown purpose)
      // uint8 _volume1; //< Volume (unused?)

      static SubpatchHeader from_istream(std::istream& stream)
      {
          SubpatchHeader d {};
          BinaryReader(stream)
              .big_endian(d.unity_pitch,
                          d.attenuation,
                          d.pan,
                          d.instrument,
                          d.root_key,
                          d.detune,
                          d.note_min,
                          d.note_max,
                          d.pitch_wheel_range_low,
                          d.pitch_wheel_range_high,
                          d.id,
                          d.attack_time)
              .ignore_t<uint8>(2)
              .big_endian(d.decay_time)
              .ignore_t<uint8>(2)
              .assert_size(20);

          return d;
      }
  };

  struct WaveTable {
      uint32 start; //< Start of ROM offset
      uint32 size; //< Size of sound
      // uint32 _pad0;
      uint32 pitch; //< Pitch correction
      uint32 loop_id; //< Index ID for the loop table
      // uint32 _pad1;

      static WaveTable from_istream(std::istream& stream)
      {
          WaveTable d {};
          BinaryReader(stream)
              .big_endian(d.start, d.size)
              .ignore_t<uint32>()
              .big_endian(d.pitch, d.loop_id)
              .ignore_t<uint32>()
              .assert_size(24);
          return d;
      }
  };

  struct LoopInfo {
      uint16 num_sounds; //< Sound count (124)
      // uint16 _pad0;
      uint16 num_loops; //< Loop data count (23)
      // uint16 _num_sounds2; //< Sound count again? (124)

      static constexpr size_t size = 8;

      static LoopInfo from_istream(std::istream& stream)
      {
          LoopInfo d {};
          BinaryReader(stream)
              .big_endian(d.num_sounds)
              .ignore(2)
              .big_endian(d.num_loops)
              .ignore(2)
              .assert_size(size);
          return d;
      }
  };

  struct LoopTable {
      uint32 loop_start;
      uint32 loop_end;
      // char _pad0[40]; //< Garbage in rom, but changed/set in game

      static LoopTable from_istream(std::istream& stream)
      {
          LoopTable d {};
          BinaryReader(stream)
              .big_endian(d.loop_start, d.loop_end)
              .ignore(40)
              .assert_size(48);
          return d;
      }
  };

  struct PredictorTable {
      uint32 order; //< Order ID
      uint32 num_predictors; //< Number of predictors
      int16 predictors[128];

      static PredictorTable from_istream(std::istream& stream)
      {
          PredictorTable d {};
          BinaryReader(stream)
              .big_endian(d.order,
                          d.num_predictors,
                          d.predictors)
              .assert_size(264);
          return d;
      }
  };

  struct SseqHeader {
      // char id[4];
      uint32 game_id; //< Must be 2
      // uint32 _pad0;
      uint32 num_entries; //< Number of entries
      // uint32 _pad1;
      // uint32 _pad2;
      uint32 entry_size; //< sizeof(entry) * num_entries
      // uint32 _pad3;

      static constexpr size_t size = 32;

      static SseqHeader from_istream(std::istream& stream)
      {
          SseqHeader d {};
          BinaryReader(stream)
              .magic("SSEQ")
              .big_endian(d.game_id)
              .ignore(4)
              .big_endian(d.num_entries)
              .ignore(8)
              .big_endian(d.entry_size)
              .ignore(4)
              .assert_size(size);
          return d;
      }
  };

  struct EntryHeader {
      uint16 num_tracks; //< Number of tracks
      // uint16 _pad0;
      uint32 length; //< Length of entry
      uint32 offset; //< Offset in SSeq file
      // uint32 _pad1;

      static constexpr size_t binary_size = 16;

      static EntryHeader from_istream(std::istream& stream)
      {
          EntryHeader d {};
          BinaryReader(stream)
              .big_endian(d.num_tracks)
              .ignore(2)
              .big_endian(d.length,
                          d.offset)
              .ignore(4)
              .assert_size(binary_size);
          return d;
      }
  };

  struct TrackHeader {
      uint16 flag; //< Usually0 on sounds, 0x100 on music
      uint16 id; //< Subpatch ID
      // uint16 _pad0;
      uint8 volume; //< Default volume
      uint8 pan; //< Default pan
      // uint16 _pad1;
      uint16 bpm; //< Beats per minute
      uint16 timediv; //< Time division
      uint16 loop; //< (Boolean) 0 if no loop, 1 if yes
      // uint16 _pad2;
      uint16 size; //< Size of MIDI data

      static constexpr size_t binary_size = 20;

      static TrackHeader from_istream(std::istream& stream)
      {
          TrackHeader d {};
          BinaryReader(stream)
              .big_endian(d.flag,
                          d.id)
              .ignore(2)
              .big_endian(d.volume,
                          d.pan)
              .ignore(2)
              .big_endian(d.bpm,
                          d.timediv,
                          d.loop)
              .ignore(2)
              .big_endian(d.size)
              .assert_size(binary_size);
          return d;
      }
  };

  struct MidiTrackHeader {
      char id[4];
      int length;

      static MidiTrackHeader from_istream(std::istream& stream)
      {
          MidiTrackHeader d {};
          BinaryReader(stream)
              .magic("MTrk")
              .big_endian(d.length);
          return d;
      }
  };

  struct MidiHeader {
      char id[4];
      int32 chunk_size;
      int16 type;
      uint16 num_tracks;
      uint16 delta;
      uint32 size;
  };

  struct Generator {
      int type;
      int ival;

      explicit Generator(int type, int value):
          type(type), ival(value) {}

      explicit Generator(int type, uint8 lo, uint8 hi):
          type(type), ival((hi << 8) | lo) {}
  };

  struct Instrument {
      size_t sample_id;
      int note_min;
      int note_max;
      Vector<Generator> generators;
  };

  struct Preset {
      String name;
      int bank;
      int prog;
      Vector<Instrument> instruments;
  };

  std::vector<Preset> presets_;
  std::vector<fluid_sample_t> samples_;
  std::vector<short> sample_data_;

  std::unique_ptr<PatchHeader[]> patches_;
  std::unique_ptr<SubpatchHeader[]> subpatches_;
  std::vector<std::string> midis_;
  size_t new_bank_offset_ {};

  void decode8(std::istream& in, short* out, int index, const short* pred1, short* last_sample)
  {
      static const short itable[16] = {
          0, 1, 2, 3, 4, 5, 6, 7,
          -8, -7, -6, -5, -4, -3, -2, -1,
      };

      std::fill_n(out, 8, 0);

      auto pred2 = pred1 + 8;

      short tmp[8];
      for (size_t i {}; i + 1 < 8; i += 2) {
          auto c = in.get();
          tmp[i] = itable[c >> 4] << index;
          tmp[i + 1] = itable[c & 0xf] << index;
      }

      for (int i {}; i < 8; ++i) {
          int64 total = pred1[i] * last_sample[6] + pred2[i] * last_sample[7];

          if (i > 0) {
              for (int j { i - 1 }; j >= 0; --j) {
                  total += tmp[(i - 1) - j] * pred2[j];
              }
          }

          int64 result = ((tmp[i] << 0xb) + total) >> 0xb;
          int16 sample {};

          if (result > 0x7fff) {
              sample = 0x7fff;
          } else if (result < -0x8000) {
              sample = -0x8000;
          } else {
              sample = static_cast<int16>(result);
          }

          out[i] = sample;
      }

      std::copy_n(out, 8, last_sample);
  }

  void decode_vadpcm(std::istream& in, short* out, size_t len, const PredictorTable& book)
  {
      short last_sample[8] {};

      for (size_t i {}; i + 8 < len; i += 9) {
          int c = in.get();

          auto index = (c >> 4) & 0xf;
          auto pred = &book.predictors[(c & 0xf) * 16];

          decode8(in, out,     index, pred, last_sample);
          decode8(in, out + 8, index, pred, last_sample);
          out += 16;
      }
  }

  const SubpatchHeader& get_subpatch_by_note(const PatchHeader& patch, int note)
  {
      if (note >= 0) {
          for (size_t i {}; i < patch.length; ++i) {
              auto& s = subpatches_[patch.offset + i];

              if (note >= s.note_min && note <= s.note_max)
                  return s;
          }
      }

      return subpatches_[patch.offset];
  }

  double s_usec_to_timecents(int usec)
  {
      auto t = static_cast<double>(usec) / 1000.0;
      return 1200 * log2(t);
  }

  double s_pan_to_percent(int pan)
  {
      auto p = static_cast<double>((pan - 64) * 25) / 32.0;
      return p / 0.1;
  }

  int s_get_original_pitch(int key, int pitch)
  {
      return key - (pitch / 100);
  }

  double s_attenuation_to_percent(int attenuation)
  {
      double a = (double)((127 - attenuation) * 45) / 63.5f;
      return a / 0.1;
  }

  UniquePtr<WaveTable[]> wavtables;
  UniquePtr<PredictorTable[]> predictors;
  UniquePtr<LoopTable[]> loop_table;
  void load_sn64_()
  {
      auto s = g_rom.sn64();
      auto start = static_cast<size_t>(s.tellg());

      /* read header */
      sn64 = Sn64Header::from_istream(s);
      auto pos = s.tellg();

      /* read patches */
      patches_ = array_from_istream<PatchHeader>(s, sn64.num_patches);

      pos += sn64.patch_size * sn64.num_patches + sizeof(int);
      s.seekg(pos);

      /* read subpatches */
      subpatches_ = array_from_istream<SubpatchHeader>(s, sn64.num_subpatches);

      pos += sn64.subpatch_size * sn64.num_subpatches + sizeof(int);
      s.seekg(pos);

      /* find where non-instruments begin */
      for (size_t i {}; i < sn64.num_patches; ++i) {
          if (!subpatches_[patches_[i].offset].instrument) {
              new_bank_offset_ = i;
              break;
          }
      }

      /* read waves */
      wavtables = array_from_istream<WaveTable>(s, sn64.num_sounds);
      pos += sn64.sound_size * sn64.num_sounds;
      s.seekg(pos);

      /* read loop info */
      auto loop_info = LoopInfo::from_istream(s);
      pos += LoopInfo::size;

      /* read loop table */
      loop_table = array_from_istream<LoopTable>(s, loop_info.num_loops);
      pos += 2 * (loop_info.num_loops + 1) * loop_info.num_loops;
      s.seekg(pos);

      /* read predictors */
      predictors = array_from_istream<PredictorTable>(s, sn64.num_sounds);

      /* create presets */
      int bank {};
      int prog {};
      presets_.resize(sn64.num_patches);
      for (size_t i {}; i < sn64.num_patches; ++i) {
          auto& preset = presets_[i];
          auto& patch = patches_[i];

          if (!subpatches_[patch.offset].instrument && bank == 0) {
              prog = 0;
              bank = 1;
          }

          preset.prog = prog++;
          preset.bank = bank;

          for (size_t j {}; j < patch.length; ++j) {
              preset.instruments.emplace_back();
              auto& inst = preset.instruments.back();
              auto& gens = inst.generators;
              auto& subpatch = subpatches_[patch.offset + j];
              auto& wavtable = wavtables[subpatch.id];

              inst.note_min = subpatch.note_min;
              inst.note_max = subpatch.note_max;
              gens.emplace_back(GEN_KEYRANGE, subpatch.note_min, subpatch.note_max);

              if (subpatch.attenuation < 127) {
                  int val = s_attenuation_to_percent(subpatch.attenuation);
                  gens.emplace_back(GEN_ATTENUATION, val);
              }

              if (subpatch.pan != 64) {
                  int val = s_pan_to_percent(subpatch.pan);
                  gens.emplace_back(GEN_PAN, val);
              }

              if (subpatch.attack_time > 1) {
                  int val = s_usec_to_timecents(subpatch.attack_time);
                  gens.emplace_back(GEN_VOLENVATTACK, val);
              }

              if (subpatch.decay_time > 1) {
                  int val = s_usec_to_timecents(subpatch.decay_time);
                  gens.emplace_back(GEN_VOLENVRELEASE, val);
              }

              if (wavtable.loop_id != ~0U) {
                  gens.emplace_back(GEN_SAMPLEMODE, 1_u16);
              }

              // // root key override
              int val = s_get_original_pitch(subpatch.root_key, wavtable.pitch);
              if (val < 0)
                  val = 0;
              gens.emplace_back(GEN_OVERRIDEROOTKEY, val);

              // sample id
              inst.sample_id = subpatch.id;
              gens.emplace_back(GEN_SAMPLEID, inst.sample_id);
          }
      }

      fmt::print("SN64 Length: {}\n", static_cast<size_t>(s.tellg()) - start);
  }

  enum struct midi {
      program_change = 0x07,
      pitch_bend     = 0x09,
      unknown1       = 0x0b,
      global_volume  = 0x0c,
      global_panning = 0x0d,
      sustain_pedal  = 0x0e,
      play_note      = 0x11,
      stop_note      = 0x12,
      goto_loop      = 0x20,
      end_marker     = 0x22,
      set_loop       = 0x23
  };

  class MidiWriter {
      std::ostream& out_;
      char chan_ {};

      void chan_prefix()
      {
          out_.put(0xb0 | chan_);
      }

  public:
      MidiWriter(std::ostream& out, char chan):
          out_(out),
          chan_(chan) {}

      void bank_select(char bank)
      {
          chan_prefix();
          out_.put(0);
          out_.put(bank);
      }

      void program_change(char program)
      {
          out_.put(0xc0 | chan_);
          out_.put(program);
      }

      void set_volume(char volume)
      {
          chan_prefix();
          out_.put(0x07);
          out_.put(volume);
      }

      void set_pan(char pan)
      {
          chan_prefix();
          out_.put(0x0a);
          out_.put(pan);
      }

      void set_loop_position(char loop)
      {
          out_.write("\xff\x7f\x02\x00", 4);
          out_.put(loop);
      }

      void jump_loop_position(int loop)
      {
          out_.write("\xff\x7f\x04\x00", 4);
      }

      void registered_parameter_number(uint16_t num)
      {
          chan_prefix();
          out_.put(0x65);
          out_.write(reinterpret_cast<char*>(&num), sizeof(num));

          chan_prefix();
          out_.put(0x64);
          out_.write(reinterpret_cast<char*>(&num), sizeof(num));
      }

      void data_entry(char value)
      {
          chan_prefix();
          out_.put(0x06);
          out_.put(value);
          out_.put(0);

          chan_prefix();
          out_.put(0x26);
          out_.put(0);
          out_.put(0);
      }
  };

  namespace midi_meta_events {
    /* Format: FF 51 03 tttttt */
    void set_tempo(std::ostream& s, int tempo)
    {
        const char *tempo_str = reinterpret_cast<char*>(&tempo);
        s.write("\x00\xff\x51\x03", 4);
        s.put(tempo_str[2]);
        s.put(tempo_str[1]);
        s.put(tempo_str[0]);
    }

    void end_marker(std::ostream& s)
    {
        s.write("\xff\x2f", 2);
    }
  }

  void read_midi_(std::string& midi_data, const TrackHeader& track, const std::string& s, char chan, size_t index,
                  const PatchHeader &patch)
  {
      char tmp[4];
      int pitchbend {};
      int bendrange {};
      int note {};

      auto& inst = subpatches_[patch.offset];

      //header.delta = big_endian(track.timediv);
      //header.type = big_endian(inst.instrument);

      /* Avoid percussion channels (default as 9) */
      if (chan >= 9)
          chan++;

      std::ostringstream ss;
      MidiWriter w { ss, chan };

      /* Set initial tempo */
      if (!chan) {
          midi_meta_events::set_tempo(ss, 60'000'000 / track.bpm);
      }

      /* Set initial bank */
      if (!inst.instrument) {
          ss.put(0);
          w.bank_select(1);
      }

      ss.put(0);
      w.program_change(track.id - (track.id >= new_bank_offset_ ? new_bank_offset_ : 0));
      ss.put(0);
      w.set_volume(track.volume);
      ss.put(0);
      w.set_pan(track.pan);

      auto s2 = s;
      s2.resize(s2.size() + 1000);
      const char* m = s2.c_str();
      for (;;) {
          do {
              ss.put(*m);
          } while (*m++ < 0);

          if (*m == 0x22) {
              ss.write("\xff\x2f", 2);
              break;
          }

          switch (static_cast<midi>(*m)) {
          case midi::end_marker: // 0x22
              midi_meta_events::end_marker(ss);
              break;

          case midi::program_change: // 0x07
              m++;
              tmp[0] = *m++;
              w.program_change(tmp[0] - (tmp[0] >= new_bank_offset_ ? new_bank_offset_ : 0));
              m++;
              break;

          case midi::pitch_bend: // 0x09
              if (inst.instrument && inst.pitch_wheel_range_high != bendrange) {
                  bendrange = inst.pitch_wheel_range_high;

                  w.registered_parameter_number(0);
                  w.data_entry(bendrange);
                  w.registered_parameter_number(0x7f00);
              }

              ss.put(0xe0 | chan);
              m++;

              tmp[0] = *m++;
              tmp[1] = *m++;
              pitchbend = tmp[0] | (tmp[1] << 8);
              pitchbend += 0x2000;

              if (pitchbend > 0x3fff)
                  pitchbend = 0x3fff;

              if (pitchbend < 0)
                  pitchbend = 0;

              pitchbend *= 2;

              ss.put(pitchbend & 0xff);
              ss.put(pitchbend >> 8);
              break;

          case midi::unknown1: {
              m += 2;
              ss.write("\xff\x07\x13", 3);
              ss.write("UNKNOWN EVENT: 0x11", 20);
          }
              break;

          case midi::global_volume: // 0x0c
              m++;
              w.set_volume(*m++);
              break;

          case midi::global_panning: // 0x0d
              /* We're skipping the first byte for whatever reason */
              m++;
              ss.put(0xb0 | chan);
              ss.put(0x0a_u8);
              ss.put(*m++);
              break;

          case midi::sustain_pedal: // 0x0e
              /* We're skipping the first byte for whatever reason */
              m++;
              ss.put(0xb0 | chan);
              ss.put(0x40);
              ss.put(*m++);
              break;

          case midi::play_note:
              /* We're skipping the first byte for whatever reason */
              m++;
              ss.put(0x90 | chan);
              tmp[1] = *m++;
              ss.put(tmp[1]);
              ss.put(*m++);
              note = tmp[1];
              break;

          case midi::stop_note:
              /* We're skipping the first byte for whatever reason */
              m++;
              ss.put(0x90 | chan);
              ss.put(*m++);
              ss.put(0);
              break;

          case midi::goto_loop:
              ss.write("\xff\x7f\x04\x00", 4);
              ss.put(*m++);
              ss.put(*m++);
              ss.put(*m++);
              break;

          case midi::set_loop:
              ss.write("\xff\x7f\x02\x00", 4);
              ss.put(*m++);
              break;

          default:
              log::fatal("Unknown MIDI event '{}'", static_cast<int>(*m));
              break;
          }

          inst = get_subpatch_by_note(patch, note);
      }

      ss.put(0);

      const auto &data = ss.str();
      size_t size = data.size();
      midi_data.append("MTrk", 4);
      midi_data.push_back((size >> 24) & 0xff);
      midi_data.push_back((size >> 16) & 0xff);
      midi_data.push_back((size >> 8) & 0xff);
      midi_data.push_back(size & 0xff);

      midi_data.append(data);
  }

  auto load_sseq_()
  {
      auto s = g_rom.sseq();
      auto start = static_cast<size_t>(s.tellg());

      /* process header */
      auto sseq = SseqHeader::from_istream(s);

      /* process entries */
      auto entries = array_from_istream<EntryHeader>(s, sseq.num_entries);
      assert(EntryHeader::binary_size * sseq.num_entries == sseq.entry_size);

      auto track_table = static_cast<size_t>(s.tellg());
      for (size_t i {}; i < sseq.num_entries; ++i) {
          auto& entry = entries[i];
          s.seekg(track_table + entry.offset);

          midis_.emplace_back();
          auto& midi_data = midis_.back();
          midi_data.append("MThd\0\0\0\x06\0\0\0\0\0\0", 14);

          constexpr size_t type_pos = 8;
          constexpr size_t num_tracks_pos = 10;
          constexpr size_t timediv_pos = 12;

          midi_data[num_tracks_pos] = (entry.num_tracks >> 8) & 0xff;
          midi_data[num_tracks_pos + 1] = entry.num_tracks & 0xff;

          for (size_t j {}; j < entry.num_tracks; ++j) {
              auto track = TrackHeader::from_istream(s);

              const auto &patch = patches_[track.id];
              const auto &inst = subpatches_[patch.offset];
              midi_data[type_pos] = (inst.instrument >> 8) & 0xff;
              midi_data[type_pos + 1] = inst.instrument & 0xff;
              midi_data[timediv_pos] = (track.timediv >> 8) & 0xff;
              midi_data[timediv_pos + 1] = track.timediv & 0xff;

              if (track.loop) {
                  s.ignore(4);
              }

              std::string str;
              str.resize(track.size);
              s.read(&str[0], track.size);

              std::istringstream ss(str);

              if (!(track.flag & 0x100) && entry.num_tracks > 1)
                  log::fatal("Bad track {} offset [entry {:03d}], {}", j, i, static_cast<size_t>(s.tellg()));

              read_midi_(midi_data, track, str, j, i, patches_[track.id]);
          }
      }

      return s;
  }
}

std::string get_midi(size_t midi)
{
    return midis_.at(midi);
}

void rom_preset(fluid_sfont_t* sfont, fluid_preset_t* preset, size_t id)
{
    constexpr auto free = [](fluid_preset_t* preset) -> int {
        delete preset;
        return 0;
    };

    constexpr auto get_name = [](fluid_preset_t* preset) -> char* {
        return strdup(reinterpret_cast<Preset*>(preset)->name.c_str());
    };

    constexpr auto get_banknum = [](fluid_preset_t* preset) -> int {
        return reinterpret_cast<Preset*>(preset->data)->bank;
    };

    constexpr auto get_num = [](fluid_preset_t* preset) -> int {
        return reinterpret_cast<Preset*>(preset->data)->prog;
    };

    constexpr auto noteon = [](fluid_preset_t* preset, fluid_synth_t* synth, int chan, int key, int vel) -> int {
        auto& data = *reinterpret_cast<Preset*>(preset->data);

        for (auto& inst : data.instruments) {
            if (key < inst.note_min || key > inst.note_max)
                continue;

            auto voice = fluid_synth_alloc_voice(synth, &samples_[inst.sample_id], chan, key, vel);

            for (auto& g : inst.generators) {
                fluid_voice_gen_set(voice, g.type, g.ival);
            }

            fluid_synth_start_voice(synth, voice);
        }

        return FLUID_OK;
    };

    *preset = {
        &presets_[id],
        sfont,
        free,
        get_name,
        get_banknum,
        get_num,
        noteon,
        nullptr
    };
}

fluid_sfont_t* rom_sfont()
{
    /* create a RAM soundfont */
    auto sfont = fluid_ramsfont_create_sfont();
    auto ramsfont = (fluid_ramsfont_t*) sfont->data;
    fluid_ramsfont_set_name(ramsfont, "Doom64EX RomSource");

    load_sn64_();
    load_sseq_();

    size_t pcm_size {};
    for (size_t i {}; i < sn64.num_sounds; ++i) {
        pcm_size += wavtables[i].size/9*16 + 16;
    }
    sample_data_.resize(pcm_size);

    /* read samples */
    auto s = g_rom.pcm();
    auto start = static_cast<size_t>(s.tellg());
    samples_.resize(sn64.num_sounds);
    auto pcm_ptr = sample_data_.data();
    std::vector<fluid_sample_t*> fluid_samples;
    for (size_t i {}; i < sn64.num_sounds; ++i) {
        auto& sample = samples_[i];
        std::fill_n(reinterpret_cast<char*>(&sample), sizeof(sample), 0);

        auto& wavtable = wavtables[i];
        auto& predictor = predictors[i];
        auto name = fmt::format("SFX_{}", i);

        wavtable.size -= wavtable.size % 9;

        std::copy_n(name.data(), name.size(), sample.name);
        sample.start = std::distance(sample_data_.data(), pcm_ptr) + 8;
        sample.end = sample.start + wavtable.size / 9 * 16;
        sample.samplerate = 22050;
        sample.origpitch = 60;
        sample.pitchadj = 0;
        sample.sampletype = FLUID_SAMPLETYPE_MONO;

        sample.valid = true;
        sample.data = sample_data_.data();

        s.seekg(wavtable.start);
        decode_vadpcm(s, pcm_ptr + 8, wavtable.size, predictor);

        std::fill_n(pcm_ptr, 8, 0);

        pcm_ptr += wavtable.size / 9 * 16 + 16;

        std::fill_n(pcm_ptr - 8, 8, 0);

        sample.loopstart = sample.start;
        sample.loopend = sample.end - 1;

        if (wavtable.loop_id != ~0U) {
            const auto& loop = loop_table[wavtable.loop_id];
            sample.loopstart = sample.start + loop.loop_start;
            sample.loopend = sample.start + loop.loop_end;
        }
    }

    struct Soundfont {
        char name[20] = "Doom64EX RomSource";
        size_t iter;
    };

    constexpr auto free = [](fluid_sfont_t* sfont) -> int {
        delete reinterpret_cast<Soundfont*>(sfont->data);
        delete sfont;
        return 0;
    };

    constexpr auto get_name = [](fluid_sfont_t* sfont) -> char * {
        return reinterpret_cast<Soundfont*>(sfont->data)->name;
    };

    constexpr auto get_preset = [](fluid_sfont_t* sfont, uint32 bank, uint32 prog) -> fluid_preset_t* {
        auto preset = new fluid_preset_t;
        rom_preset(sfont, preset, (bank ? new_bank_offset_ : 0) + prog);
        return preset;
    };

    constexpr auto iteration_start = [](fluid_sfont_t* sfont) {
        reinterpret_cast<Soundfont*>(sfont->data)->iter = 0;
    };

    constexpr auto iteration_next = [](fluid_sfont_t* sfont, fluid_preset_t* preset) -> int {
        auto& data = *reinterpret_cast<Soundfont*>(sfont->data);
        rom_preset(sfont, preset, data.iter);
        return data.iter < presets_.size();
    };

    return new fluid_sfont_t {
        new Soundfont,
            0,
            free,
            get_name,
            get_preset,
            iteration_start,
            iteration_next
            };
}

fluid_sfloader_t* rom_soundfont()
{
    constexpr auto free = [](fluid_sfloader_t* sf) {
        delete sf;
        return 0;
    };

    constexpr auto load = [](fluid_sfloader_t*, const char *fname) -> fluid_sfont_t* {
        if (g_rom.open(fname)) {
            return rom_sfont();
        }

        return nullptr;
    };

    return new fluid_sfloader_t {
        nullptr, /* data */
        free,
        load
    };
}
