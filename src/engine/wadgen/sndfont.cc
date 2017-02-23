// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: SndFont.c 1096 2012-03-31 18:28:01Z svkaiser $
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
// DESCRIPTION: SoundFont creating/writing/utilities
//              Doom64's SN64 and SSEQ libraries are not so different
//              from SoundFonts. In fact, they're almost identical...
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <i_system.h>

#include "wadgen.h"
#include "sound.h"
#include "sndfont.h"

#define SF_NEWITEM(p, t, c) p = (t*)realloc((t*)p, sizeof(t) * ++c);

uint nsampledata = 0;
soundfont_t soundfont;
sfdata_t *data;
sfpresetlist_t *list;
sfsample_t *sample;
sfpreset_t *preset;
sfinst_t *inst;

//
// not really sure how they're calculated in the actual game
// but they have to be measured in milliseconds. wouldn't make sense
// if it wasn't
//
static double usectotimecents(int usec)
{
	double t = (double)usec / 1000.0;
	return (1200 * log2(t));
}

static double pantopercent(int pan)
{
	double p = (double)((pan - 64) * 25) / 32.0f;
	return p / 0.1;
}

//
// Doom64 does some really weird stuff involving the root key, the
// currently played note and the sample's pitch correction.
// instead of mimicing what they're doing, I'll just correct
// the pitch and update the root key directly
//
static int getoriginalpitch(int key, int pitch)
{
	return ((pitch / 100) - key) * -1;
}

static double attentopercent(int attenuation)
{
	double a = (double)((127 - attenuation) * 45) / 63.5f;
	return a / 0.1;
}

//**************************************************************
//**************************************************************
//      SF_AddPreset
//**************************************************************
//**************************************************************

static int npresets = 0;
static int curpbag = 0;

void
SF_AddPreset(sfpresetheader_t * preset, const char *name, int idx, int bank, int presetidx)
{
	SF_NEWITEM(preset->presets, sd_sfpresetinfo_t, npresets);
	strncpy(preset->presets[idx].achPresetName, name, 20);
	preset->presets[idx].wPresetBagNdx = curpbag;
	preset->presets[idx].wPreset = presetidx;
	preset->presets[idx].dwGenre = 0;
	preset->presets[idx].dwLibrary = 0;
	preset->presets[idx].dwMorphology = 0;
	preset->presets[idx].wBank = bank;
	preset->sd.size = 38 * npresets;
}

//**************************************************************
//**************************************************************
//      SF_AddInstrument
//**************************************************************
//**************************************************************

static int ninstruments = 0;
void SF_AddInstrument(sfinstheader_t * inst, const char *name, int idx)
{
	SF_NEWITEM(inst->instruments, sd_sfinstinfo_t, ninstruments);
	strncpy(inst->instruments[idx].achInstName, name, 20);
	inst->instruments[idx].wInstBagNdx = idx;
	inst->sd.size += sizeof(sd_sfinstinfo_t);
}

//**************************************************************
//**************************************************************
//      SF_AddInstrumentGenerator
//**************************************************************
//**************************************************************

static int igenpos = 0;
static int nigens = 0;
void
SF_AddInstrumentGenerator(sfinstgenheader_t * gen, generatorops_e op,
			  sd_sfgen_t value)
{
	sd_sfinstgen_t *igen;

	SF_NEWITEM(gen->info, sd_sfinstgen_t, nigens);
	igen = &gen->info[nigens - 1];
	igen->sfGenOper = op;
	igen->genAmount = value;
	gen->sd.size += sizeof(sd_sfinstgen_t);
	igenpos++;
}

//**************************************************************
//**************************************************************
//      SF_AddPresetGenerator
//**************************************************************
//**************************************************************

static int pgenpos = 0;
static int npgens = 0;
void
SF_AddPresetGenerator(sfpresetgenheader_t * gen, generatorops_e op, sd_sfgen_t value)
{
	sd_sfpresetgen_t *pgen;

	SF_NEWITEM(gen->info, sd_sfpresetgen_t, npgens);
	pgen = &gen->info[npgens - 1];
	pgen->sfGenOper = op;
	pgen->genAmount = value;
	gen->sd.size += sizeof(sd_sfpresetgen_t);
	pgenpos++;
}

//**************************************************************
//**************************************************************
//      SF_AddInstrumentBag
//**************************************************************
//**************************************************************

static int nibags = 0;
void SF_AddInstrumentBag(sfinstbagheader_t * bag)
{
	sd_sfinstbag_t *instbag;

	SF_NEWITEM(bag->instbags, sd_sfinstbag_t, nibags);
	instbag = &bag->instbags[nibags - 1];
	instbag->wInstGenNdx = igenpos;
	instbag->wInstModNdx = 0;
	bag->sd.size += sizeof(sd_sfinstbag_t);
}

