#include "VrFramebuffer.h"
#include "SDL_opengl.h"


#if !defined(_WIN32)
#include <pthread.h>
#include <GLES3/gl3.h>
#endif

double FromXrTime(const XrTime time) {
	return (time * 1e-9);
}

typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(
		GLenum target,
		GLenum attachment,
		GLuint texture,
		GLint level,
		GLint baseViewIndex,
		GLsizei numViews);

/*
================================================================================

ovrEgl

================================================================================
*/


void ovrEgl_Clear( ovrEgl * egl )
{
	egl->MajorVersion = 0;
	egl->MinorVersion = 0;
	egl->Display = 0;
	egl->Config = 0;
	egl->TinySurface = EGL_NO_SURFACE;
	egl->Context = EGL_NO_CONTEXT;
}

void ovrEgl_CreateContext( ovrEgl * egl, const ovrEgl * shareEgl )
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


void ovrFramebuffer_Clear(ovrFramebuffer* frameBuffer) {
	frameBuffer->Width = 0;
	frameBuffer->Height = 0;
	frameBuffer->TextureSwapChainLength = 0;
	frameBuffer->TextureSwapChainIndex = 0;
	frameBuffer->ColorSwapChain.Handle = XR_NULL_HANDLE;
	frameBuffer->ColorSwapChain.Width = 0;
	frameBuffer->ColorSwapChain.Height = 0;
	frameBuffer->ColorSwapChainImage = NULL;
	frameBuffer->DepthSwapChain.Handle = XR_NULL_HANDLE;
	frameBuffer->DepthSwapChain.Width = 0;
	frameBuffer->DepthSwapChain.Height = 0;
	frameBuffer->DepthSwapChainImage = NULL;
	frameBuffer->FrameBuffers = NULL;
}

bool ovrFramebuffer_Create(
		XrSession session,
		ovrFramebuffer* frameBuffer,
		const int width,
		const int height) {

	frameBuffer->Width = width;
	frameBuffer->Height = height;

	PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR =
			(PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)eglGetProcAddress(
					"glFramebufferTextureMultiviewOVR");

	XrSwapchainCreateInfo swapChainCreateInfo;
	memset(&swapChainCreateInfo, 0, sizeof(swapChainCreateInfo));
	swapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
	swapChainCreateInfo.sampleCount = 1;
	swapChainCreateInfo.width = width;
	swapChainCreateInfo.height = height;
	swapChainCreateInfo.faceCount = 1;
	swapChainCreateInfo.arraySize = 2;
	swapChainCreateInfo.mipCount = 1;

	frameBuffer->ColorSwapChain.Width = swapChainCreateInfo.width;
	frameBuffer->ColorSwapChain.Height = swapChainCreateInfo.height;
	frameBuffer->DepthSwapChain.Width = swapChainCreateInfo.width;
	frameBuffer->DepthSwapChain.Height = swapChainCreateInfo.height;

	// Create the color swapchain.
	swapChainCreateInfo.format = GL_SRGB8_ALPHA8;
	swapChainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
	OXR(xrCreateSwapchain(session, &swapChainCreateInfo, &frameBuffer->ColorSwapChain.Handle));

	// Create the depth swapchain.
	swapChainCreateInfo.format = GL_DEPTH24_STENCIL8;
	swapChainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	OXR(xrCreateSwapchain(session, &swapChainCreateInfo, &frameBuffer->DepthSwapChain.Handle));

	// Get the number of swapchain images.
	OXR(xrEnumerateSwapchainImages(
			frameBuffer->ColorSwapChain.Handle, 0, &frameBuffer->TextureSwapChainLength, NULL));

	// Allocate the swapchain images array.
	frameBuffer->ColorSwapChainImage = (XrSwapchainImageOpenGLESKHR*)malloc(
			frameBuffer->TextureSwapChainLength * sizeof(XrSwapchainImageOpenGLESKHR));
	frameBuffer->DepthSwapChainImage = (XrSwapchainImageOpenGLESKHR*)malloc(
			frameBuffer->TextureSwapChainLength * sizeof(XrSwapchainImageOpenGLESKHR));

	// Populate the swapchain image array.
	for (uint32_t i = 0; i < frameBuffer->TextureSwapChainLength; i++) {
		frameBuffer->ColorSwapChainImage[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
		frameBuffer->ColorSwapChainImage[i].next = NULL;
		frameBuffer->DepthSwapChainImage[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
		frameBuffer->DepthSwapChainImage[i].next = NULL;
	}
	OXR(xrEnumerateSwapchainImages(
			frameBuffer->ColorSwapChain.Handle,
			frameBuffer->TextureSwapChainLength,
			&frameBuffer->TextureSwapChainLength,
			(XrSwapchainImageBaseHeader*)frameBuffer->ColorSwapChainImage));
	OXR(xrEnumerateSwapchainImages(
			frameBuffer->DepthSwapChain.Handle,
			frameBuffer->TextureSwapChainLength,
			&frameBuffer->TextureSwapChainLength,
			(XrSwapchainImageBaseHeader*)frameBuffer->DepthSwapChainImage));

	frameBuffer->FrameBuffers = (GLuint*)malloc(frameBuffer->TextureSwapChainLength * sizeof(GLuint));
	for (uint32_t i = 0; i < frameBuffer->TextureSwapChainLength; i++) {
		// Create the color buffer texture.
		const GLuint colorTexture = frameBuffer->ColorSwapChainImage[i].image;
		const GLuint depthTexture = frameBuffer->DepthSwapChainImage[i].image;

		// Create the frame buffer.
		GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
		GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
		GL(glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, depthTexture, 0, 0, 2));
		GL(glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0, 0, 2));
		GL(glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTexture, 0, 0, 2));
		GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
		GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
		if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
			ALOGE("Incomplete frame buffer object: %d", renderFramebufferStatus);
			return false;
		}
	}

	return true;
}

