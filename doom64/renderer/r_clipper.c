// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2003 Tim Stump
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
// DESCRIPTION: World clipper: handles visibility checks
//
//-----------------------------------------------------------------------------

#include "r_local.h"
#include "tables.h"
#include "m_fixed.h"
#include "z_zone.h"
#include <math.h>

static GLdouble viewMatrix[16];
static GLdouble projMatrix[16];
float frustum[6][4];

typedef struct clipnode_s {
    struct clipnode_s *prev, *next;
    angle_t start, end;
} clipnode_t;

//
// GhostlyDeath <10/3/11>
//
clipnode_t *freelist    = NULL;
clipnode_t *clipnodes   = NULL;
clipnode_t *cliphead    = NULL;

static clipnode_t * R_Clipnode_NewRange(angle_t start, angle_t end);
static dboolean R_Clipper_IsRangeVisible(angle_t startAngle, angle_t endAngle);
static void R_Clipper_AddClipRange(angle_t start, angle_t end);
static void R_Clipper_RemoveRange(clipnode_t * range);
static void R_Clipnode_Free(clipnode_t *node);

static clipnode_t *R_Clipnode_GetNew(void) {
    if(freelist) {
        clipnode_t *p = freelist;
        freelist = p->next;
        return p;
    }

    return malloc(sizeof(clipnode_t));
}

static clipnode_t * R_Clipnode_NewRange(angle_t start, angle_t end) {
    clipnode_t *c = R_Clipnode_GetNew();
    c->start = start;
    c->end = end;
    c->next = c->prev=NULL;
    return c;
}

//
// R_Clipper_SafeCheckRange
//

dboolean R_Clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle) {
    if(startAngle > endAngle)
        return (R_Clipper_IsRangeVisible(startAngle, ANGLE_MAX) ||
                R_Clipper_IsRangeVisible(0, endAngle));

    return R_Clipper_IsRangeVisible(startAngle, endAngle);
}

static dboolean R_Clipper_IsRangeVisible(angle_t startAngle, angle_t endAngle) {
    clipnode_t *ci;
    ci = cliphead;

    if(endAngle == 0 && ci && ci->start == 0) {
        return false;
    }

    while(ci != NULL && ci->start < endAngle) {
        if(startAngle >= ci->start && endAngle <= ci->end) {
            return false;
        }

        ci = ci->next;
    }

    return true;
}

static void R_Clipnode_Free(clipnode_t *node) {
    node->next = freelist;
    freelist = node;
}

static void R_Clipper_RemoveRange(clipnode_t *range) {
    if(range == cliphead) {
        cliphead = cliphead->next;
    }
    else {
        if(range->prev) {
            range->prev->next = range->next;
        }

        if(range->next) {
            range->next->prev = range->prev;
        }
    }

    R_Clipnode_Free(range);
}

//
// R_Clipper_SafeAddClipRange
//

void R_Clipper_SafeAddClipRange(angle_t startangle, angle_t endangle) {
    if(startangle > endangle) {
        // The range has to added in two parts.
        R_Clipper_AddClipRange(startangle, ANGLE_MAX);
        R_Clipper_AddClipRange(0, endangle);
    }
    else {
        // Add the range as usual.
        R_Clipper_AddClipRange(startangle, endangle);
    }
}

static void R_Clipper_AddClipRange(angle_t start, angle_t end) {
    clipnode_t *node, *temp, *prevNode;
    if(cliphead) {
        //check to see if range contains any old ranges
        node = cliphead;
        while(node != NULL && node->start < end) {
            if(node->start >= start && node->end <= end) {
                temp = node;
                node = node->next;
                R_Clipper_RemoveRange(temp);
            }
            else {
                if(node->start <= start && node->end >= end) {
                    return;
                }
                else {
                    node = node->next;
                }
            }
        }
        //check to see if range overlaps a range (or possibly 2)
        node = cliphead;
        while(node != NULL) {
            if(node->start >= start && node->start <= end) {
                node->start = start;
                return;
            }
            if(node->end >= start && node->end <= end) {
                // check for possible merger
                if(node->next && node->next->start <= end) {
                    node->end = node->next->end;
                    R_Clipper_RemoveRange(node->next);
                }
                else {
                    node->end = end;
                }

                return;
            }

            node = node->next;
        }

        //just add range
        node = cliphead;
        prevNode = NULL;

        temp = R_Clipnode_NewRange(start, end);

        while(node != NULL && node->start < end) {
            prevNode = node;
            node = node->next;
        }

        temp->next = node;

        if(node == NULL) {
            temp->prev = prevNode;

            if(prevNode) {
                prevNode->next = temp;
            }

            if(!cliphead) {
                cliphead = temp;
            }
        }
        else {
            if(node == cliphead) {
                cliphead->prev = temp;
                cliphead = temp;
            }
            else {
                temp->prev = prevNode;
                prevNode->next = temp;
                node->prev = temp;
            }
        }
    }
    else {
        temp = R_Clipnode_NewRange(start, end);
        cliphead = temp;
        return;
    }
}

//
// R_Clipper_Clear
//

