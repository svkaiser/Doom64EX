// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: Sound.c 1096 2012-03-31 18:28:01Z svkaiser $
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
// $Revision: 1096 $
// $Date: 2012-03-31 11:28:01 -0700 (Sat, 31 Mar 2012) $
//
// DESCRIPTION: Finds and process Doom 64's audio library files and implements them
//              in the IWAD.
//
//-----------------------------------------------------------------------------

#include <string.h>

#include "wadgen.h"
#include "rom.h"
#include "sound.h"

#ifdef USE_SOUNDFONTS
#include "sndfont.h"
#endif

#define ROM_SNDTITLE 0x34364E53	// SN64
#define ROM_SEQTITLE 0x51455353	// SSEQ

#define WAV_HEADER_SIZE     0x2C
#define MIDI_HEADER_SIZE    0x0E

sn64_t *sn64;
patch_t *patches;
subpatch_t *subpatches;
wavtable_t *sfx;
loopinfo_t *loopinfo;
looptable_t *looptable;
predictor_t *predictors;
sseq_t *sseq;
entry_t *entries;
cache *wavtabledata;

uint sn64size;
uint sseqsize;

byte *sfxdata;

static byte fillbuffer[32768];
static uint newbankoffset = 0;

static const short itable[16] = {
	0, 1, 2, 3, 4, 5, 6, 7,
	-8, -7, -6, -5, -4, -3, -2, -1,
};

midiheader_t *midis;

//
// lump names for midi tracks
//
const char *sndlumpnames[] = {
	"NOSOUND",
	"SNDPUNCH",
	"SNDSPAWN",
	"SNDEXPLD",
	"SNDIMPCT",
	"SNDPSTOL",
	"SNDSHTGN",
	"SNDPLSMA",
	"SNDBFG",
	"SNDSAWUP",
	"SNDSWIDL",
	"SNDSAW1",
	"SNDSAW2",
	"SNDMISLE",
	"SNDBFGXP",
	"SNDPSTRT",
	"SNDPSTOP",
	"SNDDORUP",
	"SNDDORDN",
	"SNDSCMOV",
	"SNDSWCH1",
	"SNDSWCH2",
	"SNDITEM",
	"SNDSGCK",
	"SNDOOF1",
	"SNDTELPT",
	"SNDOOF2",
	"SNDSHT2F",
	"SNDLOAD1",
	"SNDLOAD2",
	"SNDPPAIN",
	"SNDPLDIE",
	"SNDSLOP",
	"SNDZSIT1",
	"SNDZSIT2",
	"SNDZSIT3",
	"SNDZDIE1",
	"SNDZDIE2",
	"SNDZDIE3",
	"SNDZACT",
	"SNDPAIN1",
	"SNDPAIN2",
	"SNDDBACT",
	"SNDSCRCH",
	"SNDISIT1",
	"SNDISIT2",
	"SNDIDIE1",
	"SNDIDIE2",
	"SNDIACT",
	"SNDSGSIT",
	"SNDSGATK",
	"SNDSGDIE",
	"SNDB1SIT",
	"SNDB1DIE",
	"SNDHDSIT",
	"SNDHDDIE",
	"SNDSKATK",
	"SNDB2SIT",
	"SNDB2DIE",
	"SNDPESIT",
	"SNDPEPN",
	"SNDPEDIE",
	"SNDBSSIT",
	"SNDBSDIE",
	"SNDBSLFT",
	"SNDBSSMP",
	"SNDFTATK",
	"SNDFTSIT",
	"SNDFTHIT",
	"SNDFTDIE",
	"SNDBDMSL",
	"SNDRVACT",
	"SNDTRACR",
	"SNDDART",
	"SNDRVHIT",
	"SNDCYSIT",
	"SNDCYDTH",
	"SNDCYHOF",
	"SNDMETAL",
	"SNDDOR2U",
	"SNDDOR2D",
	"SNDPWRUP",
	"SNDLASER",
	"SNDBUZZ",
	"SNDTHNDR",
	"SNDLNING",
	"SNDQUAKE",
	"SNDDRTHT",
	"SNDRCACT",
	"SNDRCATK",
	"SNDRCDIE",
	"SNDRCPN",
	"SNDRCSIT",
	"MUSAMB01",
	"MUSAMB02",
	"MUSAMB03",
	"MUSAMB04",
	"MUSAMB05",
	"MUSAMB06",
	"MUSAMB07",
	"MUSAMB08",
	"MUSAMB09",
	"MUSAMB10",
	"MUSAMB11",
	"MUSAMB12",
	"MUSAMB13",
	"MUSAMB14",
	"MUSAMB15",
	"MUSAMB16",
	"MUSAMB17",
	"MUSAMB18",
	"MUSAMB19",
	"MUSAMB20",
	"MUSFINAL",
	"MUSDONE",
	"MUSINTRO",
	"MUSTITLE",
	NULL
};

