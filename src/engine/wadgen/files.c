// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: Files.c 1221 2012-08-19 04:30:22Z svkaiser $
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
// $Revision: 1221 $
// $Date: 2012-08-18 21:30:22 -0700 (Sat, 18 Aug 2012) $
//
// DESCRIPTION: File handling stuff
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char rcsid[] = "$Id: Files.c 1221 2012-08-19 04:30:22Z svkaiser $";
#endif

#ifdef _WIN32
#include <direct.h>
#endif

#include "wadgen.h"
#include "files.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define ASCII_SLASH		47
#define ASCII_BACKSLASH 92

void File_StripPath(char *name);
char *File_GetExePath(void);

wgenfile_t wgenfile;

#ifdef _WIN32
//**************************************************************
//**************************************************************
//      File_Dialog
//**************************************************************
//**************************************************************

bool
File_Dialog(wgenfile_t * wgenfile, const char *type, const char *title,
	    HWND hwnd)
{
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));

	ZeroMemory(wgenfile->filePath, MAX_PATH);
	ZeroMemory(wgenfile->fileNoExt, MAX_PATH);
	ZeroMemory(wgenfile->fileName, MAX_PATH);
	ZeroMemory(wgenfile->basePath, MAX_PATH);
	ZeroMemory(wgenfile->pathOnly, MAX_PATH);

	sprintf(wgenfile->basePath, "%s", File_GetExePath());

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = type;
	ofn.lpstrFile = wgenfile->filePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		memcpy(wgenfile->fileNoExt, wgenfile->filePath, MAX_PATH);
		memcpy(wgenfile->fileName, wgenfile->filePath, MAX_PATH);
		memcpy(wgenfile->pathOnly, wgenfile->filePath, MAX_PATH);

		File_StripExt(wgenfile->fileNoExt);
		File_StripPath(wgenfile->fileName);

        File_StripFile(wgenfile->pathOnly);
		return TRUE;
	}

	return FALSE;
}
#else
bool File_Prepare(wgenfile_t * wgenfile, const char *filename)
{
	ZeroMemory(wgenfile->filePath, MAX_PATH);
	ZeroMemory(wgenfile->fileNoExt, MAX_PATH);
	ZeroMemory(wgenfile->fileName, MAX_PATH);
	ZeroMemory(wgenfile->basePath, MAX_PATH);
	ZeroMemory(wgenfile->pathOnly, MAX_PATH);

	sprintf(wgenfile->basePath, "%s", File_GetExePath());

	memcpy(wgenfile->filePath, filename, strlen(filename));
	memcpy(wgenfile->fileNoExt, wgenfile->filePath, MAX_PATH);
	memcpy(wgenfile->fileName, wgenfile->filePath, MAX_PATH);
	memcpy(wgenfile->pathOnly, wgenfile->filePath, MAX_PATH);

	File_StripExt(wgenfile->fileNoExt);
	File_StripPath(wgenfile->fileName);

    File_StripFile(wgenfile->pathOnly);
	return true;
}
#endif

//**************************************************************
//**************************************************************
//      File_Open
//**************************************************************
//**************************************************************

int File_Open(char const *name)
{
	int handle;

	if (File_Poke(name)) {
		chmod(name, S_IWRITE);
		if (remove(name))
			WGen_Complain("File_Open: Unable to overwrite %s",
				      name);
	}

	handle = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);

	if (handle == -1)
		WGen_Complain("File_Open: Couldn't create %s", name);

	return handle;
}

//**************************************************************
//**************************************************************
//      File_SetReadOnly
//**************************************************************
//**************************************************************

void File_SetReadOnly(char *name)
{
	// UH.......
	chmod(name, S_IREAD);
}

//**************************************************************
//**************************************************************
//      File_Close
//**************************************************************
//**************************************************************

void File_Close(int handle)
{
	close(handle);
}

//**************************************************************
//**************************************************************
//      File_Write
//**************************************************************
//**************************************************************

void File_Write(int handle, void *source, int length)
{
	int count;

	count = write(handle, source, length);

	if (count < length)
		WGen_Complain("File_Write: Miscounted %i < %i", count, length);
}

