#ifndef _WADGEN_SNDFONT_H_
#define _WADGEN_SNDFONT_H_

typedef enum {

	GEN_STARTADDROFS,		/**< Sample start address offset (0-32767) */

	GEN_ENDADDROFS,			/**< Sample end address offset (-32767-0) */

	GEN_STARTLOOPADDROFS,	    /**< Sample loop start address offset (-32767-32767) */

	GEN_ENDLOOPADDROFS,	    /**< Sample loop end address offset (-32767-32767) */

	GEN_STARTADDRCOARSEOFS,	    /**< Sample start address coarse offset (X 32768) */

	GEN_MODLFOTOPITCH,	    /**< Modulation LFO to pitch */

	GEN_VIBLFOTOPITCH,	    /**< Vibrato LFO to pitch */

	GEN_MODENVTOPITCH,	    /**< Modulation envelope to pitch */

	GEN_FILTERFC,			/**< Filter cutoff */

	GEN_FILTERQ,			/**< Filter Q */

	GEN_MODLFOTOFILTERFC,	    /**< Modulation LFO to filter cutoff */

	GEN_MODENVTOFILTERFC,	    /**< Modulation envelope to filter cutoff */

	GEN_ENDADDRCOARSEOFS,	    /**< Sample end address coarse offset (X 32768) */

	GEN_MODLFOTOVOL,		/**< Modulation LFO to volume */

	GEN_UNUSED1,			/**< Unused */

	GEN_CHORUSSEND,			/**< Chorus send amount */

	GEN_REVERBSEND,			/**< Reverb send amount */

	GEN_PAN,			    /**< Stereo panning */

	GEN_UNUSED2,			/**< Unused */

	GEN_UNUSED3,			/**< Unused */

	GEN_UNUSED4,			/**< Unused */

	GEN_MODLFODELAY,		/**< Modulation LFO delay */

	GEN_MODLFOFREQ,			/**< Modulation LFO frequency */

	GEN_VIBLFODELAY,		/**< Vibrato LFO delay */

	GEN_VIBLFOFREQ,			/**< Vibrato LFO frequency */

	GEN_MODENVDELAY,		/**< Modulation envelope delay */

	GEN_MODENVATTACK,		/**< Modulation envelope attack */

	GEN_MODENVHOLD,			/**< Modulation envelope hold */

	GEN_MODENVDECAY,		/**< Modulation envelope decay */

	GEN_MODENVSUSTAIN,	    /**< Modulation envelope sustain */

	GEN_MODENVRELEASE,	    /**< Modulation envelope release */

	GEN_KEYTOMODENVHOLD,	    /**< Key to modulation envelope hold */

	GEN_KEYTOMODENVDECAY,	    /**< Key to modulation envelope decay */

	GEN_VOLENVDELAY,		/**< Volume envelope delay */

	GEN_VOLENVATTACK,		/**< Volume envelope attack */

	GEN_VOLENVHOLD,			/**< Volume envelope hold */

	GEN_VOLENVDECAY,		/**< Volume envelope decay */

	GEN_VOLENVSUSTAIN,	    /**< Volume envelope sustain */

	GEN_VOLENVRELEASE,	    /**< Volume envelope release */

	GEN_KEYTOVOLENVHOLD,	    /**< Key to volume envelope hold */

	GEN_KEYTOVOLENVDECAY,	    /**< Key to volume envelope decay */

	GEN_INSTRUMENT,			/**< Instrument ID (shouldn't be set by user) */

	GEN_RESERVED1,			/**< Reserved */

	GEN_KEYRANGE,			/**< MIDI note range */

	GEN_VELRANGE,			/**< MIDI velocity range */

	GEN_STARTLOOPADDRCOARSEOFS,
				/**< Sample start loop address coarse offset (X 32768) */

	GEN_KEYNUM,			/**< Fixed MIDI note number */

	GEN_VELOCITY,			/**< Fixed MIDI velocity value */

	GEN_ATTENUATION,		/**< Initial volume attenuation */

	GEN_RESERVED2,			/**< Reserved */

	GEN_ENDLOOPADDRCOARSEOFS,   /**< Sample end loop address coarse offset (X 32768) */

	GEN_COARSETUNE,			/**< Coarse tuning */

	GEN_FINETUNE,			/**< Fine tuning */

	GEN_SAMPLEID,			/**< Sample ID (shouldn't be set by user) */

	GEN_SAMPLEMODE,			/**< Sample mode flags */

	GEN_RESERVED3,			/**< Reserved */

	GEN_SCALETUNE,			/**< Scale tuning */

	GEN_EXCLUSIVECLASS,	    /**< Exclusive class number */

	GEN_OVERRIDEROOTKEY,	    /**< Sample root note override */

	/* the initial pitch is not a "standard" generator. It is not
	 * mentioned in the list of generator in the SF2 specifications. It
	 * is used, however, as the destination for the default pitch wheel
	 * modulator. */

	GEN_PITCH,			/**< Pitch (NOTE: Not a real SoundFont generator) */

	GEN_LAST			    /**< Value defines the count of generators (#fluid_gen_type) */
} generatorops_e;