void ovrFramebuffer_Destroy(ovrFramebuffer* frameBuffer) {
	GL(glDeleteFramebuffers(frameBuffer->TextureSwapChainLength, frameBuffer->FrameBuffers));
	OXR(xrDestroySwapchain(frameBuffer->ColorSwapChain.Handle));
	OXR(xrDestroySwapchain(frameBuffer->DepthSwapChain.Handle));
	free(frameBuffer->ColorSwapChainImage);
	free(frameBuffer->DepthSwapChainImage);
	free(frameBuffer->FrameBuffers);

	ovrFramebuffer_Clear(frameBuffer);
}

void ovrFramebuffer_SetCurrent(ovrFramebuffer* frameBuffer) {
	GL(glBindFramebuffer(
			GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[frameBuffer->TextureSwapChainIndex]));

	//This is a bit of a hack, but we need to do this to correct for the fact that the engine uses linear RGB colorspace
	//but openxr uses SRGB (or something, must admit I don't really understand, but adding this works to make it look good again)
	glDisable( GL_FRAMEBUFFER_SRGB );
}

void ovrFramebuffer_SetNone() {
	GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
}

void ovrFramebuffer_Resolve(ovrFramebuffer* frameBuffer) {
	// Discard the depth buffer, so the tiler won't need to write it back out to memory.
	const GLenum depthAttachment[1] = {GL_DEPTH_ATTACHMENT};
	glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, depthAttachment);
}

void ovrFramebuffer_Acquire(ovrFramebuffer* frameBuffer) {
	// Acquire the swapchain image
	XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, NULL};
	OXR(xrAcquireSwapchainImage(
			frameBuffer->ColorSwapChain.Handle, &acquireInfo, &frameBuffer->TextureSwapChainIndex));

	XrSwapchainImageWaitInfo waitInfo;
	waitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
	waitInfo.next = NULL;
	waitInfo.timeout = 1000; /* timeout in nanoseconds */
	XrResult res = xrWaitSwapchainImage(frameBuffer->ColorSwapChain.Handle, &waitInfo);
	int i = 0;
	while (res != XR_SUCCESS) {
		res = xrWaitSwapchainImage(frameBuffer->ColorSwapChain.Handle, &waitInfo);
		i++;
		ALOGV(
				" Retry xrWaitSwapchainImage %d times due to XR_TIMEOUT_EXPIRED (duration %f micro seconds)",
				i,
				waitInfo.timeout * (1E-9));
	}
}

void ovrFramebuffer_Release(ovrFramebuffer* frameBuffer) {
	XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
	OXR(xrReleaseSwapchainImage(frameBuffer->ColorSwapChain.Handle, &releaseInfo));
}