void R_Clipper_Clear(void) {
    clipnode_t *node = cliphead;
    clipnode_t *temp;

    while(node != NULL) {
        temp = node;
        node = node->next;
        R_Clipnode_Free(temp);
    }

    cliphead = NULL;
}

//
// R_FrustumAngle
//

extern dboolean widescreen;

angle_t R_FrustumAngle(void) {
    angle_t tilt;
    float range;
    float floatangle;

    tilt = ANG45 - D_abs((int)(viewpitch - ANG90));

    if(tilt > ANG90) {
        return ANG270 - ANG90;
    }

    range = (64.0f / (r_fov.value - (widescreen ? -4.0f : 10.0f)));

    if(range > 1.0f) {
        range = 1.0f;
    }

    floatangle = (float)(tilt / ANG1) * range;

    return ANG270 - ((int)floatangle * ANG1);
}

//
// R_FrustrumSetup
//

#define CALCMATRIX(a, b, c, d, e, f, g, h)\
(float)(viewMatrix[a] * projMatrix[b] + \
viewMatrix[c] * projMatrix[d] + \
viewMatrix[e] * projMatrix[f] + \
viewMatrix[g] * projMatrix[h])

void R_FrustrumSetup(void) {
    float clip[16];

    dglGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    dglGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix);

    clip[0]  = CALCMATRIX(0, 0, 1, 4, 2, 8, 3, 12);
    clip[1]  = CALCMATRIX(0, 1, 1, 5, 2, 9, 3, 13);
    clip[2]  = CALCMATRIX(0, 2, 1, 6, 2, 10, 3, 14);
    clip[3]  = CALCMATRIX(0, 3, 1, 7, 2, 11, 3, 15);

    clip[4]  = CALCMATRIX(4, 0, 5, 4, 6, 8, 7, 12);
    clip[5]  = CALCMATRIX(4, 1, 5, 5, 6, 9, 7, 13);
    clip[6]  = CALCMATRIX(4, 2, 5, 6, 6, 10, 7, 14);
    clip[7]  = CALCMATRIX(4, 3, 5, 7, 6, 11, 7, 15);

    clip[8]  = CALCMATRIX(8, 0, 9, 4, 10, 8, 11, 12);
    clip[9]  = CALCMATRIX(8, 1, 9, 5, 10, 9, 11, 13);
    clip[10] = CALCMATRIX(8, 2, 9, 6, 10, 10, 11, 14);
    clip[11] = CALCMATRIX(8, 3, 9, 7, 10, 11, 11, 15);

    clip[12] = CALCMATRIX(12, 0, 13, 4, 14, 8, 15, 12);
    clip[13] = CALCMATRIX(12, 1, 13, 5, 14, 9, 15, 13);
    clip[14] = CALCMATRIX(12, 2, 13, 6, 14, 10, 15, 14);
    clip[15] = CALCMATRIX(12, 3, 13, 7, 14, 11, 15, 15);

    // Right plane
    frustum[0][0] = clip[ 3] - clip[ 0];
    frustum[0][1] = clip[ 7] - clip[ 4];
    frustum[0][2] = clip[11] - clip[ 8];
    frustum[0][3] = clip[15] - clip[12];

    // Left plane
    frustum[1][0] = clip[ 3] + clip[ 0];
    frustum[1][1] = clip[ 7] + clip[ 4];
    frustum[1][2] = clip[11] + clip[ 8];
    frustum[1][3] = clip[15] + clip[12];

    // Bottom plane
    frustum[2][0] = clip[ 3] + clip[ 1];
    frustum[2][1] = clip[ 7] + clip[ 5];
    frustum[2][2] = clip[11] + clip[ 9];
    frustum[2][3] = clip[15] + clip[13];

    // Top plane
    frustum[3][0] = clip[ 3] - clip[ 1];
    frustum[3][1] = clip[ 7] - clip[ 5];
    frustum[3][2] = clip[11] - clip[ 9];
    frustum[3][3] = clip[15] - clip[13];

    // Far plane
    frustum[4][0] = clip[ 3] - clip[ 2];
    frustum[4][1] = clip[ 7] - clip[ 6];
    frustum[4][2] = clip[11] - clip[10];
    frustum[4][3] = clip[15] - clip[14];

    // Near plane
    frustum[5][0] = clip[ 3] + clip[ 2];
    frustum[5][1] = clip[ 7] + clip[ 6];
    frustum[5][2] = clip[11] + clip[10];
    frustum[5][3] = clip[15] + clip[14];
}

//
// R_FrustrumTestVertex
// Returns false if polygon is not within the view frustrum
//

dboolean R_FrustrumTestVertex(vtx_t* vertex, int count) {
    int p;
    int i;

    for(p = 0; p < 6; p++) {
        for(i = 0; i < count; i++) {
            if(frustum[p][0] * vertex[i].x +
                    frustum[p][1] * vertex[i].y +
                    frustum[p][2] * vertex[i].z +
                    frustum[p][3] > 0) {
                break;
            }
        }

        if(i != count) {
            continue;
        }

        return false;
    }

    return true;
}