//**************************************************************
//**************************************************************
//      Sound_CreateMidiHeader
//
//  Create a midi header
//**************************************************************
//**************************************************************

static void Sound_CreateMidiHeader(entry_t * entry, midiheader_t * mthd)
{
	strncpy(mthd->header, "MThd", 4);

	mthd->chunksize = WGen_Swap32(6);
	mthd->type = WGen_Swap16(1);
	mthd->ntracks = WGen_Swap16(entry->ntrack);
	mthd->size = MIDI_HEADER_SIZE;
	mthd->tracks = (miditrack_t*) malloc(sizeof(miditrack_t) * entry->ntrack);
}

//**************************************************************
//**************************************************************
//      Sound_GetSubPatchByNote
//
//  Returns a subpatch based on min/max note
//**************************************************************
//**************************************************************

static subpatch_t *Sound_GetSubPatchByNote(patch_t * patch, int note)
{
	int i;

	if (note >= 0) {
		for (i = 0; i < patch->length; i++) {
			subpatch_t *s = &subpatches[patch->offset + i];

			if (note >= s->minnote && note <= s->maxnote)
				return s;
		}
	}

	return &subpatches[patch->offset];
}

//**************************************************************
//**************************************************************
//      Sound_CreateMidiTrack
//
//  Convert SSEQ midi to standard midi format
//**************************************************************
//**************************************************************

