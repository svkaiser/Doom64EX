// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
// DESCRIPTION: Low-level audio API. Incorporates a sequencer system to
//              handle all sounds and music. All code related to the sequencer
//              is kept in it's own module and seperated from the rest of the
//              game code.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <algorithm>

#include "SDL.h"
#include "fluidsynth.h"

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "i_audio.h"
#include "z_zone.h"
#include "i_swap.h"
#include "con_console.h"    // for cvars
#include <imp/Wad>
#include <imp/App>

#include "SDL.h"

// 20120203 villsa - cvar for soundfont location
StringProperty s_soundfont("s_soundfont", "doomsnd.sf2 location", "doomsnd.sf2");

//
// Mutex
//
static SDL_mutex *lock = NULL;
#define MUTEX_LOCK()    SDL_mutexP(lock);
#define MUTEX_UNLOCK()  SDL_mutexV(lock);

//
// Semaphore stuff
//

static SDL_sem *semaphore = NULL;
#define SEMAPHORE_LOCK()    if(SDL_SemWait(semaphore) == 0) {
#define SEMAPHORE_UNLOCK()  SDL_SemPost(semaphore); }

// 20120205 villsa - bool to determine if sequencer is ready or not
static dboolean seqready = false;

//
// DEFINES
//

#define MIDI_CHANNELS   64
#define MIDI_MESSAGE    0x07
#define MIDI_END        0x2f
#define MIDI_SET_TEMPO  0x51
#define MIDI_SEQUENCER  0x7f

//
// MIDI DATA DEFINITIONS
//
// These data should not be modified outside the
// audio thread unless they're being initialized
//

typedef struct {
    char        header[4];
    int         length;
    byte*       data;
    byte        channel;
} track_t;

typedef struct {
    char        header[4];
    int         chunksize;
    short       type;
    word        ntracks;
    word        delta;
    byte*       data;
    dword       length;
    track_t*    tracks;
    dword       tempo;
    double      timediv;
} song_t;

//
// SEQUENCER CHANNEL
//
// Active channels play sound or whatever
// is being fed from the midi reader. This
// is where communication between the audio
// thread and the game could get dangerous
// as they both need to access and modify
// data. In order to avoid this, certain
// properties have been divided up for
// both the audio thread and game code only
//

typedef enum {
    CHAN_STATE_READY    = 0,
    CHAN_STATE_PAUSED   = 1,
    CHAN_STATE_ENDED    = 2,
    MAXSTATETYPES
} chanstate_e;

typedef struct {
    // these should never be modified unless
    // they're initialized
    song_t*     song;
    track_t*    track;

    // channel id for identifying an active channel
    // used primarily by normal sounds
    byte        id;

    // these are both accessed by the
    // audio thread and the game code
    // and should lock and unlock
    // the mutex whenever these need
    // to be modified..
    float       volume;
    byte        pan;
    sndsrc_t*   origin;
    int         depth;

    // accessed by the audio thread only
    byte        key;
    byte        velocity;
    byte*       pos;
    byte*       jump;
    dword       tics;
    dword       nexttic;
    dword       lasttic;
    dword       starttic;
    Uint32      starttime;
    Uint32      curtime;
    chanstate_e state;
    dboolean    paused;

    // read by audio thread but only
    // modified by game code
    dboolean    stop;
    float       basevol;
} channel_t;

static channel_t playlist[MIDI_CHANNELS];   // channels active in sequencer

//
// DOOM SEQUENCER
//
// the backbone of the sequencer system. handles
// global volume and panning for all sounds/tracks
// and holds the allocated list of midi songs.
// all data is modified through the game and read
// by the audio thread.
//

typedef int seqsignal_e;
enum {
    SEQ_SIGNAL_IDLE     = 0,    // idles. does nothing
    SEQ_SIGNAL_SHUTDOWN,        // signal the sequencer to shutdown, cleaning up anything in the process
    SEQ_SIGNAL_READY,           // sequencer will read and play any midi track fed to it
    SEQ_SIGNAL_RESET,
    SEQ_SIGNAL_PAUSE,
    SEQ_SIGNAL_RESUME,
    SEQ_SIGNAL_STOPALL,
    SEQ_SIGNAL_SETGAIN,         // signal the sequencer to update output gain
    MAXSIGNALTYPES
};

typedef union {
    sndsrc_t*   valsrc;
    int         valint;
    float       valfloat;
} seqmessage_t;

