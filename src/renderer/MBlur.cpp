#ifndef LIBRW
#define WITHD3D
#endif
#include "common.h"
#ifndef LIBRW
#include <d3d8caps.h>
#endif

#include "main.h"
#include "RwHelper.h"
#include "Timer.h"
#include "Camera.h"
#include "MBlur.h"
#include "postfx.h"

// Originally taken from RW example 'mblur'

RwRaster *CMBlur::pFrontBuffer;
bool CMBlur::ms_bJustInitialised;
bool CMBlur::ms_bScaledBlur;
bool CMBlur::BlurOn;

static RwIm2DVertex Vertex[4];
static RwImVertexIndex Index[6] = { 0, 1, 2, 0, 2, 3 };

#ifndef LIBRW
extern "C" D3DCAPS8 _RwD3D8DeviceCaps;
#endif
RwBool
CMBlur::MotionBlurOpen(RwCamera *cam)
{
#ifdef EXTENDED_COLOURFILTER
	CPostFX::Open(cam);
	return TRUE;
#else
#ifdef GTA_PS2
	RwRect rect = {0, 0, 0, 0};
	
	if (pFrontBuffer)
		return TRUE;
	
	BlurOn = true;
	
	rect.w = RwRasterGetWidth(RwCameraGetRaster(cam));
	rect.h = RwRasterGetHeight(RwCameraGetRaster(cam));
	
	pFrontBuffer = RwRasterCreate(0, 0, 0, rwRASTERDONTALLOCATE|rwRASTERTYPECAMERATEXTURE);
	if (!pFrontBuffer)
	{
		printf("Error creating raster\n");
		return FALSE;
	}
	
	RwRaster *raster = RwRasterSubRaster(pFrontBuffer, RwCameraGetRaster(cam), &rect);
	if (!raster)
	{
		RwRasterDestroy(pFrontBuffer);
		pFrontBuffer = NULL;
		printf("Error subrastering\n");
		return FALSE;
	}
	
	CreateImmediateModeData(cam, &rect);
#else
	RwRect rect = { 0, 0, 0, 0 };

	if(pFrontBuffer)
		MotionBlurClose();
	
#ifndef LIBRW
	extern void _GetVideoMemInfo(LPDWORD total, LPDWORD avaible);
	DWORD total, avaible;
	
	_GetVideoMemInfo(&total, &avaible);
	debug("Available video memory %d\n", avaible);
#endif
		
	if(BlurOn)
	{
		uint32 width  = Pow(2.0f, int32(log2(RwRasterGetWidth (RwCameraGetRaster(cam))))+1);
		uint32 height = Pow(2.0f, int32(log2(RwRasterGetHeight(RwCameraGetRaster(cam))))+1);
		uint32 depth  = RwRasterGetDepth(RwCameraGetRaster(cam));
		
#ifndef LIBRW
		extern DWORD _dwMemTotalVideo;
		if ( _RwD3D8DeviceCaps.MaxTextureWidth >= width && _RwD3D8DeviceCaps.MaxTextureHeight >= height )
		{
			total = _dwMemTotalVideo - 3 *
				( RwRasterGetDepth(RwCameraGetRaster(cam))
				* RwRasterGetHeight(RwCameraGetRaster(cam))
				* RwRasterGetWidth(RwCameraGetRaster(cam)) / 8 );
			BlurOn = total >= height*width*(depth/8) + (12*1024*1024) /*12 MB*/;
		}
		else
			BlurOn = false;
#endif
		
		if ( BlurOn )
		{
			ms_bScaledBlur = false;
			rect.w = width;
			rect.h = height;
			
			pFrontBuffer = RwRasterCreate(rect.w, rect.h, depth, rwRASTERTYPECAMERATEXTURE);
			if ( !pFrontBuffer )
			{
				debug("MBlurOpen can't create raster.");
				BlurOn = false;
				rect.w = RwRasterGetWidth(RwCameraGetRaster(cam));
				rect.h = RwRasterGetHeight(RwCameraGetRaster(cam));
			}
			else
				ms_bJustInitialised = true;
		}
		else
		{
			rect.w = RwRasterGetWidth(RwCameraGetRaster(cam));
			rect.h = RwRasterGetHeight(RwCameraGetRaster(cam));
		}
		
#ifndef LIBRW
		_GetVideoMemInfo(&total, &avaible);
		debug("Available video memory %d\n", avaible);
#endif
		CreateImmediateModeData(cam, &rect);
	}
	else
	{
		rect.w = RwRasterGetWidth(RwCameraGetRaster(cam));
		rect.h = RwRasterGetHeight(RwCameraGetRaster(cam));
		CreateImmediateModeData(cam, &rect);
	}
	
	return TRUE;
#endif
#endif
}

