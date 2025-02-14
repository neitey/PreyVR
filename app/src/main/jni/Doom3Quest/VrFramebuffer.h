#pragma once

#include "VrBase.h"

void ovrApp_Clear(ovrApp* app);
void ovrApp_Destroy(ovrApp* app);
int ovrApp_HandleXrEvents(ovrApp* app);

void ovrEgl_Clear( ovrEgl * egl );
void ovrEgl_CreateContext( ovrEgl * egl, const ovrEgl * shareEgl );

void ovrFramebuffer_Acquire(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_Resolve(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_Release(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_SetCurrent(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_SetNone();

void ovrRenderer_Create(XrSession session, ovrRenderer* renderer, int width, int height);
void ovrRenderer_Destroy(ovrRenderer* renderer);
void ovrRenderer_MouseCursor(ovrRenderer* renderer, int x, int y, int sx, int sy);
#ifdef ANDROID
void ovrRenderer_SetFoveation(XrInstance* instance, XrSession* session, ovrRenderer* renderer, XrFoveationLevelFB level, float verticalOffset, XrFoveationDynamicFB dynamic);
#endif