//**************************************************************
//**************************************************************
//      SF_AddPresetBag
//**************************************************************
//**************************************************************

static int npbags = 0;
void SF_AddPresetBag(sfpresetbagheader_t * bag)
{
	sd_sfpresetbag_t *presetbag;

	SF_NEWITEM(bag->presetbags, sd_sfpresetbag_t, npbags);
	presetbag = &bag->presetbags[npbags - 1];
	presetbag->wGenNdx = pgenpos;
	presetbag->wModNdx = 0;
	bag->sd.size += sizeof(sd_sfpresetbag_t);

	curpbag = npbags;
}

//**************************************************************
//**************************************************************
//      SF_AddSample
//**************************************************************
//**************************************************************

static int nsamples = 0;
void
SF_AddSample(sfsample_t * sample, const char *name, uint size, uint offset,
	     int loopid)
{
	sd_sfsampleinfo_t *info;
	looptable_t *lptbl;

	SF_NEWITEM(sample->info, sd_sfsampleinfo_t, nsamples);
	sample->sd.size = 46 * nsamples;

	if (loopid != -1)
		lptbl = &looptable[loopid];

	info = &sample->info[nsamples - 1];
	strncpy(info->achSampleName, name, 20);
	info->byOriginalPitch = 60;
	info->chPitchCorrection = 0;
	info->dwSampleRate = 22050;
	info->sfSampleType = (loopid != -1);
	info->wSampleLink = 0;
	info->dwStart = offset / 2;
	info->dwEnd = (size / 2) + (offset / 2);

	if (loopid == -1) {
		info->dwStartloop = offset / 2;
		info->dwEndloop = ((size / 2) + (offset / 2)) - 1;
	} else {
		info->dwStartloop = (offset / 2) + lptbl->loopstart;
		info->dwEndloop = lptbl->loopend + (offset / 2);
	}
}

//**************************************************************
//**************************************************************
//      SF_AddSampleData
//**************************************************************
//**************************************************************

static uint offset = 0;
void
SF_AddSampleData(soundfont_t * sf, cache in, size_t insize,
		 char *newname, int loopid)
{
	sfdata_t *data;

	data = &sf->data;
	data->sd.size += (insize + 64);
	data->sd.listsize = data->sd.size + 12;
	data->wavdata = (byte *) realloc((byte *) data->wavdata, data->sd.size);
	memset(data->wavdata + (data->sd.size - 64), 0, 64);
	memcpy(data->wavdata + offset, in, insize);
	SF_AddSample(&sf->samples, newname, insize, offset, loopid);

	offset = data->sd.size;
}

//**************************************************************
//**************************************************************
//      SF_SetupModulators
//**************************************************************
//**************************************************************

void SF_SetupModulators(void)
{
	preset->mod.sfModTransOper = 1;
	preset->mod.modAmount = 0;
	preset->mod.sfModAmtSrcOper = 0;
	preset->mod.sfModDestOper = 0;
	preset->mod.sfModSrcOper = 0;
	preset->mod.size = 0x0a;

	inst->mod.sfModTransOper = 3;
	inst->mod.modAmount = 0;
	inst->mod.sfModAmtSrcOper = 0;
	inst->mod.sfModDestOper = 0;
	inst->mod.sfModSrcOper = 0;
	inst->mod.size = 0x0a;
}

//**************************************************************
//**************************************************************
//      SF_FinalizeChunkSizes
//**************************************************************
//**************************************************************

void SF_FinalizeChunkSizes(void)
{
	list->sd.listsize = 0x4C;
	list->sd.listsize += preset->info.sd.size;
	list->sd.listsize += preset->bag.sd.size;
	list->sd.listsize += preset->mod.size;
	list->sd.listsize += preset->gen.sd.size;
	list->sd.listsize += inst->info.sd.size;
	list->sd.listsize += inst->bag.sd.size;
	list->sd.listsize += inst->mod.size;
	list->sd.listsize += inst->gen.sd.size;
	list->sd.listsize += sample->sd.size;

	soundfont.sd.filesize += list->sd.listsize + 8 + data->sd.listsize;
}

//**************************************************************
//**************************************************************
//      SF_CreatePresets
//**************************************************************
//**************************************************************

uint totalpresets = 0;
uint totalinstruments = 0;

