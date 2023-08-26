/************************************************************************************

Filename	:	Q2VR_SurfaceView.c based on VrCubeWorld_SurfaceView.c
Content		:	This sample uses a plain Android SurfaceView and handles all
				Activity and Surface life cycle events in native code. This sample
				does not use the application framework and also does not use LibOVR.
				This sample only uses the VrApi.
Created		:	March, 2015
Authors		:	J.M.P. van Waveren / Simon Brown

Copyright	:	Copyright 2015 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>					// for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/native_window_jni.h>	// for native window JNI
#include <android/input.h>

float screenYaw;

#include "VrInput.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_mutex.h>

#include "VrApi_Helpers.h"
#include "VrApi_SystemUtils.h"
#include "VrApi_Input.h"
#include "VrCompositor.h"
#include "VrInput.h"

#include "../../../VrApi/Include/VrApi_Types.h"
#include "../../../VrApi/Include/VrApi.h"


//Define all variables here that were externs in the VrCommon.h
float playerYaw;
float vrFOV = 0.0f;
bool shutdown;

#if !defined( EGL_OPENGL_ES3_BIT_KHR )
#define EGL_OPENGL_ES3_BIT_KHR		0x0040
#endif

// EXT_texture_border_clamp
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER			0x812D
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR		0x1004
#endif


// Must use EGLSyncKHR because the VrApi still supports OpenGL ES 2.0
#define EGL_SYNC

#if defined EGL_SYNC
// EGL_KHR_reusable_sync
PFNEGLCREATESYNCKHRPROC			eglCreateSyncKHR;
PFNEGLDESTROYSYNCKHRPROC		eglDestroySyncKHR;
PFNEGLCLIENTWAITSYNCKHRPROC		eglClientWaitSyncKHR;
PFNEGLSIGNALSYNCKHRPROC			eglSignalSyncKHR;
PFNEGLGETSYNCATTRIBKHRPROC		eglGetSyncAttribKHR;
#endif

//Let's go to the maximum!
const int CPU_LEVEL			= 4;
const int GPU_LEVEL			= 4;

//Passed in from the Java code
int NUM_MULTI_SAMPLES	= -1;
float SS_MULTIPLIER    = -1.0f;

vrClientInfo vr;
vrClientInfo *pVRClientInfo;

jclass clazz;

float radians(float deg) {
	return (deg * M_PI) / 180.0;
}

float degrees(float rad) {
	return (rad * 180.0) / M_PI;
}

char **argv;
int argc=0;

/*
================================================================================

System Clock Time in millis

================================================================================
*/

double GetTimeInMilliSeconds()
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return ( now.tv_sec * 1e9 + now.tv_nsec ) * (double)(1e-6);
}


bool forceVirtualScreen = false;
bool inMenu = false;
bool inGameGuiActive = false;
bool objectiveSystemActive = false;
bool inCinematic = false;
bool loading = false;

void Doom3Quest_setUseScreenLayer(int screen)
{
	inMenu = screen & 0x1;
	inGameGuiActive = !!(screen & 0x2);
	objectiveSystemActive = !!(screen & 0x4);
	inCinematic = !!(screen & 0x8);
	loading = !!(screen & 0x10);

	pVRClientInfo->inMenu = inMenu;
}

bool Doom3Quest_useScreenLayer()
{
	return inMenu || forceVirtualScreen || inCinematic || loading || pVRClientInfo->consoleShown/*Lubos*/;
}

static void UnEscapeQuotes( char *arg )
{
	char *last = NULL;
	while( *arg ) {
		if( *arg == '"' && *last == '\\' ) {
			char *c_curr = arg;
			char *c_last = last;
			while( *c_curr ) {
				*c_last = *c_curr;
				c_last = c_curr;
				c_curr++;
			}
			*c_last = '\0';
		}
		last = arg;
		arg++;
	}
}

static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	char *lastp = NULL;
	int argc, last_argc;
	argc = last_argc = 0;
	for ( bufp = cmdline; *bufp; ) {
		while ( isspace(*bufp) ) {
			++bufp;
		}
		if ( *bufp == '"' ) {
			++bufp;
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			while ( *bufp && ( *bufp != '"' || *lastp == '\\' ) ) {
				lastp = bufp;
				++bufp;
			}
		} else {
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			while ( *bufp && ! isspace(*bufp) ) {
				++bufp;
			}
		}
		if ( *bufp ) {
			if ( argv ) {
				*bufp = '\0';
			}
			++bufp;
		}
		if( argv && last_argc != argc ) {
			UnEscapeQuotes( argv[last_argc] );
		}
		last_argc = argc;
	}
	if ( argv ) {
		argv[argc] = NULL;
	}
	return(argc);
}

/*
================================================================================

ovrEgl

================================================================================
*/

typedef struct
{
	EGLint		MajorVersion;
	EGLint		MinorVersion;
	EGLDisplay	Display;
	EGLConfig	Config;
	EGLSurface	TinySurface;
	EGLContext	Context;
} ovrEgl;

static void ovrEgl_Clear( ovrEgl * egl )
{
	egl->MajorVersion = 0;
	egl->MinorVersion = 0;
	egl->Display = 0;
	egl->Config = 0;
	egl->TinySurface = EGL_NO_SURFACE;
	egl->Context = EGL_NO_CONTEXT;
}

static void ovrEgl_CreateContext( ovrEgl * egl, const ovrEgl * shareEgl )
{
	if ( egl->Display != 0 )
	{
		return;
	}

	egl->Display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	eglInitialize( egl->Display, &egl->MajorVersion, &egl->MinorVersion );
	// Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
	// flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
	// settings, and that is completely wasted for our warp target.
	const int MAX_CONFIGS = 1024;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs = 0;
	if ( eglGetConfigs( egl->Display, configs, MAX_CONFIGS, &numConfigs ) == EGL_FALSE )
	{
		ALOGE( "        eglGetConfigs() failed: %d", eglGetError() );
		return;
	}
	const EGLint configAttribs[] =
	{
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE,		8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE,		8, // need alpha for the multi-pass timewarp compositor
		EGL_DEPTH_SIZE,		24,
		EGL_STENCIL_SIZE,	8,
		EGL_SAMPLES,		0,
		EGL_NONE
	};
	egl->Config = 0;
	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint value = 0;

		eglGetConfigAttrib( egl->Display, configs[i], EGL_RENDERABLE_TYPE, &value );
		if ( ( value & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR )
		{
			continue;
		}

		// The pbuffer config also needs to be compatible with normal window rendering
		// so it can share textures with the window context.
		eglGetConfigAttrib( egl->Display, configs[i], EGL_SURFACE_TYPE, &value );
		if ( ( value & ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) ) != ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) )
		{
			continue;
		}

		int	j = 0;
		for ( ; configAttribs[j] != EGL_NONE; j += 2 )
		{
			eglGetConfigAttrib( egl->Display, configs[i], configAttribs[j], &value );
			if ( value != configAttribs[j + 1] )
			{
				break;
			}
		}
		if ( configAttribs[j] == EGL_NONE )
		{
			egl->Config = configs[i];
			break;
		}
	}
	if ( egl->Config == 0 )
	{
		ALOGE( "        eglChooseConfig() failed: %d", eglGetError() );
		return;
	}
	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	ALOGV( "        Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )" );
	egl->Context = eglCreateContext( egl->Display, egl->Config, ( shareEgl != NULL ) ? shareEgl->Context : EGL_NO_CONTEXT, contextAttribs );
	if ( egl->Context == EGL_NO_CONTEXT )
	{
		ALOGE( "        eglCreateContext() failed: %d", eglGetError() );
		return;
	}
	const EGLint surfaceAttribs[] =
	{
		EGL_WIDTH, 16,
		EGL_HEIGHT, 16,
		EGL_NONE
	};
	ALOGV( "        TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )" );
	egl->TinySurface = eglCreatePbufferSurface( egl->Display, egl->Config, surfaceAttribs );
	if ( egl->TinySurface == EGL_NO_SURFACE )
	{
		ALOGE( "        eglCreatePbufferSurface() failed: %d", eglGetError() );
		eglDestroyContext( egl->Display, egl->Context );
		egl->Context = EGL_NO_CONTEXT;
		return;
	}
	ALOGV( "        eglMakeCurrent( Display, TinySurface, TinySurface, Context )" );
	if ( eglMakeCurrent( egl->Display, egl->TinySurface, egl->TinySurface, egl->Context ) == EGL_FALSE )
	{
		ALOGE( "        eglMakeCurrent() failed: %d", eglGetError() );
		eglDestroySurface( egl->Display, egl->TinySurface );
		eglDestroyContext( egl->Display, egl->Context );
		egl->Context = EGL_NO_CONTEXT;
		return;
	}
}