/*
================================================================================

ovrRenderer

================================================================================
*/

void ovrRenderer_Clear(ovrRenderer* renderer) {
	ovrFramebuffer_Clear(&renderer->FrameBuffer);
}

void ovrRenderer_Create(
		XrSession session,
		ovrRenderer* renderer,
		int suggestedEyeTextureWidth,
		int suggestedEyeTextureHeight) {
	// Create the frame buffers.
	ovrFramebuffer_Create(
			session,
			&renderer->FrameBuffer,
			suggestedEyeTextureWidth,
			suggestedEyeTextureHeight);
}

void ovrRenderer_Destroy(ovrRenderer* renderer) {
	ovrFramebuffer_Destroy(&renderer->FrameBuffer);
}

void ovrRenderer_MouseCursor(ovrRenderer* renderer, int x, int y, int sx, int sy) {
#if XR_USE_GRAPHICS_API_OPENGL_ES || XR_USE_GRAPHICS_API_OPENGL
	GL(glEnable(GL_SCISSOR_TEST));
	GL(glScissor(x, y, sx, sy));
	GL(glViewport(x, y, sx, sy));
	GL(glClearColor(1.0f, 1.0f, 1.0f, 1.0f));
	GL(glClear(GL_COLOR_BUFFER_BIT));
	GL(glDisable(GL_SCISSOR_TEST));
#endif
}

#ifdef ANDROID
void ovrRenderer_SetFoveation(XrInstance* instance, XrSession* session, ovrRenderer* renderer, XrFoveationLevelFB level, float verticalOffset, XrFoveationDynamicFB dynamic) {
	PFN_xrCreateFoveationProfileFB pfnCreateFoveationProfileFB;
	OXR(xrGetInstanceProcAddr(*instance, "xrCreateFoveationProfileFB", (PFN_xrVoidFunction*)(&pfnCreateFoveationProfileFB)));

	PFN_xrDestroyFoveationProfileFB pfnDestroyFoveationProfileFB;
	OXR(xrGetInstanceProcAddr(*instance, "xrDestroyFoveationProfileFB", (PFN_xrVoidFunction*)(&pfnDestroyFoveationProfileFB)));

	PFN_xrUpdateSwapchainFB pfnUpdateSwapchainFB;
	OXR(xrGetInstanceProcAddr(*instance, "xrUpdateSwapchainFB", (PFN_xrVoidFunction*)(&pfnUpdateSwapchainFB)));

	XrFoveationLevelProfileCreateInfoFB levelProfileCreateInfo;
	memset(&levelProfileCreateInfo, 0, sizeof(levelProfileCreateInfo));
	levelProfileCreateInfo.type = XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB;
	levelProfileCreateInfo.level = level;
	levelProfileCreateInfo.verticalOffset = verticalOffset;
	levelProfileCreateInfo.dynamic = dynamic;

	XrFoveationProfileCreateInfoFB profileCreateInfo;
	memset(&profileCreateInfo, 0, sizeof(profileCreateInfo));
	profileCreateInfo.type = XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB;
	profileCreateInfo.next = &levelProfileCreateInfo;

	XrFoveationProfileFB foveationProfile;

	pfnCreateFoveationProfileFB(*session, &profileCreateInfo, &foveationProfile);

	XrSwapchainStateFoveationFB foveationUpdateState;
	memset(&foveationUpdateState, 0, sizeof(foveationUpdateState));
	foveationUpdateState.type = XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB;
	foveationUpdateState.profile = foveationProfile;

	pfnUpdateSwapchainFB(renderer->FrameBuffer.ColorSwapChain.Handle, (XrSwapchainStateBaseHeaderFB*)(&foveationUpdateState));

	pfnDestroyFoveationProfileFB(foveationProfile);
}
#endif

/*
================================================================================

ovrApp

================================================================================
*/

