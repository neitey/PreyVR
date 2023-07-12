// Simple beam model. different with idRenderModelBeam, the line's start point is not view origin.

#include "idlib/precompiled.h"
#include "renderer/tr_local.h"

#include "renderer/Model_local.h"

static idCVar harm_r_skipHHBeam("harm_r_skipHHBeam",                                        "0", CVAR_RENDERER | CVAR_BOOL, "[Harmattan]: Skip beam model render");
static const struct viewDef_s *current_view; // temp, should as a parameter

void hhRenderModelBeam::InitFromFile( const char *fileName )
{
	name = fileName;
	declBeam = static_cast<const hhDeclBeam *>(declManager->FindType(DECL_BEAM, fileName));
}

void hhRenderModelBeam::LoadModel()
{
	idRenderModelStatic::LoadModel();
}

dynamicModel_t hhRenderModelBeam::IsDynamicModel() const
{
	return DM_CONTINUOUS;	// regenerate for every view
}

idRenderModel* hhRenderModelBeam::InstantiateDynamicModel( const struct renderEntity_s *renderEntity, const struct viewDef_s *view, idRenderModel *cachedModel )
{
	idRenderModelStatic *staticModel;
	srfTriangles_t *tri;
	modelSurface_t surf;

	if (cachedModel) {
		delete cachedModel;
		cachedModel = NULL;
	}

	if (renderEntity == NULL || view == NULL) {
		delete cachedModel;
		return NULL;
	}

	if(harm_r_skipHHBeam.GetBool())
		return NULL;

	if(!declBeam || !renderEntity->beamNodes || declBeam->numNodes < 2)
		return NULL;

	if (cachedModel != NULL) {

		assert(dynamic_cast<idRenderModelStatic *>(cachedModel) != NULL);

		staticModel = static_cast<idRenderModelStatic *>(cachedModel);
	} else {

		staticModel = new idRenderModelStatic;
		int id = 0;
		// line
		for(int i = 0; i < declBeam->numBeams; i++)
		{
			int vertNum = declBeam->numNodes - 1;
			tri = R_AllocStaticTriSurf();
			R_AllocStaticTriSurfVerts(tri, 4 * vertNum);
			R_AllocStaticTriSurfIndexes(tri, 6 * vertNum);

			for(int m = 0; m < vertNum; m++)
			{
				tri->verts[4 * m + 0].Clear();
				tri->verts[4 * m + 0].st[0] = 0;
				tri->verts[4 * m + 0].st[1] = 0;
				tri->verts[4 * m + 0].color[0] = 1.0;
				tri->verts[4 * m + 0].color[1] = 1.0;
				tri->verts[4 * m + 0].color[2] = 1.0;
				tri->verts[4 * m + 0].color[3] = 1.0;

				tri->verts[4 * m + 1].Clear();
				tri->verts[4 * m + 1].st[0] = 0;
				tri->verts[4 * m + 1].st[1] = 1;
				tri->verts[4 * m + 1].color[0] = 1.0;
				tri->verts[4 * m + 1].color[1] = 1.0;
				tri->verts[4 * m + 1].color[2] = 1.0;
				tri->verts[4 * m + 1].color[3] = 1.0;

				tri->verts[4 * m + 2].Clear();
				tri->verts[4 * m + 2].st[0] = 1;
				tri->verts[4 * m + 2].st[1] = 0;
				tri->verts[4 * m + 2].color[0] = 1.0;
				tri->verts[4 * m + 2].color[1] = 1.0;
				tri->verts[4 * m + 2].color[2] = 1.0;
				tri->verts[4 * m + 2].color[3] = 1.0;

				tri->verts[4 * m + 3].Clear();
				tri->verts[4 * m + 3].st[0] = 1;
				tri->verts[4 * m + 3].st[1] = 1;
				tri->verts[4 * m + 3].color[0] = 1.0;
				tri->verts[4 * m + 3].color[1] = 1.0;
				tri->verts[4 * m + 3].color[2] = 1.0;
				tri->verts[4 * m + 3].color[3] = 1.0;

				tri->indexes[6 * m + 0] = 4 * m + 0;
				tri->indexes[6 * m + 1] = 4 * m + 2;
				tri->indexes[6 * m + 2] = 4 * m + 1;
				tri->indexes[6 * m + 3] = 4 * m + 2;
				tri->indexes[6 * m + 4] = 4 * m + 3;
				tri->indexes[6 * m + 5] = 4 * m + 1;
			}

			tri->numVerts = 4 * vertNum;
			tri->numIndexes = 6 * vertNum;

			surf.geometry = tri;
			surf.id = id++;
//			surf.shader = tr.defaultMaterial;
			surf.shader = declBeam->shader[i];

			staticModel->AddSurface(surf);
		}

		// quad
		for(int i = 0; i < declBeam->numBeams; i++)
		{
			for(int m = 0; m < 2; m++)
			{
				if(!declBeam->quadShader[i][m]) // no material
					continue;

				tri = R_AllocStaticTriSurf();
				R_AllocStaticTriSurfVerts(tri, 4);
				R_AllocStaticTriSurfIndexes(tri, 6);

				tri->verts[0].Clear();
				tri->verts[0].st[0] = 0;
				tri->verts[0].st[1] = 0;
				tri->verts[0].color[0] = 1.0;
				tri->verts[0].color[1] = 1.0;
				tri->verts[0].color[2] = 1.0;
				tri->verts[0].color[3] = 1.0;

				tri->verts[1].Clear();
				tri->verts[1].st[0] = 0;
				tri->verts[1].st[1] = 1;
				tri->verts[1].color[0] = 1.0;
				tri->verts[1].color[1] = 1.0;
				tri->verts[1].color[2] = 1.0;
				tri->verts[1].color[3] = 1.0;

				tri->verts[2].Clear();
				tri->verts[2].st[0] = 1;
				tri->verts[2].st[1] = 0;
				tri->verts[2].color[0] = 1.0;
				tri->verts[2].color[1] = 1.0;
				tri->verts[2].color[2] = 1.0;
				tri->verts[2].color[3] = 1.0;

				tri->verts[3].Clear();
				tri->verts[3].st[0] = 1;
				tri->verts[3].st[1] = 1;
				tri->verts[3].color[0] = 1.0;
				tri->verts[3].color[1] = 1.0;
				tri->verts[3].color[2] = 1.0;
				tri->verts[3].color[3] = 1.0;

				tri->indexes[0] = 0;
				tri->indexes[1] = 2;
				tri->indexes[2] = 1;
				tri->indexes[3] = 2;
				tri->indexes[4] = 3;
				tri->indexes[5] = 1;

				tri->numVerts = 4;
				tri->numIndexes = 6;

				surf.geometry = tri;
				surf.id = id++;
				//			surf.shader = tr.defaultMaterial;
				surf.shader = declBeam->quadShader[i][m];

				staticModel->AddSurface(surf);
			}
		}
	}

	current_view = view; // should as a parameter
	int id = 0;
	for(int i = 0; i < declBeam->numBeams; i++)
	{
		UpdateSurface(renderEntity, i, renderEntity->beamNodes, const_cast<modelSurface_t *>(staticModel->Surface(i)));
		id++;
	}
	for(int i = 0; i < declBeam->numBeams; i++)
	{
		for(int m = 0; m < 2; m++)
		{
			if(!declBeam->quadShader[i][m])
				continue;
			UpdateQuadSurface(renderEntity, i, m, renderEntity->beamNodes, const_cast<modelSurface_t *>(staticModel->Surface(id++)));
		}
	}

	staticModel->bounds = Bounds(renderEntity);

	//LOGI("BOUND %s %s %s", Name(), staticModel->bounds[0].ToString(), staticModel->bounds[1].ToString())
	return staticModel;
}