typedef struct {
    // library specific stuff. should never
    // be modified after initialization
    fluid_settings_t*       settings;
    fluid_synth_t*          synth;
    fluid_audio_driver_t*   driver;
    int                     sfont_id; // 20120112 bkw: needs to be signed
    SDL_Thread*             thread;
    dword                   playtime;

    dword                   voices;

    // tweakable settings for the sequencer
    float                   musicvolume;
    float                   soundvolume;

    // keep track of midi songs
    song_t*                 songs;
    int                     nsongs;

    seqmessage_t            message[3];

    // game code signals the sequencer to do stuff. game will
    // wait (while loop) until the audio thread signals itself
    // to be ready again
    // MP2E 12102013 - Only change using Seq_SetStatus
    seqsignal_e             signal;

    // 20120316 villsa - gain property (tweakable)
    float                   gain;
} doomseq_t;

static doomseq_t doomseq = {0};   // doom sequencer

typedef void(*eventhandler)(doomseq_t*, channel_t*);
typedef int(*signalhandler)(doomseq_t*);

//
// Audio_Play
//
// Callback for SDL
//
static void Audio_Play(void *user, Uint8 *stream, int len)
{
    fluid_synth_t *synth = (fluid_synth_t *) user;
    fluid_synth_write_s16(synth, len / (2 * sizeof(short)) , stream, 0, 2, stream, 1, 2);
}

//
// Seq_SetGain
//
// Set the 'master' volume for the sequencer. Affects
// all sounds that are played
//

static void Seq_SetGain(doomseq_t* seq) {
    fluid_synth_set_gain(seq->synth, seq->gain);
}

//
// Seq_SetReverb
//

static void Seq_SetReverb(doomseq_t* seq,
                          float size,
                          float damp,
                          float width,
                          float level) {
    fluid_synth_set_reverb(seq->synth, size, damp, width, level);
    fluid_synth_set_reverb_on(seq->synth, 1);
}

//
// Seq_SetConfig
//

static void Seq_SetConfig(doomseq_t* seq, const char* setting, int value) {
    fluid_settings_setint(seq->settings, setting, value);
}

//
// Song_GetTimeDivision
//

static double Song_GetTimeDivision(song_t* song) {
    return (double)song->tempo / (double)song->delta / 1000.0;
}

//
// Seq_SetStatus
//

static void Seq_SetStatus(doomseq_t* seq, int status) {
    MUTEX_LOCK()
    seq->signal = status;
    MUTEX_UNLOCK()
}

//
// Seq_WaitOnSignal
//

/*static void Seq_WaitOnSignal(doomseq_t* seq)
{
    while(1)
    {
        if(seq->signal == SEQ_SIGNAL_READY)
            break;
    }
}*/

//
// Chan_SetMusicVolume
//
// Should be set by the audio thread
//

static void Chan_SetMusicVolume(doomseq_t* seq, channel_t* chan) {
    int vol;

    vol = (int)((chan->volume * seq->musicvolume) / 127.0f);

    fluid_synth_cc(seq->synth, chan->track->channel, 0x07, vol);
}

//
// Chan_SetSoundVolume
//
// Should be set by the audio thread
//

static void Chan_SetSoundVolume(doomseq_t* seq, channel_t* chan) {
    int vol;
    int pan;

    vol = (int)((chan->volume * seq->soundvolume) / 127.0f);
    pan = chan->pan;

    fluid_synth_cc(seq->synth, chan->id, 0x07, vol);
    fluid_synth_cc(seq->synth, chan->id, 0x0A, pan);
}

//
// Chan_GetNextMidiByte
//
// Gets the next byte in a midi track
//

static byte Chan_GetNextMidiByte(channel_t* chan) {
    if((dword)(chan->pos - chan->song->data) >= chan->song->length) {
        I_Error("Chan_GetNextMidiByte: Unexpected end of track");
    }

    return *chan->pos++;
}

//
// Chan_CheckTrackEnd
//
// Checks if the midi reader has reached the end
//

static dboolean Chan_CheckTrackEnd(channel_t* chan) {
    return ((dword)(chan->pos - chan->song->data) >= chan->song->length);
}

//
// Chan_GetNextTick
//
// Read the midi track to get the next delta time
//

static dword Chan_GetNextTick(channel_t* chan) {
    dword tic;
    int i;

    tic = Chan_GetNextMidiByte(chan);
    if(tic & 0x80) {
        byte mb;

        tic = tic & 0x7f;

        //
        // the N64 version loops infinitely but since the
        // delta time can only be four bytes long, just loop
        // for the remaining three bytes..
        //
        for(i = 0; i < 3; i++) {
            mb = Chan_GetNextMidiByte(chan);
            tic = (mb & 0x7f) + (tic << 7);

            if(!(mb & 0x80)) {
                break;
            }
        }
    }

    return (chan->starttic + (dword)((double)tic * chan->song->timediv));
}

//
// Chan_StopTrack
//
// Stops a specific channel and any played sounds
//

static void Chan_StopTrack(doomseq_t* seq, channel_t* chan) {
    int c;

    if(chan->song->type >= 1) {
        c = chan->track->channel;
    }
    else {
        c = chan->id;
    }

    fluid_synth_cc(seq->synth, c, 0x78, 0);
}

