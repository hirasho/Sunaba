#include <stdio.h>
#include "Base/Base.h"
#include "Graphics/Graphics.h"
#include "Base/Utility.h"
#include "Base/Os.h"
#include "Machine/Machine.h"
#include <algorithm>
#include <cstring>

#include <GLUT/GLUT.h>

#ifdef __APPLE__
extern "C" void setContextMyGLView();
extern "C" void flushContentMyGLView();
#endif

namespace Sunaba {

class Graphics::Impl{
public:
	struct Vertex{
		void set(float x, float y, float u, float v){
			mX = x;
			mY = y;
			mZ = 0.f;
			mRhW = 1.f;
			mU = u;
			mV = v;
		}
		float mX, mY, mZ, mRhW;
		float mU, mV;
	};
	bool isError() const{
		return mError;
	}
	Impl(
	void* windowHandle, 
	int texWidth, 
	int texHeight, 
	int screenWidth,
	int screenHeight) :
	mTextureWidth(texWidth),
	mTextureHeight(texHeight),
	mTextureWidthPow2(0),
	mTextureHeightPow2(0),
	mScreenWidth(screenWidth),
	mScreenHeight(screenHeight),
	mScreenTexture(0),
	mVram(0),
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
#if 0
		mHdc = GetDC(mWindowHandle);
		if (!mHdc){
			WRITE_LOG("GetDC() failed.");
			mError = true;
			return;
		}
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		pfd.cColorBits = 24; 
		pfd.cAlphaBits = 8;
		pfd.cDepthBits = 24; 
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int nFormat = ChoosePixelFormat(mHdc, &pfd);
		BOOL ret = SetPixelFormat(mHdc, nFormat, &pfd);
		if(ret == FALSE){
			WRITE_LOG("SetPixelFormat() failed.");
			mError = true;
			return;
		}
		mGlRC = wglCreateContext(mHdc);
		if (!mGlRC){
			WRITE_LOG("wglCreateContext() failed.");
			mError = true;
			return;
		}
		ret = wglMakeCurrent(mHdc, mGlRC);
		if (ret == FALSE){
			WRITE_LOG("wglMakeCurrent() failed.");
			mError = true;
			return;
		}

		//拡張でVSync
		bool swapIntervalExist = false;
		WglGetExtensionsStringEXT wglGetExtensionsStringEXT = 
			reinterpret_cast<WglGetExtensionsStringEXT>(wglGetProcAddress("wglGetExtensionsStringEXT"));
		if (wglGetExtensionsStringEXT){
			const char* wglExtString = wglGetExtensionsStringEXT();
//			WRITE_LOG(wglExtString);
			if (!wglExtString){
				WRITE_LOG("can't get WGL Extensions String");
			}else{
				if (std::strstr(wglExtString, "WGL_EXT_swap_control") != 0){
					swapIntervalExist = true; //発見
				}
			}
		}
		if (!swapIntervalExist){
			const char* glExtString = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
//			WRITE_LOG(glExtString);
			if (!glExtString){
				WRITE_LOG("can't get OpenGL Extensions String");
			}else{
				if (std::strstr(glExtString, "WGL_EXT_swap_control") != 0){
					swapIntervalExist = true; //発見
				}
			}
		}
		if (swapIntervalExist){
			WglSwapIntervalEXT wglSwapIntervalEXT = 
				reinterpret_cast<WglSwapIntervalEXT>(wglGetProcAddress("wglSwapIntervalEXT"));
			if (!wglSwapIntervalEXT){
				WRITE_LOG("can't get address of wglSwapIntervalEXT().");
			}else{
				BOOL ret = wglSwapIntervalEXT(0);
				if (!ret){
					WRITE_LOG("wglSwapIntervalEXT() failed.");
				}
			}
		}
#endif
		//リアルテクスチャサイズは2羃
		mTextureWidthPow2 = roundUpToPow2(mTextureWidth);
		mTextureHeightPow2 = roundUpToPow2(mTextureHeight);
		//VRAM受け取りバッファ
		int pixelCount = mTextureWidthPow2 * mTextureHeightPow2;
		mVram = new int[pixelCount];
		memset(mVram, 0, pixelCount * 4);
        
        setContextMyGLView();

