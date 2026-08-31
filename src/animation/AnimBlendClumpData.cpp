#include "common.h"

#include "AnimBlendClumpData.h"
#include "MemoryMgr.h"

CAnimBlendClumpData::CAnimBlendClumpData(void)
{
	numFrames = 0;
	velocity2d = nil;
	frames = nil;
	prevMatrices = nil;
	realMatrices = nil;
	prevMatricesValid = false;
	matricesInterpolated = false;
	link.Init();
}

CAnimBlendClumpData::~CAnimBlendClumpData(void)
{
	link.Remove();
	if(frames)
		RwFreeAlign(frames);
	FreeInterpolationMatrices();
}

void
CAnimBlendClumpData::SetNumberOfFrames(int n)
{
	if(frames)
		RwFreeAlign(frames);
	numFrames = n;
	frames = (AnimBlendFrameData*)RwMallocAlign(numFrames * sizeof(AnimBlendFrameData), 64);
	FreeInterpolationMatrices();
}

// Called once the frames are known, allocates what the interpolation keeps for them
void
CAnimBlendClumpData::AllocInterpolationMatrices(void)
{
	FreeInterpolationMatrices();
	prevMatrices = (RwMatrix*)RwMallocAlign(numFrames * sizeof(RwMatrix), 64);
	realMatrices = (RwMatrix*)RwMallocAlign(numFrames * sizeof(RwMatrix), 64);
}

void
CAnimBlendClumpData::FreeInterpolationMatrices(void)
{
	if(prevMatrices)
		RwFreeAlign(prevMatrices);
	if(realMatrices)
		RwFreeAlign(realMatrices);
	prevMatrices = nil;
	realMatrices = nil;
	prevMatricesValid = false;
	matricesInterpolated = false;
}

void
CAnimBlendClumpData::ForAllFrames(void (*cb)(AnimBlendFrameData*, void*), void *arg)
{
	int i;
	for(i = 0; i < numFrames; i++)
		cb(&frames[i], arg);
}
