#pragma once

#ifdef EXTENDED_COLOURFILTER

class CPostFX
{
public:
	enum {
		MBLUR_OFF,
		MBLUR_FAINT,
		MBLUR_STRONG
	};
	enum {
		DROPLETS_OFF,
		DROPLETS_XBOX,	// the round Neo lens drops
		DROPLETS_PC	// the PC water and blood drops
	};
	enum {
		POSTFX_OFF,
		POSTFX_SIMPLE,
		POSTFX_NORMAL,
		POSTFX_MOBILE
	};
	static RwRaster *pFrontBuffer;
	static RwRaster *pBackBuffer;
	static bool bJustInitialised;
	static int EffectSwitch;
	static bool bTrails;	// the PC game's trails: the last frame again, a little down and to the right
	static int8 MotionBlur;	// MBLUR_OFF, MBLUR_FAINT or MBLUR_STRONG
	static float Intensity;
	static int8 Droplets;	// DROPLETS_OFF, DROPLETS_XBOX or DROPLETS_PC
	static const int8 DefaultDroplets = DROPLETS_PC;

	// smooth blur color
	enum { NUMAVERAGE = 20 };
	static int PrevRed[NUMAVERAGE], AvgRed;
	static int PrevGreen[NUMAVERAGE], AvgGreen;
	static int PrevBlue[NUMAVERAGE], AvgBlue;
	static int PrevAlpha[NUMAVERAGE], AvgAlpha;
	static int Next;
	static int NumValues;

	static void InitOnce(void);
	static void Open(RwCamera *cam);
	static void Close(void);
	static void RenderOverlayBlur(RwCamera *cam, int32 r, int32 g, int32 b, int32 a);
	static void RenderPlainTrail(int32 r, int32 g, int32 b);
	static void RenderOverlaySniper(RwCamera *cam, int32 r, int32 g, int32 b, int32 a);
	static void RenderOverlayShader(RwCamera *cam, int32 r, int32 g, int32 b, int32 a);
	static void RenderMotionBlur(RwCamera *cam, uint32 blur);
	static void Render(RwCamera *cam, uint32 red, uint32 green, uint32 blue, uint32 blur, int32 type, uint32 bluralpha);
	static void SmoothColor(uint32 red, uint32 green, uint32 blue, uint32 alpha);
	static bool NeedBackBuffer(void);
	static void GetBackBuffer(RwCamera *cam);
	static bool UseBlurColours(void) { return EffectSwitch != POSTFX_SIMPLE; }
	// the trails are the motion blur's last frame drawn again a little moved, so only the normal filter with motion blur can show them
	static bool AreTrailsPossible(void) { return EffectSwitch == POSTFX_NORMAL && MotionBlur != MBLUR_OFF; }
	static bool AreTrailsDrawn(void) { return bTrails && AreTrailsPossible(); }
};

#endif
