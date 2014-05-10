// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
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
// DESCRIPTION: BSP rendering code. Seg/Subsector rendering
//
//-----------------------------------------------------------------------------

#include <math.h>

#include "r_local.h"
#include "r_clipper.h"
#include "i_system.h"
#include "doomstat.h"
#include "d_main.h"
#include "m_misc.h"
#include "z_zone.h"
#include "r_sky.h"
#include "r_drawlist.h"
#include "con_console.h"
#include "p_local.h"
#include "gl_texture.h"

sector_t    *frontsector;

static vtx_t  *subsector_buffer = NULL;

static void R_AddLeaf(subsector_t *sub);
static void R_AddLine(seg_t *line);
static void AddSegToDrawlist(drawlist_t *dl, seg_t *line, int texid, int sidetype);

CVAR_EXTERNAL(i_interpolateframes);
CVAR_EXTERNAL(r_texturecombiner);

//
// R_AddClipLine
// Clips the given segment
// and adds any visible pieces to the line list.
//

void R_AddClipLine(seg_t* line) {
    angle_t angle1;
    angle_t angle2;

    if(line->v1->validcount != validcount) {
        line->v1->clipspan = R_PointToAngle2(line->v1->x, line->v1->y, viewx, viewy);
        line->v1->validcount = validcount;
    }

    if(line->v2->validcount != validcount) {
        line->v2->clipspan = R_PointToAngle2(line->v2->x, line->v2->y, viewx, viewy);
        line->v2->validcount = validcount;
    }

    angle1 = line->v1->clipspan;
    angle2 = line->v2->clipspan;

    // Back side, i.e. backface culling    - read: endAngle >= startAngle!
    if(angle2 - angle1 < ANG180 || !line->linedef) {
        return;
    }

    if(!R_Clipper_SafeCheckRange(angle2, angle1)) {
        return;
    }

    if(!(line->linedef->flags & (ML_DRAWMIDTEXTURE|ML_DONTOCCLUDE))) {
        if(line->backsector) {
            if((line->frontsector->ceilingpic != skyflatnum &&
                    line->frontsector->floorpic != skyflatnum) &&
                    (line->backsector->ceilingpic != skyflatnum &&
                     line->backsector->floorpic != skyflatnum)) {
                if((line->backsector->floorheight == line->backsector->ceilingheight) ||
                        line->backsector->ceilingheight <= line->frontsector->floorheight ||
                        line->backsector->floorheight >= line->frontsector->ceilingheight) {
                    R_Clipper_SafeAddClipRange(angle2, angle1);
                }
            }
        }
        else if(!line->backsector) { // sanity check
            R_Clipper_SafeAddClipRange(angle2, angle1);
        }
    }

    line->linedef->flags |= ML_MAPPED;

    R_AddLine(line);
}

int checkcoord[12][4] = {
    {3,0,2,1},
    {3,0,2,0},
    {3,1,2,0},
    {0},
    {2,0,2,1},
    {0,0,0,0},
    {3,1,3,0},
    {0},
    {2,0,3,1},
    {2,1,3,1},
    {2,1,3,0}
};

//
// R_CheckBBox
//

dboolean R_CheckBBox(fixed_t* bspcoord) {
    angle_t     angle1;
    angle_t     angle2;
    int         boxpos;
    const int*  check;

    // Find the corners of the box
    // that define the edges from current viewpoint.
    boxpos = (viewx <= bspcoord[BOXLEFT] ? 0 : viewx < bspcoord[BOXRIGHT] ? 1 : 2) +
             (viewy >= bspcoord[BOXTOP] ? 0 : viewy > bspcoord[BOXBOTTOM] ? 4 : 8);

    if(boxpos == 5) {
        return true;
    }

    check = checkcoord[boxpos];
    angle1 = R_PointToAngle2(bspcoord[check[0]], bspcoord[check[1]], viewx, viewy) - viewangle;
    angle2 = R_PointToAngle2(bspcoord[check[2]], bspcoord[check[3]], viewx, viewy) - viewangle;

    return R_Clipper_SafeCheckRange(angle2 + viewangle, angle1 + viewangle);
}

//
// AddSwitchQuad
// Draw the switch box on a linedef
//

