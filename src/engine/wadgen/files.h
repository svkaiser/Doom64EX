#ifndef _WADGEN_FILES_H_
#define _WADGEN_FILES_H_

typedef struct {
	path filePath;
	path fileNoExt;
	path fileName;
	path basePath;
	path pathOnly;
} wgenfile_t;

extern wgenfile_t wgenfile;

#ifdef _WIN32
#define FILEDLGTYPE		"Doom64 Rom File (.n64;.z64;.v64) \0*.n64;*.z64;*.v64\0All Files (*.*)\0*.*\0"

bool File_Dialog(wgenfile_t * wgenfile, const char *type, const char *title,
		 HWND hwnd);
#else
bool File_Prepare(wgenfile_t * wgenfile, const char *filename);
#endif
int File_Open(char const *name);
void File_SetReadOnly(char *name);
void File_Close(int handle);
void File_StripExt(char *name);
void File_StripFile(char *name);
void File_Write(int handle, void *source, int length);
int File_Read(char const *name, cache * buffer);
bool File_Poke(const char *file);

#endif
