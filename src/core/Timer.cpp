#define WITHWINDOWS
#include "common.h"
#include "crossplatform.h"

#include "DMAudio.h"
#include "Record.h"
#include "SpecialFX.h"
#include "Timer.h"

uint32 CTimer::m_snTimeInMilliseconds;
uint32 CTimer::m_snTimeInMillisecondsPauseMode = 1;
uint32 CTimer::m_snTimeInMillisecondsNonClipped;
uint32 CTimer::m_snPreviousTimeInMilliseconds;
uint32 CTimer::m_FrameCounter;
float CTimer::ms_fTimeScale;
float CTimer::ms_fTimeStep;
float CTimer::ms_fTimeStepNonClipped;
bool  CTimer::m_UserPause;
bool  CTimer::m_CodePause;
uint32 CTimer::m_LogicalFramesPassed;
float CTimer::ms_fLogicalFrameFraction;
float CTimer::ms_fRenderFrameLength;
float CTimer::ms_fRenderTimeStep;
float CTimer::ms_fRenderTimeStepNonClipped;

uint32 _nCyclesPerMS = 1;

#ifdef _WIN32
LARGE_INTEGER _oldPerfCounter;
LARGE_INTEGER perfSuspendCounter;
#define RsTimerType uint32
#else
#define RsTimerType double
#endif

RsTimerType oldPcTimer;

RsTimerType suspendPcTimer;

uint32 suspendDepth;

// time from Update() that no logical frame has used yet
static double logicalFrameTime;
// the game clock counts milliseconds, these keep the rest for the next logical frame
static double pauseModeFraction;
static double gameTimeFraction;
// what SetTimeStepForRender() added to the game clock for the rendered frame
static uint32 renderTimeOffset;
static uint32 renderPauseTimeOffset;

void CTimer::Initialise(void)
{
	debug("Initialising CTimer...\n");

	ms_fTimeScale = 1.0f;
	ms_fTimeStep = 1.0f;
	ms_fTimeStepNonClipped = 1.0f;
	ms_fRenderTimeStep = 1.0f;
	ms_fRenderTimeStepNonClipped = 1.0f;
	ms_fLogicalFrameFraction = 0.0f;
	ms_fRenderFrameLength = 1.0f;
	m_LogicalFramesPassed = 0;
	logicalFrameTime = 0.0;
	pauseModeFraction = 0.0;
	gameTimeFraction = 0.0;
	suspendDepth = 0;
	m_UserPause = false;
	m_CodePause = false;
	m_snTimeInMillisecondsNonClipped = 0;
	m_snPreviousTimeInMilliseconds = 0;
	m_snTimeInMilliseconds = 1;

#ifdef _WIN32
	LARGE_INTEGER perfFreq;
	if ( QueryPerformanceFrequency(&perfFreq) )
	{
		OutputDebugString("Performance counter available\n");
		_nCyclesPerMS = uint32(perfFreq.QuadPart / 1000);
		QueryPerformanceCounter(&_oldPerfCounter);
	}
	else
#endif
	{
		OutputDebugString("Performance counter not available, using millesecond timer\n");
		_nCyclesPerMS = 0;
		oldPcTimer = RsTimer();
	}

	m_snTimeInMilliseconds = m_snPreviousTimeInMilliseconds;

	m_FrameCounter = 0;

	DMAudio.ResetTimers(m_snPreviousTimeInMilliseconds);

	debug("CTimer ready\n");
}

void CTimer::Shutdown(void)
{
	;
}

// Called once per rendered frame. Measures the time since the last call, works out how
// many logical frames that is and what time step the rendered frame gets. The game clock
// itself is not touched here, that happens in UpdateLogicalFrame().
void CTimer::Update(void)
{
	double frameTime;
	double updInMs;

#ifdef _WIN32
	if ( (double)_nCyclesPerMS != 0.0 )
	{
		LARGE_INTEGER pc;
		QueryPerformanceCounter(&pc);

		int32 updInCycles = (pc.LowPart - _oldPerfCounter.LowPart); // & 0x7FFFFFFF; pointless

		_oldPerfCounter = pc;

		updInMs = (double)updInCycles / (double)_nCyclesPerMS;
	}
	else
#endif
	{
		RsTimerType timer = RsTimer();

		updInMs = (double)(timer - oldPcTimer);

		oldPcTimer = timer;
	}

	// bugfix from VC
	frameTime = GetIsPaused() ? updInMs : updInMs * ms_fTimeScale;

	m_LogicalFramesPassed = 0;
	logicalFrameTime += updInMs;
	while(logicalFrameTime >= LOGICAL_FRAME_MS){
		logicalFrameTime -= LOGICAL_FRAME_MS;
		m_LogicalFramesPassed++;
	}
	if(m_LogicalFramesPassed > MAX_LOGICAL_FRAMES_PER_UPDATE){
		m_LogicalFramesPassed = MAX_LOGICAL_FRAMES_PER_UPDATE;
		logicalFrameTime = 0.0;
	}
	ms_fLogicalFrameFraction = (float)(logicalFrameTime / LOGICAL_FRAME_MS);
	ms_fRenderFrameLength = (float)(updInMs / LOGICAL_FRAME_MS);

	if ( GetIsPaused() )
		ms_fRenderTimeStep = 0.0f;
	else
		ms_fRenderTimeStep = frameTime / 1000.0f * 50.0f;

	if ( ms_fRenderTimeStep < 0.01f && !GetIsPaused() && !CSpecialFX::bSnapShotActive )
		ms_fRenderTimeStep = 0.01f;

	ms_fRenderTimeStepNonClipped = ms_fRenderTimeStep;

	if ( !CRecordDataForGame::IsPlayingBack() )
		ms_fRenderTimeStep = Min(3.0f, ms_fRenderTimeStep);
}