static void AddSwitchQuad(seg_t *line) {
    int texid = 0;

    if(!SWITCHMASK(line->linedef->flags)) {
        return;
    }

    if(SWITCHMASK(line->linedef->flags) == ML_SWITCHX02) {
        texid = line->sidedef->toptexture;
    }
    else if(SWITCHMASK(line->linedef->flags) == ML_SWITCHX04) {
        texid = line->sidedef->bottomtexture;
    }
    else {
        if(!line->backsector) {
            return;
        }

        texid = line->sidedef->midtexture;
    }

    AddSegToDrawlist(&drawlist[DLT_WALL], line, texid, 3);
}

//
// R_GenerateSwitchPlane
//

dboolean R_GenerateSwitchPlane(seg_t *line, vtx_t *v) {
    fixed_t     bottom = 0;
    fixed_t     top = 0;
    int         offset = 0;
    fixed_t     cenx;
    fixed_t     ceny;
    fixed_t     x1;
    fixed_t     x2;
    fixed_t     y1;
    fixed_t     y2;
    fixed_t     f1;
    fixed_t     f2;
    fixed_t     s1;
    fixed_t     s2;

    cenx = (line->linedef->v1->x + line->linedef->v2->x) >> 1;
    ceny = (line->linedef->v1->y + line->linedef->v2->y) >> 1;

    f1 = FixedMul(2*FRACUNIT, dcos(line->angle+ANG90));
    f2 = FixedMul(2*FRACUNIT, dsin(line->angle+ANG90));

    s1 = FixedMul(16*FRACUNIT, dcos(line->angle));
    s2 = FixedMul(16*FRACUNIT, dsin(line->angle));

    x1 = cenx - s1;
    x2 = cenx + s1;

    y1 = ceny - s2;
    y2 = ceny + s2;

    x1 -= f1;
    x2 -= f1;

    y1 -= f2;
    y2 -= f2;

    v[0].tu=v[2].tu=0.0f;
    v[1].tu=v[3].tu=1.0f;

    v[0].tv=v[1].tv=0.0f;
    v[2].tv=v[3].tv=1.0f;

    v[0].x=v[2].x=F2D3D(x1);
    v[1].x=v[3].x=F2D3D(x2);

    v[0].y=v[2].y=F2D3D(y1);
    v[1].y=v[3].y=F2D3D(y2);

    R_LightToVertex(v, line->frontsector->colors[LIGHT_THING], 4);

    if(SWITCHMASK(line->linedef->flags) == ML_SWITCHX02) {
        if(line->backsector) {
            offset = 16*FRACUNIT - (line->sidedef->rowoffset);
            top = line->backsector->floorheight - offset;
            bottom = top - (32*FRACUNIT);
        }
        else {
            offset = 16*FRACUNIT + (line->sidedef->rowoffset);
            bottom = line->frontsector->floorheight + offset;
            top = bottom + (32*FRACUNIT);
        }
    }
    else if(SWITCHMASK(line->linedef->flags) == ML_SWITCHX04) {
        if(line->backsector) {
            offset = 16*FRACUNIT + (line->sidedef->rowoffset);
            bottom = line->backsector->ceilingheight + offset;
            top = bottom + (32*FRACUNIT);
        }
        else {
            offset = 16*FRACUNIT + (line->sidedef->rowoffset);
            bottom = line->frontsector->floorheight + offset;
            top = bottom + (32*FRACUNIT);
        }
    }
    else {
        if(line->backsector) {
            if(line->backsector->floorheight > line->frontsector->floorheight) {
                offset = 16*FRACUNIT - (line->sidedef->rowoffset);
                top = line->backsector->floorheight - offset;
                bottom = top - (32*FRACUNIT);
            }
            else if(line->backsector->ceilingheight < line->frontsector->ceilingheight) {
                offset = 16*FRACUNIT + (line->sidedef->rowoffset);
                bottom = line->backsector->ceilingheight + offset;
                top = bottom + (32*FRACUNIT);
            }
        }
        else {
            return false;
        }
    }

    v[0].z = F2D3D(top);
    v[1].z = F2D3D(top);
    v[2].z = F2D3D(bottom);
    v[3].z = F2D3D(bottom);

    return true;
}

