#pragma once

#include "AnimBlendList.h"


struct AnimBlendFrameData
{
	enum {
		IGNORE_ROTATION = 2,
		IGNORE_TRANSLATION = 4,
		VELOCITY_EXTRACTION = 8,
		VELOCITY_EXTRACTION_3D = 0x10,
		UPDATE_KEYFRAMES = 0x20,
		COMPRESSED = 0x40
	};

	uint8 flag;
	RwV3d resetPos;
	union {
		RwFrame *frame;
		RpHAnimStdInterpFrame *hanimFrame;
	};
	int32 nodeID;
};
VALIDATE_SIZE(AnimBlendFrameData, 0x18);


class CAnimBlendClumpData
{
public:
	CAnimBlendLink link;
	int32 numFrames;
	union {
		CVector2D *velocity2d;
		CVector *velocity3d;
	};
	// order of frames is determined by RW hierarchy
	AnimBlendFrameData *frames;
	// the bones at the end of the previous logical frame and the real current ones, kept
	// while the drawn ones are put between the two, see RpAnimBlendClumpInterpolateFrames().
	// a skinned clump animates the interp frames of its hierarchy, any other the matrices
	// of its RwFrames
	bool skinned;
	bool prevMatricesValid;
	bool matricesInterpolated;
	RwMatrix *prevMatrices;
	RwMatrix *realMatrices;
	RpHAnimStdInterpFrame *prevFrames;
	RpHAnimStdInterpFrame *realFrames;

	CAnimBlendClumpData(void);
	~CAnimBlendClumpData(void);
	void SetNumberOfFrames(int n);
	void SetNumberOfBones(int n) { SetNumberOfFrames(n); }
	void ForAllFrames(void (*cb)(AnimBlendFrameData*, void*), void *arg);
	void AllocInterpolationMatrices(bool isSkinned);
	void FreeInterpolationMatrices(void);
};