static void
Sound_CreateMidiTrack(midiheader_t * mthd, track_t * track, byte * data,
		      int chan, int index, patch_t * patch)
{
	miditrack_t *mtrk;
	byte *midi;
	byte *buff;
	int i;
	int len;
	int pitchbend;
	byte temp = 0;
	int tracksize;
	char unkevent[32];
	subpatch_t *inst;
	int note = -1;
	int bendrange = 0;

	//
	// doesn't matter what order it's in. all subpatches per patch
	// should all either be an instrument or a non-instrument
	//
	inst = &subpatches[patch->offset];

	//
	// I can only assume that all tracks for a midi song
	// have the same time division
	//
	mthd->delta = WGen_Swap16(track->timediv);

	//
	// update midi type
	//
	mthd->type = WGen_Swap16(inst->instrument);

	mtrk = &mthd->tracks[chan];
	strncpy(mtrk->header, "MTrk", 4);

	//
	// avoid percussion channels (default as 9)
	//
	if (chan >= 0x09)
		chan += 1;

	memset(fillbuffer, 0, 32768);
	tracksize = 0;
	buff = fillbuffer;
	midi = data;

#define PRINTSTRING(evt)                                \
    sprintf(unkevent, "UNKNOWN EVENT: 0x%x", evt);      \
    len = strlen(unkevent);                             \
    *buff++ = 0xff;                                     \
    *buff++ = 0x07;                                     \
    *buff++ = (byte)len;                                \
    tracksize += 3;                                     \
    for(i = 0; i < len; i++)                            \
    {                                                   \
        *buff++ = unkevent[i];                          \
        tracksize++;                                    \
    }

	//
	// set initial tempo
	//
	if (chan == 0) {
		int tempo = 60000000 / track->bpm;
		byte *tmp = (byte *) & tempo;

		*buff++ = 0;
		*buff++ = 0xff;
		*buff++ = 0x51;
		*buff++ = 3;
		*buff++ = tmp[2];
		*buff++ = tmp[1];
		*buff++ = tmp[0];

		tracksize += 7;
	}
	//
	// set initial bank
	//
	if (!inst->instrument) {
		*buff++ = 0;
		*buff++ = ((0x0b << 4) | chan);
		*buff++ = 0x00;	// bank select
		*buff++ = 1;	// select bank #1

		tracksize += 4;
	}
	//
	// set initial program
	//
	*buff++ = 0;
	*buff++ = ((0x0c << 4) | chan);	// program change

	temp = (byte) track->id;
	if (temp >= newbankoffset)
		temp = (temp - newbankoffset);

	*buff++ = temp;
	tracksize += 3;

	//
	// set initial volume
	//
	*buff++ = 0;
	*buff++ = ((0x0b << 4) | chan);
	*buff++ = 0x07;
	*buff++ = track->volume;
	tracksize += 4;

	//
	// set initial pan
	//
	*buff++ = 0;
	*buff++ = ((0x0b << 4) | chan);
	*buff++ = 0x0a;
	*buff++ = track->pan;
	tracksize += 4;

	while (1) {
		do {
			*buff++ = *midi;
			tracksize++;
		} while (*midi++ & 0x80);

		// end marker
		if (*midi == 0x22) {
			*buff++ = 0xff;
			*buff++ = 0x2f;
			tracksize += 2;
			break;
		}

		switch (*midi) {
		case 0x02:	// unknown 0x02
			PRINTSTRING(*midi);
			(void)*midi++;
			break;
		case 0x07:	// program change
			*buff++ = ((0x0c << 4) | chan);
			(void)*midi++;

			temp = *midi++;
			if (temp >= newbankoffset)
				temp = (temp - newbankoffset);

			*buff++ = temp;
			(void)*midi++;
			tracksize += 2;

			break;
		case 0x09:	// pitch bend

			//
			// set pitch wheel sensitivity
			//
			if (inst->instrument
			    && inst->pwheelrange_h != bendrange) {
				bendrange = inst->pwheelrange_h;

				*buff++ = ((0x0b << 4) | chan);
				*buff++ = 0x65;
				*buff++ = 0;
				*buff++ = 0;
				tracksize += 4;
				*buff++ = ((0x0b << 4) | chan);
				*buff++ = 0x64;
				*buff++ = 0;
				*buff++ = 0;
				tracksize += 4;
				*buff++ = ((0x0b << 4) | chan);
				*buff++ = 0x06;
				*buff++ = bendrange;
				*buff++ = 0;
				tracksize += 4;
				*buff++ = ((0x0b << 4) | chan);
				*buff++ = 0x26;
				*buff++ = 0;
				*buff++ = 0;
				tracksize += 4;
				*buff++ = ((0x0b << 4) | chan);
				*buff++ = 0x65;
				*buff++ = 127;
				*buff++ = 0;
				tracksize += 4;
				*buff++ = ((0x0b << 4) | chan);
				*buff++ = 0x64;
				*buff++ = 127;
				*buff++ = 0;
				tracksize += 4;
			}

			*buff++ = ((0x0e << 4) | chan);
			(void)*midi++;

			temp = *midi++;
			pitchbend = temp;
			temp = *midi++;
			pitchbend = (signed short)((temp << 8) | pitchbend);
			pitchbend += 8192;

			if (pitchbend > 16383)
				pitchbend = 16383;

			if (pitchbend < 0)
				pitchbend = 0;

			pitchbend = (pitchbend << 1);

			*buff++ = (pitchbend & 0xff);
			*buff++ = (pitchbend >> 8);
			tracksize += 3;
			break;
		case 0x0b:	// unknown 0x0b
			PRINTSTRING(*midi);
			(void)*midi++;
			(void)*midi++;
			break;
		case 0x0c:	// global volume
			*buff++ = ((0x0b << 4) | chan);
			(void)*midi++;
			*buff++ = 0x07;
			*buff++ = *midi++;
			tracksize += 3;
			break;
		case 0x0d:	// global panning
			*buff++ = ((0x0b << 4) | chan);
			(void)*midi++;
			*buff++ = 0x0a;
			*buff++ = *midi++;
			tracksize += 3;
			break;
		case 0x0e:	// sustain pedal
			*buff++ = ((0x0b << 4) | chan);
			(void)*midi++;
			*buff++ = 0x40;
			*buff++ = *midi++;
			tracksize += 3;
			break;
		case 0x0f:	// unknown 0x0f
			PRINTSTRING(*midi);
			(void)*midi++;
			(void)*midi++;
			break;
		case 0x11:	// play note
			*buff++ = ((0x09 << 4) | chan);
			(void)*midi++;
			*buff++ = note = *midi++;
			*buff++ = *midi++;
			tracksize += 3;
			break;
		case 0x12:	// stop note
			*buff++ = ((0x09 << 4) | chan);
			(void)*midi++;
			*buff++ = *midi++;
			*buff++ = 0;
			tracksize += 3;
			break;
		case 0x20:	// jump to loop position
			*buff++ = 0xff;
			*buff++ = 0x7f;
			*buff++ = 4;
			*buff++ = 0;
			*buff++ = *midi++;
			*buff++ = *midi++;
			*buff++ = *midi++;
			tracksize += 7;
			break;
		case 0x23:	// set loop position
			*buff++ = 0xff;
			*buff++ = 0x7f;
			*buff++ = 2;
			*buff++ = 0;
			*buff++ = *midi++;
			tracksize += 5;
			break;
		}

		inst = Sound_GetSubPatchByNote(patch, note);
	}

	tracksize++;

	mtrk->length = WGen_Swap32(tracksize);
	mtrk->data = (byte*) malloc(tracksize);
	memcpy(mtrk->data, fillbuffer, tracksize);

	mthd->size += tracksize + 8;
}