d_inline static void GetSideTopBottom(sector_t* sector, rfloat *top, rfloat *bottom) {
    if(i_interpolateframes.value) {
        fixed_t frame_c = sector->frame_z2[1];
        fixed_t frame_f = sector->frame_z1[1];

        *bottom = F2D3D(frame_f);
        *top = F2D3D(frame_c);
    }
    else {
        *top = F2D3D(sector->ceilingheight);
        *bottom = F2D3D(sector->floorheight);
    }
}

//
// R_GenerateLowerSegPlane
//

static dboolean R_GenerateLowerSegPlane(seg_t *line, vtx_t* v) {
    line_t*     linedef;
    side_t*     sidedef;
    rfloat      top;
    rfloat      bottom;
    rfloat      btop;
    rfloat      bbottom;
    int         height;
    int         width;
    rfloat      length;
    rfloat      rowoffs;
    rfloat      coloffs;
    float       x;
    float       y;

    x = F2D3D(line->v1->x);
    y = F2D3D(line->v1->y);

    linedef = line->linedef;
    sidedef = line->sidedef;

    v[0].x = v[2].x = x;
    v[0].y = v[2].y = y;
    v[1].x = v[3].x = F2D3D(line->v2->x);
    v[1].y = v[3].y = F2D3D(line->v2->y);

    length = (rfloat)line->length;

    R_SetSegLineColor(line, v, 0);
    GetSideTopBottom(line->frontsector, &top, &bottom);
    GetSideTopBottom(line->backsector, &btop, &bbottom);

    if((line->frontsector->ceilingpic == skyflatnum) && (line->backsector->ceilingpic == skyflatnum)) {
        btop = top;
    }

    if(bottom < bbottom) {
        v[0].z = v[1].z = bbottom;
        v[2].z = v[3].z = bottom;

        R_SetSegLineColor(line, v, 2);

        width = texturewidth[sidedef->bottomtexture];
        height = textureheight[sidedef->bottomtexture];

        rowoffs = F2D3D(sidedef->rowoffset) / height;
        coloffs = F2D3D(sidedef->textureoffset + line->offset) / width;

        v[0].tu = v[2].tu = coloffs;
        v[1].tu = v[3].tu = length / width + coloffs;

        if(linedef->flags & ML_DONTPEGBOTTOM) {
            v[0].tv = v[1].tv = rowoffs + (top - bbottom) / height;
            v[2].tv = v[3].tv = rowoffs + (top - bottom) / height;
        }
        else {
            v[0].tv = v[1].tv = rowoffs;
            v[2].tv = v[3].tv = rowoffs + (bbottom - bottom) / height;
        }

        return true;
    }

    return false;
}

//
// R_GenerateUpperSegPlane
//

static dboolean R_GenerateUpperSegPlane(seg_t *line, vtx_t* v) {
    line_t*     linedef;
    side_t*     sidedef;
    rfloat      top;
    rfloat      bottom;
    rfloat      btop;
    rfloat      bbottom;
    int         height;
    int         width;
    rfloat      length;
    rfloat      rowoffs;
    rfloat      coloffs;
    float       x;
    float       y;

    x = F2D3D(line->v1->x);
    y = F2D3D(line->v1->y);

    linedef = line->linedef;
    sidedef = line->sidedef;

    v[0].x = v[2].x = x;
    v[0].y = v[2].y = y;
    v[1].x = v[3].x = F2D3D(line->v2->x);
    v[1].y = v[3].y = F2D3D(line->v2->y);

    length = (rfloat)line->length;

    R_SetSegLineColor(line, v, 0);
    GetSideTopBottom(line->frontsector, &top, &bottom);
    GetSideTopBottom(line->backsector, &btop, &bbottom);

    if((line->frontsector->ceilingpic == skyflatnum) && (line->backsector->ceilingpic == skyflatnum)) {
        btop = top;
    }

    if(top > btop) {
        v[0].z = v[1].z = top;
        v[2].z = v[3].z = btop;

        R_SetSegLineColor(line, v, 1);

        width = texturewidth[sidedef->toptexture];
        height = textureheight[sidedef->toptexture];

        rowoffs = F2D3D(sidedef->rowoffset) / height;
        coloffs = F2D3D(sidedef->textureoffset + line->offset) / width;

        v[0].tu = v[2].tu = coloffs;
        v[1].tu = v[3].tu = length / width + coloffs;

        if(line->linedef->flags & ML_VMIRROR) {
            rowoffs = F2D3D(sidedef->rowoffset + (height * FRACUNIT)) / height;
        }

        if(linedef->flags & ML_DONTPEGTOP) {
            v[0].tv = v[1].tv = 1 + rowoffs;
            v[2].tv = v[3].tv = 1 + rowoffs + (top - btop) / height;
        }
        else {
            v[2].tv = v[3].tv = 1 + rowoffs;
            v[0].tv = v[1].tv = 1 + rowoffs - (top - btop) / height;
        }

        return true;
    }

    return false;
}

