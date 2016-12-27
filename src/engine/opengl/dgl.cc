// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Inlined OpenGL-exclusive functions
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "gl_main.h"
#include "gl_texture.h"
#include "con_console.h"
#include "i_system.h"

#define MAXINDICES  0x10000

word statindice = 0;

static word indicecnt = 0;
static word drawIndices[MAXINDICES];

extern BoolProperty r_drawtris;

//
// dglLogError
//

#ifdef USE_DEBUG_GLFUNCS
void dglLogError(const char *message, const char *file, int line) {
    GLint err = glGetError();
    if(err != GL_NO_ERROR) {
        char str[64];

        switch(err) {
        case GL_INVALID_ENUM:
            dstrcpy(str, "INVALID_ENUM");
            break;
        case GL_INVALID_VALUE:
            dstrcpy(str, "INVALID_VALUE");
            break;
        case GL_INVALID_OPERATION:
            dstrcpy(str, "INVALID_OPERATION");
            break;
        case GL_STACK_OVERFLOW:
            dstrcpy(str, "STACK_OVERFLOW");
            break;
        case GL_STACK_UNDERFLOW:
            dstrcpy(str, "STACK_UNDERFLOW");
            break;
        case GL_OUT_OF_MEMORY:
            dstrcpy(str, "OUT_OF_MEMORY");
            break;
        default:
            sprintf(str, "0x%x", err);
            break;
        }

        I_Printf("\nGL ERROR (%s) on gl function: %s (file = %s, line = %i)\n\n", str, message, file, line);
        I_Sleep(1);
    }
}
#endif

//
// dglSetVertex
//

static vtx_t *dgl_prevptr = NULL;

void dglSetVertex(vtx_t *vtx) {
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglSetVertex(vtx=0x%p)\n", vtx);
#endif

    // 20120623 villsa - avoid redundant calls by checking for
    // the previous pointer that was set
    if(dgl_prevptr == vtx) {
        return;
    }

    dglTexCoordPointer(2, GL_FLOAT, sizeof(vtx_t), &vtx->tu);
    dglVertexPointer(3, GL_FLOAT, sizeof(vtx_t), vtx);
    dglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vtx_t), &vtx->r);

    dgl_prevptr = vtx;
}

//
// dglTriangle
//

void dglTriangle(int v0, int v1, int v2) {
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglTriangle(v0=%i, v1=%i, v2=%i)\n", v0, v1, v2);
#endif
    if(indicecnt + 3 >= MAXINDICES) {
        I_Error("Triangle indice overflow");
    }

    drawIndices[indicecnt++] = v0;
    drawIndices[indicecnt++] = v1;
    drawIndices[indicecnt++] = v2;
}

//
// dglDrawGeometry
//

void dglDrawGeometry(dword count, vtx_t *vtx) {
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglDrawGeometry(count=0x%x, vtx=0x%p)\n", count, vtx);
#endif

    if(GLAD_GL_EXT_compiled_vertex_array) {
        dglLockArraysEXT(0, count);
    }

    dglDrawElements(GL_TRIANGLES, indicecnt, GL_UNSIGNED_SHORT, drawIndices);

    if(GLAD_GL_EXT_compiled_vertex_array) {
        dglUnlockArraysEXT();
    }

    if(r_drawtris) {
        dword j = 0;
        byte b;

        for(j = 0; j < count; j++) {
            vtx[j].r = 0xff;
            vtx[j].g = 0xff;
            vtx[j].b = 0xff;
            vtx[j].a = 0xff;
        }

        dglGetBooleanv(GL_FOG, &b);

        if(b) {
            dglDisable(GL_FOG);
        }

        dglDisableClientState(GL_TEXTURE_COORD_ARRAY);
        dglDisable(GL_TEXTURE_2D);
        dglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        dglDepthRange(0.0f, 0.0f);

        if(GLAD_GL_EXT_compiled_vertex_array) {
            dglLockArraysEXT(0, count);
        }

        dglDrawElements(GL_TRIANGLES, indicecnt, GL_UNSIGNED_SHORT, drawIndices);

        if(GLAD_GL_EXT_compiled_vertex_array) {
            dglUnlockArraysEXT();
        }

        dglDepthRange(0.0f, 1.0f);
        dglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        dglEnableClientState(GL_TEXTURE_COORD_ARRAY);
        dglEnable(GL_TEXTURE_2D);

        if(b) {
            dglEnable(GL_FOG);
        }
    }

    if(devparm) {
        statindice += indicecnt;
    }

    indicecnt = 0;
}

//
// dglViewFrustum
//

void dglViewFrustum(int width, int height, rfloat fovy, rfloat znear) {
    rfloat left;
    rfloat right;
    rfloat bottom;
    rfloat top;
    rfloat aspect;
    rfloat m[16];

#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglViewFrustum(width=%i, height=%i, fovy=%f, znear=%f)\n", width, height, fovy, znear);
#endif

    aspect = (rfloat)width / (rfloat)height;
    top = znear * (rfloat)tan((double)fovy * M_PI / 360.0f);
    bottom = -top;
    left = bottom * aspect;
    right = top * aspect;

    m[ 0] = (2 * znear) / (right - left);
    m[ 4] = 0;
    m[ 8] = (right + left) / (right - left);
    m[12] = 0;

    m[ 1] = 0;
    m[ 5] = (2 * znear) / (top - bottom);
    m[ 9] = (top + bottom) / (top - bottom);
    m[13] = 0;

    m[ 2] = 0;
    m[ 6] = 0;
    m[10] = -1;
    m[14] = -2 * znear;

    m[ 3] = 0;
    m[ 7] = 0;
    m[11] = -1;
    m[15] = 0;

    dglMultMatrixf(m);
}

