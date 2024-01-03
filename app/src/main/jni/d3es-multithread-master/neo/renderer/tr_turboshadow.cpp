/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "idlib/precompiled.h"
#include "renderer/tr_local.h"

int	c_turboUsedVerts;
int c_turboUnusedVerts;

/*
=====================
R_CreateVertexProgramTurboShadowVolume

are dangling edges that are outside the light frustum still making planes?
=====================
*/
srfTriangles_t *R_CreateVertexProgramTurboShadowVolume(const idRenderEntityLocal *ent,
        const srfTriangles_t *tri, const idRenderLightLocal *light,
        srfCullInfo_t &cullInfo) {
	int		i, j;
	srfTriangles_t	*newTri;
	silEdge_t	*sil;
	const glIndex_t *indexes;
	const byte *facing;

	R_CalcInteractionFacing(ent, tri, light, cullInfo);

	if (r_useShadowProjectedCull.GetBool()) {
		R_CalcInteractionCullBits(ent, tri, light, cullInfo);
	}

	int numFaces = tri->numIndexes / 3;
	int	numShadowingFaces = 0;
	facing = cullInfo.facing;

	// if all the triangles are inside the light frustum
	if (cullInfo.cullBits == LIGHT_CULL_ALL_FRONT || !r_useShadowProjectedCull.GetBool()) {

		// count the number of shadowing faces
		for (i = 0; i < numFaces; i++) {
			numShadowingFaces += facing[i];
		}

		numShadowingFaces = numFaces - numShadowingFaces;

	} else {

		// make all triangles that are outside the light frustum "facing", so they won't cast shadows
		indexes = tri->indexes;
		byte *modifyFacing = cullInfo.facing;
		const byte *cullBits = cullInfo.cullBits;

		for (j = i = 0; i < tri->numIndexes; i += 3, j++) {
			if (!modifyFacing[j]) {
				int	i1 = indexes[i+0];
				int	i2 = indexes[i+1];
				int	i3 = indexes[i+2];

				if (cullBits[i1] & cullBits[i2] & cullBits[i3]) {
					modifyFacing[j] = 1;
				} else {
					numShadowingFaces++;
				}
			}
		}
	}

	if (!numShadowingFaces) {
		// no faces are inside the light frustum and still facing the right way
		return NULL;
	}


	// count shadow indexes
	int	numShadowIndexes = 0;
	for (sil = tri->silEdges, i = tri->numSilEdges; i > 0; i--, sil++) {
		if (facing[sil->p1] ^ facing[sil->p2]) {
			numShadowIndexes += 6;
		}
	}

	// shadowVerts will be NULL on these surfaces, so the shadowVerts will be taken from the ambient surface
	newTri = R_AllocStaticTriSurf();

	// we aren't bothering to separate front and back caps on these
	newTri->numIndexes = newTri->numShadowIndexesNoFrontCaps = numShadowIndexes + numShadowingFaces * 6;
	newTri->numShadowIndexesNoCaps = numShadowIndexes;
	newTri->shadowCapPlaneBits = SHADOW_CAP_INFINITE;
	newTri->numVerts = tri->numVerts * 2;

	// alloc indexes
#ifdef USE_TRI_DATA_ALLOCATOR
	R_AllocStaticTriSurfIndexes(newTri, newTri->numIndexes);
	glIndex_t *shadowIndexes = newTri->indexes;
#else
	glIndex_t *shadowIndexes = (glIndex_t *)_alloca16(newTri->numIndexes * sizeof(glIndex_t));
#endif

	// create new triangles along sil planes
	for (sil = tri->silEdges, i = tri->numSilEdges; i > 0; i--, sil++) {

		int f1 = facing[sil->p1];
		int f2 = facing[sil->p2];

		if (!(f1 ^ f2)) {
			continue;
		}

		int v1 = sil->v1 << 1;
		int v2 = sil->v2 << 1;

		// set the two triangle winding orders based on facing
		// without using a poorly-predictable branch

		shadowIndexes[0] = v1;
		shadowIndexes[1] = v2 ^ f1;
		shadowIndexes[2] = v2 ^ f2;
		shadowIndexes[3] = v1 ^ f2;
		shadowIndexes[4] = v1 ^ f1;
		shadowIndexes[5] = v2 ^ 1;

		shadowIndexes += 6;
	}

	// these have no effect, because they extend to infinity
	newTri->bounds.Clear();

	// put some faces on the model and some on the distant projection
	indexes = tri->indexes;
	shadowIndexes = newTri->indexes + numShadowIndexes;

	for (i = 0, j = 0; i < tri->numIndexes; i += 3, j++) {
		if (facing[j]) {
			continue;
		}

		int i0 = indexes[i+0] << 1;
		shadowIndexes[2] = i0;
		shadowIndexes[3] = i0 ^ 1;
		int i1 = indexes[i+1] << 1;
		shadowIndexes[1] = i1;
		shadowIndexes[4] = i1 ^ 1;
		int i2 = indexes[i+2] << 1;
		shadowIndexes[0] = i2;
		shadowIndexes[5] = i2 ^ 1;

		shadowIndexes += 6;
	}

	return newTri;
}