//**************************************************************
//**************************************************************
//      Sound_CreateWavHeader
//
//  Create a simple WAV header for output
//**************************************************************
//**************************************************************

static void Sound_CreateWavHeader(byte * out, int SAMPLERATE, int nsamp)
{
	uint scratch;

	memcpy(out, "RIFF", 4);
	out += 4;

	scratch = nsamp * 2 + 36;
	memcpy(out, &scratch, 4);
	out += 4;

	memcpy(out, "WAVE", 4);
	out += 4;

	memcpy(out, "fmt ", 4);
	out += 4;

	scratch = 16;		// fmt length
	memcpy(out, &scratch, 4);
	out += 4;

	scratch = 1;		// uncompressed pcm
	memcpy(out, &scratch, 2);
	out += 2;

	scratch = 1;		// mono
	memcpy(out, &scratch, 2);
	out += 2;

	scratch = SAMPLERATE;	// samplerate
	memcpy(out, &scratch, 4);
	out += 4;

	scratch = 2 * SAMPLERATE;	// bytes per second
	memcpy(out, &scratch, 4);
	out += 4;

	scratch = 2;		// blockalign
	memcpy(out, &scratch, 2);
	out += 2;

	scratch = 16;		// bits per sample
	memcpy(out, &scratch, 2);
	out += 2;

	memcpy(out, "data", 4);
	out += 4;

	scratch = nsamp * 2;	// length of pcm data
	memcpy(out, &scratch, 4);
	out += 4;
}

//**************************************************************
//**************************************************************
//      Sound_Decode8
//
//  Decompress current vadpcm sample
//**************************************************************
//**************************************************************

static void
Sound_Decode8(byte * in, short *out, int index, short *pred1, short lastsmp[8])
{
	int i;
	short tmp[8];
	long total = 0;
	short sample = 0;
	short *pred2;
	long result;

	memset(out, 0, sizeof(signed short) * 8);

	pred2 = (pred1 + 8);

	for (i = 0; i < 8; i++)
		tmp[i] =
		    itable[(i & 1) ? (*in++ & 0xf) : ((*in >> 4) & 0xf)] <<
		    index;

	for (i = 0; i < 8; i++) {
		total = (pred1[i] * lastsmp[6]);
		total += (pred2[i] * lastsmp[7]);

		if (i > 0) {
			int x;

			for (x = i - 1; x > -1; x--)
				total += (tmp[((i - 1) - x)] * pred2[x]);
		}

		result = ((tmp[i] << 0xb) + total) >> 0xb;

		if (result > 32767)
			sample = 32767;
		else if (result < -32768)
			sample = -32768;
		else
			sample = (signed short)result;

		out[i] = sample;
	}

	// update the last sample set for subsequent iterations
	memcpy(lastsmp, out, sizeof(signed short) * 8);
}

//**************************************************************
//**************************************************************
//      Sound_DecodeVADPCM
//
//  Decode compressed data
//**************************************************************
//**************************************************************