/*
================================================================================

ovrFramebuffer

================================================================================
*/


static void ovrFramebuffer_Clear( ovrFramebuffer * frameBuffer )
{
	frameBuffer->Width = 0;
	frameBuffer->Height = 0;
	frameBuffer->Multisamples = 0;
	frameBuffer->TextureSwapChainLength = 0;
	frameBuffer->ProcessingTextureSwapChainIndex = 0;
	frameBuffer->ReadyTextureSwapChainIndex = 0;
	frameBuffer->ColorTextureSwapChain = NULL;
	frameBuffer->DepthBuffers = NULL;
	frameBuffer->FrameBuffers = NULL;
}

typedef void (GL_APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);

#if !defined(GL_OVR_multiview)
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(
		GLenum target,
		GLenum attachment,
		GLuint texture,
		GLint level,
		GLint baseViewIndex,
		GLsizei numViews);
#endif

#if !defined(GL_OVR_multiview_multisampled_render_to_texture)
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)(
		GLenum target,
		GLenum attachment,
		GLuint texture,
		GLint level,
		GLsizei samples,
		GLint baseViewIndex,
		GLsizei numViews);
#endif

static bool ovrFramebuffer_Create(
		ovrFramebuffer* frameBuffer,
		const bool useMultiview,
		const GLenum colorFormat,
		const int width,
		const int height,
		const int multisamples) {
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
			(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress(
					"glRenderbufferStorageMultisampleEXT");
	PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
			(PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress(
					"glFramebufferTexture2DMultisampleEXT");

	PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR =
			(PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)eglGetProcAddress(
					"glFramebufferTextureMultiviewOVR");
	PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glFramebufferTextureMultisampleMultiviewOVR =
			(PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)eglGetProcAddress(
					"glFramebufferTextureMultisampleMultiviewOVR");

	frameBuffer->Width = width;
	frameBuffer->Height = height;
	frameBuffer->Multisamples = multisamples;
	frameBuffer->UseMultiview =
			(useMultiview && (glFramebufferTextureMultiviewOVR != NULL)) ? true : false;

	frameBuffer->ColorTextureSwapChain = vrapi_CreateTextureSwapChain3(
			frameBuffer->UseMultiview ? VRAPI_TEXTURE_TYPE_2D_ARRAY : VRAPI_TEXTURE_TYPE_2D,
			colorFormat,
			width,
			height,
			1,
			3);
	frameBuffer->TextureSwapChainLength =
			vrapi_GetTextureSwapChainLength(frameBuffer->ColorTextureSwapChain);
	frameBuffer->DepthBuffers =
			(GLuint*)malloc(frameBuffer->TextureSwapChainLength * sizeof(GLuint));
	frameBuffer->FrameBuffers =
			(GLuint*)malloc(frameBuffer->TextureSwapChainLength * sizeof(GLuint));

	ALOGV("        frameBuffer->UseMultiview = %d", frameBuffer->UseMultiview);

	for (int i = 0; i < frameBuffer->TextureSwapChainLength; i++) {
		// Create the color buffer texture.
		const GLuint colorTexture =
				vrapi_GetTextureSwapChainHandle(frameBuffer->ColorTextureSwapChain, i);
		GLenum colorTextureTarget = frameBuffer->UseMultiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
		GL(glBindTexture(colorTextureTarget, colorTexture));
		GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
		GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
		GLfloat borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
		GL(glTexParameterfv(colorTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor));
		GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL(glBindTexture(colorTextureTarget, 0));

		if (frameBuffer->UseMultiview) {
			// Create the depth buffer texture.
			GL(glGenTextures(1, &frameBuffer->DepthBuffers[i]));
			GL(glBindTexture(GL_TEXTURE_2D_ARRAY, frameBuffer->DepthBuffers[i]));
			GL(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT32F, width, height, 2));
			GL(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));

			// Create the frame buffer.
			GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
			GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
			if (multisamples > 1 && (glFramebufferTextureMultisampleMultiviewOVR != NULL)) {
				GL(glFramebufferTextureMultisampleMultiviewOVR(
						GL_DRAW_FRAMEBUFFER,
						GL_DEPTH_ATTACHMENT,
						frameBuffer->DepthBuffers[i],
						0 /* level */,
						multisamples /* samples */,
						0 /* baseViewIndex */,
						2 /* numViews */));
				GL(glFramebufferTextureMultisampleMultiviewOVR(
						GL_DRAW_FRAMEBUFFER,
						GL_COLOR_ATTACHMENT0,
						colorTexture,
						0 /* level */,
						multisamples /* samples */,
						0 /* baseViewIndex */,
						2 /* numViews */));
			} else {
				GL(glFramebufferTextureMultiviewOVR(
						GL_DRAW_FRAMEBUFFER,
						GL_DEPTH_ATTACHMENT,
						frameBuffer->DepthBuffers[i],
						0 /* level */,
						0 /* baseViewIndex */,
						2 /* numViews */));
				GL(glFramebufferTextureMultiviewOVR(
						GL_DRAW_FRAMEBUFFER,
						GL_COLOR_ATTACHMENT0,
						colorTexture,
						0 /* level */,
						0 /* baseViewIndex */,
						2 /* numViews */));
			}

			GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
			GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
			if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
				ALOGE(
						"Incomplete frame buffer object: %d", renderFramebufferStatus);
				return false;
			}
		} else {
			if (multisamples > 1 && glRenderbufferStorageMultisampleEXT != NULL &&
				glFramebufferTexture2DMultisampleEXT != NULL) {
				// Create multisampled depth buffer.
				GL(glGenRenderbuffers(1, &frameBuffer->DepthBuffers[i]));
				GL(glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
				GL(glRenderbufferStorageMultisampleEXT(
						GL_RENDERBUFFER, multisamples, GL_DEPTH_COMPONENT32F, width, height));
				GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

				// Create the frame buffer.
				// NOTE: glFramebufferTexture2DMultisampleEXT only works with GL_FRAMEBUFFER.
				GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
				GL(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
				GL(glFramebufferTexture2DMultisampleEXT(
						GL_FRAMEBUFFER,
						GL_COLOR_ATTACHMENT0,
						GL_TEXTURE_2D,
						colorTexture,
						0,
						multisamples));
				GL(glFramebufferRenderbuffer(
						GL_FRAMEBUFFER,
						GL_DEPTH_ATTACHMENT,
						GL_RENDERBUFFER,
						frameBuffer->DepthBuffers[i]));
				GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER));
				GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
				if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
					ALOGE(
							"Incomplete frame buffer object: %d", renderFramebufferStatus);
					return false;
				}
			} else {
				// Create depth buffer.
				GL(glGenRenderbuffers(1, &frameBuffer->DepthBuffers[i]));
				GL(glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
				GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height));
				GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

				// Create the frame buffer.
				GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
				GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
				GL(glFramebufferRenderbuffer(
						GL_DRAW_FRAMEBUFFER,
						GL_DEPTH_ATTACHMENT,
						GL_RENDERBUFFER,
						frameBuffer->DepthBuffers[i]));
				GL(glFramebufferTexture2D(
						GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0));
				GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
				GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
				if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
					ALOGE(
							"Incomplete frame buffer object: %d", renderFramebufferStatus);
					return false;
				}
			}
		}
	}

	return true;
}