//
// Song_ClearPlaylist
//

static void Song_ClearPlaylist(void) {
    int i;

    for(i = 0; i < MIDI_CHANNELS; i++) {
        dmemset(&playlist[i], 0, sizeof(song_t));

        playlist[i].id      = i;
        playlist[i].state   = CHAN_STATE_READY;
    }
}

//
// Chan_RemoveTrackFromPlaylist
//

static dboolean Chan_RemoveTrackFromPlaylist(doomseq_t* seq, channel_t* chan) {
    if(!chan->song || !chan->track) {
        return false;
    }

    Chan_StopTrack(seq, chan);

    chan->song      = NULL;
    chan->track     = NULL;
    chan->jump      = NULL;
    chan->tics      = 0;
    chan->nexttic   = 0;
    chan->lasttic   = 0;
    chan->starttic  = 0;
    chan->curtime   = 0;
    chan->starttime = 0;
    chan->pos       = 0;
    chan->key       = 0;
    chan->velocity  = 0;
    chan->depth     = 0;
    chan->state     = CHAN_STATE_ENDED;
    chan->paused    = false;
    chan->stop      = false;
    chan->volume    = 0.0f;
    chan->basevol   = 0.0f;
    chan->pan       = 0;
    chan->origin    = NULL;

    seq->voices--;

    return true;
}

//
// Song_AddTrackToPlaylist
//
// Add a song to the playlist for the sequencer to play.
// Sets any default values to the channel in the process
//

static channel_t* Song_AddTrackToPlaylist(doomseq_t* seq, song_t* song, track_t* track) {
    int i;

    for(i = 0; i < MIDI_CHANNELS; i++) {
        if(playlist[i].song == NULL) {
            playlist[i].song        = song;
            playlist[i].track       = track;
            playlist[i].tics        = 0;
            playlist[i].lasttic     = 0;
            playlist[i].starttic    = 0;
            playlist[i].pos         = track->data;
            playlist[i].jump        = NULL;
            playlist[i].state       = CHAN_STATE_READY;
            playlist[i].paused      = false;
            playlist[i].stop        = false;
            playlist[i].key         = 0;
            playlist[i].velocity    = 0;

            // channels 0 through 15 are reserved for music only
            // channel ids should only be accessed by non-music sounds
            playlist[i].id          = 0x0f + i;

            playlist[i].volume      = 127.0f;
            playlist[i].basevol     = 127.0f;
            playlist[i].pan         = 64;
            playlist[i].origin      = NULL;
            playlist[i].depth       = 0;
            playlist[i].starttime   = 0;
            playlist[i].curtime     = 0;

            // immediately start reading the midi track
            playlist[i].nexttic     = Chan_GetNextTick(&playlist[i]);

            seq->voices++;

            return &playlist[i];
        }
    }

    return NULL;
}

//
// Event_NoteOff
//

static void Event_NoteOff(doomseq_t* seq, channel_t* chan) {
    chan->key       = Chan_GetNextMidiByte(chan);
    chan->velocity  = 0;

    fluid_synth_noteoff(seq->synth, chan->track->channel, chan->key);
}

//
// Event_NoteOn
//

static void Event_NoteOn(doomseq_t* seq, channel_t* chan) {
    chan->key       = Chan_GetNextMidiByte(chan);
    chan->velocity  = Chan_GetNextMidiByte(chan);

    fluid_synth_cc(seq->synth, chan->id, 0x5B, chan->depth);
    fluid_synth_noteon(seq->synth, chan->track->channel, chan->key, chan->velocity);
}

//
// Event_ControlChange
//

static void Event_ControlChange(doomseq_t* seq, channel_t* chan) {
    int ctrl;
    int val;

    ctrl = Chan_GetNextMidiByte(chan);
    val = Chan_GetNextMidiByte(chan);

    if(ctrl == 0x07) {  // update volume
        if(chan->song->type == 1) {
            chan->volume = ((float)val * seq->musicvolume) / 127.0f;
            Chan_SetMusicVolume(seq, chan);
        }
        else {
            chan->volume = ((float)val * chan->volume) / 127.0f;
            Chan_SetSoundVolume(seq, chan);
        }
    }
    else {
        fluid_synth_cc(seq->synth, chan->track->channel, ctrl, val);
    }
}

//
// Event_ProgramChange
//

static void Event_ProgramChange(doomseq_t* seq, channel_t* chan) {
    int program;

    program = Chan_GetNextMidiByte(chan);

    fluid_synth_program_change(seq->synth, chan->track->channel, program);
}

//
// Event_ChannelPressure
//