// Called once per logical frame. Moves the game clock on by one logical frame,
// so the game gets the same clock it got when it rendered at the logical frame rate.
void CTimer::UpdateLogicalFrame(void)
{
	double frameTime;

	m_snTimeInMilliseconds -= renderTimeOffset;
	m_snTimeInMillisecondsNonClipped -= renderTimeOffset;
	m_snTimeInMillisecondsPauseMode -= renderPauseTimeOffset;
	renderTimeOffset = 0;
	renderPauseTimeOffset = 0;

	m_snPreviousTimeInMilliseconds = m_snTimeInMilliseconds;

	pauseModeFraction += LOGICAL_FRAME_MS;
	m_snTimeInMillisecondsPauseMode += uint32(pauseModeFraction);
	pauseModeFraction -= uint32(pauseModeFraction);

	if ( GetIsPaused() )
		ms_fTimeStep = 0.0f;
	else
	{
		frameTime = LOGICAL_FRAME_MS * ms_fTimeScale;
		gameTimeFraction += frameTime;
		m_snTimeInMilliseconds += uint32(gameTimeFraction);
		m_snTimeInMillisecondsNonClipped += uint32(gameTimeFraction);
		gameTimeFraction -= uint32(gameTimeFraction);
		ms_fTimeStep = frameTime / 1000.0f * 50.0f;
	}

	if ( ms_fTimeStep < 0.01f && !GetIsPaused() && !CSpecialFX::bSnapShotActive )
		ms_fTimeStep = 0.01f;

	ms_fTimeStepNonClipped = ms_fTimeStep;

	if ( !CRecordDataForGame::IsPlayingBack() )
	{
		ms_fTimeStep = Min(3.0f, ms_fTimeStep);

		if ( (m_snTimeInMilliseconds - m_snPreviousTimeInMilliseconds) > 60 )
			m_snTimeInMilliseconds = m_snPreviousTimeInMilliseconds + 60;
	}

	if ( CRecordDataForChase::IsRecording() )
	{
		ms_fTimeStep = 1.0f;
		m_snTimeInMilliseconds = m_snPreviousTimeInMilliseconds + 16;
	}

	m_FrameCounter++;
}

// The camera and everything else that runs once per rendered frame gets the time the
// rendered frame took, so it is smooth whatever the gap between logical frames. The
// game clock is moved into the next logical frame by the same amount, camera moves are
// timed by it, and moved back before that logical frame comes
void CTimer::SetTimeStepForRender(void)
{
	ms_fTimeStep = ms_fRenderTimeStep;
	ms_fTimeStepNonClipped = ms_fRenderTimeStepNonClipped;

	m_snTimeInMilliseconds -= renderTimeOffset;
	m_snTimeInMillisecondsNonClipped -= renderTimeOffset;
	m_snTimeInMillisecondsPauseMode -= renderPauseTimeOffset;
	renderTimeOffset = GetIsPaused() ? 0 : uint32(ms_fLogicalFrameFraction * LOGICAL_FRAME_MS * ms_fTimeScale);
	renderPauseTimeOffset = uint32(ms_fLogicalFrameFraction * LOGICAL_FRAME_MS);
	m_snTimeInMilliseconds += renderTimeOffset;
	m_snTimeInMillisecondsNonClipped += renderTimeOffset;
	m_snTimeInMillisecondsPauseMode += renderPauseTimeOffset;
}

// Whether the next Update() would produce a logical frame if it happened after
// msSinceUpdate more milliseconds. The frame limiter uses this to render
// one frame per logical frame
bool CTimer::IsLogicalFrameDue(float msSinceUpdate)
{
	return logicalFrameTime + msSinceUpdate >= LOGICAL_FRAME_MS;
}

void CTimer::Suspend(void)
{
	if ( ++suspendDepth > 1 )
		return;

#ifdef _WIN32
	if ( (double)_nCyclesPerMS != 0.0 )
		QueryPerformanceCounter(&perfSuspendCounter);
	else
#endif
		suspendPcTimer = RsTimer();
}

void CTimer::Resume(void)
{
	if ( --suspendDepth != 0 )
		return;

#ifdef _WIN32
	if ( (double)_nCyclesPerMS != 0.0 )
	{
		LARGE_INTEGER pc;
		QueryPerformanceCounter(&pc);

		_oldPerfCounter.LowPart += pc.LowPart - perfSuspendCounter.LowPart;
	}
	else
#endif
		oldPcTimer += RsTimer() - suspendPcTimer;
}

uint32 CTimer::GetCyclesPerMillisecond(void)
{
#ifdef _WIN32
	if (_nCyclesPerMS != 0)
		return _nCyclesPerMS;
	else
#endif
		return 1;
}

uint32 CTimer::GetCurrentTimeInCycles(void)
{
#ifdef _WIN32
	if ( _nCyclesPerMS != 0 )
	{
		LARGE_INTEGER pc;
		QueryPerformanceCounter(&pc);
		return (pc.LowPart - _oldPerfCounter.LowPart); // & 0x7FFFFFFF; pointless
	}
	else
#endif
		return RsTimer() - oldPcTimer;
}

bool CTimer::GetIsSlowMotionActive(void)
{
	return ms_fTimeScale < 1.0f;
}

void CTimer::Stop(void)
{
	m_snPreviousTimeInMilliseconds = m_snTimeInMilliseconds;
}

void CTimer::StartUserPause(void)
{
	m_UserPause = true;
}

void CTimer::EndUserPause(void)
{
	m_UserPause = false;
}

uint32 CTimer::GetCyclesPerFrame()
{
	return 20;
}
