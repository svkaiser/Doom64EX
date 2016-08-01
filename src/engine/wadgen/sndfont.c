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
#ifdef RCSID
static const char rcsid[] =
    "$Id: SndFont.c 1096 2012-03-31 18:28:01Z svkaiser $";
#endif

#include <math.h>

#include "wadgen.h"
#include "files.h"
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
// UTILITES
//

static double mylog2(double x)
{
	return (log10(x) / log10(2));
}

//
// not really sure how they're calculated in the actual game
// but they have to be measured in milliseconds. wouldn't make sense
// if it wasn't
//
static double usectotimecents(int usec)
{
	double t = (double)usec / 1000.0;
	return (1200 * mylog2(t));
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
SF_AddPreset(sfpheader_t * preset, char *name, int idx, int bank, int presetidx)
{
	SF_NEWITEM(preset->presets, sfpresetinfo_t, npresets);
	strncpy(preset->presets[idx].achPresetName, name, 20);
	preset->presets[idx].wPresetBagNdx = curpbag;
	preset->presets[idx].wPreset = presetidx;
	preset->presets[idx].dwGenre = 0;
	preset->presets[idx].dwLibrary = 0;
	preset->presets[idx].dwMorphology = 0;
	preset->presets[idx].wBank = bank;
	preset->size = 38 * npresets;
}

//**************************************************************
//**************************************************************
//      SF_AddInstrument
//**************************************************************
//**************************************************************

static int ninstruments = 0;
void SF_AddInstrument(sfinstheader_t * inst, char *name, int idx)
{
	SF_NEWITEM(inst->instruments, sfinstinfo_t, ninstruments);
	strncpy(inst->instruments[idx].achInstName, name, 20);
	inst->instruments[idx].wInstBagNdx = idx;
	inst->size += sizeof(sfinstinfo_t);
}

//**************************************************************
//**************************************************************
//      SF_AddInstrumentGenerator
//**************************************************************
//**************************************************************

static int igenpos = 0;
static int nigens = 0;
void
SF_AddInstrumentGenerator(sfigenheader_t * gen, generatorops_e op,
			  gentypes_t value)
{
	sfinstgen_t *igen;

	SF_NEWITEM(gen->info, sfinstgen_t, nigens);
	igen = &gen->info[nigens - 1];
	igen->sfGenOper = op;
	igen->genAmount = value;
	gen->size += sizeof(sfinstgen_t);
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
SF_AddPresetGenerator(sfpgenheader_t * gen, generatorops_e op, gentypes_t value)
{
	sfpresetgen_t *pgen;

	SF_NEWITEM(gen->info, sfpresetgen_t, npgens);
	pgen = &gen->info[npgens - 1];
	pgen->sfGenOper = op;
	pgen->genAmount = value;
	gen->size += sizeof(sfpresetgen_t);
	pgenpos++;
}

//**************************************************************
//**************************************************************
//      SF_AddInstrumentBag
//**************************************************************
//**************************************************************

static int nibags = 0;
void SF_AddInstrumentBag(sfibagheader_t * bag)
{
	sfinstbag_t *instbag;

	SF_NEWITEM(bag->instbags, sfinstbag_t, nibags);
	instbag = &bag->instbags[nibags - 1];
	instbag->wInstGenNdx = igenpos;
	instbag->wInstModNdx = 0;
	bag->size += sizeof(sfinstbag_t);
}

//**************************************************************
//**************************************************************
//      SF_AddPresetBag
//**************************************************************
//**************************************************************

static int npbags = 0;
void SF_AddPresetBag(sfpbagheader_t * bag)
{
	sfpresetbag_t *presetbag;

	SF_NEWITEM(bag->presetbags, sfpresetbag_t, npbags);
	presetbag = &bag->presetbags[npbags - 1];
	presetbag->wGenNdx = pgenpos;
	presetbag->wModNdx = 0;
	bag->size += sizeof(sfpresetbag_t);

	curpbag = npbags;
}

//**************************************************************
//**************************************************************
//      SF_AddSample
//**************************************************************
//**************************************************************

static int nsamples = 0;
void
SF_AddSample(sfsample_t * sample, char *name, size_t size, size_t offset,
	     int loopid)
{
	sfsampleinfo_t *info;
	looptable_t *lptbl;

	SF_NEWITEM(sample->info, sfsampleinfo_t, nsamples);
	sample->size = 46 * nsamples;

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
	data->size += (insize + 64);
	data->listsize = data->size + 12;
	data->wavdata = (byte *) realloc((byte *) data->wavdata, data->size);
	memset(data->wavdata + (data->size - 64), 0, 64);
	memcpy(data->wavdata + offset, in, insize);
	SF_AddSample(&sf->samples, newname, insize, offset, loopid);

	offset = data->size;
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
	list->listsize = 0x4C;
	list->listsize += preset->info.size;
	list->listsize += preset->bag.size;
	list->listsize += preset->mod.size;
	list->listsize += preset->gen.size;
	list->listsize += inst->info.size;
	list->listsize += inst->bag.size;
	list->listsize += inst->mod.size;
	list->listsize += inst->gen.size;
	list->listsize += sample->size;

	soundfont.filesize += list->listsize + 8 + data->listsize;
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
	gentypes_t value;
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
				    (int)attentopercent(sp->attenuation);
				SF_AddInstrumentGenerator(&inst->gen,
							  GEN_ATTENUATION,
							  value);
			}
			//
			// panning
			//
			if (sp->pan != 64) {
				value.wAmount = (int)pantopercent(sp->pan);
				SF_AddInstrumentGenerator(&inst->gen, GEN_PAN,
							  value);
			}
			//
			// attack time
			//
			if (sp->attacktime > 1) {
				value.wAmount =
				    (int)usectotimecents(sp->attacktime);
				SF_AddInstrumentGenerator(&inst->gen,
							  GEN_VOLENVATTACK,
							  value);
			}
			//
			// release time
			//
			if (sp->decaytime > 1) {
				value.wAmount =
				    (int)usectotimecents(sp->decaytime);
				SF_AddInstrumentGenerator(&inst->gen,
							  GEN_VOLENVRELEASE,
							  value);
			}
			//
			// sample loops
			//
			if (wt->loopid != -1) {
				value.wAmount = 1;
				SF_AddInstrumentGenerator(&inst->gen,
							  GEN_SAMPLEMODE,
							  value);
			}
			//
			// root key override
			//
			value.wAmount =
			    getoriginalpitch(sp->rootkey, wt->pitch);
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
	int handle;
	int i;
	path outFile;

    // MP2E: Use ROM directory instead of executable directory
    sprintf(outFile, "%s/doomsnd.sf2", wgenfile.pathOnly);
	handle = File_Open(outFile);

	File_Write(handle, soundfont.RIFF, 4);
	File_Write(handle, &soundfont.filesize, 4);
	File_Write(handle, soundfont.sfbk, 4);
	File_Write(handle, soundfont.LIST, 4);
	File_Write(handle, &soundfont.listsize, 4);
	File_Write(handle, soundfont.INFO, 4);
	File_Write(handle, &soundfont.version, sizeof(sfversion_t));
	File_Write(handle, soundfont.name.INAM, 4);
	File_Write(handle, &soundfont.name.size, 4);
	File_Write(handle, soundfont.name.name, soundfont.name.size);
	File_Write(handle, data->LIST, 4);
	File_Write(handle, &data->listsize, 4);
	File_Write(handle, data->sdta, 4);
	File_Write(handle, data->smpl, 4);
	File_Write(handle, &data->size, 4);
	File_Write(handle, data->wavdata, data->size);
	File_Write(handle, list->LIST, 4);
	File_Write(handle, &list->listsize, 4);
	File_Write(handle, list->pdta, 4);
	File_Write(handle, preset->info.phdr, 4);
	File_Write(handle, &preset->info.size, 4);

	for (i = 0; i < npresets; i++) {
		File_Write(handle, preset->info.presets[i].achPresetName, 20);
		File_Write(handle, &preset->info.presets[i].wPreset, 2);
		File_Write(handle, &preset->info.presets[i].wBank, 2);
		File_Write(handle, &preset->info.presets[i].wPresetBagNdx, 2);
		File_Write(handle, &preset->info.presets[i].dwLibrary, 4);
		File_Write(handle, &preset->info.presets[i].dwGenre, 4);
		File_Write(handle, &preset->info.presets[i].dwMorphology, 4);
	}

	File_Write(handle, preset->bag.pbag, 4);
	File_Write(handle, &preset->bag.size, 4);
	File_Write(handle, preset->bag.presetbags,
		   sizeof(sfpresetbag_t) * npbags);
	File_Write(handle, preset->mod.pmod, 4);
	File_Write(handle, &preset->mod.size, 4);
	File_Write(handle, &preset->mod.sfModSrcOper, 2);
	File_Write(handle, &preset->mod.sfModDestOper, 2);
	File_Write(handle, &preset->mod.modAmount, 2);
	File_Write(handle, &preset->mod.sfModAmtSrcOper, 2);
	File_Write(handle, &preset->mod.sfModTransOper, 2);
	File_Write(handle, preset->gen.pgen, 4);
	File_Write(handle, &preset->gen.size, 4);
	File_Write(handle, preset->gen.info, sizeof(sfpresetgen_t) * npgens);
	File_Write(handle, inst->info.inst, 4);
	File_Write(handle, &inst->info.size, 4);
	File_Write(handle, inst->info.instruments,
		   sizeof(sfinstinfo_t) * ninstruments);
	File_Write(handle, inst->bag.ibag, 4);
	File_Write(handle, &inst->bag.size, 4);
	File_Write(handle, inst->bag.instbags, sizeof(sfinstbag_t) * nibags);
	File_Write(handle, inst->mod.imod, 4);
	File_Write(handle, &inst->mod.size, 4);
	File_Write(handle, &inst->mod.sfModSrcOper, 2);
	File_Write(handle, &inst->mod.sfModDestOper, 2);
	File_Write(handle, &inst->mod.modAmount, 2);
	File_Write(handle, &inst->mod.sfModAmtSrcOper, 2);
	File_Write(handle, &inst->mod.sfModTransOper, 2);
	File_Write(handle, inst->gen.igen, 4);
	File_Write(handle, &inst->gen.size, 4);
	File_Write(handle, inst->gen.info, sizeof(sfinstgen_t) * nigens);
	File_Write(handle, sample->shdr, 4);
	File_Write(handle, &sample->size, 4);

	for (i = 0; i < nsamples; i++) {
		File_Write(handle, sample->info[i].achSampleName, 20);
		File_Write(handle, &sample->info[i].dwStart, 4);
		File_Write(handle, &sample->info[i].dwEnd, 4);
		File_Write(handle, &sample->info[i].dwStartloop, 4);
		File_Write(handle, &sample->info[i].dwEndloop, 4);
		File_Write(handle, &sample->info[i].dwSampleRate, 4);
		File_Write(handle, &sample->info[i].byOriginalPitch, 1);
		File_Write(handle, &sample->info[i].chPitchCorrection, 1);
		File_Write(handle, &sample->info[i].wSampleLink, 2);
		File_Write(handle, &sample->info[i].sfSampleType, 2);
	}

	File_Close(handle);
	File_SetReadOnly(outFile);

	Mem_Free((void **)&sample->info);
	Mem_Free((void **)&data->wavdata);
	Mem_Free((void **)&preset->info.presets);
	Mem_Free((void **)&preset->bag.presetbags);
	Mem_Free((void **)&preset->gen.info);
	Mem_Free((void **)&inst->info.instruments);
	Mem_Free((void **)&inst->bag.instbags);
	Mem_Free((void **)&inst->gen.info);

	WGen_Printf("Successfully created %s", outFile);
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
	strncpy(soundfont.RIFF, "RIFF", 4);
	strncpy(soundfont.sfbk, "sfbk", 4);
	strncpy(soundfont.LIST, "LIST", 4);
	strncpy(soundfont.INFO, "INFO", 4);
	strncpy(soundfont.data.LIST, "LIST", 4);
	strncpy(soundfont.data.sdta, "sdta", 4);
	strncpy(soundfont.data.smpl, "smpl", 4);
	strncpy(soundfont.version.ifil, "ifil", 4);
	strncpy(soundfont.name.INAM, "INAM", 4);
	strncpy(list->LIST, "LIST", 4);
	strncpy(list->pdta, "pdta", 4);
	strncpy(preset->info.phdr, "phdr", 4);
	strncpy(preset->gen.pgen, "pgen", 4);
	strncpy(preset->bag.pbag, "pbag", 4);
	strncpy(preset->mod.pmod, "pmod", 4);
	strncpy(inst->info.inst, "inst", 4);
	strncpy(inst->gen.igen, "igen", 4);
	strncpy(inst->bag.ibag, "ibag", 4);
	strncpy(inst->mod.imod, "imod", 4);
	strncpy(soundfont.samples.shdr, "shdr", 4);

	//
	// version
	//
	soundfont.version.size = 4;
	soundfont.version.wMajor = 2;
	soundfont.version.wMinor = 1;

	//
	// name
	//
	soundfont.name.size = strlen("doomsnd.sf2") + 1;
	_PAD4(soundfont.name.size);
	soundfont.name.name = (char *)Mem_Alloc(soundfont.name.size);
	strncpy(soundfont.name.name, "doomsnd.sf2", soundfont.name.size);

	//
	// initalize chunks
	//
	soundfont.listsize = 24 + soundfont.name.size;
	soundfont.filesize = 20 + soundfont.listsize;

	sample->size = 0;
	sample->info = (sfsampleinfo_t *) Mem_Alloc(1);

	data->size = 0;
	data->wavdata = (byte *) Mem_Alloc(1);

	preset->info.size = 0;
	preset->bag.size = 0;
	preset->gen.size = 0;
	preset->info.presets = (sfpresetinfo_t *) Mem_Alloc(1);
	preset->bag.presetbags = (sfpresetbag_t *) Mem_Alloc(1);
	preset->gen.info = (sfpresetgen_t *) Mem_Alloc(1);

	inst->info.size = 0;
	inst->bag.size = 0;
	inst->gen.size = 0;
	inst->info.instruments = (sfinstinfo_t *) Mem_Alloc(1);
	inst->bag.instbags = (sfinstbag_t *) Mem_Alloc(1);
	inst->gen.info = (sfinstgen_t *) Mem_Alloc(1);
}