		glEnable(GL_TEXTURE_2D); //念のため
		glGenTextures(1, &mScreenTexture);
		glBindTexture(GL_TEXTURE_2D, mScreenTexture);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTextureWidthPow2, mTextureHeightPow2, 0, GL_RGBA, GL_UNSIGNED_BYTE, mVram);
        
#if 0 // 今コントロールのサイズを指定していないので・・・無効化しておく
		//radeonのバグへの対処としてビューポートを明に指定
		glViewport(0, 0, mScreenWidth, mScreenHeight);
#endif

		//時刻測定
		mPrevTime = getTimeInMilliSecond();
	}
	~Impl(){
		DELETE_ARRAY(mVram);
#if 0
		if (mScreenTexture){
			glDeleteTextures(1, &mScreenTexture);
			mScreenTexture = 0;
		}
		BOOL ret = wglMakeCurrent(NULL, NULL);
		if (!ret){
//			WRITE_LOG("wglMakeCurrent() failed."); //一時的に削除。毎フレームやってる時は多重になり失敗する。
		}
		ret = wglDeleteContext(mGlRC);
		if (!ret){
			WRITE_LOG("wglDeleteContext() failed.");
		}
		ReleaseDC(mWindowHandle, mHdc);
#endif
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
		const int* srcLine = ioState->vramReadPointer();
		int* dstLine = mVram;
		for (int y = 0; y < mTextureHeight; ++y){
			const int* src = srcLine;
			int* dst = dstLine;
			for (int x = 0; x < mTextureWidth; ++x){
				dst[x] = src[x];
			}
			srcLine += mTextureWidth;
			dstLine += mTextureWidthPow2;
		}
	}
	void endFrame(){
		//色変換
		int* line = mVram;
		for (int y = 0; y < mTextureHeight; ++y){
			int* p = line;
			for (int x = 0; x < mTextureWidth; ++x){
				*p = convertColor100to256(*p);
				++p;
			}
			line += mTextureWidthPow2; //ここ注意
		}
        
#if 1
        // Objective-C/C++ 側でコンテキストセットを読んでおく.
        //
        setContextMyGLView();
        
		glDisable(GL_BLEND); //おかしなドライバ対応
		glDisable(GL_CULL_FACE); //おかしなドライバ対応
		glDisable(GL_SCISSOR_TEST); //おかしなドライバ対応
		glDisable(GL_STENCIL_TEST); //おかしなドライバ対応
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mScreenTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTextureWidthPow2, mTextureHeightPow2, 0, GL_RGBA, GL_UNSIGNED_BYTE, mVram);
		//デバグ用真赤塗り。そんなに重くないよね？理想的には無駄負荷。
		glClearColor(1.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		// 本体描画
		float x0 = -1.0f;
		float y0 = 1.f;
		float x1 = 1.f;
		float y1 = -1.f;
		float u1 = static_cast<float>(mTextureWidth) / static_cast<float>(mTextureWidthPow2);
		float v1 = static_cast<float>(mTextureHeight) / static_cast<float>(mTextureHeightPow2);
		Vertex v[4];
		v[0].set(x0, y0, 0.0f, 0.0f);
		v[1].set(x1, y0, u1, 0.0f);
		v[2].set(x0, y1, 0.0f, v1);
		v[3].set(x1, y1, u1, v1);

    GLfloat vertics[] = {
      x0, y0,
      x1, y0,
      x0, y1,
      x1, y1,
    };
    GLfloat texVertics[] = {
      0.f, 0.f,
      u1,  0.f,
      0.f, v1,
      u1,  v1,
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertics);
    glTexCoordPointer(2, GL_FLOAT, 0, texVertics);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glFlush();

		//エラー出てる？
		GLenum error = glGetError();
		if (error != GL_NO_ERROR){
			std::ostringstream oss;
			oss << "OpenGL ERROR(" << std::hex << error << std::dec << ")" << std::endl;
			WRITE_LOG(oss.str().c_str());
		}
        flushContentMyGLView();

#endif
        
        
#if 0
		//いるのこれ？
		BOOL ret = wglMakeCurrent(mHdc, mGlRC);
		if (ret == FALSE){
			WRITE_LOG("wglMakeCurrent() failed.");
		}
		glDisable(GL_BLEND); //おかしなドライバ対応
		glDisable(GL_CULL_FACE); //おかしなドライバ対応
		glDisable(GL_SCISSOR_TEST); //おかしなドライバ対応
		glDisable(GL_STENCIL_TEST); //おかしなドライバ対応
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mScreenTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTextureWidthPow2, mTextureHeightPow2, 0, GL_RGBA, GL_UNSIGNED_BYTE, mVram);
		//デバグ用真赤塗り。そんなに重くないよね？理想的には無駄負荷。
		glClearColor(1.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		// 本体描画
		float x0 = -1.0f;
		float y0 = 1.f;
		float x1 = 1.f;
		float y1 = -1.f;
		float u1 = static_cast<float>(mTextureWidth) / static_cast<float>(mTextureWidthPow2);
		float v1 = static_cast<float>(mTextureHeight) / static_cast<float>(mTextureHeightPow2);
		Vertex v[4];
		v[0].set(x0, y0, 0.0f, 0.0f);
		v[1].set(x1, y0, u1, 0.0f);
		v[2].set(x0, y1, 0.0f, v1);
		v[3].set(x1, y1, u1, v1);
		glBegin(GL_TRIANGLE_STRIP);
		{
			glColor4f(1.f, 1.f, 1.f, 1.f); //いらんだろ
			glTexCoord2f(v[0].mU, v[0].mV); 
			glVertex2f(v[0].mX, v[0].mY);

			glColor4f(1.f, 1.f, 1.f, 1.f); //いらんだろ
			glTexCoord2f(v[1].mU, v[1].mV); 
			glVertex2f(v[1].mX, v[1].mY);

			glColor4f(1.f, 1.f, 1.f, 1.f); //いらんだろ
			glTexCoord2f(v[2].mU, v[2].mV);
			glVertex2f(v[2].mX, v[2].mY);

			glColor4f(1.f, 1.f, 1.f, 1.f); //いらんだろ
			glTexCoord2f(v[3].mU, v[3].mV); 
			glVertex2f(v[3].mX, v[3].mY);
		}
		glEnd();
		glFlush(); //いるのこれ？
		ret = SwapBuffers(mHdc);
		if (ret == FALSE){
			WRITE_LOG("SwapBuffers() failed.");
		}
		//エラー出てる？
		GLenum error = glGetError();
		if (error != GL_NO_ERROR){
			std::ostringstream oss;
			oss << "OpenGL ERROR(" << std::hex << error << std::dec << ")" << std::endl;
			WRITE_LOG(oss.str().c_str());
		}
		//いるのこれ？
		ret = wglMakeCurrent(mHdc, NULL);
		if (ret == FALSE){
			WRITE_LOG("wglMakeCurrent() failed.");
		}
#endif
		adjastFrameRate();
	}