//
// dglSetVertexColor
//

void dglSetVertexColor(vtx_t *v, rcolor c, word count) {
    int i = 0;
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglSetVertexColor(v=0x%p, c=0x%x, count=0x%x)\n", v, c, count);
#endif
    for(i = 0; i < count; i++) {
        *(rcolor*)&v[i].r = c;
    }
}

//
// dglGetColorf
//

void dglGetColorf(rcolor color, float* argb) {
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglGetColorf(color=0x%x, argb=0x%p)\n", color, argb);
#endif
    argb[3] = (float)((color >> 24) & 0xff) / 255.0f;
    argb[2] = (float)((color >> 16) & 0xff) / 255.0f;
    argb[1] = (float)((color >> 8) & 0xff) / 255.0f;
    argb[0] = (float)(color & 0xff) / 255.0f;
}

//
// dglTexCombReplace
//

void dglTexCombReplace(void) {
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglTexCombReplace\n");
#endif
    GL_SetTextureMode(GL_COMBINE_ARB);
    GL_SetCombineState(GL_REPLACE);
    GL_SetCombineSourceRGB(0, GL_TEXTURE);
    GL_SetCombineOperandRGB(0, GL_SRC_COLOR);
}

//
// dglTexCombColor
//

void dglTexCombColor(int t, rcolor c, int func) {
    float f[4];
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglTexCombColor(t=0x%x, c=0x%x)\n", t, c);
#endif
    dglGetColorf(c, f);
    GL_SetTextureMode(GL_COMBINE_ARB);
    GL_SetEnvColor(f);
    GL_SetCombineState(func);
    GL_SetCombineSourceRGB(0, t);
    GL_SetCombineOperandRGB(0, GL_SRC_COLOR);
    GL_SetCombineSourceRGB(1, GL_CONSTANT);
    GL_SetCombineOperandRGB(1, GL_SRC_COLOR);
}

//
// dglTexCombColorf
//

void dglTexCombColorf(int t, float* f, int func) {
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglTexCombColorf(t=0x%x, f=%p)\n", t, f);
#endif
    GL_SetTextureMode(GL_COMBINE_ARB);
    GL_SetEnvColor(f);
    GL_SetCombineState(func);
    GL_SetCombineSourceRGB(0, t);
    GL_SetCombineOperandRGB(0, GL_SRC_COLOR);
    GL_SetCombineSourceRGB(1, GL_CONSTANT);
    GL_SetCombineOperandRGB(1, GL_SRC_COLOR);
}

//
// dglTexCombModulate
//

void dglTexCombModulate(int t, int s) {
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglTexCombFinalize(t=0x%x)\n", t);
#endif
    GL_SetTextureMode(GL_COMBINE_ARB);
    GL_SetCombineState(GL_MODULATE);
    GL_SetCombineSourceRGB(0, t);
    GL_SetCombineOperandRGB(0, GL_SRC_COLOR);
    GL_SetCombineSourceRGB(1, s);
    GL_SetCombineOperandRGB(1, GL_SRC_COLOR);
}

//
// dglTexCombAdd
//

void dglTexCombAdd(int t, int s) {
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglTexCombFinalize(t=0x%x)\n", t);
#endif
    GL_SetTextureMode(GL_COMBINE_ARB);
    GL_SetCombineState(GL_ADD);
    GL_SetCombineSourceRGB(0, t);
    GL_SetCombineOperandRGB(0, GL_SRC_COLOR);
    GL_SetCombineSourceRGB(1, s);
    GL_SetCombineOperandRGB(1, GL_SRC_COLOR);
}

//
// dglTexCombInterpolate
//

void dglTexCombInterpolate(int t, float a) {
    float f[4];
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglTexCombInterpolate(t=0x%x, a=%f)\n", t, a);
#endif
    f[0] = f[1] = f[2] = 0.0f;
    f[3] = a;

    GL_SetTextureMode(GL_COMBINE_ARB);
    GL_SetCombineState(GL_INTERPOLATE);
    GL_SetEnvColor(f);
    GL_SetCombineSourceRGB(0, GL_TEXTURE);
    GL_SetCombineOperandRGB(0, GL_SRC_COLOR);
    GL_SetCombineSourceRGB(1, t);
    GL_SetCombineOperandRGB(1, GL_SRC_COLOR);
    GL_SetCombineSourceRGB(2, GL_CONSTANT);
    GL_SetCombineOperandRGB(2, GL_SRC_ALPHA);
}

//
// dglTexCombReplaceAlpha
//

void dglTexCombReplaceAlpha(int t) {
#ifdef LOG_GLFUNC_CALLS
    I_Printf("dglTexCombReplaceAlpha(t=0x%x)\n", t);
#endif
    GL_SetTextureMode(GL_COMBINE_ARB);
    GL_SetCombineStateAlpha(GL_MODULATE);
    GL_SetCombineSourceAlpha(0, t);
    GL_SetCombineOperandAlpha(0, GL_SRC_ALPHA);
    GL_SetCombineSourceAlpha(1, GL_PRIMARY_COLOR);
    GL_SetCombineOperandAlpha(1, GL_SRC_ALPHA);
}
