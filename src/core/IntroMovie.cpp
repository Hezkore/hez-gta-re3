#include "common.h"
#include "crossplatform.h"
#include "IntroMovie.h"
#include "main.h"
#include "RwHelper.h"
#include "Sprite2d.h"
#include "Pad.h"
#include "ControllerConfig.h"
#include "Frontend.h"

#include <stddef.h>
#include <stdio.h>
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#ifdef AUDIO_OAL
#include <AL/al.h>
#include <AL/alc.h>
#endif

// Follows the SDL player example that comes with pl_mpeg, which decodes by elapsed time
// with plm_decode() and takes the frames and sound in its callbacks

#define LOGO_MOVIE "movies/Logo.mpg"
#define TITLES_MOVIE "movies/GTAtitles.mpg"
#define TITLES_MOVIE_GERMAN "movies/GTAtitlesGER.mpg"

#define PROBE_BYTES (5000 * 1024)

// the longest step the movie takes at once, so a stall in the main loop pauses it instead of skipping ahead
#define MAX_STEP_SECONDS (1.0 / 30.0)
#define MS_PER_SECOND 1000.0

static plm_t *movie;
static CSprite2d frameSprite;
static bool hasNewFrame;
static RwUInt32 lastTime;

#ifdef AUDIO_OAL
// how far the sound is decoded ahead of the picture, as in the example
#define AUDIO_LEAD_SAMPLES 4096
// room for the lead time and the frame playing, with plenty to spare
#define NUM_AUDIO_BUFFERS 16
// pl_mpeg decodes stereo floats
#define AUDIO_CHANNELS 2
#define PCM16_SCALE 32767.0f

static ALCdevice *audioDevice;
static ALCcontext *audioContext;
static ALuint audioSource;
static ALuint audioBuffers[NUM_AUDIO_BUFFERS];
static ALuint freeBuffers[NUM_AUDIO_BUFFERS];
static int numFreeBuffers;

static bool
OpenAudio(void)
{
	audioDevice = alcOpenDevice(nil);
	if(audioDevice == nil)
		return false;
	audioContext = alcCreateContext(audioDevice, nil);
	if(audioContext == nil || !alcMakeContextCurrent(audioContext))
		return false;

	alGenSources(1, &audioSource);
	alGenBuffers(NUM_AUDIO_BUFFERS, audioBuffers);
	for(int i = 0; i < NUM_AUDIO_BUFFERS; i++)
		freeBuffers[i] = audioBuffers[i];
	numFreeBuffers = NUM_AUDIO_BUFFERS;
	return alGetError() == AL_NO_ERROR;
}

static void
CloseAudio(void)
{
	if(audioContext){
		alSourceStop(audioSource);
		alDeleteSources(1, &audioSource);
		alDeleteBuffers(NUM_AUDIO_BUFFERS, audioBuffers);
		alcMakeContextCurrent(nil);
		alcDestroyContext(audioContext);
		audioContext = nil;
	}
	if(audioDevice){
		alcCloseDevice(audioDevice);
		audioDevice = nil;
	}
}

static void
ReclaimPlayedBuffers(void)
{
	ALint processed = 0;
	alGetSourcei(audioSource, AL_BUFFERS_PROCESSED, &processed);
	while(processed-- > 0)
		alSourceUnqueueBuffers(audioSource, 1, &freeBuffers[numFreeBuffers++]);
}

static void
OnAudio(plm_t *plm, plm_samples_t *samples, void *user)
{
	static int16 pcm[PLM_AUDIO_SAMPLES_PER_FRAME * AUDIO_CHANNELS];

	ReclaimPlayedBuffers();
	if(numFreeBuffers == 0){
		debug("Intro movie sound dropped a frame, no free buffer\n");
		return;
	}

	for(int i = 0; i < samples->count * AUDIO_CHANNELS; i++)
		pcm[i] = (int16)(Clamp(samples->interleaved[i], -1.0f, 1.0f) * PCM16_SCALE);
	ALuint buffer = freeBuffers[--numFreeBuffers];
	alBufferData(buffer, AL_FORMAT_STEREO16, pcm, samples->count * AUDIO_CHANNELS * sizeof(int16), plm_get_samplerate(plm));
	alSourceQueueBuffers(audioSource, 1, &buffer);

	// the source stops when its queue runs dry, such as while the window has lost focus
	ALint state;
	alGetSourcei(audioSource, AL_SOURCE_STATE, &state);
	if(state != AL_PLAYING)
		alSourcePlay(audioSource);
}

static void
StartAudio(const char *path)
{
	if(plm_get_num_audio_streams(movie) == 0){
		plm_set_audio_enabled(movie, FALSE);
		return;
	}
	if(!OpenAudio()){
		debug("Intro movie %s plays without sound, no audio device\n", path);
		CloseAudio();
		plm_set_audio_enabled(movie, FALSE);
		return;
	}
	plm_set_audio_decode_callback(movie, OnAudio, nil);
	plm_set_audio_enabled(movie, TRUE);
	plm_set_audio_stream(movie, 0);
	plm_set_audio_lead_time(movie, (double)AUDIO_LEAD_SAMPLES / plm_get_samplerate(movie));
}
#else
static void
StartAudio(const char *path)
{
	plm_set_audio_enabled(movie, FALSE);
}
#endif

