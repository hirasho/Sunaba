#include <windows.h>
#undef DELETE

#include "Base/Base.h"
#include "Graphics/Graphics.h"
#include "Base/Utility.h"
#include "Base/Os.h"
#include "Machine/Machine.h"
#include <algorithm>
#include <cstring>

namespace Sunaba{

class Graphics::Impl{
public:
	Impl(
	void* windowHandle, 
	int texWidth, 
	int texHeight, 
	int screenWidth,
	int screenHeight) :
	mTextureWidth(texWidth),
	mTextureHeight(texHeight),
	mScreenWidth(screenWidth),
	mScreenHeight(screenHeight),
	mVram(0),
	mHdc(NULL),
	mBitmapHdc(NULL),
	mBitmap(NULL),
	mBitmapPixels(0),
	mWindowHandle(static_cast<HWND>(windowHandle)),
	mError(false),
	mSleepTime(16.f), //ここから
	mSleepTimeVelocity(0.f),
	mPrevTime(0){
		//スクリーンサイズは最小1x1
		if (mScreenWidth < 1){
			mScreenWidth = 1;
		}
		if (mScreenHeight < 1){
			mScreenHeight = 1;
		}
		mHdc = GetDC(mWindowHandle);
		if (!mHdc){
			WRITE_LOG("GetDC() failed.");
			mError = true;
			return;
		}
		//VRAM受け取りバッファ
		int pixelCount = mTextureWidth * mTextureHeight;
		mVram = new int[pixelCount];
		memset(mVram, 0, pixelCount * sizeof(int));

		//ビットマップDC生成
		mBitmapHdc = CreateCompatibleDC(mHdc);
		//ビットマップ生成
		BITMAPINFO bmInfo;
		memset(&bmInfo, 0, sizeof(BITMAPINFO));
		bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmInfo.bmiHeader.biWidth = mTextureWidth;
		bmInfo.bmiHeader.biHeight = -mTextureHeight; //上から下
		bmInfo.bmiHeader.biPlanes = 1;
		bmInfo.bmiHeader.biBitCount = 24;
		bmInfo.bmiHeader.biCompression = BI_RGB;

		mBitmap = CreateDIBSection(mHdc, &bmInfo, DIB_RGB_COLORS, &mBitmapPixels, NULL, 0);
		if ((mBitmap == NULL) || (mBitmapPixels == NULL)){
			WRITE_LOG("CreateDIBSection() failed.");
			mError = true;
			return;
		}
		SelectObject(mBitmapHdc, mBitmap);
		//時刻測定
		mPrevTime = getTimeInMilliSecond();
	}
	~Impl(){
		DELETE_ARRAY(mVram);
		ReleaseDC(mWindowHandle, mHdc);
		mHdc = NULL;
		DeleteDC(mBitmapHdc);
		mBitmapHdc = NULL;
		DeleteObject(mBitmap);
		mBitmap = NULL;
		mBitmapPixels = 0;
	}
	void adjastFrameRate(){
		static const float SPRING = 0.1f;
		static const float DAMPING = 0.9f;
		//フレームレート調整
		if (mSleepTime >= 0.5f){
			sleepMilliSecond(static_cast<int>(mSleepTime + 0.5f));
		}
		unsigned now = getTimeInMilliSecond();
		float diff = (100.f / 6.f) - static_cast<float>(now - mPrevTime);
		mPrevTime = now;
		float force = (diff * SPRING) - (mSleepTimeVelocity * DAMPING);
		mSleepTimeVelocity += force;
		//負に落とすような速度は
		if ((mSleepTimeVelocity < 0.f) && ((mSleepTime - mSleepTimeVelocity) < 0.f)){
			mSleepTimeVelocity = -mSleepTime;
		}
		mSleepTime += mSleepTimeVelocity;
		if (mSleepTime < 0.f){
			mSleepTime = 0.f;
		}
	}
	void getScreenData(const IoState* ioState){
		memcpy(mVram, ioState->vramReadPointer(), mTextureHeight * mTextureWidth * sizeof(int));
	}
	void endFrame(){
		//色変換しつつ書きこみ
		const int* srcLine = mVram;
		unsigned char* dstLine = static_cast<unsigned char*>(mBitmapPixels);
		for (int y = 0; y < mTextureHeight; ++y){
			const int* src = srcLine;
			unsigned char* dst = dstLine;
			for (int x = 0; x < mTextureWidth; ++x){
				int c = convertColor100to256(*src);
				dst[0] = static_cast<unsigned char>((c >> 16) & 0xff);
				dst[1] = static_cast<unsigned char>((c >> 8) & 0xff);
				dst[2] = static_cast<unsigned char>((c >> 0) & 0xff);
				dst += 3;
				++src;
			}
			srcLine += mTextureWidth;
			dstLine += mTextureWidth * 3;
		}
		PAINTSTRUCT paint;
		HDC hdc = BeginPaint(mWindowHandle, &paint);
		if (hdc == NULL){
			WRITE_LOG("BeginPaint() failed.");
			mError = true;
			return;
		}
		BOOL ret = StretchBlt(mHdc, 0, 0, mScreenWidth, mScreenHeight, mBitmapHdc, 0, 0, mTextureWidth, mTextureHeight, SRCCOPY);
		if (ret == 0){
			WRITE_LOG("StretchBlt() failed.");
			mError = true;
			return;
		}
		EndPaint(mWindowHandle, &paint);

		adjastFrameRate();
	}
	bool isError() const{
		return mError;
	}
private:
	enum{
		FRAME_TIME_COUNT = 60,
	};
	int mTextureWidth;
	int mTextureHeight;
	int mScreenWidth;
	int mScreenHeight;
	int* mVram;
    HDC mHdc;
	HDC mBitmapHdc;
	HBITMAP mBitmap;
	void* mBitmapPixels;

    HWND mWindowHandle;
	bool mError;
	//フレームレート調整。物理シミュレーションを応用して60FPSを保とうとする。
	float mSleepTimeVelocity;
	float mSleepTime;
	unsigned mPrevTime;
};

Graphics::Graphics(
void* windowHandle, 
int texWidth, 
int texHeight, 
int screenWidth,
int screenHeight) : 
mImpl(0){
	mImpl = new Impl(windowHandle, texWidth, texHeight, screenWidth, screenHeight);
}

Graphics::~Graphics(){
	DELETE(mImpl);
}

void Graphics::getScreenData(const IoState* ioState){
	mImpl->getScreenData(ioState);
}

void Graphics::endFrame(){
	mImpl->endFrame();
}

bool Graphics::isError() const{
	return mImpl->isError();
}

} //namespace Sunaba