RwBool
CMBlur::MotionBlurClose(void)
{
#ifdef EXTENDED_COLOURFILTER
	CPostFX::Close();
#else
	if(pFrontBuffer){
		RwRasterDestroy(pFrontBuffer);
		pFrontBuffer = nil;
		
		return TRUE;
	}
#endif
	return FALSE;
}

void
CMBlur::CreateImmediateModeData(RwCamera *cam, RwRect *rect)
{
	float zero, xmax, ymax;

	if(RwRasterGetDepth(RwCameraGetRaster(cam)) == 16){
		zero = HALFPX;
		xmax = rect->w + HALFPX;
		ymax = rect->h + HALFPX;
	}else{
		zero = -HALFPX;
		xmax = rect->w - HALFPX;
		ymax = rect->h - HALFPX;
	}

	RwIm2DVertexSetScreenX(&Vertex[0], zero);
	RwIm2DVertexSetScreenY(&Vertex[0], zero);
	RwIm2DVertexSetScreenZ(&Vertex[0], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex[0], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex[0], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex[0], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex[0], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex[0], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&Vertex[1], zero);
	RwIm2DVertexSetScreenY(&Vertex[1], ymax);
	RwIm2DVertexSetScreenZ(&Vertex[1], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex[1], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex[1], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex[1], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex[1], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex[1], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&Vertex[2], xmax);
	RwIm2DVertexSetScreenY(&Vertex[2], ymax);
	RwIm2DVertexSetScreenZ(&Vertex[2], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex[2], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex[2], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex[2], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex[2], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex[2], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&Vertex[3], xmax);
	RwIm2DVertexSetScreenY(&Vertex[3], zero);
	RwIm2DVertexSetScreenZ(&Vertex[3], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex[3], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex[3], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex[3], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex[3], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex[3], 255, 255, 255, 255);
}

void
CMBlur::MotionBlurRender(RwCamera *cam, uint32 red, uint32 green, uint32 blue, uint32 blur, int32 type, uint32 bluralpha)
{
#ifdef EXTENDED_COLOURFILTER
	CPostFX::Render(cam, red, green, blue, blur, type, bluralpha);
#else
	PUSH_RENDERGROUP("CMBlur::MotionBlurRender");
	UpdateTrailStep();
	RwRGBA color = { (RwUInt8)red, (RwUInt8)green, (RwUInt8)blue, (RwUInt8)blur };
#ifdef GTA_PS2
	if( pFrontBuffer )
		OverlayRender(cam, pFrontBuffer, color, type, bluralpha);
#else
	if(BlurOn){
		if(pFrontBuffer){
			if(ms_bJustInitialised)
				ms_bJustInitialised = false;
			else
				OverlayRender(cam, pFrontBuffer, color, type, bluralpha);
		}
		if(IsTrailStepDone()){
			RwRasterPushContext(pFrontBuffer);
			RwRasterRenderFast(RwCameraGetRaster(cam), 0, 0);
			RwRasterPopContext();
		}
	}else{
		OverlayRender(cam, nil, color, type, bluralpha);
	}
#endif
	POP_RENDERGROUP();
#endif
}

// The trail is drawn on each rendered frame but only accumulates, that is the frame is only
// copied into the ghost buffer, TRAIL_STEP_RATE times a second. Any faster and a step keeps
// so much of the ghost and adds so little of the new frame that 8 bit colour rounds the
// difference away, the picture goes dark and the colours come loose. At this rate the
// ghost still refreshes far too often to see it step.
#define TRAIL_STEP_RATE 60

static float trailLength = 1.0f;	// length of the current trail step in logical frames
static bool trailStepDone;

// Called once per rendered frame before the trail is drawn
void
CMBlur::UpdateTrailStep(void)
{
	static float msInStep;

	msInStep += CTimer::GetRenderFrameLength()*LOGICAL_FRAME_MS;
	trailStepDone = msInStep >= 1000.0f/TRAIL_STEP_RATE;
	if(trailStepDone){
		trailLength = msInStep/LOGICAL_FRAME_MS;
		msInStep = 0.0f;
	}
}

// Whether this frame ends a trail step and has to be copied into the ghost buffer
bool
CMBlur::IsTrailStepDone(void)
{
	return trailStepDone;
}

