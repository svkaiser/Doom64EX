
#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_IMAGE_H__
#define __DOOM64EX_KEXLIB_FRAMEWORK_IMAGE_H__

#include "instream.h"

KEX_C_BEGIN

struct Image;

enum ImageFormat {
    IF_UNKNOWN,
    IF_PNG,
};

typedef struct Image Image;
typedef enum ImageFormat ImageFormat;

KEXAPI ImageFormat Image_DetectFormat(InStream *stream);
KEXAPI Image *Image_Load(InStream *stream);
KEXAPI void Image_Free(Image *image);

#define Image_GetWidth(image) (image->header.width)
#define Image_GetHeight(image) (image->header.height)

KEX_C_END

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_IMAGE_H__