static void Event_ChannelPressure(doomseq_t* seq, channel_t* chan) {
    int val;

    val = Chan_GetNextMidiByte(chan);

    fluid_synth_channel_pressure(seq->synth, chan->track->channel, val);
}

//
// Event_PitchBend
//

static void Event_PitchBend(doomseq_t* seq, channel_t* chan) {
    int b1;
    int b2;

    b1 = Chan_GetNextMidiByte(chan);
    b2 = Chan_GetNextMidiByte(chan);

    fluid_synth_pitch_bend(seq->synth, chan->track->channel, ((b2 << 8) | b1) >> 1);
}

//
// Event_Meta
//

static void Event_Meta(doomseq_t* seq, channel_t* chan) {
    int meta;
    int b;
    int i;
    char string[256];

    meta = Chan_GetNextMidiByte(chan);

    switch(meta) {
    // mostly for debugging/logging
    case MIDI_MESSAGE:
        b = Chan_GetNextMidiByte(chan);
        dmemset(string, 0, 256);

        for(i = 0; i < b; i++) {
            string[i] = Chan_GetNextMidiByte(chan);
        }

        string[b + 1] = '\n';
        break;

    case MIDI_END:
        b = Chan_GetNextMidiByte(chan);
        Chan_RemoveTrackFromPlaylist(seq, chan);
        break;

    case MIDI_SET_TEMPO:
        b = Chan_GetNextMidiByte(chan);   // length

        if(b != 3) {
            return;
        }

        chan->song->tempo =
            (Chan_GetNextMidiByte(chan) << 16) |
            (Chan_GetNextMidiByte(chan) << 8)  |
            (Chan_GetNextMidiByte(chan) & 0xff);

        if(chan->song->tempo == 0) {
            return;
        }

        chan->song->timediv = Song_GetTimeDivision(chan->song);
        chan->starttime = chan->curtime;
        break;

    // game-specific midi event
    case MIDI_SEQUENCER:
        b = Chan_GetNextMidiByte(chan);   // length
        b = Chan_GetNextMidiByte(chan);   // manufacturer (should be 0)
        if(!b) {
            b = Chan_GetNextMidiByte(chan);
            if(b == 0x23) {
                // set jump position
                chan->jump = chan->pos;
            }
            else if(b == 0x20) {
                b = Chan_GetNextMidiByte(chan);
                b = Chan_GetNextMidiByte(chan);

                // goto jump position
                if(chan->jump) {
                    chan->pos = chan->jump;
                }
            }
        }
        break;

    default:
        break;
    }
}

static const eventhandler seqeventlist[7] = {
    Event_NoteOff,
    Event_NoteOn,
    NULL,
    Event_ControlChange,
    Event_ProgramChange,
    Event_ChannelPressure,
    Event_PitchBend
};

//
// Signal_Idle
//

static int Signal_Idle(doomseq_t* seq) {
    return 0;
}

//
// Signal_Shutdown
//

static int Signal_Shutdown(doomseq_t* seq) {
    return -1;
}

//
// Signal_StopAll
//