void ovrFramebuffer_Destroy(ovrFramebuffer* frameBuffer) {
	GL(glDeleteFramebuffers(frameBuffer->TextureSwapChainLength, frameBuffer->FrameBuffers));
	if (frameBuffer->UseMultiview) {
		GL(glDeleteTextures(frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers));
	} else {
		GL(glDeleteRenderbuffers(frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers));
	}
	vrapi_DestroyTextureSwapChain(frameBuffer->ColorTextureSwapChain);

	free(frameBuffer->DepthBuffers);
	free(frameBuffer->FrameBuffers);

	ovrFramebuffer_Clear(frameBuffer);
}

void ovrFramebuffer_SetCurrent( ovrFramebuffer * frameBuffer )
{
	GL( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer->FrameBuffers[frameBuffer->ProcessingTextureSwapChainIndex] ) );
}

void ovrFramebuffer_SetNone()
{
	GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
}

void ovrFramebuffer_Resolve( ovrFramebuffer * frameBuffer )
{
	// Discard the depth buffer, so the tiler won't need to write it back out to memory.
	const GLenum depthAttachment[1] = { GL_DEPTH_ATTACHMENT };
	glInvalidateFramebuffer( GL_DRAW_FRAMEBUFFER, 1, depthAttachment );
}

void ovrFramebuffer_Advance( ovrFramebuffer * frameBuffer )
{
	// Advance to the next texture from the set.
    frameBuffer->ReadyTextureSwapChainIndex = frameBuffer->ProcessingTextureSwapChainIndex;
	frameBuffer->ProcessingTextureSwapChainIndex = ( frameBuffer->ProcessingTextureSwapChainIndex + 1 ) % frameBuffer->TextureSwapChainLength;
}


void ovrFramebuffer_ClearEdgeTexels( ovrFramebuffer * frameBuffer )
{
	GL( glEnable( GL_SCISSOR_TEST ) );
	GL( glViewport( 0, 0, frameBuffer->Width, frameBuffer->Height ) );

	// Explicitly clear the border texels to black because OpenGL-ES does not support GL_CLAMP_TO_BORDER.
	// Clear to fully opaque black.
	GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );

	// bottom
	GL( glScissor( 0, 0, frameBuffer->Width, 1 ) );
	GL( glClear( GL_COLOR_BUFFER_BIT ) );
	// top
	GL( glScissor( 0, frameBuffer->Height - 1, frameBuffer->Width, 1 ) );
	GL( glClear( GL_COLOR_BUFFER_BIT ) );
	// left
	GL( glScissor( 0, 0, 1, frameBuffer->Height ) );
	GL( glClear( GL_COLOR_BUFFER_BIT ) );
	// right
	GL( glScissor( frameBuffer->Width - 1, 0, 1, frameBuffer->Height ) );
	GL( glClear( GL_COLOR_BUFFER_BIT ) );


	GL( glScissor( 0, 0, 0, 0 ) );
	GL( glDisable( GL_SCISSOR_TEST ) );
}


/*
================================================================================

ovrRenderer

================================================================================
*/


void ovrRenderer_Clear( ovrRenderer * renderer )
{
	ovrFramebuffer_Clear( &renderer->FrameBuffer );
	renderer->ProjectionMatrix = ovrMatrix4f_CreateIdentity();
	renderer->NumBuffers = VRAPI_FRAME_LAYER_EYE_MAX;
}

float Doom3Quest_GetFOV();

void ovrRenderer_Create( int width, int height, ovrRenderer * renderer, const ovrJava * java )
{
	renderer->NumBuffers = 1; // Multiview

	//Now using a symmetrical render target, based on the horizontal FOV
	Doom3Quest_GetFOV();

	// Create the multi view frame buffer
	ovrFramebuffer_Create( &renderer->FrameBuffer,
						   true,
						   GL_RGBA8,
						   width,
						   height,
						   NUM_MULTI_SAMPLES );

	// Setup the projection matrix.
	renderer->ProjectionMatrix = ovrMatrix4f_CreateProjectionFov(
			vrFOV, vrFOV, 0.0f, 0.0f, 1.0f, 0.0f );

}

void ovrRenderer_Destroy( ovrRenderer * renderer )
{
	ovrFramebuffer_Destroy( &renderer->FrameBuffer );
	renderer->ProjectionMatrix = ovrMatrix4f_CreateIdentity();
}


#ifndef EPSILON
#define EPSILON 0.001f
#endif

static ovrVector3f normalizeVec(ovrVector3f vec) {
    //NOTE: leave w-component untouched
    //@@const float EPSILON = 0.000001f;
    float xxyyzz = vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
    //@@if(xxyyzz < EPSILON)
    //@@    return *this; // do nothing if it is zero vector

    //float invLength = invSqrt(xxyyzz);
    ovrVector3f result;
    float invLength = 1.0f / sqrtf(xxyyzz);
    result.x = vec.x * invLength;
    result.y = vec.y * invLength;
    result.z = vec.z * invLength;
    return result;
}

void NormalizeAngles(vec3_t angles)
{
	while (angles[0] >= 90) angles[0] -= 180;
	while (angles[1] >= 180) angles[1] -= 360;
	while (angles[2] >= 180) angles[2] -= 360;
	while (angles[0] < -90) angles[0] += 180;
	while (angles[1] < -180) angles[1] += 360;
	while (angles[2] < -180) angles[2] += 360;
}