//
// R_GenerateMiddleSegPlane
//

static dboolean R_GenerateMiddleSegPlane(seg_t *line, vtx_t* v) {
    line_t*     linedef;
    side_t*     sidedef;
    rfloat      top;
    rfloat      bottom;
    rfloat      btop;
    rfloat      bbottom;
    int         height;
    int         width;
    rfloat      length;
    rfloat      rowoffs;
    rfloat      coloffs;
    float       x;
    float       y;

    x = F2D3D(line->v1->x);
    y = F2D3D(line->v1->y);
    btop = 0;
    bbottom = 0;

    linedef = line->linedef;
    sidedef = line->sidedef;

    v[0].x = v[2].x = x;
    v[0].y = v[2].y = y;
    v[1].x = v[3].x = F2D3D(line->v2->x);
    v[1].y = v[3].y = F2D3D(line->v2->y);

    length = (rfloat)line->length;

    R_SetSegLineColor(line, v, 0);
    GetSideTopBottom(line->frontsector, &top, &bottom);

    length = (rfloat)line->length;

    if(line->backsector) {
        GetSideTopBottom(line->backsector, &btop, &bbottom);

        if((line->frontsector->ceilingpic == skyflatnum) && (line->backsector->ceilingpic == skyflatnum)) {
            btop = top;
        }

        if(bottom < bbottom) {
            bottom = bbottom;
        }

        if(top > btop) {
            top = btop;
        }
    }

    v[0].z = v[1].z = top;
    v[2].z = v[3].z = bottom;

    if(line->backsector) {
        R_SetSegLineColor(line, v, 3);
    }

    width = texturewidth[sidedef->midtexture];
    height = textureheight[sidedef->midtexture];

    rowoffs = F2D3D(sidedef->rowoffset) / height;
    coloffs = F2D3D(sidedef->textureoffset + line->offset) / width;

    v[0].tu = v[2].tu = coloffs;
    v[1].tu = v[3].tu = length / width + coloffs;

    if(!(line->linedef->flags & ML_SWITCHX02 && line->linedef->flags & ML_SWITCHX04)) {
        // ML_DONTPEGMID is extremly hacky and it appears to be used only once in the entire game
        if(linedef->flags & ML_DONTPEGMID && line->backsector) {
            v[0].tv = v[1].tv = 1 + rowoffs - ((top - btop) / height);
            v[2].tv = v[3].tv = 1 + rowoffs + (((top + btop) - (bottom + bbottom)) / height)/2;
        }
        else if(linedef->flags & ML_DONTPEGTOP && !line->backsector) {
            rowoffs = ((F2D3D(sidedef->rowoffset) - bottom) - (top - bottom)) / height;

            v[0].tv = v[1].tv = rowoffs;
            v[2].tv = v[3].tv = rowoffs + (top - bottom) / height;
        }
        else if(linedef->flags & ML_DONTPEGBOTTOM) {
            if(line->linedef->flags & ML_VMIRROR) {
                rowoffs = F2D3D(sidedef->rowoffset + (height * FRACUNIT)) / height;
            }

            v[0].tv = v[1].tv = 1 + rowoffs - (top - bottom) / height;
            v[2].tv = v[3].tv = 1 + rowoffs;
        }
        else {
            v[0].tv = v[1].tv = rowoffs;
            v[2].tv = v[3].tv = rowoffs + (top - bottom) / height;
        }
    }

    return true;
}

//
// AddSegToDrawlist
//

