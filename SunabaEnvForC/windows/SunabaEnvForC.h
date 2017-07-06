#ifndef INCLUDED_SUNABAENVFORC_H
#define INCLUDED_SUNABAENVFORC_H

void SunabaEnv_main(); /*これは自分で作る*/
void SunabaEnv_disableAutoSync(); /* memory[55001] -> 1 */
void SunabaEnv_sync(); /* memory[55000] -> 1 */
int SunabaEnv_getMouseX(); /* memory[50000] */
int SunabaEnv_getMouseY(); /* memory[50001] */
int SunabaEnv_getMouseLeftButton(); /* memory[50002] */
int SunabaEnv_getMouseRightButton(); /* memory[50003] */
int SunabaEnv_getKeyUp(); /* meomry[50004] */
int SunabaEnv_getKeyDown(); /* meomry[50005] */
int SunabaEnv_getKeyLeft(); /* meomry[50006] */
int SunabaEnv_getKeyRight(); /* meomry[50007] */
int SunabaEnv_getKeySpace(); /* meomry[50008] */
int SunabaEnv_getKeyEnter(); /* meomry[50009] */
void SunabaEnv_drawPoint(int x, int y, int color);

/*-----------------以下実装-------------------------*/
#ifdef __cplusplus
#define SUNABA_INLINE inline
#else
#define SUNABA_INLINE 
#endif

#include <assert.h>
struct SunabaEnvPlatform;
/* データ保持構造体 */
enum{
   INPUT_MOUSE_LEFT,
   INPUT_MOUSE_RIGHT,
   INPUT_KEY_UP,
   INPUT_KEY_DOWN,
   INPUT_KEY_LEFT,
   INPUT_KEY_RIGHT,
   INPUT_KEY_SPACE,
   INPUT_KEY_ENTER,
      
   INPUT_COUNT,
};
typedef struct SunabaEnv{
   int disableAutoSync;
   int mouseX;
   int mouseY;
   int input[INPUT_COUNT];
   int screen[100 * 100];
   struct SunabaEnvPlatform* platform; /* 実体はエントリポイント関数のローカル変数 */
} SunabaEnv;

/*機種依存関数プロトタイプ*/
void SunabaEnvInternalPlatform_processEvent(SunabaEnv*);
void SunabaEnvInternalPlatform_draw(SunabaEnv*);

/*機種非依存部実装*/

SUNABA_INLINE SunabaEnv* SunabaEnvInternal_getInstance(){
   static SunabaEnv env;
   return &env;
};

SUNABA_INLINE int SunabaEnvInternal_getInput(int index){
   SunabaEnv* env = SunabaEnvInternal_getInstance();
   if (env->disableAutoSync == 0){
      SunabaEnvInternalPlatform_processEvent(env);
   }
   return env->input[index];
}

SUNABA_INLINE void SunabaEnv_disableAutoSync(){
   SunabaEnvInternal_getInstance()->disableAutoSync = 1;
}

SUNABA_INLINE void SunabaEnv_drawPoint(int x, int y, int color){
   if ((x >= 0) && (x < 100) && (y >= 0) && (y < 100)){
      SunabaEnv* env = SunabaEnvInternal_getInstance();
      env->screen[(y * 100) + x] = color;
      if (env->disableAutoSync == 0){
         SunabaEnvInternalPlatform_draw(env);
      }
   }
}

SUNABA_INLINE void SunabaEnv_sync(){
   SunabaEnv* env = SunabaEnvInternal_getInstance();
   SunabaEnvInternalPlatform_processEvent(env);
   SunabaEnvInternalPlatform_draw(env);
}

SUNABA_INLINE int SunabaEnv_getMouseX(){
   SunabaEnv* env = SunabaEnvInternal_getInstance();
   if (env->disableAutoSync == 0){
      SunabaEnvInternalPlatform_processEvent(env);
   }
   return env->mouseX;
}

SUNABA_INLINE int SunabaEnv_getMouseY(){
   SunabaEnv* env = SunabaEnvInternal_getInstance();
   if (env->disableAutoSync == 0){
      SunabaEnvInternalPlatform_processEvent(env);
   }
   return env->mouseY;
}

SUNABA_INLINE int SunabaEnv_getMouseLeftButton(){
   return SunabaEnvInternal_getInput(INPUT_MOUSE_LEFT);
}

