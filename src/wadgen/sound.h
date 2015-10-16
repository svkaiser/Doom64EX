#ifndef _WADGEN_SOUND_H_
#define _WADGEN_SOUND_H_

#define SFX_DATASIZE    0x1716D0

extern byte *sfxdata;

typedef struct {
	char id[4];
	uint game_id;		// must be 2
	uint pad1;
	uint version_id;	// always 100. not read or used in game
	uint pad2;
	uint pad3;
	uint len1;		// length of file minus header size
	uint pad4;
	uint ninst;		// number of instruments (31)
	word npatch;		// number of patches
	word patchsiz;		// sizeof patch struct
	word nsubpatch;		// number of subpatches
	word subpatchsiz;	// sizeof subpatch struct
	word nsfx;		// number of sfx
	word sfxsiz;		// sizeof sfx struct
	uint pad5;
	uint pad6;
} sn64_t;

extern sn64_t *sn64;
extern uint sn64size;

typedef struct {
	word length;		// length (little endian)
	word offset;		// offset in sn64 file
} patch_t;

typedef struct {
	byte unitypitch;	// unknown; used by N64 library
	byte attenuation;	// attenuation
	byte pan;		// left/right panning
	byte instrument;	// (boolean) treat this as an instrument?
	byte rootkey;		// instrument's root key
	byte detune;		// detune key (20120327 villsa - identified)
	byte minnote;		// use this subpatch if played note > minnote
	byte maxnote;		// use this subpatch if played note < maxnote
	byte pwheelrange_l;	// pitch wheel low range (20120326 villsa - identified)
	byte pwheelrange_h;	// pitch wheel high range (20120326 villsa - identified)
	word id;		// sfx id
	short attacktime;	// attack time (fade in)
	byte unk_0e;
	byte unk_0f;
	short decaytime;	// decay time (fade out)
	byte volume1;		// volume (unknown purpose)
	byte volume2;		// volume (unused?)
} subpatch_t;

typedef struct {
	uint start;		// start of rom offset
	uint size;		// size of sfx
	uint wavsize;		// suppose to be pad1 but used as temp size for wavdata
	uint pitch;		// correction pitch
	uint loopid;		// index id for loop table
	uint ptrindex;		// suppose to be pad3 but used as pointer index by wadgen
} wavtable_t;

extern wavtable_t *sfx;
extern cache *wavtabledata;

typedef struct {
	word nsfx1;		// sfx count (124)
	word pad1;
	word ncount;		// loop data count (23)
	word nsfx2;		// sfx count again?? (124)
} loopinfo_t;

typedef struct {
	uint loopstart;
	uint loopend;
	uint data[10];		// its nothing but pure initialized garbage in rom but changed/set in game
} looptable_t;

extern looptable_t *looptable;

typedef struct {
	uint order;		// order id
	uint npredictors;	// number of predictors
	short preds[128];	// predictor array
} predictor_t;

typedef struct {
	char id[4];
	uint game_id;		// must be 2
	uint pad1;
	uint nentry;		// number of entries
	uint pad2;
	uint pad3;
	uint entrysiz;		// sizeof(entry) * nentry
	uint pad4;
} sseq_t;

extern sseq_t *sseq;
extern uint sseqsize;

typedef struct {
	word ntrack;		// number of tracks
	word pad1;
	uint length;		// length of entry
	uint offset;		// offset in sseq file
	uint pad2;
} entry_t;

extern entry_t *entries;

typedef struct {
	word flag;		// usually 0 on sounds, 0x100 on musics
	word id;		// subpatch id
	word pad1;
	byte volume;		// default volume
	byte pan;		// default pan
	word pad2;
	word bpm;		// beats per minute
	word timediv;
	word loop;		// 0 if no looping, 1 if yes
	word pad3;
	word datalen;		// length of midi data
} track_t;

typedef struct {
	char header[4];
	int length;
	byte *data;
} miditrack_t;

typedef struct {
	char header[4];
	int chunksize;
	short type;
	word ntracks;
	word delta;
	uint size;
	miditrack_t *tracks;
} midiheader_t;

extern midiheader_t *midis;
extern const char *sndlumpnames[];

void Sound_Setup(void);

#endif
