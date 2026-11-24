#pragma once

// Plays the logo and title movies before the game starts, the same way on every platform
class CIntroMovie
{
public:
	static RwUInt32 FirstState(void);
	static void RunState(void);
	static void Close(void);
};