void GetAnglesFromVectors(const ovrVector3f forward, const ovrVector3f right, const ovrVector3f up, vec3_t angles)
{
	float sr, sp, sy, cr, cp, cy;

	sp = -forward.z;

	float cp_x_cy = forward.x;
	float cp_x_sy = forward.y;
	float cp_x_sr = -right.z;
	float cp_x_cr = up.z;

	float yaw = atan2(cp_x_sy, cp_x_cy);
	float roll = atan2(cp_x_sr, cp_x_cr);

	cy = cos(yaw);
	sy = sin(yaw);
	cr = cos(roll);
	sr = sin(roll);

	if (fabs(cy) > EPSILON)
	{
	cp = cp_x_cy / cy;
	}
	else if (fabs(sy) > EPSILON)
	{
	cp = cp_x_sy / sy;
	}
	else if (fabs(sr) > EPSILON)
	{
	cp = cp_x_sr / sr;
	}
	else if (fabs(cr) > EPSILON)
	{
	cp = cp_x_cr / cr;
	}
	else
	{
	cp = cos(asin(sp));
	}

	float pitch = atan2(sp, cp);

	angles[0] = pitch / (M_PI*2.f / 360.f);
	angles[1] = yaw / (M_PI*2.f / 360.f);
	angles[2] = roll / (M_PI*2.f / 360.f);

	NormalizeAngles(angles);
}

void QuatToYawPitchRoll(ovrQuatf q, vec3_t rotation, vec3_t out) {

    ovrMatrix4f mat = ovrMatrix4f_CreateFromQuaternion( &q );

    if (rotation[0] != 0.0f || rotation[1] != 0.0f || rotation[2] != 0.0f)
	{
		ovrMatrix4f rot = ovrMatrix4f_CreateRotation(radians(rotation[0]), radians(rotation[1]), radians(rotation[2]));
		mat = ovrMatrix4f_Multiply(&mat, &rot);
	}

    ovrVector4f v1 = {0, 0, -1, 0};
    ovrVector4f v2 = {1, 0, 0, 0};
    ovrVector4f v3 = {0, 1, 0, 0};

    ovrVector4f forwardInVRSpace = ovrVector4f_MultiplyMatrix4f(&mat, &v1);
    ovrVector4f rightInVRSpace = ovrVector4f_MultiplyMatrix4f(&mat, &v2);
    ovrVector4f upInVRSpace = ovrVector4f_MultiplyMatrix4f(&mat, &v3);

	ovrVector3f forward = {-forwardInVRSpace.z, -forwardInVRSpace.x, forwardInVRSpace.y};
	ovrVector3f right = {-rightInVRSpace.z, -rightInVRSpace.x, rightInVRSpace.y};
	ovrVector3f up = {-upInVRSpace.z, -upInVRSpace.x, upInVRSpace.y};

	ovrVector3f forwardNormal = normalizeVec(forward);
	ovrVector3f rightNormal = normalizeVec(right);
	ovrVector3f upNormal = normalizeVec(up);

	GetAnglesFromVectors(forwardNormal, rightNormal, upNormal, out);
}

void updateHMDOrientation()
{
    //Position
    VectorSubtract(vr.hmdposition_last, vr.hmdposition, vr.hmdposition_delta);

    //Keep this for our records
    VectorCopy(vr.hmdposition, vr.hmdposition_last);
}

void setHMDPosition( float x, float y, float z, float yaw )
{
	VectorSet(vr.hmdposition, x, y, z);
}

void setHMDOrientation( float x, float y, float z, float w )
{
    Vector4Set(vr.hmdorientation_quat, x, y, z, w);
}


/*
========================
Doom3Quest_Vibrate
========================
*/

//0 = left, 1 = right
float vibration_channel_intensity[2][2] = {{0.0f,0.0f},{0.0f,0.0f}};
int vibration_length[2] = {0, 0};

int Android_GetCVarInteger(const char* cvar);

void Doom3Quest_Vibrate(int channel, float low, float high, int length)
{
	if (Android_GetCVarInteger("vr_haptics")) {
		vibration_channel_intensity[channel][0] = low;
		vibration_channel_intensity[channel][1] = high;
		vibration_length[channel] = length;
	}
}

void jni_haptic_event(const char* event, int position, int flags, int intensity, float angle, float yHeight);
void jni_haptic_updateevent(const char* event, int intensity, float angle);
void jni_haptic_stopevent(const char* event);
void jni_haptic_endframe();
void jni_haptic_enable();
void jni_haptic_disable();

void Doom3Quest_HapticEvent(const char* event, int position, int flags, int intensity, float angle, float yHeight )
{
    jni_haptic_event(event, position, flags, intensity, angle, yHeight);
}

void Doom3Quest_HapticUpdateEvent(const char* event, int intensity, float angle )
{
    jni_haptic_updateevent(event, intensity, angle);
}

void Doom3Quest_HapticEndFrame()
{
	jni_haptic_endframe();
}

void Doom3Quest_HapticStopEvent(const char* event)
{
	jni_haptic_stopevent(event);
}

void Doom3Quest_HapticEnable()
{
    jni_haptic_enable();
}

void Doom3Quest_HapticDisable()
{
    jni_haptic_disable();
}

void VR_Doom3Main(int argc, char** argv);

//Lubos BEGIN
float hmdposition_last[3] = {};

void VR_GetMove( float *joy_forward, float *joy_side, float *hmd_forward, float *hmd_side, float *up, float *yaw, float *pitch, float *roll ) {
	float dx = pVRClientInfo->hmdposition_last[0] - hmdposition_last[0];
	float dy = pVRClientInfo->hmdposition_last[1] - hmdposition_last[1];
	float dz = pVRClientInfo->hmdposition_last[2] - hmdposition_last[2];
	if (fabs(dx) + fabs(dy) + fabs(dz) > 1) {
		dx = 0; dy = 0; dz = 0;
	}

	vec2_t v;
	rotateAboutOrigin(dx,-dz, -pVRClientInfo->hmdorientation_temp[YAW], v);
	*hmd_forward = v[0] * 100.0f;
	*hmd_side = v[1] * 100.0f;
	*joy_side = remote_movementSideways;
	*joy_forward = remote_movementForward;
	*up = pVRClientInfo->hmdposition_last[1];
	if (Doom3Quest_useScreenLayer()) {
		*up = 1.5f;
	}

	if (fabs(vr.hmdorientation_diff[PITCH]) > 1) vr.hmdorientation_offset[PITCH] += vr.hmdorientation_diff[PITCH] * 0.1f;
	if (fabs(vr.hmdorientation_diff[ROLL]) > 1) vr.hmdorientation_offset[ROLL] += vr.hmdorientation_diff[ROLL] * 0.1f;
	*pitch = vr.hmdorientation_temp[PITCH] - vr.hmdorientation_offset[PITCH];
	*roll = vr.hmdorientation_temp[ROLL] - vr.hmdorientation_offset[ROLL];
	*yaw = vr.hmdorientation_temp[YAW] + vr.snapTurn;

	hmdposition_last[0] = pVRClientInfo->hmdposition_last[0];
	hmdposition_last[1] = pVRClientInfo->hmdposition_last[1];
	hmdposition_last[2] = pVRClientInfo->hmdposition_last[2];
}

