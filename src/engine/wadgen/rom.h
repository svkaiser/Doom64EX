#ifndef _WADGEN_ROM_H_
#define _WADGEN_ROM_H_

#define ROMHEADERSIZE	64

/* From Daedalus */
typedef struct {
	byte x1;		/* initial PI_BSB_DOM1_LAT_REG value */
	byte x2;		/* initial PI_BSB_DOM1_PGS_REG value */
	byte x3;		/* initial PI_BSB_DOM1_PWD_REG value */
	byte x4;		/* initial PI_BSB_DOM1_RLS_REG value */
	unsigned int ClockRate;
	unsigned int BootAddress;
	unsigned int Release;
	unsigned int CRC1;
	unsigned int CRC2;
	unsigned int Unknown0;
	unsigned int Unknown1;
	char Name[20];
	unsigned int Unknown2;
	unsigned short int Unknown3;
	byte Unknown4;
	byte Manufacturer;
	unsigned short int CartID;
	char CountryID;
	byte VersionID;
} romHeader_t;

typedef struct {
	romHeader_t header;
	cache data;
	uint length;
} rom_t;

typedef struct {
	char *name;
	char *countryID;	// E = US, J = Japan, P = PAL/Europe, X = US (v1.1)
} romLumpSpecial_t;

extern rom_t RomFile;

void Rom_Open(char *path);
void Rom_Close(void);
bool Rom_VerifyRomCode(const romLumpSpecial_t * l);

#endif