void
SF_CreatePresets(patch_t * patch, int npatch,
		 subpatch_t * subpatch, int nsubpatch, wavtable_t * wavtable)
{
	int i;
	int j;
	uint banks = 0;
	uint presetcount = 0;
	sd_sfgen_t value;
	char name[20];

	WGen_UpdateProgress("Building Soundfont Presets...");

	for (i = 0; i < npatch; i++) {
		patch_t *p;

		p = &patch[i];

		//
		// I can only assume that normal sounds come after all instruments...
		// it would suck if they didn't...
		//
		if (!subpatch[p->offset].instrument && !banks) {
			presetcount = 0;
			banks++;
		}

		sprintf(name, "PRESET_%03d", totalpresets);
		SF_AddPreset(&preset->info, name, totalpresets, banks,
			     presetcount++);

		for (j = 0; j < p->length; j++) {
			subpatch_t *sp;
			wavtable_t *wt;

			SF_AddPresetBag(&preset->bag);
			value.wAmount = totalinstruments;
			SF_AddPresetGenerator(&preset->gen, GEN_INSTRUMENT,
					      value);

			sp = &subpatch[p->offset + j];
			wt = &wavtable[sp->id];

			sprintf(name, "INST_%03d", totalinstruments);
			SF_AddInstrument(&inst->info, name, totalinstruments);
			SF_AddInstrumentBag(&inst->bag);

			//
			// min/max notes
			//
			value.ranges.byLo = sp->minnote;
			value.ranges.byHi = sp->maxnote;
			SF_AddInstrumentGenerator(&inst->gen, GEN_KEYRANGE,
						  value);

			//
			// attenuation
			//
			if (sp->attenuation < 127) {
				value.wAmount =
				    (uint16)attentopercent(sp->attenuation);
				SF_AddInstrumentGenerator(&inst->gen,
							  GEN_ATTENUATION,
							  value);
			}
			//
			// panning
			//
			if (sp->pan != 64) {
				value.wAmount = (uint16)pantopercent(sp->pan);
				SF_AddInstrumentGenerator(&inst->gen, GEN_PAN,
							  value);
			}
			//
			// attack time
			//
			if (sp->attacktime > 1) {
				value.wAmount =
				    (uint16)usectotimecents(sp->attacktime);
				SF_AddInstrumentGenerator(&inst->gen,
							  GEN_VOLENVATTACK,
							  value);
			}
			//
			// release time
			//
			if (sp->decaytime > 1) {
				value.wAmount =
				    (uint16)usectotimecents(sp->decaytime);
				SF_AddInstrumentGenerator(&inst->gen,
							  GEN_VOLENVRELEASE,
							  value);
			}
			//
			// sample loops
			//
			if (wt->loopid != ~0U) {
				value.wAmount = 1;
				SF_AddInstrumentGenerator(&inst->gen,
							  GEN_SAMPLEMODE,
							  value);
			}
			//
			// root key override
			//
			value.wAmount =
				(uint16)getoriginalpitch(sp->rootkey, wt->pitch);
			SF_AddInstrumentGenerator(&inst->gen,
						  GEN_OVERRIDEROOTKEY, value);

			//
			// sample id
			//
			value.wAmount = sp->id;
			SF_AddInstrumentGenerator(&inst->gen, GEN_SAMPLEID,
						  value);

			totalinstruments++;
		}

		totalpresets++;
	}

	SF_AddPreset(&preset->info, "EOP", totalpresets, 0, ++banks);
	SF_AddPresetBag(&preset->bag);
	value.wAmount = 0;
	SF_AddPresetGenerator(&preset->gen, GEN_INSTRUMENT, value);

	SF_AddInstrument(&inst->info, "EOI", totalinstruments);
	SF_AddInstrumentBag(&inst->bag);
	value.wAmount = 0;
	SF_AddInstrumentGenerator(&inst->gen, GEN_SAMPLEID, value);
}

//**************************************************************
//**************************************************************
//      SF_WriteSoundFont
//**************************************************************
//**************************************************************