extern ovrVector2f *pPrimaryJoystick;

void VR_GetJoystick( float *x, float *y ) {
	*x = -pPrimaryJoystick->x;
	*y = -pPrimaryJoystick->y;
}
//Lubos END

/*
================================================================================

ovrRenderThread

================================================================================
*/


/*
================================================================================

ovrApp

================================================================================
*/

#define MAX_TRACKING_SAMPLES 4

typedef struct
{
	ovrJava				Java;
	ovrEgl				Egl;
	ANativeWindow *		NativeWindow;
	bool				Resumed;
	ovrMobile *			Ovr;
    ovrScene			Scene;
	SDL_mutex *			RenderThreadFrameIndex_Mutex;
	long long			RenderThreadFrameIndex;
	long long			MainThreadFrameIndex;
	double 				DisplayTime[MAX_TRACKING_SAMPLES];
    ovrTracking2        Tracking[MAX_TRACKING_SAMPLES];
	int					SwapInterval;
	int					CpuLevel;
	int					GpuLevel;
	int					MainThreadTid;
	int					RenderThreadTid;
	ovrLayer_Union2		Layers[ovrMaxLayerCount];
	int					LayerCount;
	ovrRenderer			Renderer;
} ovrApp;

static void ovrApp_Clear( ovrApp * app )
{
	app->Java.Vm = NULL;
	app->Java.Env = NULL;
	app->Java.ActivityObject = NULL;
	app->Ovr = NULL;
	app->RenderThreadFrameIndex_Mutex = SDL_CreateMutex();
    app->RenderThreadFrameIndex = 1;
    app->MainThreadFrameIndex = 1;
    memset(app->DisplayTime, 0, MAX_TRACKING_SAMPLES * sizeof(double));
    memset(app->Tracking, 0, MAX_TRACKING_SAMPLES * sizeof(ovrTracking2));
	app->SwapInterval = 1;
	app->CpuLevel = CPU_LEVEL;
	app->GpuLevel = GPU_LEVEL;
	app->MainThreadTid = 0;
	app->RenderThreadTid = 0;

	ovrEgl_Clear( &app->Egl );

	ovrScene_Clear( &app->Scene );
	ovrRenderer_Clear( &app->Renderer );
}

static ovrApp gAppState;
static ovrJava java;
static JavaVM *jVM;
static bool destroyed = false;

float Doom3Quest_GetFOV()
{
	vrFOV = vrapi_GetSystemPropertyInt(&gAppState.Java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y);

	return vrFOV;
}

int Doom3Quest_GetRefresh()
{
	if ( gAppState.Ovr == NULL ) {
		return 60;
	} else if (pVRClientInfo && pVRClientInfo->hackFramerate) {
		return 60;
	}
	return vrapi_GetSystemPropertyInt(&gAppState.Java, VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE);
}

static void ovrApp_HandleVrModeChanges( ovrApp * app )
{
	if ( app->Resumed != false && app->NativeWindow != NULL )
	{
		if ( app->Ovr == NULL )
		{
            ovrJava sJava = java;
            sJava.Env = NULL;
            (*sJava.Vm)->AttachCurrentThread(sJava.Vm, &sJava.Env, NULL);

			ovrModeParms parms = vrapi_DefaultModeParms( &sJava );
			// Must reset the FLAG_FULLSCREEN window flag when using a SurfaceView
			parms.Flags |= VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;

			parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
			parms.Display = (size_t)app->Egl.Display;
			parms.WindowSurface = (size_t)app->NativeWindow;
			parms.ShareContext = (size_t)app->Egl.Context;

			ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );

			ALOGV( "        vrapi_EnterVrMode()" );

			app->Ovr = vrapi_EnterVrMode( &parms );

			ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );

			// If entering VR mode failed then the ANativeWindow was not valid.
			if ( app->Ovr == NULL )
			{
				ALOGE( "Invalid ANativeWindow!" );
				app->NativeWindow = NULL;
			}

			// Set performance parameters once we have entered VR mode and have a valid ovrMobile.
			if ( app->Ovr != NULL )
			{
				vrapi_SetClockLevels( app->Ovr, app->CpuLevel, app->GpuLevel );

				ALOGV( "		vrapi_SetClockLevels( %d, %d )", app->CpuLevel, app->GpuLevel );

				vrapi_SetPerfThread( app->Ovr, VRAPI_PERF_THREAD_TYPE_MAIN, app->MainThreadTid );

				ALOGV( "		vrapi_SetPerfThread( MAIN, %d )", app->MainThreadTid );

				vrapi_SetPerfThread( app->Ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, app->RenderThreadTid );

				ALOGV( "		vrapi_SetPerfThread( RENDERER, %d )", app->RenderThreadTid );

                vrapi_SetExtraLatencyMode(app->Ovr, VRAPI_EXTRA_LATENCY_MODE_ON);
			}

		}
	}
	else
	{
		if ( app->Ovr != NULL )
		{
			ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );

			ALOGV( "        vrapi_LeaveVrMode()" );

			vrapi_LeaveVrMode( app->Ovr );
			app->Ovr = NULL;

			ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
		}
	}
}

int m_width;
int m_height;

void Doom3Quest_GetScreenRes(int *width, int *height)
{
    *width = m_width;
    *height = m_height;
}

//void initialize_gl4es();

void VR_Init()
{
	//Initialise all our variables
	screenYaw = 0.0f;
	remote_movementSideways = 0.0f;
	remote_movementForward = 0.0f;
	vr.snapTurn = 0.0f;
	vr.visible_hud = true;

	//init randomiser
	srand(time(NULL));

	shutdown = false;
}

long renderThreadCPUTime = 0;

void Doom3Quest_prepareEyeBuffer( )
{
	ovrRenderer *renderer = Doom3Quest_useScreenLayer() ? &gAppState.Scene.CylinderRenderer : &gAppState.Renderer;

	ovrFramebuffer *frameBuffer = &(renderer->FrameBuffer);
	ovrFramebuffer_SetCurrent(frameBuffer);

	renderThreadCPUTime = GetTimeInMilliSeconds();

	GL(glEnable(GL_SCISSOR_TEST));
	GL(glDepthMask(GL_TRUE));
	GL(glEnable(GL_DEPTH_TEST));
	GL(glDepthFunc(GL_LEQUAL));

	//Weusing the size of the render target
	GL(glViewport(0, 0, frameBuffer->Width, frameBuffer->Height));
	GL(glScissor(0, 0, frameBuffer->Width, frameBuffer->Height));

	GL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	GL(glDisable(GL_SCISSOR_TEST));
}

void Doom3Quest_finishEyeBuffer( )
{
    renderThreadCPUTime = GetTimeInMilliSeconds() - renderThreadCPUTime;

    {
        //__android_log_print(ANDROID_LOG_INFO, "Doom3Quest", "RENDER THREAD TOTAL CPU TIME: %ld", renderThreadCPUTime);
    }

    GLCheckErrors( __LINE__ );

	ovrRenderer *renderer = Doom3Quest_useScreenLayer() ? &gAppState.Scene.CylinderRenderer : &gAppState.Renderer;

	ovrFramebuffer *frameBuffer = &(renderer->FrameBuffer);

	//Clear edge to prevent smearing
	ovrFramebuffer_ClearEdgeTexels(frameBuffer);
	ovrFramebuffer_Resolve(frameBuffer);
	ovrFramebuffer_Advance(frameBuffer);

	ovrFramebuffer_SetNone();
}