typedef struct {
	byte byLo;
	byte byHi;
} rangetype_t;

typedef union {
	rangetype_t ranges;
	short shAmount;
	word wAmount;
} gentypes_t;

typedef struct {
	char ifil[4];
	uint size;
	word wMajor;
	word wMinor;
} sfversion_t;

typedef struct {
	char INAM[4];
	uint size;
	char *name;
} sfname_t;

typedef struct {
	char LIST[4];
	uint listsize;
	char sdta[4];
	char smpl[4];
	uint size;
	byte *wavdata;
} sfdata_t;

typedef struct {
	char achPresetName[20];
	word wPreset;
	word wBank;
	word wPresetBagNdx;
	uint dwLibrary;
	uint dwGenre;
	uint dwMorphology;
} sfpresetinfo_t;

typedef struct {
	char phdr[4];
	uint size;
	sfpresetinfo_t *presets;
} sfpheader_t;

typedef struct {
	word wGenNdx;
	word wModNdx;
} sfpresetbag_t;

typedef struct {
	char pbag[4];
	uint size;
	sfpresetbag_t *presetbags;
} sfpbagheader_t;

typedef struct {
	char pmod[4];
	uint size;
	short sfModSrcOper;
	short sfModDestOper;
	short modAmount;
	short sfModAmtSrcOper;
	short sfModTransOper;
} sfpresetmod_t;

typedef struct {
	word sfGenOper;
	gentypes_t genAmount;
} sfpresetgen_t;

typedef struct {
	char pgen[4];
	uint size;
	sfpresetgen_t *info;
} sfpgenheader_t;

typedef struct {
	sfpheader_t info;
	sfpbagheader_t bag;
	sfpresetmod_t mod;
	sfpgenheader_t gen;
} sfpreset_t;

typedef struct {
	char achInstName[20];
	word wInstBagNdx;
} sfinstinfo_t;

typedef struct {
	char imod[4];
	uint size;
	short sfModSrcOper;
	short sfModDestOper;
	short modAmount;
	short sfModAmtSrcOper;
	short sfModTransOper;
} sfinstmod_t;

typedef struct {
	char inst[4];
	uint size;
	sfinstinfo_t *instruments;
} sfinstheader_t;

typedef struct {
	word wInstGenNdx;
	word wInstModNdx;
} sfinstbag_t;

typedef struct {
	char ibag[4];
	uint size;
	sfinstbag_t *instbags;
} sfibagheader_t;

typedef struct {
	word sfGenOper;
	gentypes_t genAmount;
} sfinstgen_t;

typedef struct {
	char igen[4];
	uint size;
	sfinstgen_t *info;
} sfigenheader_t;

typedef struct {
	sfinstheader_t info;
	sfibagheader_t bag;
	sfinstmod_t mod;
	sfigenheader_t gen;
} sfinst_t;

typedef struct {
	char LIST[4];
	uint listsize;
	char pdta[4];
	sfpreset_t preset;
	sfinst_t inst;
} sfpresetlist_t;

typedef enum {
	monoSample = 1,
	rightSample = 2,
	leftSample = 4,
	linkedSample = 8,
	RomMonoSample = 0x8001,
	RomRightSample = 0x8002,
	RomLeftSample = 0x8004,
	RomLinkedSample = 0x8008
} sfsamplelink_e;

typedef struct {
	char achSampleName[20];
	uint dwStart;
	uint dwEnd;
	uint dwStartloop;
	uint dwEndloop;
	uint dwSampleRate;
	byte byOriginalPitch;
	char chPitchCorrection;
	word wSampleLink;
	word sfSampleType;
} sfsampleinfo_t;

typedef struct {
	char shdr[4];
	uint size;
	sfsampleinfo_t *info;
} sfsample_t;

typedef struct {
	char RIFF[4];
	uint filesize;
	char sfbk[4];
	char LIST[4];
	uint listsize;
	char INFO[4];
	sfversion_t version;
	sfname_t name;
	sfdata_t data;
	sfpresetlist_t presetlist;
	sfsample_t samples;
} soundfont_t;

extern soundfont_t soundfont;

void SF_Setup(void);
void SF_AddSampleData(soundfont_t * sf, cache in, size_t insize, char *newname,
		      int loopid);
void SF_AddSample(sfsample_t * sample, char *name, size_t size, size_t offset,
		  int loopid);
void SF_CreatePresets(patch_t * patch, int npatch, subpatch_t * subpatch,
		      int nsubpatch, wavtable_t * wavtable);
void SF_SetupModulators(void);
void SF_FinalizeChunkSizes(void);
void SF_WriteSoundFont(void);

#endif
