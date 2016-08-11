// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: Rom.c 1100 2012-04-08 19:17:31Z svkaiser $
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
// $Revision: 1100 $
// $Date: 2012-04-08 12:17:31 -0700 (Sun, 08 Apr 2012) $
//
// DESCRIPTION: Rom handling stuff
//
//-----------------------------------------------------------------------------

#include <string.h>

#include "wadgen.h"
#include "files.h"
#include "rom.h"
#include "wad.h"

rom_t RomFile;

int Rom_Verify(void);
void Rom_SwapBigEndian(int swaptype);
void Rom_GetIwad(void);
void Rom_VerifyChecksum(void);

//**************************************************************
//**************************************************************
//      Rom_Open
//**************************************************************
//**************************************************************

void Rom_Open(void)
{
	int id = 0;

	RomFile.length = File_Read(wgenfile.filePath, &RomFile.data);
	if (RomFile.length <= 0)
		WGen_Complain("Rom_Open: Rom file length <= 0");

	id = Rom_Verify();

	if (!id)
		WGen_Complain("VGen_RomOpen: Not a valid n64 rom..");

	Rom_SwapBigEndian(id);

	memcpy(&RomFile.header, RomFile.data, ROMHEADERSIZE);
	strupr(RomFile.header.Name);

	Rom_VerifyChecksum();

	Wad_GetIwad();
}

//**************************************************************
//**************************************************************
//      Rom_Close
//**************************************************************
//**************************************************************

void Rom_Close(void)
{
	Mem_Free((void **)&RomFile.data);
}

//**************************************************************
//**************************************************************
//      Rom_VerifyChecksum
//**************************************************************
//**************************************************************

static const md5_digest_t rom_digests[10] = {
	{0x2e, 0x1c, 0xea, 0x7e, 0x24, 0x82, 0xb6, 0xe8, 0x3d, 0x6d, 0x64, 0x54,
	 0x34, 0x44, 0x52, 0x95},
	{0x0a, 0x91, 0x28, 0x16, 0x15, 0x9b, 0x4a, 0xbd, 0x6c, 0xd7, 0xd1, 0xce,
	 0x0a, 0x3e, 0x3c, 0x4e},
	{0x54, 0x77, 0x4e, 0x31, 0x19, 0x0a, 0x01, 0xf1, 0x23, 0x68, 0x00, 0x79,
	 0x6c, 0x03, 0xae, 0x87},
	{0x28, 0x90, 0x92, 0xce, 0x53, 0x08, 0x6b, 0x62, 0x43, 0xe8, 0xb6, 0x08,
	 0xc9, 0x93, 0x3f, 0xa6},
	{0xec, 0xf2, 0x25, 0x4f, 0x39, 0x37, 0x4a, 0xb1, 0x2d, 0xe3, 0x2f, 0x9b,
	 0xf8, 0x48, 0x50, 0xd1},
	{0x28, 0x90, 0x92, 0xce, 0x53, 0x08, 0x6b, 0x62, 0x43, 0xe8, 0xb6, 0x08,
	 0xc9, 0x93, 0x3f, 0xa6},
	{0xd7, 0xef, 0x12, 0x47, 0xce, 0xdd, 0x7c, 0x75, 0x61, 0x94, 0x83, 0xfa,
	 0x23, 0x3a, 0xac, 0x86},
	{0xc8, 0xe8, 0x96, 0x69, 0xf9, 0x00, 0x00, 0xd2, 0x6f, 0x50, 0xb5, 0x47,
	 0x9a, 0xd1, 0xc3, 0x1b},
	{0x4a, 0x34, 0xfc, 0xf3, 0x3f, 0xa9, 0xa3, 0x74, 0x7b, 0x2e, 0xd0, 0x59,
	 0xf3, 0xf4, 0x8c, 0xee},
	{0xb9, 0xcc, 0x68, 0x8f, 0x59, 0xa0, 0x64, 0xe2, 0xce, 0x13, 0x02, 0xfc,
	 0x3e, 0x51, 0xb7, 0xd9}
};