static uint
Sound_DecodeVADPCM(byte * in, short *out, uint len, predictor_t * book,
		   subpatch_t * sp)
{
	short lastsmp[8];
	int index;
	int pred;
	int _len = (len / 9) * 9;	//make sure length was actually a multiple of 9
	int samples = 0;
	int x;

	for (x = 0; x < 8; x++)
		lastsmp[x] = 0;

	while (_len) {
		short *pred1;

		index = (*in >> 4) & 0xf;
		pred = (*in & 0xf);
		_len--;

		pred1 = &book->preds[pred * 16];

		Sound_Decode8(++in, out, index, pred1, lastsmp);
		in += 4;
		_len -= 4;

#ifndef USE_SOUNDFONTS
		//
		// cut speed of sound by half
		//
		if (!sp->instrument) {
			int j;
			short *rover = out;

			for (j = 0; j < 8; j++) {
				*rover = lastsmp[j];
				rover++;
				*rover = lastsmp[j];
				rover++;
			}

			out += 16;
		} else
#endif
			out += 8;

		Sound_Decode8(in, out, index, pred1, lastsmp);
		in += 4;
		_len -= 4;

#ifndef USE_SOUNDFONTS
		//
		// cut speed of sound by half
		//
		if (!sp->instrument) {
			int j;
			short *rover = out;

			for (j = 0; j < 8; j++) {
				*rover = lastsmp[j];
				rover++;
				*rover = lastsmp[j];
				rover++;
			}

			out += 16;
		} else
#endif
			out += 8;

		samples += 16;
	}

	return samples;
}

//**************************************************************
//**************************************************************
//      Sound_ProcessData
//
//  Setup sound data and convert to 22050hz PCM audio
//**************************************************************
//**************************************************************

static void
Sound_ProcessData(subpatch_t * in, wavtable_t * wavtable, int samplerate,
		  predictor_t * pred)
{
	byte *indata;
	uint indatalen;
	short *outdata;
	short *wavdata;
	uint nsamp;
#ifdef USE_SOUNDFONTS
	char samplename[20];
#endif
	uint outsize;

	indatalen = wavtable->size;
	indata = (sfxdata + wavtable->start);

	// not sure if this is the right way to go, it might be possible to simply decode a partial last block
	if (indatalen % 9) {
		while (indatalen % 9)
			indatalen--;
	}

	nsamp = (indatalen / 9) * 16;

#ifndef USE_SOUNDFONTS
	//
	// cut tempo in half for non-instruments.
	// this is a temp workaround until a synth system is implemented
	//
	if (!in->instrument)
		nsamp = nsamp * 2;
#endif

	outsize = (nsamp * sizeof(short)) + WAV_HEADER_SIZE;
	outdata = (short*) malloc(outsize);
	wavdata = outdata + (WAV_HEADER_SIZE / 2);

	Sound_CreateWavHeader((byte *) outdata, samplerate, nsamp);
	Sound_DecodeVADPCM(indata, wavdata, indatalen, pred, in);

#ifdef USE_SOUNDFONTS
	sprintf(samplename, "SFX_%03d", in->id);
	SF_AddSampleData(&soundfont, (byte *) wavdata, nsamp * 2, samplename,
			 wavtable->loopid);
#endif

	wavtable->wavsize = outsize;
	wavtabledata[wavtable->ptrindex] = (byte *) outdata;
}

//**************************************************************
//**************************************************************
//      Sound_ProcessSN64
//
//  Searches for SN64 file and processes it (byte swapping, etc)
//**************************************************************
//**************************************************************