SUNABA_INLINE int SunabaEnv_getMouseRightButton(){
   return SunabaEnvInternal_getInput(INPUT_MOUSE_RIGHT);
}

SUNABA_INLINE int SunabaEnv_getKeyUp(){
   return SunabaEnvInternal_getInput(INPUT_KEY_UP);
}

SUNABA_INLINE int SunabaEnv_getKeyDown(){
   return SunabaEnvInternal_getInput(INPUT_KEY_DOWN);
}

SUNABA_INLINE int SunabaEnv_getKeyLeft(){
   return SunabaEnvInternal_getInput(INPUT_KEY_LEFT);
}

SUNABA_INLINE int SunabaEnv_getKeyRight(){
   return SunabaEnvInternal_getInput(INPUT_KEY_RIGHT);
}

SUNABA_INLINE int SunabaEnv_getKeySpace(){
   return SunabaEnvInternal_getInput(INPUT_KEY_SPACE);
}

SUNABA_INLINE int SunabaEnv_getKeyEnter(){
   return SunabaEnvInternal_getInput(INPUT_KEY_ENTER);
}

/*機種依存部*/
#ifdef UNICODE
#undef UNICODE
#endif
#include <Windows.h>

typedef struct SunabaEnvPlatform{
   void* bitmapPixels;
   HWND hwnd;
   HDC bitmapHdc;
   LARGE_INTEGER oldTime;
   double performanceCounterFrequency;
} SunabaEnvPlatform;

SUNABA_INLINE LRESULT CALLBACK myWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp){
   SunabaEnv* env = SunabaEnvInternal_getInstance();
   LRESULT r = 0;
   switch (msg){
      case WM_DESTROY:
         PostQuitMessage(0);
         break;
      case WM_LBUTTONDOWN:
         env->input[INPUT_MOUSE_LEFT] = 1;
         env->mouseX = LOWORD(lp) / 3;
         env->mouseY = HIWORD(lp) / 3;
         break;
      case WM_LBUTTONUP:
         env->input[INPUT_MOUSE_LEFT] = 0;
         env->mouseX = LOWORD(lp) / 3;
         env->mouseY = HIWORD(lp) / 3;
         break;
      case WM_RBUTTONDOWN:
         env->input[INPUT_MOUSE_RIGHT] = 1;
         env->mouseX = LOWORD(lp) / 3;
         env->mouseY = HIWORD(lp) / 3;
         break;
      case WM_RBUTTONUP:
         env->input[INPUT_MOUSE_RIGHT] = 0;
         env->mouseX = LOWORD(lp) / 3;
         env->mouseY = HIWORD(lp) / 3;
         break;
      case WM_MOUSEMOVE:
         env->mouseX = LOWORD(lp) / 3;
         env->mouseY = HIWORD(lp) / 3;
         break;
      case WM_KEYDOWN:
         switch (wp){
            case VK_UP: env->input[INPUT_KEY_UP] = 1; break;
            case VK_DOWN: env->input[INPUT_KEY_DOWN] = 1; break;
            case VK_LEFT: env->input[INPUT_KEY_LEFT] = 1; break;
            case VK_RIGHT: env->input[INPUT_KEY_RIGHT] = 1; break;
            case VK_SPACE: env->input[INPUT_KEY_SPACE] = 1; break;
            case VK_RETURN: env->input[INPUT_KEY_ENTER] = 1; break;
         }
         break;
      case WM_KEYUP:
         switch (wp){
            case VK_UP: env->input[INPUT_KEY_UP] = 0; break;
            case VK_DOWN: env->input[INPUT_KEY_DOWN] = 0; break;
            case VK_LEFT: env->input[INPUT_KEY_LEFT] = 0; break;
            case VK_RIGHT: env->input[INPUT_KEY_RIGHT] = 0; break;
            case VK_SPACE: env->input[INPUT_KEY_SPACE] = 0; break;
            case VK_RETURN: env->input[INPUT_KEY_ENTER] = 0; break;
         }
         break;
      default:
         r = DefWindowProc(hwnd, msg, wp, lp);
         break;
   } 
   return r;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE notUsed0, char* notUsed1, int cmdShow){
   /*変数定義*/
   WNDCLASSEXA wc;
   HWND hwnd;
   HDC hdc;
   HDC bitmapHdc;
   BITMAPINFO bi;
   BITMAPINFOHEADER* bih = 0;
   void* bitmapPixels;
   HBITMAP bitmap;
   SunabaEnv* env = 0;
   SunabaEnvPlatform envPlatform;
   LARGE_INTEGER freq;
   /*ウィンドウクラス登録*/
   wc.cbSize = sizeof(WNDCLASSEXA);
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = myWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = hInstance;
   wc.hIcon = 0;
   wc.hCursor = LoadCursorA(0, IDC_ARROW);
   wc.hbrBackground = 0;
   wc.lpszMenuName = 0;
   wc.lpszClassName = "SunabaEnvForC";
   wc.hIconSm = 0;
   RegisterClassEx(&wc);
   /*ウィンドウ生成*/
   hwnd = CreateWindowA(
      "SunabaEnvForC",
      "SunabaEnvForC",
      WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      300,
      300,
      0,
      0,
      hInstance,
      0);
   if (!hwnd){
      return 1;
   }
   /*GDI初期化*/
   hdc = GetDC(hwnd);
   if (!hdc){
      return 2;
   }
   bitmapHdc = CreateCompatibleDC(hdc);
	
   memset(&bi, 0, sizeof(BITMAPINFO));
   bih = &(bi.bmiHeader);
   bih->biSize = sizeof(BITMAPINFOHEADER);
   bih->biWidth = 100;
   bih->biHeight = -100; /*上から下だとマイナス*/
   bih->biPlanes = 1;
   bih->biBitCount = 24;
   bih->biCompression = BI_RGB;
   bitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &bitmapPixels, 0, 0);	
   if (!bitmap){
      return 3;
   }
   SelectObject(bitmapHdc, bitmap);

   /*SunabaEnv初期化 */
   env = SunabaEnvInternal_getInstance();
   envPlatform.bitmapPixels = bitmapPixels;
   envPlatform.hwnd = hwnd;
   envPlatform.bitmapHdc = bitmapHdc;
   QueryPerformanceFrequency(&freq);
   envPlatform.performanceCounterFrequency = (double)(freq.QuadPart);
   QueryPerformanceCounter(&(envPlatform.oldTime));
   env->platform = &envPlatform;

   ShowWindow(hwnd, cmdShow);
    
   SunabaEnv_main();

   /*リソース破棄*/
   ReleaseDC(hwnd, hdc);
   DeleteDC(bitmapHdc);
   DeleteObject(bitmap);
   return 0;
}