//**************************************************************
//**************************************************************
//      File_Read
//**************************************************************
//**************************************************************

int File_Read(char const *name, cache * buffer)
{
	int handle;
	int count;
	int length;
	struct stat fileinfo;
	byte *buf;

#ifndef _WIN32
	// on *NIX, look in the same places the game looks for the IWAD
	path fullpath;

	sprintf(fullpath, "%s", name);	// current dir first
	handle = open(fullpath, O_RDONLY | O_BINARY, 0666);

	if (handle == -1) {
		sprintf(fullpath, "/usr/share/games/doom64/%s", name);
		handle = open(fullpath, O_RDONLY | O_BINARY, 0666);

		if (handle == -1) {
			sprintf(fullpath, "/usr/local/share/games/doom64/%s",
				name);
			handle = open(fullpath, O_RDONLY | O_BINARY, 0666);

			if (handle == -1) {
				sprintf(fullpath, "/usr/local/share/doom64/%s",
					name);
				handle =
				    open(fullpath, O_RDONLY | O_BINARY, 0666);
			}
		}
	}
#else
	handle = open(name, O_RDONLY | O_BINARY, 0666);
#endif

	if (handle == -1)
		WGen_Complain("Couldn't read file %s", name);
	if (fstat(handle, &fileinfo) == -1)
		WGen_Complain("Couldn't read file %s", name);

	length = fileinfo.st_size;
	buf = Mem_Alloc(length);
	count = read(handle, buf, length);

	close(handle);

	if (count < length)
		WGen_Complain("Couldn't read file %s", name);

	*buffer = buf;
	return length;
}

//**************************************************************
//**************************************************************
//      File_Poke
//**************************************************************
//**************************************************************

bool File_Poke(const char *file)
{
	return access(file, 0) != -1;
}

//**************************************************************
//**************************************************************
//      File_GetExePath
//**************************************************************
//**************************************************************

char *File_GetExePath(void)
{
	static char current_dir_dummy[] = { "./" };
	static char *base;
	if (!base)		// cache multiple requests
	{
		size_t len = strlen(*myargv);
		char *p = (base = Mem_Alloc(len + 1)) + len - 1;

		strcpy(base, *myargv);

		while (p > base && *p != '/' && *p != '\\')
			*p-- = 0;

		if (*p == '/' || *p == '\\')
			*p-- = 0;

		if (strlen(base) < 2) {
			Mem_Free((void **)&base);
			base = Mem_Alloc(1024);
			if (!getcwd(base, 1024))
				strcpy(base, current_dir_dummy);
		}
	}

	return base;
}

//**************************************************************
//**************************************************************
//      File_StripExt
//**************************************************************
//**************************************************************

void File_StripExt(char *name)
{
	char *search;

	search = name + strlen(name) - 1;
	while (*search != ASCII_BACKSLASH && *search != ASCII_SLASH
	       && search != name) {
		if (*search == '.') {
			*search = '\0';
			return;
		}
		search--;
	}
}

//**************************************************************
//**************************************************************
//      File_StripPath
//**************************************************************
//**************************************************************

void File_StripPath(char *name)
{
	char *search;
	int len = 0;
	int pos = 0;
	int i = 0;

	len = strlen(name) - 1;
	pos = len + 1;

	for (search = name + len;
	     *search != ASCII_BACKSLASH && *search != ASCII_SLASH;
	     search--, pos--) {
		if (search == name)
			return;
	}

	if (pos <= 0)
		return;

	for (i = 0; pos < len + 1; pos++, i++)
		name[i] = name[pos];

	name[i] = '\0';
}

//**************************************************************
//**************************************************************
//      File_StripFile
//**************************************************************
//**************************************************************

void File_StripFile(char *name)
{
	int len = 0;
	char *pos = NULL;
    int i;

	len = strlen(name);

    pos = strrchr(name, ASCII_BACKSLASH);

	if (pos == NULL)
        pos = strrchr(name, ASCII_SLASH);

    if (pos == NULL)
        return;

    for(i = (strlen(name)-strlen(pos)); i < len+1; i++) {
            name[i] = '\0';
    }
}