static void AddSegToDrawlist(drawlist_t *dl, seg_t *line, int texid, int sidetype) {
    vtxlist_t *list;

    list = DL_AddVertexList(dl);
    list->data = (seg_t*)line;

    switch(sidetype) {
    case 0:
        list->callback = R_GenerateLowerSegPlane;
        break;
    case 1:
        list->callback = R_GenerateUpperSegPlane;
        break;
    case 2:
        list->callback = R_GenerateMiddleSegPlane;
        break;
    case 3:
        list->callback = R_GenerateSwitchPlane;
        break;
    default:
        return;
    }

    if(line->linedef->flags & ML_HMIRROR) {
        list->flags |= DLF_MIRRORS;
    }

    if(line->linedef->flags & ML_VMIRROR) {
        list->flags |= DLF_MIRRORT;
    }

    if(line->frontsector->lightlevel) {
        // add seg's gamma glow values

        list->flags |= DLF_GLOW;
        list->params = line->frontsector->lightlevel;
    }

    list->texid = (list->flags << 16) | texid;
}

//
// R_AddLine
//

static void R_AddLine(seg_t *line) {
    vtx_t       v[4];
    line_t*     linedef;
    side_t*     sidedef;
    rfloat      top;
    rfloat      bottom;
    rfloat      btop;
    rfloat      bbottom;
    float       x;
    float       y;

    x = F2D3D(line->v1->x);
    y = F2D3D(line->v1->y);

    linedef = line->linedef;
    sidedef = line->sidedef;

    if(!linedef) {
        return;
    }

    v[0].x = v[2].x = x;
    v[0].y = v[2].y = y;
    v[1].x = v[3].x = F2D3D(line->v2->x);
    v[1].y = v[3].y = F2D3D(line->v2->y);

    GetSideTopBottom(line->frontsector, &top, &bottom);

    if(line->backsector) {
        GetSideTopBottom(line->backsector, &btop, &bbottom);

        if((line->frontsector->ceilingpic == skyflatnum) && (line->backsector->ceilingpic == skyflatnum)) {
            btop = top;
        }

        //
        // botom side line
        //
        if(bottom < bbottom) {
            v[0].z = v[1].z = bbottom;
            v[2].z = v[3].z = bottom;

            if(line->sidedef[0].bottomtexture != 1) {
                if(R_FrustrumTestVertex(v, 4)) {
                    AddSegToDrawlist(&drawlist[DLT_WALL], line, sidedef->bottomtexture, 0);
                    AddSwitchQuad(line);
                }
            }

            bottom = bbottom;
        }

        //
        // upper side line
        //
        if(top > btop) {
            v[0].z = v[1].z = top;
            v[2].z = v[3].z = btop;

            if(line->sidedef[0].toptexture != 1) {
                if(R_FrustrumTestVertex(v, 4)) {
                    AddSegToDrawlist(&drawlist[DLT_WALL], line, sidedef->toptexture, 1);
                    AddSwitchQuad(line);
                }
            }

            top = btop;
        }
    }

    //
    // middle side line
    //
    if(sidedef->midtexture != 1) {
        v[0].z = v[1].z = top;
        v[2].z = v[3].z = bottom;

        if(!R_FrustrumTestVertex(v, 4)) {
            return;
        }

        if(line->backsector) {
            if(!(line->linedef->flags & ML_DRAWMIDTEXTURE)) {
                return;
            }
        }

        if(!(line->linedef->flags & ML_SWITCHX02 && line->linedef->flags & ML_SWITCHX04)) {
            AddSegToDrawlist(&drawlist[DLT_WALL], line, sidedef->midtexture, 2);
            AddSwitchQuad(line);
        }
    }
}

//
// R_Subsector
//

void R_Subsector(int num) {
    subsector_t    *sub;

    sub = &subsectors[num];
    frontsector = sub->sector;

    R_AddLeaf(sub);
    R_AddSprites(sub);
}

//
// R_RenderBSPNode
//

void R_RenderBSPNode(int bspnum) {
    node_t  *bsp;
    int     side;

    while(!(bspnum & NF_SUBSECTOR)) {
        bsp = &nodes[bspnum];

        // Decide which side the view point is on.
        side = R_PointOnSide(viewx, viewy, bsp);

        // check the front space
        if(R_CheckBBox(bsp->bbox[side])) {
            R_RenderBSPNode(bsp->children[side]);
        }

        // continue down the back space
        if(!R_CheckBBox(bsp->bbox[side^1])) {
            return;
        }

        bspnum = bsp->children[side^1];
    }

    // subsector with contents
    // add all the drawable elements in the subsector
    if(bspnum == -1) {
        bspnum = 0;
        //CON_Warnf("R_RenderBSPNode: bspnum = -1!\n");
    }

    R_Subsector(bspnum & ~NF_SUBSECTOR);
}