SUNABA_INLINE void SunabaEnvInternalPlatform_processEvent(SunabaEnv* env){
   MSG msg;
   BOOL r = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
   if (r){
      if (msg.message == WM_QUIT){
         exit(0); /* 強制終了でいいだろう */
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
}

SUNABA_INLINE void SunabaEnvInternalPlatform_draw(SunabaEnv* env){
   PAINTSTRUCT paint;
   HDC hdc;
   SunabaEnvPlatform* pf = env->platform;
   unsigned char* dst = (unsigned char*)pf->bitmapPixels;
   int i;
   LARGE_INTEGER newTime;
   double diff;
   int sleepSec;

   /* ピクセルコピー */
   for (i = 0; i < 10000; ++i){
      int c = env->screen[i];
      unsigned char r, g, b;
      r = c / 10000;
      c -= r * 10000;
      g = c / 100;
      c -= g * 100;
      b = c;
      dst[0] = 255 * b / 100;
      dst[1] = 255 * g / 100;
      dst[2] = 255 * r / 100;
      dst += 3;
   }
   InvalidateRect(pf->hwnd, 0, FALSE);
   hdc = BeginPaint(pf->hwnd, &paint);
   StretchBlt(hdc, 0, 0, 300, 300, pf->bitmapHdc, 0, 0, 100, 100, SRCCOPY);
   QueryPerformanceCounter(&newTime);
   diff = (double)(newTime.QuadPart - env->platform->oldTime.QuadPart);
   diff /= env->platform->performanceCounterFrequency;
   diff *= 1000.0;
   sleepSec = 16 - (int)diff;
   if (sleepSec > 0){
      Sleep(sleepSec);
   }
   EndPaint(pf->hwnd, &paint);
   env->platform->oldTime = newTime;
}

#endif