static void Sound_ProcessSN64(void)
{
	int32 *rover;
	int32 *head;
	uint ptr;
	int i;
	byte *sn64file;

	head = (int32 *) (RomFile.data);
	rover = (int32 *) (RomFile.data + RomFile.length) - 1;

	WGen_UpdateProgress("Processing SN64 Banks...");

	while (rover - head) {
		if (*rover == ROM_SNDTITLE)
			break;

		rover--;
	}

	sn64file = (byte *) rover;
	sn64 = (sn64_t *) sn64file;
	ptr = sizeof(sn64_t);

	//
	// sanity check
	//
	if (strcmp(sn64->id, "SN64"))
		WGen_Complain("Sound_ProcessSN64: Invalid SN64 id!");

	//
	// process header
	//
	sn64->game_id = WGen_Swap32(sn64->game_id);
	sn64->ninst = WGen_Swap32(sn64->ninst);
	sn64->version_id = WGen_Swap32(sn64->version_id);
	sn64->len1 = WGen_Swap32(sn64->len1);
	sn64->npatch = WGen_Swap16(sn64->npatch);
	sn64->patchsiz = WGen_Swap16(sn64->patchsiz);
	sn64->nsubpatch = WGen_Swap16(sn64->nsubpatch);
	sn64->subpatchsiz = WGen_Swap16(sn64->subpatchsiz);
	sn64->nsfx = WGen_Swap16(sn64->nsfx);
	sn64->sfxsiz = WGen_Swap16(sn64->sfxsiz);

	//
	// process patches
	//
	patches = (patch_t *) (sn64file + ptr);
	ptr += (sn64->patchsiz * sn64->npatch) + sizeof(int);

	for (i = 0; i < sn64->npatch; i++)
		patches[i].offset = WGen_Swap16(patches[i].offset);

	//
	// process subpatches
	//
	subpatches = (subpatch_t *) (sn64file + ptr);
	ptr += (sn64->subpatchsiz * sn64->nsubpatch) + sizeof(int);

	for (i = 0; i < sn64->nsubpatch; i++) {
		subpatches[i].id = WGen_Swap16(subpatches[i].id);
		subpatches[i].attacktime =
		    WGen_Swap16(subpatches[i].attacktime);
		subpatches[i].decaytime = WGen_Swap16(subpatches[i].decaytime);
	}

	//
	// find where non-instruments begin
	//
	for (i = 0; i < sn64->npatch; i++) {
		if (!subpatches[patches[i].offset].instrument) {
			newbankoffset = i;
			break;
		}
	}

	//
	// prcess sfx
	//
	sfx = (wavtable_t *) (sn64file + ptr);
	ptr += (sn64->sfxsiz * sn64->nsfx);

	for (i = 0; i < sn64->nsfx; i++) {
		sfx[i].size = WGen_Swap32(sfx[i].size);
		sfx[i].start = WGen_Swap32(sfx[i].start);
		sfx[i].loopid = WGen_Swap32(sfx[i].loopid);
		sfx[i].pitch = WGen_Swap32(sfx[i].pitch);
		sfx[i].ptrindex = i;
	}

	wavtabledata = (byte**) malloc(sizeof(cache) * sn64->nsfx);

	for (i = 0; i < sn64->nsfx; i++)
		wavtabledata[i] = NULL;

	//
	// process loopinfo
	//
	loopinfo = (loopinfo_t *) (sn64file + ptr);
	ptr += sizeof(loopinfo_t);

	loopinfo->ncount = WGen_Swap16(loopinfo->ncount);
	loopinfo->nsfx1 = WGen_Swap16(loopinfo->nsfx1);
	loopinfo->nsfx2 = WGen_Swap16(loopinfo->nsfx2);

	//
	// skip looptable
	//
	looptable = (looptable_t *) (sn64file + ptr);
	ptr += (((loopinfo->ncount + 1) << 1) * loopinfo->ncount);

	for (i = 0; i < loopinfo->ncount; i++) {
		looptable[i].loopstart = WGen_Swap32(looptable[i].loopstart);
		looptable[i].loopend = WGen_Swap32(looptable[i].loopend);
	}

	//
	// process predictors
	//
	predictors = (predictor_t *) (sn64file + ptr);

	for (i = 0; i < sn64->nsfx; i++) {
		int j;

		predictors[i].npredictors =
		    WGen_Swap32(predictors[i].npredictors);
		predictors[i].order = WGen_Swap32(predictors[i].order);

		for (j = 0; j < 128; j++)
			predictors[i].preds[j] =
			    WGen_Swap16(predictors[i].preds[j]);
	}

	sn64size = sizeof(sn64_t) + sn64->len1;
	_PAD16(sn64size);
}

//**************************************************************
//**************************************************************
//      Sound_ProcessSSEQ
//
//  Searches for SSEQ file and processes it (byte swapping, etc)
//**************************************************************
//**************************************************************