void Doom3Quest_processMessageQueue() {
	if ( gAppState.Ovr == NULL && destroyed == false )
	{
		ovrApp_HandleVrModeChanges( &gAppState );
	}
}


void shutdownVR() {
    SDL_DestroyMutex(gAppState.RenderThreadFrameIndex_Mutex);
	ovrRenderer_Destroy( &gAppState.Renderer );
	(*java.Vm)->DetachCurrentThread( java.Vm );
}

void jni_shutdown();

/* Called before SDL_main() to initialize JNI bindings in SDL library */
extern void SDL_Android_Init(JNIEnv* env, jclass cls);

//Calld on the main thread before the rendering thread is started
void DeactivateContext()
{
    eglMakeCurrent( gAppState.Egl.Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
}

//Caled by the rendering thread to take charge of the context
void ActivateContext()
{
    eglMakeCurrent( gAppState.Egl.Display, gAppState.Egl.TinySurface, gAppState.Egl.TinySurface, gAppState.Egl.Context );

    gAppState.RenderThreadTid = gettid();
}

int questType;

void * AppThreadFunction(void * parm ) {

	java.Vm = jVM;
	(*java.Vm)->AttachCurrentThread(java.Vm, &java.Env, NULL);

    pVRClientInfo = &vr;

	// Note that AttachCurrentThread will reset the thread name.
	prctl(PR_SET_NAME, (long) "OVR::Main", 0, 0, 0);

	const ovrInitParms initParms = vrapi_DefaultInitParms(&java);
	int32_t initResult = vrapi_Initialize(&initParms);
	if (initResult != VRAPI_INITIALIZE_SUCCESS) {
		// If intialization failed, vrapi_* function calls will not be available.
		exit(0);
	}

    VR_Init();

	ovrApp_Clear(&gAppState);
	gAppState.Java = java;

	// This app will handle android gamepad events itself.
	vrapi_SetPropertyInt(&gAppState.Java, VRAPI_EAT_NATIVE_GAMEPAD_EVENTS, 0);

	gAppState.CpuLevel = CPU_LEVEL;
	gAppState.GpuLevel = GPU_LEVEL;
	gAppState.MainThreadTid = gettid();

	ovrEgl_CreateContext(&gAppState.Egl, NULL);

    chdir("/sdcard/PreyVR");

	// This app will handle android gamepad events itself.
	vrapi_SetPropertyInt(&gAppState.Java, VRAPI_EAT_NATIVE_GAMEPAD_EVENTS, 0);

	//Set device defaults
	if (vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_DEVICE_TYPE) == VRAPI_DEVICE_TYPE_OCULUSQUEST)
	{
		questType = 1;
		if (SS_MULTIPLIER == -1.0f)
		{
			SS_MULTIPLIER = 1.0f;
		}

		if (NUM_MULTI_SAMPLES == -1)
		{
			NUM_MULTI_SAMPLES = 1;
		}
	}
	else if (vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_DEVICE_TYPE) == VRAPI_DEVICE_TYPE_OCULUSQUEST2)
	{
		questType = 2;
		if (SS_MULTIPLIER == -1.0f)
		{
			SS_MULTIPLIER = 1.0f;//Lubos
		}

		if (NUM_MULTI_SAMPLES == -1)
		{
			NUM_MULTI_SAMPLES = 1;//Lubos
		}
	} else {
	    //Don't know what headset this is!? abort
        return NULL;
	}

	//Using a symmetrical render target
	m_height = m_width = (int)(vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH) *  SS_MULTIPLIER);

    //First handle any messages in the queue
    while ( gAppState.Ovr == NULL ) {
        Doom3Quest_processMessageQueue();
    }

    ovrRenderer_Create(m_width, m_height, &gAppState.Renderer, &java);

    if ( gAppState.Ovr == NULL )
    {
        return NULL;
    }

    // Create the scene if not yet created.
    ovrScene_Create( m_width, m_height, &gAppState.Scene, &java );

    //Should now be all set up and ready - start the Doom3 main loop
    VR_Doom3Main(argc, argv);

    //Take the context back
    ActivateContext();

	//We are done, shutdown cleanly
	shutdownVR();

	//Ask Java to shut down
    jni_shutdown();

	return NULL;
}

//All the stuff we want to do each frame
void Doom3Quest_FrameSetup(int controlscheme, int switch_sticks, int refresh)
{
	//Use floor based tracking space
	vrapi_SetTrackingSpace(gAppState.Ovr, VRAPI_TRACKING_SPACE_LOCAL_FLOOR);

	int device = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_DEVICE_TYPE);
	switch (device)
    {
        case VRAPI_DEVICE_TYPE_OCULUSQUEST:
            //Force 60hz for Quest 1
            vrapi_SetDisplayRefreshRate(gAppState.Ovr, 60);
            break;
        case VRAPI_DEVICE_TYPE_OCULUSQUEST2:
            vrapi_SetDisplayRefreshRate(gAppState.Ovr, refresh);
            break;
    }

	if (!Doom3Quest_useScreenLayer())
	{
		screenYaw = vr.hmdorientation_temp[YAW];
	}
	pVRClientInfo->right_handed = controlscheme != 0;

    Doom3Quest_processHaptics();
    Doom3Quest_getHMDOrientation();
    Doom3Quest_getTrackedRemotesOrientation(controlscheme, switch_sticks);
}

void Doom3Quest_processHaptics() {//Handle haptics

	float beat;
	bool enable;
	for (int h = 0; h < 2; ++h) {
		beat = fabs( vibration_channel_intensity[h][0] - vibration_channel_intensity[h][1] ) / 65535;
		if (vibration_length[h] > 0) {
			vibration_length[h]--;
		} else if (vibration_length[h] == 0) {
			beat = 0;
		}
		if(beat > 0.0f)
			vrapi_SetHapticVibrationSimple(gAppState.Ovr, controllerIDs[1 - h], beat);
		else
			vrapi_SetHapticVibrationSimple(gAppState.Ovr, controllerIDs[1 - h], 0.0f);
	}
}