idBounds hhRenderModelBeam::Bounds( const struct renderEntity_s *renderEntity ) const
{
	idBounds	b;

	b.Zero();

	if (!renderEntity || !renderEntity->beamNodes) {
		b.ExpandSelf(8.0f);
	} else {
		for(int i = 0; i < declBeam->numBeams; i++)
		{
			for(int m = 0; m < declBeam->numNodes; m++)
			{
				const idVec3	&target = renderEntity->beamNodes->nodes[m];
				idBounds tb;
				tb.Zero();
				tb.AddPoint(target);
				tb.ExpandSelf(declBeam->thickness[i] * 0.5);
				b += tb;
			}
		}
	}

	return b;
}

//Lubos BEGIN
idVec3 hhRenderModelBeam::CalculateMinorVector( const struct renderEntity_s *renderEntity, idVec3 &up, idVec3 &right, const idVec3 &pos )
{
	// Get world position of the vertex
	idVec3 worldpos;
	renderEntity->axis.ProjectVector(pos, worldpos);
	worldpos += renderEntity->origin;

	// Convert world -> camera
	idVec3 view = ( worldpos - current_view->renderView.vieworg ) * current_view->renderView.viewaxis.Inverse();
	idVec3 cam(-view[1], view[2], view[0]);
	if (cam.z <= 0) return up;
	float x = cam.x / tan( current_view->renderView.fov_x * 0.5f * idMath::M_DEG2RAD ) / cam.z;
	float y = cam.y / tan( current_view->renderView.fov_y * 0.5f * idMath::M_DEG2RAD ) / cam.z;

	// Interpolate the minor vector
	idVec3 output(0);
	float length = fabs(x) + fabs(y);
	output += -right * fmax(-y, 0.0f) / length;
	output += right * fmax(y, 0.0f) / length;
	output += -up * fmax(-x, 0.0f) / length;
	output += up * fmax(x, 0.0f) / length;
	return output;
}
//Lubos END