void ovrApp_Clear(ovrApp* app) {
	app->Focused = false;
	app->Instance = XR_NULL_HANDLE;
	app->Session = XR_NULL_HANDLE;
	memset(&app->ViewportConfig, 0, sizeof(XrViewConfigurationProperties));
	memset(&app->ViewConfigurationView, 0, ovrMaxNumEyes * sizeof(XrViewConfigurationView));
	app->SystemId = XR_NULL_SYSTEM_ID;
	app->HeadSpace = XR_NULL_HANDLE;
	app->StageSpace = XR_NULL_HANDLE;
	app->FakeStageSpace = XR_NULL_HANDLE;
	app->CurrentSpace = XR_NULL_HANDLE;
	app->SessionActive = false;
	app->SwapInterval = 1;
	memset(app->Layers, 0, sizeof(ovrCompositorLayer_Union) * ovrMaxLayerCount);
	app->LayerCount = 0;
	app->MainThreadTid = 0;
	app->RenderThreadTid = 0;

	ovrRenderer_Clear(&app->Renderer);
}

void ovrApp_Destroy(ovrApp* app) {
	ovrApp_Clear(app);
}


void ovrApp_HandleSessionStateChanges(ovrApp* app, XrSessionState state) {
	if (state == XR_SESSION_STATE_READY) {
		XrSessionBeginInfo sessionBeginInfo;
		memset(&sessionBeginInfo, 0, sizeof(sessionBeginInfo));
		sessionBeginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
		sessionBeginInfo.next = NULL;
		sessionBeginInfo.primaryViewConfigurationType = app->ViewportConfig.viewConfigurationType;

		XrResult result;
		OXR(result = xrBeginSession(app->Session, &sessionBeginInfo));
		app->SessionActive = (result == XR_SUCCESS);
	} else if (state == XR_SESSION_STATE_STOPPING) {
		OXR(xrEndSession(app->Session));
		app->SessionActive = false;
	}
}

int ovrApp_HandleXrEvents(ovrApp* app) {
	XrEventDataBuffer eventDataBuffer = {};
	int recenter = 0;

	// Poll for events
	for (;;) {
		XrEventDataBaseHeader* baseEventHeader = (XrEventDataBaseHeader*)(&eventDataBuffer);
		baseEventHeader->type = XR_TYPE_EVENT_DATA_BUFFER;
		baseEventHeader->next = NULL;
		XrResult r;
		OXR(r = xrPollEvent(app->Instance, &eventDataBuffer));
		if (r != XR_SUCCESS) {
			break;
		}

		switch (baseEventHeader->type) {
			case XR_TYPE_EVENT_DATA_EVENTS_LOST:
				ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_EVENTS_LOST event");
				break;
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				const XrEventDataInstanceLossPending* instance_loss_pending_event =
						(XrEventDataInstanceLossPending*)(baseEventHeader);
				ALOGV(
						"xrPollEvent: received XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING event: time %f",
						FromXrTime(instance_loss_pending_event->lossTime));
			} break;
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
				ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED event");
				break;
			case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT: {
				const XrEventDataPerfSettingsEXT* perf_settings_event =
						(XrEventDataPerfSettingsEXT*)(baseEventHeader);
				ALOGV(
						"xrPollEvent: received XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT event: type %d subdomain %d : level %d -> level %d",
						perf_settings_event->type,
						perf_settings_event->subDomain,
						perf_settings_event->fromLevel,
						perf_settings_event->toLevel);
			} break;
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
				XrEventDataReferenceSpaceChangePending* ref_space_change_event =
						(XrEventDataReferenceSpaceChangePending*)(baseEventHeader);
				ALOGV(
						"xrPollEvent: received XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING event: changed space: %d for session %p at time %f",
						ref_space_change_event->referenceSpaceType,
						(void*)ref_space_change_event->session,
						FromXrTime(ref_space_change_event->changeTime));
				recenter = 1;
			} break;
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				const XrEventDataSessionStateChanged* session_state_changed_event =
						(XrEventDataSessionStateChanged*)(baseEventHeader);
				ALOGV(
						"xrPollEvent: received XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: %d for session %p at time %f",
						session_state_changed_event->state,
						(void*)session_state_changed_event->session,
						FromXrTime(session_state_changed_event->time));

				switch (session_state_changed_event->state) {
					case XR_SESSION_STATE_FOCUSED:
						app->Focused = true;
						break;
					case XR_SESSION_STATE_VISIBLE:
						app->Focused = false;
						break;
					case XR_SESSION_STATE_READY:
					case XR_SESSION_STATE_STOPPING:
						ovrApp_HandleSessionStateChanges(app, session_state_changed_event->state);
						break;
					default:
						break;
				}
			} break;
			default:
				ALOGV("xrPollEvent: Unknown event");
				break;
		}
	}
	return recenter;
}