// The previous frame is blended over the new one. Per logical frame that is, for each
// channel, F = keep*G + pass*C + white, G the previous frame and C the new one. A trail
// step covers only a part of a logical frame, so it gets that map to the power of the
// part: G keeps keep^l, C and the white get (1-keep^l)/(1-keep) of theirs. That way the
// trail fades at the same rate at any frame rate instead of per rendered frame, and it
// is smooth. A single quad cannot scale each channel of C on its own, so it takes
// three: darken C, add G, add the white. strength shortens the trail: the step is taken
// as 1/strength as long, so the previous frames fade out that much sooner while the
// colour the overlay gives the picture is kept.
void
CMBlur::RenderTrail(RwIm2DVertex *quad, RwRaster *ghost, const float *keep, const float *pass, const float *white, float strength)
{
	float length = trailLength/strength;
	int32 k[3], p[3], w[3];
	int i;

	for(i = 0; i < 3; i++){
		float kept = Pow(keep[i], length);
		float part = keep[i] < 1.0f ? (1.0f - kept)/(1.0f - keep[i]) : length;
		k[i] = Min(kept*255.0f, 255.0f);
		p[i] = Min(pass[i]*part*255.0f, 255.0f);
		w[i] = Min(white[i]*part*255.0f, 255.0f);
	}

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
	// the ghost pass adds colour whatever its alpha is, so no alpha test on it
	SetAlphaTest(0);

	// what the new frame keeps
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
	for(i = 0; i < 4; i++)
		RwIm2DVertexSetIntRGBA(&quad[i], p[0], p[1], p[2], 255);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, quad, 4, Index, 6);

	// plus what is left of the previous one
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, ghost);
	for(i = 0; i < 4; i++)
		RwIm2DVertexSetIntRGBA(&quad[i], k[0], k[1], k[2], 255);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, quad, 4, Index, 6);

	// plus the white
	if(w[0] > 0 || w[1] > 0 || w[2] > 0){
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
		for(i = 0; i < 4; i++)
			RwIm2DVertexSetIntRGBA(&quad[i], w[0], w[1], w[2], 255);
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, quad, 4, Index, 6);
	}

	RestoreAlphaTest();
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}

void
CMBlur::OverlayRender(RwCamera *cam, RwRaster *raster, RwRGBA color, int32 type, int32 bluralpha)
{
	int r, g, b, a;

	r = color.red;
	g = color.green;
	b = color.blue;
	a = color.alpha;

	DefinedState();

	switch(type)
	{
	case MOTION_BLUR_SECURITY_CAM:
		r = 0;
		g = 255;
		b = 0;
		a = 128;
		break;
	case MOTION_BLUR_INTRO:
		r = 100;
		g = 220;
		b = 230;
		a = 158;
		break;
	case MOTION_BLUR_INTRO2:
		r = 80;
		g = 255;
		b = 230;
		a = 138;
		break;
	case MOTION_BLUR_INTRO3:
		r = 255;
		g = 60;
		b = 60;
		a = 200;
		break;
	case MOTION_BLUR_INTRO4:
		r = 255;
		g = 180;
		b = 180;
		a = 128;
		break;
	}

	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);

	if(!BlurOn){
		r = Min(r*0.6f, 255.0f);
		g = Min(g*0.6f, 255.0f);
		b = Min(b*0.6f, 255.0f);
		if(type != MOTION_BLUR_SNIPER)
			a = Min(a*0.6f, 255.0f);
		// game clamps to 255 here, but why?
		RwIm2DVertexSetIntRGBA(&Vertex[0], r, g, b, a);
		RwIm2DVertexSetIntRGBA(&Vertex[1], r, g, b, a);
		RwIm2DVertexSetIntRGBA(&Vertex[2], r, g, b, a);
		RwIm2DVertexSetIntRGBA(&Vertex[3], r, g, b, a);

		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, Vertex, 4, Index, 6);
	}else{
		// the previous frame, tinted, with alpha a over the new one and then some white
		// on top, see RenderTrail
		int32 whiteAlpha = bluralpha/2;
		if(whiteAlpha < 30)
			whiteAlpha = 30;
		float alpha = a/255.0f;
		float lit = 1.0f - whiteAlpha/255.0f;
		float keep[3] = { lit*alpha*r/255.0f, lit*alpha*g/255.0f, lit*alpha*b/255.0f };
		float pass[3] = { lit*(1.0f - alpha), lit*(1.0f - alpha), lit*(1.0f - alpha) };
		float white[3] = { 1.0f - lit, 1.0f - lit, 1.0f - lit };
		RenderTrail(Vertex, raster, keep, pass, white, 1.0f);
	}

	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}