//
// R_AllocSubsectorBuffer
// Allocate a large enough buffer to hold vertex data
// for a subsector
//

void R_AllocSubsectorBuffer(void) {
    int             i;
    subsector_t*    sub;
    int             numverts;
    int             maxSubVerts = 0;

    numverts = 0;
    for(i = 0, sub = subsectors; i < numsubsectors; i++, sub++) {
        if(sub->numleafs > numverts) {
            numverts = sub->numleafs;
        }
    }
    if(numverts <= 2) {
        I_Error("R_AllocSubsectorBuffer: Subsector has incomplete vertices");
    }

    if(numverts > maxSubVerts) {
        if(subsector_buffer) {
            Z_Free(subsector_buffer);
        }

        subsector_buffer = (vtx_t *)Z_Malloc(numverts * sizeof(vtx_t), PU_STATIC, NULL);
        maxSubVerts = numverts;
    }
}

//
// AddLeafToDrawlist
//

static void AddLeafToDrawlist(drawlist_t *dl, subsector_t *sub, int texid) {
    vtxlist_t *list;
    sector_t *sector;

    list = DL_AddVertexList(dl);
    list->data = (subsector_t*)sub;
    list->callback = NULL;

    sector = sub->sector;

    if(sector->lightlevel) {
        // add subsector's gamma glow values

        list->flags |= DLF_GLOW;
        list->params = sector->lightlevel;
    }

    list->texid = (list->flags << 16) | texid;
}

//
// R_AddLeaf
//

static void R_AddLeaf(subsector_t *sub) {
    int             i;
    int             count;
    float           x;
    float           y;
    vtx_t*          v;
    leaf_t*         leaf;

    if(sub->numleafs < 3) {
        return;
    }

    count = sub->numleafs;
    v = subsector_buffer;
    i = 0;

    while(count--) {
        leaf = &leafs[sub->leaf + i];

        x = F2D3D(leaf->vertex->x);
        y = F2D3D(leaf->vertex->y);
        v->x = x;
        v->y = y;
        v->z = F2D3D(sub->sector->floorheight);
        v++;

        if(leaf->seg != NULL) {
            R_AddClipLine(leaf->seg);
        }

        i++;
    }

    // FLOOR

    if(sub->sector->floorpic != skyflatnum) {
        if(R_FrustrumTestVertex(subsector_buffer, sub->numleafs) &&
                viewz > sub->sector->floorheight) {
            drawlist_t *dl = &drawlist[DLT_FLAT];

            if(sub->sector->flags & MS_LIQUIDFLOOR) {
                AddLeafToDrawlist(dl, sub, sub->sector->floorpic);
                dl->list[dl->index - 1].flags |= DLF_WATER1;

                AddLeafToDrawlist(dl, sub, sub->sector->floorpic + 1);
                dl->list[dl->index - 1].flags |= DLF_WATER2;
            }
            else {
                AddLeafToDrawlist(dl, sub, sub->sector->floorpic);
            }
        }
    }
    else {
        bRenderSky = true;
    }

    // CEILING

    if(sub->sector->ceilingpic != skyflatnum) {
        for(i = 0; i < sub->numleafs; i++) {
            leaf = &leafs[(sub->leaf + (sub->numleafs - 1)) - i];

            subsector_buffer[i].z = F2D3D(sub->sector->ceilingheight);
            subsector_buffer[i].x = F2D3D(leaf->vertex->x);
            subsector_buffer[i].y = F2D3D(leaf->vertex->y);
        }

        if(R_FrustrumTestVertex(subsector_buffer, sub->numleafs) &&
                viewz < sub->sector->ceilingheight) {
            drawlist_t *dl = &drawlist[DLT_FLAT];

            AddLeafToDrawlist(dl, sub, sub->sector->ceilingpic);
            dl->list[dl->index - 1].flags |= DLF_CEILING;
        }
    }
    else {
        bRenderSky = true;
    }
}





