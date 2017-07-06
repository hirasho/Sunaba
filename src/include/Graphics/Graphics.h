#ifndef INCLUDED_SUNABA_GRAPHICS_GRAPHICS_H
#define INCLUDED_SUNABA_GRAPHICS_GRAPHICS_H

#include "Base/Array.h"
#include "Base/String.h"

namespace Sunaba{
class IoState;

class Graphics{
public:
	Graphics(
		void* windowHandle, 
		int textureWidth, 
		int textureHeight, 
		int screenWidth,
		int screenHeight);
	~Graphics();
	void getScreenData(const IoState*);
	bool isError() const;
	void endFrame();
private:
	class Impl;
	Impl* mImpl;
};

} //namespace Sunaba

#endif