static int Signal_StopAll(doomseq_t* seq) {
    channel_t* c;
    int i;

    SEMAPHORE_LOCK()
    for(i = 0; i < MIDI_CHANNELS; i++) {
        c = &playlist[i];

        if(c->song) {
            Chan_RemoveTrackFromPlaylist(seq, c);
        }
    }
    SEMAPHORE_UNLOCK()

    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

//
// Signal_Reset
//

static int Signal_Reset(doomseq_t* seq) {
    fluid_synth_system_reset(seq->synth);

    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

//
// Signal_Pause
//
// Pause all currently playing songs
//

static int Signal_Pause(doomseq_t* seq) {
    int i;
    channel_t* c;

    SEMAPHORE_LOCK()
    for(i = 0; i < MIDI_CHANNELS; i++) {
        c = &playlist[i];

        if(c->song && !c->paused) {
            c->paused = true;
            Chan_StopTrack(seq, c);
        }
    }
    SEMAPHORE_UNLOCK()

    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

//
// Signal_Resume
//
// Resume all songs that were paused
//

static int Signal_Resume(doomseq_t* seq) {
    int i;
    channel_t* c;

    SEMAPHORE_LOCK()
    for(i = 0; i < MIDI_CHANNELS; i++) {
        c = &playlist[i];

        if(c->song && c->paused) {
            c->paused = false;
            fluid_synth_noteon(seq->synth, c->track->channel, c->key, c->velocity);
        }
    }
    SEMAPHORE_UNLOCK()

    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

//
// Signal_UpdateGain
//

static int Signal_UpdateGain(doomseq_t* seq) {
    SEMAPHORE_LOCK()

    Seq_SetGain(seq);

    SEMAPHORE_UNLOCK()

    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

static const signalhandler seqsignallist[MAXSIGNALTYPES] = {
    Signal_Idle,
    Signal_Shutdown,
    NULL,
    Signal_Reset,
    Signal_Pause,
    Signal_Resume,
    Signal_StopAll,
    Signal_UpdateGain
};

//
// Chan_CheckState
//

static dboolean Chan_CheckState(doomseq_t* seq, channel_t* chan) {
    if(chan->state == CHAN_STATE_ENDED) {
        return true;
    }
    else if(chan->state == CHAN_STATE_READY && chan->paused) {
        chan->state = CHAN_STATE_PAUSED;
        chan->lasttic = chan->nexttic - chan->tics;
        return true;
    }
    else if(chan->state == CHAN_STATE_PAUSED) {
        if(!chan->paused) {
            chan->nexttic = chan->tics + chan->lasttic;
            chan->state = CHAN_STATE_READY;
        }
        else {
            return true;
        }
    }

    return false;
}

//
// Chan_RunSong
//
// Main midi parsing routine
//

static void Chan_RunSong(doomseq_t* seq, channel_t* chan, dword msecs) {
    byte event;
    byte c;
    song_t* song;
    byte channel;
    track_t* track;

    song = chan->song;
    track = chan->track;

    //
    // get next tic
    //
    if(chan->starttime == 0) {
        chan->starttime = msecs;
    }

    // villsa 12292013 - try to get precise timing to avoid de-syncs
    chan->curtime = msecs;
    chan->tics += ((chan->curtime - chan->starttime) - chan->tics);

    if(Chan_CheckState(seq, chan)) {
        return;
    }

    //
    // keep parsing through midi track until
    // the end is reached or until it reaches next
    // delta time
    //
    while(chan->state != CHAN_STATE_ENDED) {
        if(chan->song->type == 0) {
            chan->volume = chan->basevol;
            Chan_SetSoundVolume(seq, chan);
        }
        else {
            Chan_SetMusicVolume(seq, chan);
        }

        //
        // not ready to execute events yet
        //
        if(chan->tics < chan->nexttic) {
            return;
        }

        chan->starttic = chan->nexttic;
        c = Chan_GetNextMidiByte(chan);

        if(c == 0xff) {
            Event_Meta(seq, chan);
        }
        else {
            eventhandler eventhandle;

            event = (c >> 4) - 0x08;
            channel = c & 0x0f;

            if(event >= 0 && event < 7) {
                //
                // for music, use the generic midi channel
                // but for sounds, use the assigned id
                //
                if(song->type >= 1) {
                    track->channel = channel;
                }
                else {
                    track->channel = chan->id;
                }

                eventhandle = seqeventlist[event];

                if(eventhandle != NULL) {
                    eventhandle(seq, chan);
                }
            }
        }

        //
        // check for end of the track, otherwise get
        // the next delta time
        //
        if(chan->state != CHAN_STATE_ENDED) {
            if(Chan_CheckTrackEnd(chan)) {
                chan->state = CHAN_STATE_ENDED;
            }
            else {
                chan->nexttic = Chan_GetNextTick(chan);
            }
        }
    }
}

//
// Seq_RunSong
//

static void Seq_RunSong(doomseq_t* seq, dword msecs) {
    int i;
    channel_t* chan;

    seq->playtime = msecs;

    SEMAPHORE_LOCK()
    for(i = 0; i < MIDI_CHANNELS; i++) {
        chan = &playlist[i];

        if(!chan->song) {
            continue;
        }

        if(chan->stop) {
            Chan_RemoveTrackFromPlaylist(seq, chan);
        }
        else {
            Chan_RunSong(seq, chan, msecs);
        }
    }
    SEMAPHORE_UNLOCK()
}

//
// Song_RegisterTracks
//
// Allocate data for all tracks for a midi song
//

static dboolean Song_RegisterTracks(song_t* song) {
    int i;
    byte* data;

    song->tracks = (track_t*)Z_Calloc(sizeof(track_t) * song->ntracks, PU_STATIC, 0);
    data = song->data + 0x0e;

    for(i = 0; i < song->ntracks; i++) {
        track_t* track = &song->tracks[i];

        dmemcpy(track, data, 8);
        if(dstrncmp(track->header, "MTrk", 4)) {
            return false;
        }

        data = data + 8;

        track->length   = I_SwapBE32(track->length);
        track->data     = data;

        data = data + track->length;
    }

    return true;
}

//
// Seq_RegisterSongs
//
// Allocate data for all midi songs
//

// Doom64EX expects audio to be loaded in this order since the sound indices are hardcoded in info.cc
static const std::array<StringView, 117> audio_lumps_ {{
    "NOSOUND", "SNDPUNCH", "SNDSPAWN", "SNDEXPLD", "SNDIMPCT", "SNDPSTOL", "SNDSHTGN", "SNDPLSMA", "SNDBFG",
    "SNDSAWUP", "SNDSWIDL", "SNDSAW1", "SNDSAW2", "SNDMISLE", "SNDBFGXP", "SNDPSTRT", "SNDPSTOP", "SNDDORUP",
    "SNDDORDN", "SNDSCMOV", "SNDSWCH1", "SNDSWCH2", "SNDITEM", "SNDSGCK", "SNDOOF1", "SNDTELPT", "SNDOOF2",
    "SNDSHT2F", "SNDLOAD1", "SNDLOAD2", "SNDPPAIN", "SNDPLDIE", "SNDSLOP", "SNDZSIT1", "SNDZSIT2", "SNDZSIT3",
    "SNDZDIE1", "SNDZDIE2", "SNDZDIE3", "SNDZACT", "SNDPAIN1", "SNDPAIN2", "SNDDBACT", "SNDSCRCH", "SNDISIT1",
    "SNDISIT2", "SNDIDIE1", "SNDIDIE2", "SNDIACT", "SNDSGSIT", "SNDSGATK", "SNDSGDIE", "SNDB1SIT", "SNDB1DIE",
    "SNDHDSIT", "SNDHDDIE", "SNDSKATK", "SNDB2SIT", "SNDB2DIE", "SNDPESIT", "SNDPEPN", "SNDPEDIE", "SNDBSSIT",
    "SNDBSDIE", "SNDBSLFT", "SNDBSSMP", "SNDFTATK", "SNDFTSIT", "SNDFTHIT", "SNDFTDIE", "SNDBDMSL", "SNDRVACT",
    "SNDTRACR", "SNDDART", "SNDRVHIT", "SNDCYSIT", "SNDCYDTH", "SNDCYHOF", "SNDMETAL", "SNDDOR2U", "SNDDOR2D",
    "SNDPWRUP", "SNDLASER", "SNDBUZZ", "SNDTHNDR", "SNDLNING", "SNDQUAKE", "SNDDRTHT", "SNDRCACT", "SNDRCATK",
    "SNDRCDIE", "SNDRCPN", "SNDRCSIT", "MUSAMB01", "MUSAMB02", "MUSAMB03", "MUSAMB04", "MUSAMB05", "MUSAMB06",
    "MUSAMB07", "MUSAMB08", "MUSAMB09", "MUSAMB10", "MUSAMB11", "MUSAMB12", "MUSAMB13", "MUSAMB14", "MUSAMB15",
    "MUSAMB16", "MUSAMB17", "MUSAMB18", "MUSAMB19", "MUSAMB20", "MUSFINAL", "MUSDONE", "MUSINTRO", "MUSTITLE" }};

size_t Seq_SoundLookup(StringView name) {
    return std::distance(audio_lumps_.begin(), std::find(audio_lumps_.begin(), audio_lumps_.end(), name));
}

static bool Seq_RegisterSongs(doomseq_t* seq) {
    seq->nsongs = audio_lumps_.size();

    seq->songs = (song_t*)Z_Calloc(seq->nsongs * sizeof(song_t), PU_STATIC, 0);

    size_t fail {};
    size_t i {};
    for(auto name : audio_lumps_) {
        auto opt = wad::find(name);

        if (!opt || opt->section() != wad::Section::sounds) {
            fail++;
            continue;
        }

        auto& lump = *opt;
        song_t* song;

        auto bytes = lump.as_bytes();
        auto memory = new char[bytes.size()];
        std::copy(bytes.begin(), bytes.end(), memory);
        song = &seq->songs[i++];
        song->data = (byte*) memory;
        song->length = bytes.size();

        if(!song->length) {
            continue;
        }

        dmemcpy(song, song->data, 0x0e);
        if(dstrncmp(song->header, "MThd", 4)) {
            fail++;
            continue;
        }

        song->chunksize = I_SwapBE32(song->chunksize);
        song->ntracks   = I_SwapBE16(song->ntracks);
        song->delta     = I_SwapBE16(song->delta);
        song->type      = I_SwapBE16(song->type);
        song->timediv   = Song_GetTimeDivision(song);
        song->tempo     = 480000;

        if(!Song_RegisterTracks(song)) {
            fail++;
            continue;
        }
    }

    if (fail) {
        I_Printf("Failed to load %d MIDI tracks.\n", fail);
    }

    return true;
}

//
// Seq_Shutdown
//

static void Seq_Shutdown(doomseq_t* seq) {
    //
    // signal the sequencer to shut down
    //
    Seq_SetStatus(seq, SEQ_SIGNAL_SHUTDOWN);

    //
    // wait until the audio thread is finished
    //
    SDL_WaitThread(seq->thread, NULL);

    //
    // prevent calls to Audio_Play()
    //
    SDL_CloseAudio();

    //
    // fluidsynth cleanup stuff
    //
    delete_fluid_synth(seq->synth);
    delete_fluid_settings(seq->settings);

    seq->synth = NULL;
    seq->driver = NULL;
    seq->settings = NULL;
}

//
// Thread_PlayerHandler
//
// Main routine of the audio thread
//

static int SDLCALL Thread_PlayerHandler(void *param) {
    doomseq_t* seq = (doomseq_t*)param;
    long start = SDL_GetTicks();
    long delay = 0;
    int status;
    dword count = 0;
    signalhandler signal;

    while(1) {
        //
        // check status of the sequencer
        //
        signal = seqsignallist[seq->signal];

        if(signal) {
            status = signal(seq);

            if(status == 0) {
                // villsa 12292013 - add a delay here so we don't
                // thrash the loop while idling
                SDL_Delay(1);
                continue;
            }

            if(status == -1) {
                return 1;
            }
        }

        //
        // play some songs
        //
        Seq_RunSong(seq, SDL_GetTicks() - start);
        count++;

        // try to avoid incremental time de-syncs
        delay = count - (SDL_GetTicks() - start);

        if(delay > 0) {
            SDL_Delay(delay);
        }
    }

    return 0;
}

//
// I_InitSequencer
//

fluid_sfloader_t* rom_soundfont();
void I_InitSequencer(void) {
    dboolean sffound;
    Optional<String> sfpath;

    CON_DPrintf("--------Initializing Software Synthesizer--------\n");

    //
    // init mutex
    //
    lock = SDL_CreateMutex();
    if(lock == NULL) {
        CON_Warnf("I_InitSequencer: failed to create mutex");
        return;
    }

    //
    // init semaphore
    //
    semaphore = SDL_CreateSemaphore(1);
    if(semaphore == NULL) {
        CON_Warnf("I_InitSequencer: failed to create semaphore");
        return;
    }

    dmemset(&doomseq, 0, sizeof(doomseq_t));

    //
    // init sequencer thread
    //

    // villsa 12292013 - I can't guarantee that this will resolve
    // the issue of the titlemap/intermission/finale music to be
    // off-sync when uncapped framerates are enabled but for some
    // reason, calling SDL_GetTicks before initalizing the thread
    // will reduce the chances of it happening
    SDL_GetTicks();

    doomseq.thread = SDL_CreateThread(Thread_PlayerHandler, "SynthPlayer", &doomseq);
    if(doomseq.thread == NULL) {
        CON_Warnf("I_InitSequencer: failed to create audio thread");
        return;
    }

    //
    // init settings
    //
    doomseq.settings = new_fluid_settings();
    Seq_SetConfig(&doomseq, "synth.midi-channels", 0x10 + MIDI_CHANNELS);
    Seq_SetConfig(&doomseq, "synth.polyphony", 256);

    //
    // init synth
    //
    doomseq.synth = new_fluid_synth(doomseq.settings);
    if(doomseq.synth == NULL) {
        CON_Warnf("I_InitSequencer: failed to create synthesizer");
        return;
    }

    fluid_synth_add_sfloader(doomseq.synth, rom_soundfont());

    sffound = false;
    if (!s_soundfont->empty()) {
        if (app::file_exists(*s_soundfont)) {
            I_Printf("Found SoundFont %s\n", s_soundfont->c_str());
            doomseq.sfont_id = fluid_synth_sfload(doomseq.synth, s_soundfont->c_str(), 1);

            CON_DPrintf("Loading %s\n", s_soundfont->c_str());

            sffound = true;
        } else {
            CON_Warnf("CVar s_soundfont doesn't point to a file.");
        }
    }

    if (!sffound && (sfpath = app::find_data_file("doomsnd.sf2"))) {
        I_Printf("Found SoundFont %s\n", sfpath->c_str());
        doomseq.sfont_id = fluid_synth_sfload(doomseq.synth, sfpath->c_str(), 1);

        CON_DPrintf("Loading %s\n", sfpath->c_str());

        sffound = true;
    }

    //
    // set state
    //
    doomseq.gain = 1.0f;

    Seq_SetStatus(&doomseq, SEQ_SIGNAL_READY);
    Seq_SetGain(&doomseq);
    Seq_SetReverb(&doomseq, 0.65f, 0.0f, 2.0f, 1.0f);

    //
    // if something went terribly wrong, then shutdown everything
    //
    if(!Seq_RegisterSongs(&doomseq)) {
        CON_Warnf("I_InitSequencer: Failed to register songs\n");
        Seq_Shutdown(&doomseq);
        return;
    }

    //
    // where's the soundfont file? not found then shutdown everything
    //
    if(doomseq.sfont_id == -1) {
        CON_Warnf("I_InitSequencer: Failed to find soundfont file\n");
        Seq_Shutdown(&doomseq);
        return;
    }

    Song_ClearPlaylist();

    if (!SDL_WasInit(0))
        SDL_Init(0);

    if (!SDL_WasInit(SDL_INIT_AUDIO))
        SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_AudioSpec spec;

    spec.format = AUDIO_S16;
    spec.freq = 44100;
    spec.samples = 4096;
    spec.channels = 2;
    spec.callback = Audio_Play;
    spec.userdata = doomseq.synth;

    SDL_OpenAudio(&spec, NULL);
    SDL_PauseAudio(SDL_FALSE);

    // 20120205 villsa - sequencer is now ready
    seqready = true;
}

//
// I_GetMaxChannels
//

int I_GetMaxChannels(void) {
    return MIDI_CHANNELS;
}

//
// I_GetVoiceCount
//

int I_GetVoiceCount(void) {
    return doomseq.voices;
}

//
// I_GetSoundSource
//

sndsrc_t* I_GetSoundSource(int c) {
    if(playlist[c].song == NULL) {
        return NULL;
    }

    return playlist[c].origin;
}

//
// I_RemoveSoundSource
//

void I_RemoveSoundSource(int c) {
    playlist[c].origin = NULL;
}

//
// I_UpdateChannel
//

void I_UpdateChannel(int c, int volume, int pan) {
    channel_t* chan;

    chan            = &playlist[c];
    chan->basevol   = (float)volume;
    chan->pan       = (byte)(pan >> 1);
}

//
// I_ShutdownSound
//

void I_ShutdownSound(void) {
    if(doomseq.synth) {
        Seq_Shutdown(&doomseq);
    }
}

//
// I_SetMusicVolume
//

void I_SetMusicVolume(float volume) {
    doomseq.musicvolume = (volume * 1.125f);
}

//
// I_SetSoundVolume
//

void I_SetSoundVolume(float volume) {
    doomseq.soundvolume = (volume * 0.925f);
}

//
// I_ResetSound
//

void I_ResetSound(void) {
    if(!seqready) {
        return;
    }

    Seq_SetStatus(&doomseq, SEQ_SIGNAL_RESET);
    //Seq_WaitOnSignal(&doomseq);
}

//
// I_PauseSound
//

void I_PauseSound(void) {
    if(!seqready) {
        return;
    }

    Seq_SetStatus(&doomseq, SEQ_SIGNAL_PAUSE);
    //Seq_WaitOnSignal(&doomseq);
}

//
// I_ResumeSound
//

void I_ResumeSound(void) {
    if(!seqready) {
        return;
    }

    Seq_SetStatus(&doomseq, SEQ_SIGNAL_RESUME);
    //Seq_WaitOnSignal(&doomseq);
}

//
// I_SetGain
//

void I_SetGain(float db) {
    if(!seqready) {
        return;
    }

    doomseq.gain = db;

    Seq_SetStatus(&doomseq, SEQ_SIGNAL_SETGAIN);
    //Seq_WaitOnSignal(&doomseq);
}

//
// I_StartMusic
//

void I_StartMusic(int mus_id) {
    song_t* song;
    channel_t* chan;
    int i;

    if(!seqready) {
        return;
    }

    SEMAPHORE_LOCK()
    song = &doomseq.songs[mus_id];
    for(i = 0; i < song->ntracks; i++) {
        chan = Song_AddTrackToPlaylist(&doomseq, song, &song->tracks[i]);

        if(chan == NULL) {
            break;
        }

        chan->volume = doomseq.musicvolume;
    }
    SEMAPHORE_UNLOCK()
}

//
// I_StopSound
//

void I_StopSound(sndsrc_t* origin, int sfx_id) {
    song_t* song;
    channel_t* c;
    int i;

    if(!seqready) {
        return;
    }

    SEMAPHORE_LOCK()
    song = &doomseq.songs[sfx_id];
    for(i = 0; i < MIDI_CHANNELS; i++) {
        c = &playlist[i];

        if(song == c->song || (origin && c->origin == origin)) {
            c->stop = true;
        }
    }
    SEMAPHORE_UNLOCK()
}

//
// I_StartSound
//

void I_StartSound(int sfx_id, sndsrc_t* origin, int volume, int pan, int reverb) {
    song_t* song;
    channel_t* chan;
    int i;

    if(!seqready) {
        return;
    }

    if(doomseq.nsongs <= 0) {
        return;
    }

    SEMAPHORE_LOCK()
    song = &doomseq.songs[sfx_id];
    for(i = 0; i < song->ntracks; i++) {
        chan = Song_AddTrackToPlaylist(&doomseq, song, &song->tracks[i]);

        if(chan == NULL) {
            break;
        }

        chan->volume = (float)volume;
        chan->pan = (byte)(pan >> 1);
        chan->origin = origin;
        chan->depth = reverb;
    }
    SEMAPHORE_UNLOCK()
}