private:
	enum{
		FRAME_TIME_COUNT = 60,
	};
	int mTextureWidth;
	int mTextureHeight;
	int mTextureWidthPow2;
	int mTextureHeightPow2;
	int mScreenWidth;
	int mScreenHeight;
    GLuint mScreenTexture;
	int* mVram;
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



} // Sunaba



#if 0
#include <windows.h>
#undef DELETE
#ifdef SUNABA_USE_GDI

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


#else //SUNABA_USE_GDI

#include <GL/GL.h>
#pragma comment(lib, "opengl32.lib")

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
	struct Vertex{
		void set(float x, float y, float u, float v){
			mX = x;
			mY = y;
			mZ = 0.f;
			mRhW = 1.f;
			mU = u;
			mV = v;
		}
		float mX, mY, mZ, mRhW;
		float mU, mV;
	};
	bool isError() const{
		return mError;
	}
	Impl(
	void* windowHandle, 
	int texWidth, 
	int texHeight, 
	int screenWidth,
	int screenHeight) :
	mTextureWidth(texWidth),
	mTextureHeight(texHeight),
	mTextureWidthPow2(0),
	mTextureHeightPow2(0),
	mScreenWidth(screenWidth),
	mScreenHeight(screenHeight),
	mScreenTexture(0),
	mVram(0),
	mHdc(NULL),
	mGlRC(0),
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
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		pfd.cColorBits = 24; 
		pfd.cAlphaBits = 8;
		pfd.cDepthBits = 24; 
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int nFormat = ChoosePixelFormat(mHdc, &pfd);
		BOOL ret = SetPixelFormat(mHdc, nFormat, &pfd);
		if(ret == FALSE){
			WRITE_LOG("SetPixelFormat() failed.");
			mError = true;
			return;
		}
		mGlRC = wglCreateContext(mHdc);
		if (!mGlRC){
			WRITE_LOG("wglCreateContext() failed.");
			mError = true;
			return;
		}
		ret = wglMakeCurrent(mHdc, mGlRC);
		if (ret == FALSE){
			WRITE_LOG("wglMakeCurrent() failed.");
			mError = true;
			return;
		}

		//拡張でVSync
		bool swapIntervalExist = false;
		WglGetExtensionsStringEXT wglGetExtensionsStringEXT = 
			reinterpret_cast<WglGetExtensionsStringEXT>(wglGetProcAddress("wglGetExtensionsStringEXT"));
		if (wglGetExtensionsStringEXT){
			const char* wglExtString = wglGetExtensionsStringEXT();
//			WRITE_LOG(wglExtString);
			if (!wglExtString){
				WRITE_LOG("can't get WGL Extensions String");
			}else{
				if (std::strstr(wglExtString, "WGL_EXT_swap_control") != 0){
					swapIntervalExist = true; //発見
				}
			}
		}
		if (!swapIntervalExist){
			const char* glExtString = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
//			WRITE_LOG(glExtString);
			if (!glExtString){
				WRITE_LOG("can't get OpenGL Extensions String");
			}else{
				if (std::strstr(glExtString, "WGL_EXT_swap_control") != 0){
					swapIntervalExist = true; //発見
				}
			}
		}
		if (swapIntervalExist){
			WglSwapIntervalEXT wglSwapIntervalEXT = 
				reinterpret_cast<WglSwapIntervalEXT>(wglGetProcAddress("wglSwapIntervalEXT"));
			if (!wglSwapIntervalEXT){
				WRITE_LOG("can't get address of wglSwapIntervalEXT().");
			}else{
				BOOL ret = wglSwapIntervalEXT(0);
				if (!ret){
					WRITE_LOG("wglSwapIntervalEXT() failed.");
				}
			}
		}
		//リアルテクスチャサイズは2羃
		mTextureWidthPow2 = roundUpToPow2(mTextureWidth);
		mTextureHeightPow2 = roundUpToPow2(mTextureHeight);
		//VRAM受け取りバッファ
		int pixelCount = mTextureWidthPow2 * mTextureHeightPow2;
		mVram = new int[pixelCount];
		memset(mVram, 0, pixelCount * 4);

		glEnable(GL_TEXTURE_2D); //念のため
		glGenTextures(1, &mScreenTexture);
		glBindTexture(GL_TEXTURE_2D, mScreenTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTextureWidthPow2, mTextureHeightPow2, 0, GL_RGBA, GL_UNSIGNED_BYTE, mVram);
		//radeonのバグへの対処としてビューポートを明に指定
		glViewport(0, 0, mScreenWidth, mScreenHeight);
		//時刻測定
		mPrevTime = getTimeInMilliSecond();
	}
	~Impl(){
		DELETE_ARRAY(mVram);
		if (mScreenTexture){
			glDeleteTextures(1, &mScreenTexture);
			mScreenTexture = 0;
		}
		BOOL ret = wglMakeCurrent(NULL, NULL);
		if (!ret){
//			WRITE_LOG("wglMakeCurrent() failed."); //一時的に削除。毎フレームやってる時は多重になり失敗する。
		}
		ret = wglDeleteContext(mGlRC);
		if (!ret){
			WRITE_LOG("wglDeleteContext() failed.");
		}
		ReleaseDC(mWindowHandle, mHdc);
		mGlRC = 0;
		mHdc = 0;
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
		const int* srcLine = ioState->vramReadPointer();
		int* dstLine = mVram;
		for (int y = 0; y < mTextureHeight; ++y){
			const int* src = srcLine;
			int* dst = dstLine;
			for (int x = 0; x < mTextureWidth; ++x){
				dst[x] = src[x];
			}
			srcLine += mTextureWidth;
			dstLine += mTextureWidthPow2;
		}
	}
	void endFrame(){
		//色変換
		int* line = mVram;
		for (int y = 0; y < mTextureHeight; ++y){
			int* p = line;
			for (int x = 0; x < mTextureWidth; ++x){
				*p = convertColor100to256(*p);
				++p;
			}
			line += mTextureWidthPow2; //ここ注意
		}
		//いるのこれ？
		BOOL ret = wglMakeCurrent(mHdc, mGlRC);
		if (ret == FALSE){
			WRITE_LOG("wglMakeCurrent() failed.");
		}
		glDisable(GL_BLEND); //おかしなドライバ対応
		glDisable(GL_CULL_FACE); //おかしなドライバ対応
		glDisable(GL_SCISSOR_TEST); //おかしなドライバ対応
		glDisable(GL_STENCIL_TEST); //おかしなドライバ対応
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mScreenTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTextureWidthPow2, mTextureHeightPow2, 0, GL_RGBA, GL_UNSIGNED_BYTE, mVram);
		//デバグ用真赤塗り。そんなに重くないよね？理想的には無駄負荷。
		glClearColor(1.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		// 本体描画
		float x0 = -1.0f;
		float y0 = 1.f;
		float x1 = 1.f;
		float y1 = -1.f;
		float u1 = static_cast<float>(mTextureWidth) / static_cast<float>(mTextureWidthPow2);
		float v1 = static_cast<float>(mTextureHeight) / static_cast<float>(mTextureHeightPow2);
		Vertex v[4];
		v[0].set(x0, y0, 0.0f, 0.0f);
		v[1].set(x1, y0, u1, 0.0f);
		v[2].set(x0, y1, 0.0f, v1);
		v[3].set(x1, y1, u1, v1);
		glBegin(GL_TRIANGLE_STRIP);
		{
			glColor4f(1.f, 1.f, 1.f, 1.f); //いらんだろ
			glTexCoord2f(v[0].mU, v[0].mV); 
			glVertex2f(v[0].mX, v[0].mY);

			glColor4f(1.f, 1.f, 1.f, 1.f); //いらんだろ
			glTexCoord2f(v[1].mU, v[1].mV); 
			glVertex2f(v[1].mX, v[1].mY);

			glColor4f(1.f, 1.f, 1.f, 1.f); //いらんだろ
			glTexCoord2f(v[2].mU, v[2].mV);
			glVertex2f(v[2].mX, v[2].mY);

			glColor4f(1.f, 1.f, 1.f, 1.f); //いらんだろ
			glTexCoord2f(v[3].mU, v[3].mV); 
			glVertex2f(v[3].mX, v[3].mY);
		}
		glEnd();
		glFlush(); //いるのこれ？
		ret = SwapBuffers(mHdc);
		if (ret == FALSE){
			WRITE_LOG("SwapBuffers() failed.");
		}
		//エラー出てる？
		GLenum error = glGetError();
		if (error != GL_NO_ERROR){
			std::ostringstream oss;
			oss << "OpenGL ERROR(" << std::hex << error << std::dec << ")" << std::endl;
			WRITE_LOG(oss.str().c_str());
		}
		//いるのこれ？
		ret = wglMakeCurrent(mHdc, NULL);
		if (ret == FALSE){
			WRITE_LOG("wglMakeCurrent() failed.");
		}
		adjastFrameRate();
	}
private:
	enum{
		FRAME_TIME_COUNT = 60,
	};
	typedef BOOL (APIENTRY *WglSwapIntervalEXT)(int);
	typedef const char* (APIENTRY *WglGetExtensionsStringEXT)();

	int mTextureWidth;
	int mTextureHeight;
	int mTextureWidthPow2;
	int mTextureHeightPow2;
	int mScreenWidth;
	int mScreenHeight;
    GLuint mScreenTexture;
	int* mVram;
    HDC mHdc;
    HGLRC mGlRC;

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
	//mImpl->getScreenData(ioState);
}

void Graphics::endFrame(){
	//mImpl->endFrame();
}

bool Graphics::isError() const{
	return mImpl->isError();
}

} //namespace Sunaba

#endif //SUNABA_USE_GDI

#endif