void SF_WriteSoundFont(void)
{
	FILE *handle;
    char *outFile;

    // MP2E: Use ROM directory instead of executable directory
    outFile = I_GetUserFile("doomsnd.sf2");
	if (!(handle = fopen(outFile, "wb"))) {
        perror("Couldn't open soundfont file for writing: ");
        exit(1);
	}

	fwrite(&soundfont, sizeof(sd_soundfont_t), 1, handle);
    fwrite(&soundfont.name, 8, 1, handle);
	fwrite(soundfont.name.name, soundfont.name.size, 1, handle);
	fwrite(data, sizeof(sd_sfdata_t), 1, handle);
    fwrite(data->wavdata, data->sd.size, 1, handle);
	fwrite(list, sizeof(sd_sfpresetlist_t), 1, handle);

	fwrite(&preset->info, sizeof(sd_sfpresetheader_t), 1, handle);
	fwrite(preset->info.presets, sizeof(sd_sfpresetinfo_t), npresets, handle);
	fwrite(&preset->bag, sizeof(sd_sfpresetbagheader_t), 1, handle);
	fwrite(preset->bag.presetbags, sizeof(sd_sfpresetbag_t), npbags, handle);
    fwrite(&preset->mod, sizeof(sd_sfpresetmod_t), 1, handle);
	fwrite(&preset->gen, sizeof(sd_sfpresetgenheader_t), 1, handle);
    fwrite(preset->gen.info, sizeof(sd_sfpresetgen_t), npgens, handle);

	fwrite(&inst->info, sizeof(sd_sfinstheader_t), 1, handle);
	fwrite(inst->info.instruments, sizeof(sd_sfinstinfo_t), ninstruments, handle);
	fwrite(&inst->bag, sizeof(sd_sfinstbagheader_t), 1, handle);
    fwrite(inst->bag.instbags, sizeof(sd_sfinstbag_t), nibags, handle);

	fwrite(&inst->mod, sizeof(sd_sfinstmod_t), 1, handle);
    fwrite(&inst->gen, sizeof(sd_sfinstgenheader_t), 1, handle);
    fwrite(inst->gen.info, sizeof(sd_sfinstgen_t), nigens, handle);

    fwrite(sample, sizeof(sd_sfsample_t), 1, handle);
	fwrite(sample->info, sizeof(sd_sfsampleinfo_t), nsamples, handle);

	fclose(handle);

	free(sample->info);
	free(data->wavdata);
	free(preset->info.presets);
	free(preset->bag.presetbags);
	free(preset->gen.info);
	free(inst->info.instruments);
	free(inst->bag.instbags);
	free(inst->gen.info);

	WGen_Printf("Successfully created %s", outFile);

	free(outFile);
}

//**************************************************************
//**************************************************************
//      SF_Setup
//**************************************************************
//**************************************************************

void SF_Setup(void)
{
	data = &soundfont.data;
	list = &soundfont.presetlist;
	sample = &soundfont.samples;
	preset = &soundfont.presetlist.preset;
	inst = &soundfont.presetlist.inst;

	//
	// setup tags
	//
	strncpy(soundfont.sd.RIFF, "RIFF", 4);
	strncpy(soundfont.sd.sfbk, "sfbk", 4);
	strncpy(soundfont.sd.LIST, "LIST", 4);
	strncpy(soundfont.sd.INFO, "INFO", 4);
	strncpy(soundfont.sd.ifil, "ifil", 4);
	strncpy(soundfont.data.sd.LIST, "LIST", 4);
	strncpy(soundfont.data.sd.sdta, "sdta", 4);
	strncpy(soundfont.data.sd.smpl, "smpl", 4);
	strncpy(soundfont.name.INAM, "INAM", 4);
	strncpy(list->sd.LIST, "LIST", 4);
	strncpy(list->sd.pdta, "pdta", 4);
	strncpy(preset->info.sd.phdr, "phdr", 4);
	strncpy(preset->gen.sd.pgen, "pgen", 4);
	strncpy(preset->bag.sd.pbag, "pbag", 4);
	strncpy(preset->mod.pmod, "pmod", 4);
	strncpy(inst->info.sd.inst, "inst", 4);
	strncpy(inst->gen.sd.igen, "igen", 4);
	strncpy(inst->bag.sd.ibag, "ibag", 4);
	strncpy(inst->mod.imod, "imod", 4);
	strncpy(soundfont.samples.sd.shdr, "shdr", 4);

	//
	// version
	//
	soundfont.sd.size = 4;
	soundfont.sd.version.wMajor = 2;
	soundfont.sd.version.wMinor = 1;

	//
	// name
	//
    char sfname[] = "Doom 64 Soundfont";
	soundfont.name.size = sizeof(sfname);
	_PAD4(soundfont.name.size);
	soundfont.name.name = (char*) malloc(soundfont.name.size);
	strncpy(soundfont.name.name, sfname, sizeof(sfname));

	//
	// initalize chunks
	//
	soundfont.sd.listsize = 24 + soundfont.name.size;
	soundfont.sd.filesize = 20 + soundfont.sd.listsize;

	sample->sd.size = 0;
	sample->info = NULL;

	data->sd.size = 0;
	data->wavdata = NULL;

	preset->info.sd.size = 0;
	preset->bag.sd.size = 0;
	preset->gen.sd.size = 0;
	preset->info.presets = NULL;
	preset->bag.presetbags = NULL;
	preset->gen.info = NULL;

	inst->info.sd.size = 0;
	inst->bag.sd.size = 0;
	inst->gen.sd.size = 0;
	inst->info.instruments = NULL;
	inst->bag.instbags = NULL;
	inst->gen.info = NULL;
}