void Doom3Quest_getHMDOrientation() {

	//Update the main thread frame index in a thread safe way
	{
		SDL_LockMutex(gAppState.RenderThreadFrameIndex_Mutex);

		gAppState.MainThreadFrameIndex = gAppState.RenderThreadFrameIndex + 1;

		SDL_UnlockMutex(gAppState.RenderThreadFrameIndex_Mutex);
	}

    gAppState.DisplayTime[gAppState.MainThreadFrameIndex % MAX_TRACKING_SAMPLES] = vrapi_GetPredictedDisplayTime(gAppState.Ovr, gAppState.MainThreadFrameIndex);

    ovrTracking2 *tracking = &gAppState.Tracking[gAppState.MainThreadFrameIndex % MAX_TRACKING_SAMPLES];
	*tracking = vrapi_GetPredictedTracking2(gAppState.Ovr, gAppState.DisplayTime[gAppState.MainThreadFrameIndex % MAX_TRACKING_SAMPLES]);

	//Don't update game with tracking if we are in big screen mode
    //GB Do pass the stuff but block at my end (if big screen prompt is needed)
    const ovrQuatf quatHmd = tracking->HeadPose.Pose.Orientation;
    const ovrVector3f positionHmd = tracking->HeadPose.Pose.Position;
    //const ovrVector3f translationHmd = tracking->HeadPose.Pose.Translation;
    vec3_t rotation = {0};
    QuatToYawPitchRoll(quatHmd, rotation, vr.hmdorientation_temp);
    setHMDPosition(positionHmd.x, positionHmd.y, positionHmd.z, 0);
    //GB
    setHMDOrientation(quatHmd.x, quatHmd.y, quatHmd.z, quatHmd.w);
    //End GB
    updateHMDOrientation();
}

void Doom3Quest_getTrackedRemotesOrientation(int controlscheme, int switch_sticks) {

    //Get info for tracked remotes
    acquireTrackedRemotesData(gAppState.Ovr, gAppState.DisplayTime[gAppState.MainThreadFrameIndex % MAX_TRACKING_SAMPLES]);

    //Call additional control schemes here
    if (controlscheme == 0) {
	    HandleInput_Default(controlscheme, switch_sticks,
	                        &footTrackedRemoteState_new,
	                        &rightTrackedRemoteState_new, &rightTrackedRemoteState_old,
	                        &rightRemoteTracking_new,
	                        &leftTrackedRemoteState_new, &leftTrackedRemoteState_old,
	                        &leftRemoteTracking_new,
	                        ovrButton_A, ovrButton_B, ovrButton_X, ovrButton_Y);
	} else {
		//Left handed
	    HandleInput_Default(controlscheme, switch_sticks,
	                        &footTrackedRemoteState_new,
	                        &leftTrackedRemoteState_new, &leftTrackedRemoteState_old,
	                        &leftRemoteTracking_new,
	                        &rightTrackedRemoteState_new, &rightTrackedRemoteState_old,
	                        &rightRemoteTracking_new,
	                        ovrButton_X, ovrButton_Y, ovrButton_A, ovrButton_B);
	}
}

void Doom3Quest_submitFrame()
{
    ovrSubmitFrameDescription2 frameDesc = {0};

    long long renderThreadFrameIndex;
	{
		SDL_LockMutex(gAppState.RenderThreadFrameIndex_Mutex);
		renderThreadFrameIndex = gAppState.RenderThreadFrameIndex;
		SDL_UnlockMutex(gAppState.RenderThreadFrameIndex_Mutex);
	}

	if (!Doom3Quest_useScreenLayer()) {

        ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
        layer.HeadPose = gAppState.Tracking[renderThreadFrameIndex % MAX_TRACKING_SAMPLES].HeadPose;
        for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
        {
            ovrFramebuffer * frameBuffer = &gAppState.Renderer.FrameBuffer;
            layer.Textures[eye].ColorSwapChain = frameBuffer->ColorTextureSwapChain;
            layer.Textures[eye].SwapChainIndex = frameBuffer->ReadyTextureSwapChainIndex;

            ovrMatrix4f projectionMatrix;
            projectionMatrix = ovrMatrix4f_CreateProjectionFov(vrFOV, vrFOV,
                                                               0.0f, 0.0f, 0.1f, 0.0f);

            layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&projectionMatrix);

            layer.Textures[eye].TextureRect.x = 0;
            layer.Textures[eye].TextureRect.y = 0;
            layer.Textures[eye].TextureRect.width = 1.0f;
            layer.Textures[eye].TextureRect.height = 1.0f;
        }
        layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

        // Set up the description for this frame.
        const ovrLayerHeader2 *layers[] =
                {
                        &layer.Header
                };

        frameDesc.Flags = 0;
        frameDesc.SwapInterval = gAppState.SwapInterval;
        frameDesc.FrameIndex = renderThreadFrameIndex;
        frameDesc.DisplayTime = gAppState.DisplayTime[renderThreadFrameIndex % MAX_TRACKING_SAMPLES];
        frameDesc.LayerCount = 1;
        frameDesc.Layers = layers;

        // Hand over the eye images to the time warp.
        vrapi_SubmitFrame2(gAppState.Ovr, &frameDesc);

    } else {
        // Set-up the compositor layers for this frame.
        // NOTE: Multiple independent layers are allowed, but they need to be added
        // in a depth consistent order.
        memset( gAppState.Layers, 0, sizeof( ovrLayer_Union2 ) * ovrMaxLayerCount );
        gAppState.LayerCount = 0;

        // Add a simple cylindrical layer
        gAppState.Layers[gAppState.LayerCount++].Cylinder =
                BuildCylinderLayer(&gAppState.Scene.CylinderRenderer,
                                   gAppState.Scene.CylinderWidth, gAppState.Scene.CylinderHeight, &gAppState.Tracking[renderThreadFrameIndex % MAX_TRACKING_SAMPLES], radians(playerYaw) );

        // Compose the layers for this frame.
        const ovrLayerHeader2 * layerHeaders[ovrMaxLayerCount] = { 0 };
        for ( int i = 0; i < gAppState.LayerCount; i++ )
        {
            layerHeaders[i] = &gAppState.Layers[i].Header;
        }

        // Set up the description for this frame.
        frameDesc.Flags = 0;
        frameDesc.SwapInterval = gAppState.SwapInterval;
        frameDesc.FrameIndex = renderThreadFrameIndex;
        frameDesc.DisplayTime = gAppState.DisplayTime[renderThreadFrameIndex % MAX_TRACKING_SAMPLES];
        frameDesc.LayerCount = gAppState.LayerCount;
        frameDesc.Layers = layerHeaders;

        // Hand over the eye images to the time warp.
        vrapi_SubmitFrame2(gAppState.Ovr, &frameDesc);
    }

	{
		SDL_LockMutex(gAppState.RenderThreadFrameIndex_Mutex);
		gAppState.RenderThreadFrameIndex++;
		SDL_UnlockMutex(gAppState.RenderThreadFrameIndex_Mutex);
	}

	Doom3Quest_HapticEndFrame();
}

/*
================================================================================

Activity lifecycle

================================================================================
*/

jmethodID android_shutdown;
jmethodID android_haptic_event;
jmethodID android_haptic_updateevent;
jmethodID android_haptic_stopevent;
jmethodID android_haptic_endframe;
jmethodID android_haptic_enable;
jmethodID android_haptic_disable;
static jobject jniCallbackObj=0;

void jni_shutdown()
{
    ALOGV("Calling: jni_shutdown");
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }
    return (*env)->CallVoidMethod(env, jniCallbackObj, android_shutdown);
}

void jni_haptic_event(const char* event, int position, int flags, int intensity, float angle, float yHeight)
{
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    jstring StringArg1 = (*env)->NewStringUTF(env, event);

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_event, StringArg1, position, flags, intensity, angle, yHeight);
}