static void Sound_ProcessSSEQ(void)
{
	int32 *rover;
	int32 *head;
	uint ptr;
	int i;
	byte *sseqfile;

	WGen_UpdateProgress("Processing SSEQ Tracks...");

	head = (int32 *) (RomFile.data);
	rover = (int32 *) (RomFile.data + RomFile.length) - 1;

	while (rover - head) {
		if (*rover == ROM_SEQTITLE)
			break;

		rover--;
	}

	sseqfile = (byte *) rover;
	sseq = (sseq_t *) sseqfile;
	ptr = sizeof(sseq_t);

	//
	// sanity check
	//
	if (strcmp(sseq->id, "SSEQ"))
		WGen_Complain("Sound_ProcessSSEQ: Invalid SSEQ id!");

	//
	// process header
	//
	sseq->entrysiz = WGen_Swap32(sseq->entrysiz);
	sseq->nentry = WGen_Swap32(sseq->nentry);
	sseq->game_id = WGen_Swap32(sseq->game_id);

	//
	// process entries
	//
	entries = (entry_t *) (sseqfile + ptr);
	ptr += sseq->entrysiz;

	midis = (midiheader_t*) malloc(sizeof(midiheader_t) * sseq->nentry);

	for (i = 0; i < (int)sseq->nentry; i++) {
		track_t *track;
		int j;
		int *loopid;
		int tptr = 0;
		byte *data;

		WGen_UpdateProgress("Converting track %03d", i);

		entries[i].length = WGen_Swap32(entries[i].length);
		entries[i].ntrack = WGen_Swap16(entries[i].ntrack);
		entries[i].offset = WGen_Swap32(entries[i].offset);

		Sound_CreateMidiHeader(&entries[i], &midis[i]);

		//
		// process tracks
		//
		for (j = 0; j < entries[i].ntrack; j++) {
			track = (track_t *) ((sseqfile + ptr) + tptr);

			track->flag = WGen_Swap16(track->flag);
			track->id = WGen_Swap16(track->id);
			track->bpm = WGen_Swap16(track->bpm);
			track->timediv = WGen_Swap16(track->timediv);
			track->datalen = WGen_Swap16(track->datalen);
			track->loop = WGen_Swap16(track->loop);

			if (track->loop) {
				loopid =
				    (int *)(((sseqfile + ptr) + tptr) +
					    sizeof(track_t));
				data =
				    ((sseqfile + ptr) + tptr) +
				    sizeof(track_t) + 4;
				*loopid = WGen_Swap32(*loopid);
			} else
				data =
				    ((sseqfile + ptr) + tptr) + sizeof(track_t);

			//
			// something went off-sync somewhere...
			//
			if (!(track->flag & 0x100) && entries[i].ntrack > 1)
				WGen_Complain
				    ("Sound_ProcessSSEQ: bad track %i offset [entry %03d]\n",
				     j, i);

			Sound_CreateMidiTrack(&midis[i], track, data, j, i,
					      &patches[track->id]);

			tptr += (sizeof(track_t) + track->datalen);

			if (track->loop)
				tptr += 4;
		}

		ptr += entries[i].length;
		tptr = 0;
	}

	i--;

	sseqsize =
	    sseq->entrysiz + (entries[i].length + entries[i].offset) +
	    sizeof(sseq_t);
	_PAD16(sseqsize);
}

//**************************************************************
//**************************************************************
//      Sound_Setup
//**************************************************************
//**************************************************************

void Sound_Setup(void)
{
	int i;

	Sound_ProcessSN64();
	Sound_ProcessSSEQ();

	sfxdata = (((byte *) sn64 + sn64size) + sseqsize);

#ifdef USE_SOUNDFONTS
	SF_Setup();
#endif

	for (i = 0; i < sn64->nsfx; i++) {
		subpatch_t *subpatch;
		wavtable_t *wavtable;
		int j;

		for (j = 0; j < sn64->npatch; j++) {
			subpatch = &subpatches[patches[j].offset];

			if (subpatch->id != i)
				continue;

			wavtable = &sfx[subpatch->id];
      
			if (wavtabledata[wavtable->ptrindex])
				continue;

			WGen_UpdateProgress("Decompressing audio...");
			Sound_ProcessData(subpatch, wavtable, 22050,
					  &predictors[subpatch->id]);
		}
	}

#ifdef USE_SOUNDFONTS
	SF_AddSample(&soundfont.samples, "EOS", 0, 0, 0);
	SF_SetupModulators();
	SF_CreatePresets(patches, sn64->npatch, subpatches, sn64->nsubpatch,
			 sfx);
	SF_FinalizeChunkSizes();
#endif
}