void Rom_VerifyChecksum(void)
{
	md5_context_t md5_context;
	md5_digest_t digest;
	int i;

	MD5_Init(&md5_context);
	MD5_Update(&md5_context, &RomFile.header.x1, 1);
	MD5_Update(&md5_context, &RomFile.header.x2, 1);
	MD5_Update(&md5_context, &RomFile.header.x3, 1);
	MD5_Update(&md5_context, &RomFile.header.x4, 1);
	MD5_UpdateInt32(&md5_context, RomFile.header.ClockRate);
	MD5_UpdateInt32(&md5_context, RomFile.header.BootAddress);
	MD5_UpdateInt32(&md5_context, RomFile.header.Release);
	MD5_UpdateInt32(&md5_context, RomFile.header.CRC1);
	MD5_UpdateInt32(&md5_context, RomFile.header.CRC2);
	MD5_UpdateString(&md5_context, RomFile.header.Name);
	MD5_Update(&md5_context, &RomFile.header.Manufacturer, 1);
	MD5_Update(&md5_context, &RomFile.header.CountryID, 1);
	MD5_Update(&md5_context, &RomFile.header.VersionID, 1);
	MD5_UpdateInt32(&md5_context, RomFile.header.CartID);
	MD5_UpdateInt32(&md5_context, RomFile.length);
	MD5_Final(digest, &md5_context);

#if 0
#ifdef _DEBUG
	{
		FILE *md5info;
		path tbuff;
		int i = 0;
		int j = 0;

		do {
			sprintf(tbuff, "md5rominfo%02d.txt", j++);
		} while (File_Poke(tbuff));

		md5info = fopen(tbuff, "w");

		fprintf(md5info, "static const md5_digest_t <rename me> =\n");
		fprintf(md5info, "{ ");

		for (i = 0; i < 16; i++) {
			fprintf(md5info, "0x%02x", digest[i]);
			if (i < 15)
				fprintf(md5info, ",");
			else
				fprintf(md5info, " ");
		}

		fprintf(md5info, "};\n");
		fclose(md5info);
	}
#endif
#endif

	for (i = 0; i < 10; i++) {
		if (!memcmp(rom_digests[i], digest, sizeof(md5_digest_t)))
			return;
	}

	WGen_Complain
	    ("Rom checksum verification failed. Rom is either broken or corrupted");
}

//**************************************************************
//**************************************************************
//      Rom_SwapBigEndian
//
//      Convert ROM into big endian format before processing iwad
//**************************************************************
//**************************************************************

void Rom_SwapBigEndian(int swaptype)
{
	uint len;

	// v64
	if (swaptype == 3) {
		int *swap;

		for (swap = (int *)RomFile.data, len = 0;
		     len < RomFile.length / 4; len++)
			swap[len] = _SWAP32(swap[len]);
	}
	// n64
	else if (swaptype == 2) {
		short *swap;

		for (swap = (short *)RomFile.data, len = 0;
		     len < RomFile.length / 2; len++)
			swap[len] = _SWAP16(swap[len]);
	}
	// z64 (do nothing)
}

//**************************************************************
//**************************************************************
//      Rom_Verify
//
//      Checks the beginning of a rom to verify if its a valid N64
//      rom and that its a Doom64 rom.
//**************************************************************
//**************************************************************

int Rom_Verify(void)
{
	if (strstr(wgenfile.filePath, ".z64"))	// big endian
		return 1;

	if (strstr(wgenfile.filePath, ".n64"))	// little endian
		return 2;

	if (strstr(wgenfile.filePath, ".v64"))	// byte swapped
		return 3;

	return 0;
}

//**************************************************************
//**************************************************************
//      Rom_VerifyRomCode
//**************************************************************
//**************************************************************

bool Rom_VerifyRomCode(const romLumpSpecial_t * l)
{
	if (RomFile.header.VersionID == 0x1) {
		if (strstr(l->countryID, "X")
		    && RomFile.header.CountryID == 'E')
			return true;
	}
	if (strstr(l->countryID, &RomFile.header.CountryID))
		return true;

	return false;
}