void jni_haptic_updateevent(const char* event, int intensity, float angle)
{
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    jstring StringArg1 = (*env)->NewStringUTF(env, event);

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_updateevent, StringArg1, intensity, angle);
}

void jni_haptic_stopevent(const char* event)
{
    ALOGV("Calling: jni_haptic_stopevent");
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    jstring StringArg1 = (*env)->NewStringUTF(env, event);

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_stopevent, StringArg1);
}

void jni_haptic_endframe()
{
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_endframe);
}

void jni_haptic_enable()
{
    ALOGV("Calling: jni_haptic_enable");
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_enable);
}

void jni_haptic_disable()
{
    ALOGV("Calling: jni_haptic_disable");
    JNIEnv *env;
    jobject tmp;
    if (((*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4))<0)
    {
        (*jVM)->AttachCurrentThread(jVM,&env, NULL);
    }

    return (*env)->CallVoidMethod(env, jniCallbackObj, android_haptic_disable);
}

JNIEXPORT jint JNICALL SDL_JNI_OnLoad(JavaVM* vm, void* reserved);

int JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
    jVM = vm;
	if((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
	{
		ALOGE("Failed JNI_OnLoad");
		return -1;
	}

	return SDL_JNI_OnLoad(vm, reserved);
}

JNIEXPORT void JNICALL Java_com_lvonasek_preyvr_GLES3JNILib_onCreate( JNIEnv * env, jclass activityClass, jobject activity,
																	   jstring commandLineParams, jlong refresh, jfloat ss, jlong msaa)
{
	ALOGV( "    GLES3JNILib::onCreate()" );

	jboolean iscopy;
	const char *arg = (*env)->GetStringUTFChars(env, commandLineParams, &iscopy);


	char *cmdLine = NULL;
	if (arg && strlen(arg))
	{
		cmdLine = strdup(arg);
	}

	(*env)->ReleaseStringUTFChars(env, commandLineParams, arg);

	ALOGV("Command line %s", cmdLine);
	argv = malloc(sizeof(char*) * 255);
	argc = ParseCommandLine(strdup(cmdLine), argv);

	if (ss != -1.0f)
	{
		SS_MULTIPLIER = ss;
	}

	if (msaa != -1)
	{
		NUM_MULTI_SAMPLES = msaa;
	}

	java.ActivityObject = (*env)->NewGlobalRef( env, activity );
}


JNIEXPORT void JNICALL Java_com_lvonasek_preyvr_GLES3JNILib_onStart( JNIEnv * env, jobject obj, jobject obj1)
{
	ALOGV( "    GLES3JNILib::onStart()" );

    jniCallbackObj = (jobject)(*env)->NewGlobalRef(env, obj1);
    jclass callbackClass = (*env)->GetObjectClass(env, jniCallbackObj);

    android_shutdown = (*env)->GetMethodID(env,callbackClass,"shutdown","()V");
	android_haptic_event = (*env)->GetMethodID(env, callbackClass, "haptic_event", "(Ljava/lang/String;IIIFF)V");
	android_haptic_updateevent = (*env)->GetMethodID(env, callbackClass, "haptic_updateevent", "(Ljava/lang/String;IF)V");
	android_haptic_stopevent = (*env)->GetMethodID(env, callbackClass, "haptic_stopevent", "(Ljava/lang/String;)V");
	android_haptic_endframe = (*env)->GetMethodID(env, callbackClass, "haptic_endframe", "()V");
    android_haptic_enable = (*env)->GetMethodID(env, callbackClass, "haptic_enable", "()V");
    android_haptic_disable = (*env)->GetMethodID(env, callbackClass, "haptic_disable", "()V");
}

JNIEXPORT void JNICALL Java_com_lvonasek_preyvr_GLES3JNILib_onResume( JNIEnv * env, jobject obj )
{
	ALOGV( "    GLES3JNILib::onResume()" );
	gAppState.Resumed = true;
}

JNIEXPORT void JNICALL Java_com_lvonasek_preyvr_GLES3JNILib_onPause( JNIEnv * env, jobject obj )
{
	ALOGV( "    GLES3JNILib::onPause()" );
	gAppState.Resumed = false;
}

JNIEXPORT void JNICALL Java_com_lvonasek_preyvr_GLES3JNILib_onDestroy( JNIEnv * env, jobject obj )
{
	ALOGV( "    GLES3JNILib::onDestroy()" );
	gAppState.NativeWindow = NULL;
	destroyed = true;
	shutdown = true;
}

/*
================================================================================

Surface lifecycle

================================================================================
*/

JNIEXPORT void JNICALL Java_com_lvonasek_preyvr_GLES3JNILib_onSurfaceCreated( JNIEnv * env, jobject obj, jobject surface )
{
	ALOGV( "    GLES3JNILib::onSurfaceCreated()" );
	ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
	if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
	{
		// An app that is relaunched after pressing the home button gets an initial surface with
		// the wrong orientation even though android:screenOrientation="landscape" is set in the
		// manifest. The choreographer callback will also never be called for this surface because
		// the surface is immediately replaced with a new surface with the correct orientation.
		ALOGE( "        Surface not in landscape mode!" );
	}

	ALOGV( "        NativeWindow = ANativeWindow_fromSurface( env, surface )" );
	gAppState.NativeWindow = newNativeWindow;

	pthread_t thread = 0;
	const int createErr = pthread_create( &thread, NULL, AppThreadFunction, NULL );
	if ( createErr != 0 )
	{
		ALOGE( "pthread_create returned %i", createErr );
	}
}

JNIEXPORT void JNICALL Java_com_lvonasek_preyvr_GLES3JNILib_onSurfaceChanged( JNIEnv * env, jobject obj, jobject surface )
{
	ALOGV( "    GLES3JNILib::onSurfaceChanged()" );
	ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
	if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
	{
		// An app that is relaunched after pressing the home button gets an initial surface with
		// the wrong orientation even though android:screenOrientation="landscape" is set in the
		// manifest. The choreographer callback will also never be called for this surface because
		// the surface is immediately replaced with a new surface with the correct orientation.
		ALOGE( "        Surface not in landscape mode!" );
	}

	gAppState.NativeWindow = newNativeWindow;
}

JNIEXPORT void JNICALL Java_com_lvonasek_preyvr_GLES3JNILib_setText(JNIEnv *env, jclass clazz, jstring key, jstring value)
{
	jboolean iscopy;
	const char *keyStr = (*env)->GetStringUTFChars(env, key, &iscopy);
	if (pVRClientInfo) {
		if (strcmp(keyStr, "#str_lubos_title") == 0) {
			strcpy(pVRClientInfo->downloaderTitle, (*env)->GetStringUTFChars(env, value, &iscopy));
		} else if (strcmp(keyStr, "#str_lubos_text") == 0) {
			strcpy(pVRClientInfo->downloaderText, (*env)->GetStringUTFChars(env, value, &iscopy));
		} else if (strcmp(keyStr, "#str_lubos_button") == 0) {
			strcpy(pVRClientInfo->downloaderButton, (*env)->GetStringUTFChars(env, value, &iscopy));
		}
	}
}