static void
OnVideo(plm_t *plm, plm_frame_t *frame, void *user)
{
	RwRaster *raster = RwTextureGetRaster(frameSprite.m_pTexture);
	RwUInt8 *pixels = RwRasterLock(raster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH);
	// the raster has no alpha, so the byte pl_mpeg leaves alone on D3D9 is never read
#ifdef RW_GL3
	// librw keeps GL textures bottom up, the way it flips the D3D ones it converts
	plm_frame_to_rgb(frame, pixels + (raster->height - 1) * raster->stride, -raster->stride);
#else
	plm_frame_to_bgra(frame, pixels, raster->stride);
#endif
	RwRasterUnlock(raster);
	hasNewFrame = true;
}

static bool
Open(const char *path)
{
	FILE *file = fcaseopen(path, "rb");
	if(file == nil){
		debug("Intro movie %s not found\n", path);
		return false;
	}
	movie = plm_create_with_file(file, TRUE);
	if(movie == nil){
		fclose(file);
		return false;
	}
	if(!plm_probe(movie, PROBE_BYTES)){
		debug("Intro movie %s has no MPEG streams\n", path);
		CIntroMovie::Close();
		return false;
	}

	RwRaster *raster = RwRasterCreate(plm_get_width(movie), plm_get_height(movie), 0, rwRASTERTYPETEXTURE | rwRASTERFORMAT888);
	frameSprite.m_pTexture = RwTextureCreate(raster);
	RwTextureSetFilterMode(frameSprite.m_pTexture, rwFILTERLINEAR);
	plm_set_video_decode_callback(movie, OnVideo, nil);
	StartAudio(path);

	lastTime = RsTimer();
	debug("Intro movie %s playing\n", path);
	return true;
}

void
CIntroMovie::Close(void)
{
	if(movie == nil)
		return;
#ifdef AUDIO_OAL
	CloseAudio();
#endif
	frameSprite.Delete();
	plm_destroy(movie);
	movie = nil;
	debug("Intro movie closed\n");
}

// The movie keeps its shape, with black bars at the sides or top and bottom
static CRect
FrameRect(void)
{
	float scale = Min(SCREEN_WIDTH / plm_get_width(movie), SCREEN_HEIGHT / plm_get_height(movie));
	float width = plm_get_width(movie) * scale;
	float height = plm_get_height(movie) * scale;
	float left = (SCREEN_WIDTH - width) / 2;
	float top = (SCREEN_HEIGHT - height) / 2;
	return CRect(left, top, left + width, top + height);
}

static void
DrawFrame(void)
{
	if(!DoRWStuffStartOfFrame(0, 0, 0, 0, 0, 0, 255))
		return;
	CSprite2d::SetRecipNearClip();
	CSprite2d::InitPerFrame();
	DefinedState();
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
	frameSprite.Draw(FrameRect(), CRGBA(255, 255, 255, 255));
	DoRWStuffEndOfFrame();
}

// Decodes what is due and draws it, and is false once the movie is over or was never opened
static bool
Update(void)
{
	if(movie == nil)
		return false;

	RwUInt32 now = RsTimer();
	double elapsed = Min((now - lastTime) / MS_PER_SECOND, MAX_STEP_SECONDS);
	lastTime = now;
	plm_decode(movie, elapsed);
	// a frame is drawn only once, so the loop does not keep the GPU busy between frames
	if(hasNewFrame){
		hasNewFrame = false;
		DrawFrame();
	}
	return !plm_has_ended(movie);
}

static bool
IsSkipPressed(void)
{
	CPad *pad = CPad::GetPad(0);
	return ControlsManager.GetJoyButtonJustDown() != 0
		|| pad->GetLeftMouseJustDown()
		|| pad->GetEnterJustDown()
		|| pad->GetCharJustDown(' ')
		|| pad->GetAltJustDown()
		|| pad->GetTabJustDown()
		|| (pad->NewState.CheckForInput() && !pad->OldState.CheckForInput());
}

static const char *
TitlesMovie(void)
{
	if(CMenuManager::OS_Language == LANG_FRENCH || CMenuManager::OS_Language == LANG_GERMAN)
		return TITLES_MOVIE_GERMAN;
	return TITLES_MOVIE;
}

RwUInt32
CIntroMovie::FirstState(void)
{
#ifdef NO_MOVIES
	if(!IntroMoviesWanted())
		return GS_INIT_ONCE;
#endif
	return GS_INIT_LOGO_MPEG;
}

// Runs the movie states, from the logo through the titles to GS_INIT_ONCE
void
CIntroMovie::RunState(void)
{
	switch(gGameState){
	case GS_INIT_LOGO_MPEG:
		Open(LOGO_MOVIE);
		gGameState = GS_LOGO_MPEG;
		break;
	case GS_INIT_INTRO_MPEG:
		Close();
		Open(TitlesMovie());
		gGameState = GS_INTRO_MPEG;
		break;
	case GS_LOGO_MPEG:
	case GS_INTRO_MPEG:
		CPad::UpdatePads();
		if(!Update() || IsSkipPressed())
			gGameState++;
		if(gGameState == GS_INIT_ONCE)
			Close();
		break;
	}
}