void hhRenderModelBeam::UpdateSurface( const struct renderEntity_s *renderEntity, const int index, const hhBeamNodes_t *beam, modelSurface_t *surf )
{
	srfTriangles_t *tri = surf->geometry;
	idVec3 up;
	idVec3 right;
	renderEntity->axis.ProjectVector(current_view->renderView.viewaxis[2], up);
	renderEntity->axis.ProjectVector(current_view->renderView.viewaxis[1], right);

	//Lubos BEGIN
	idVec3 minor = up;
	int numLines = declBeam->numNodes - 1;
	if (numLines > 0)
		minor = CalculateMinorVector(renderEntity, up, right, beam->nodes[numLines / 2]);
	minor *= declBeam->thickness[index] * 0.5f;
	//Lubos END

	for(int i = 0; i < numLines; i++)
	{
		const idVec3	&start = beam->nodes[i];
		const idVec3	&target = beam->nodes[i + 1];

		tri->verts[4 * i + 0].xyz = start - minor;
		tri->verts[4 * i + 1].xyz = start + minor;
		tri->verts[4 * i + 2].xyz = target - minor;
		tri->verts[4 * i + 3].xyz = target + minor;
	}
	tri->bounds = Bounds(renderEntity);//Lubos
}

void hhRenderModelBeam::UpdateQuadSurface( const struct renderEntity_s *renderEntity, const int index, int quadIndex, const hhBeamNodes_t *beam, modelSurface_t *surf )
{
	srfTriangles_t *tri = surf->geometry;
	idVec3 up;
	idVec3 right;
	renderEntity->axis.ProjectVector(current_view->renderView.viewaxis[2], up);
	renderEntity->axis.ProjectVector(current_view->renderView.viewaxis[1], right);
	idVec3 target = quadIndex == 0 ? beam->nodes[0] : beam->nodes[declBeam->numNodes - 1];

	idVec3 upw = up * declBeam->quadSize[index][quadIndex] * 0.5f;
	idVec3 rightw = right * declBeam->quadSize[index][quadIndex] * 0.5f;

	tri->verts[0].xyz = target - rightw - upw;
	tri->verts[1].xyz = target - rightw + upw;
	tri->verts[2].xyz = target + rightw - upw;
	tri->verts[3].xyz = target + rightw + upw;
	tri->bounds = Bounds(renderEntity);//Lubos
}
