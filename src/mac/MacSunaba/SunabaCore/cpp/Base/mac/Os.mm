#include <stdio.h>

#include <cstdlib>
#include <locale.h>
#include <sstream>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <wchar.h>

#include "Base/Os.h"
#include "Base/Utility.h"

#include "Base/inc/sjisToUnicode.h"

#include <sstream> // for test

namespace Sunaba{

void writeToConsole(const wchar_t* s){
    Sunaba::Array<char> strUtf8;
    int len = (int)wcslen( s );
    Sunaba::convertUnicodeToUtf8( &strUtf8, s, len );
    writeToConsole( strUtf8.pointer() );
}

void writeToConsole(const char* s){
    printf( "%s", s );
}


void die(const char* filename, int line, const char* message){
	writeLog(filename, line, message);
#if 0
#ifdef SUNABA_USE_CLI
	System::Diagnostics::Debug::Assert(false);
#else
	if (gWindowHandle){
		MessageBoxA(gWindowHandle, s.c_str(), "致命的エラー", MB_OK | MB_ICONERROR);
	}
	::DebugBreak();
#endif
#endif
}

void writeLog(const char* filename, int line, const char* message){
	std::ostringstream oss;
	oss << filename << '(' << line << ')' << message << std::endl;
	std::string s = oss.str();

	writeLog(s.c_str());
}

void writeLog(const char* message){
	writeToConsole(message);
    Array<wchar_t> path;
    makeWorkingFilePath( &path, "SunabaSystemError.txt" );
	bool append = false;
	if (fileExist( path.pointer())){
		append = true;
	}
	OutputFile f( path.pointer(), true);
	if (!(f.isError())){
		if (append){
			const char* newLine = "----begin appending---\n";
			int l = (int)std::strlen(newLine);
			f.write(reinterpret_cast<const unsigned char*>(newLine), l);
		}
		int l = 0;
		while (message[l]){
			++l;
		}
		f.write(reinterpret_cast<const unsigned char*>(message), l);
	}
}
void beginTimer(){
	//timeBeginPeriod(1);
}

unsigned getTimeInMilliSecond(){
	//return timeGetTime();
    timeval t;
    gettimeofday( &t, NULL );
    
    uint64_t msec = t.tv_sec * 1000;
    msec += t.tv_usec / 1000;
    return static_cast<uint32_t>(msec);
}

void endTimer(){
	//timeEndPeriod(1);
}

void sleepMilliSecond(int t){
	//Sleep(t);
    usleep( t * 1000 );
}

void convertSjisToUnicode(Array<wchar_t>* out, const char* in, int inSize, bool outputWithBOM){
    int writtenCount = 0, readCount = 0;
    int bomOffset = (outputWithBOM ? 1 : 0 );
    out->setSize( inSize + 1 + bomOffset );
    if( bomOffset ) {
        (*out)[0] = 0xFFFEu;
    }
    
    ::convertSjisToUnicode( &writtenCount, &readCount, out->pointer() + bomOffset, out->size(), in, inSize );
    
    /* 適切な長さへ */
    out->setSize( writtenCount + bomOffset );
}

namespace Internal{

void convertUtf16LToUnicode(Array<wchar_t>* out, const char* in, int inSize, bool outputWithBOM){
	ASSERT((inSize & 1) == 0);
	out->setSize(inSize + 1);
	wchar_t* dst = out->pointer();
	const unsigned char* src = reinterpret_cast<const unsigned char*>(in);
	//BOMスキップ
	if ((inSize >= 2) && (src[0] == 0xff) && (src[1] == 0xfe)){
		src += 2;
		inSize -= 2;
	}
	int n = 0;
	if (outputWithBOM){
		dst[n] = static_cast<wchar_t>(0xfeff);
		++n;
	}
	for (int i = 0; i < inSize; i += 2){
		dst[n] = src[i + 0] | (src[i + 1] << 8);
		++n;
	}
	out->setSize(n);
}

void convertUtf16BToUnicode(Array<wchar_t>* out, const char* in, int inSize, bool outputWithBOM){
	ASSERT((inSize & 1) == 0);
	out->setSize(inSize + 1);
	wchar_t* dst = out->pointer();
	const unsigned char* src = reinterpret_cast<const unsigned char*>(in);
	//BOMスキップ
	if ((inSize >= 2) && (src[0] == 0xfe) && (src[1] == 0xff)){
		src += 2;
		inSize -= 2;
	}
	int n = 0;
	if (outputWithBOM){
		dst[n] = static_cast<wchar_t>(0xfeff);
		++n;
	}
	for (int i = 0; i < inSize; i += 2){
		dst[n] = (src[i + 0] << 8) | src[i + 1];
		++n;
	}
	out->setSize(n);
}

void convertUtf8ToUnicode(Array<wchar_t>* out, const char* in, int inSize, bool outputWithBOM){
	//TODO:サイズ見積り過大
	out->setSize(inSize + 2);

	const unsigned char* src = reinterpret_cast<const unsigned char*>(in);
	wchar_t* dst = out->pointer();
	//BOMスキップ
	if ((inSize >= 3) && (src[0] == 0xef) && (src[1] == 0xbb) && (src[2] == 0xbf)){
		src += 3;
		inSize -= 3;
	}
	int n = 0; //書き出しサイズ
	//BOM
	if (outputWithBOM){
		dst[n] = static_cast<wchar_t>(0xfeff);
		++n;
	}
	//充填
	int i = 0;
	while (i < inSize){ //書き込む
		if ((src[i] & 0x80) == 0){ //1バイト
			dst[n] = src[i];
			i += 1;
		}else if ((src[i] & 0xe0) == 0xc0){ //2バイト
			wchar_t code = 0;
			code |= ((src[i] & 0x1f) << 6); 
			code |= (src[i + 1] & 0x3f); 
			dst[n] = code;
			i += 2;
		}else if ((src[i] & 0xf0) == 0xe0){ //3バイト
			wchar_t code = 0;
			code |= ((src[i] & 0x0f) << 12); 
			code |= ((src[i + 1] & 0x3f) << 6); 
			code |= (src[i + 2] & 0x3f); 
			dst[n] = code;
			i += 3;
		}else{
			out->setSize(0);
			return;
		}
		++n;
	}
	out->setSize(n);
}

bool isUtf8(const char* inS, int size){
	const unsigned char* in = reinterpret_cast<const unsigned char*>(inS);
	//パターン合ってる？
	for (int i = 0; i < size; ++i){
		unsigned char c = in[i];
		if (c >= 0xf8){ //5,6バイト文字は不正
			return false;
		}else if (c >= 0xf0){ //4バイト文字
			if (
			((i + 3) >= size) ||
			(in[i + 1] < 0x80) || (in[i + 1] > 0xbf) ||
			(in[i + 2] < 0x80) || (in[i + 2] > 0xbf) ||
			(in[i + 3] < 0x80) || (in[i + 3] > 0xbf)){
				return false;
			}
			i += 3;
		}else if (c >= 0xe0){ //3バイト文字
			if (
			((i + 2) >= size) ||
			(in[i + 1] < 0x80) || (in[i + 1] > 0xbf) ||
			(in[i + 2] < 0x80) || (in[i + 2] > 0xbf)){
				return false;
			}
			i += 2;
		}else if (c >= 0xc2){ //2バイト文字
			if (
			((i + 1) >= size) ||
			(in[i + 1] < 0x80) || (in[i + 1] > 0xbf)){
				return false;
			}
			i += 1;
		}else if (c >= 0x80){ //ありえない
			return false;
		} //この他は1バイト
	}
	return true;
}

bool isUtf16L(const char* inS, int size){
	const unsigned char* in = reinterpret_cast<const unsigned char*>(inS);
	if (size & 0x1){ //奇数はありえぬ
		return false;
	}
	for (int i = 0; i < size; i += 2){
		unsigned char c0 = in[i + 0];
		unsigned char c1 = in[i + 1];
		unsigned short c = (c1 << 8) | c0;
		if (c == 0xfffe){ //反転BOMは予約されているからありえない
			return false;
		}else if ((c > 0xd7ff) && (c < 0xe000)){ //4バイト
			if ((i + 2) >= size){
				return false;
			}
			unsigned char c2 = in[i + 2];
			unsigned char c3 = in[i + 3];
			unsigned short cNext = (c3 << 8) | c2;
			if (((cNext & 0xd800) != 0xd800) || ((c & 0xdc00) != 0xdc00)){
				return false;
			}
		}
	}
	return true;
}

bool isUtf16B(const char* inS, int size){
	const unsigned char* in = reinterpret_cast<const unsigned char*>(inS);
	if (size & 0x1){ //奇数はありえぬ
		return false;
	}
	for (int i = 0; i < size; i += 2){
		unsigned char c0 = in[i + 0];
		unsigned char c1 = in[i + 1];
		unsigned short c = (c0 << 8) | c1;
		if (c == 0xfffe){ //反転BOMは予約されているからありえない
			return false;
		}else if ((c > 0xd7ff) && (c < 0xe000)){ //4バイト
			if ((i + 2) >= size){
				return false;
			}
			unsigned char c2 = in[i + 2];
			unsigned char c3 = in[i + 3];
			unsigned short cNext = (c2 << 8) | c3;
			if (((c & 0xd800) != 0xd800) || ((cNext & 0xdc00) != 0xdc00)){
				return false;
			}
		}
	}
	return true;
}

bool isSjis(const char* inS, int size){
	const unsigned char* in = reinterpret_cast<const unsigned char*>(inS);
	for (int i = 0; i < size; ++i){
		unsigned char c = in[i];
		if ( //2バイト
		((c >= 0x81) && (c <= 0x9f)) ||
		((c >= 0xe0) && (c <= 0xfc))){
			if ((i + 1) >= size){ //次の文字ない！
				return false;
			}
			c = in[i + 1];
			if (
			((c < 0x40) || (c > 0x7e)) &&
			((c < 0x80) || (c > 0xfc))){
				return false;
			}
			++i;
		}else if ((c == 0x80) || (c >= 0xfd)){ //ここは1バイトでも使ってない
			return false;
		}
	}
	return true;
}

//文字コード判別
enum CharCode{
	CHAR_CODE_SJIS,
	CHAR_CODE_UTF8,
	CHAR_CODE_UTF16L,
	CHAR_CODE_UTF16B,
	CHAR_CODE_UNKNOWN,
};

CharCode getBomType(const char* inS, int size){
	const unsigned char* in = reinterpret_cast<const unsigned char*>(inS);
	if ((size >= 3) && (in[0] == 0xef) && (in[1] == 0xbb) && (in[2] == 0xbf)){
		return CHAR_CODE_UTF8;
	}else if (size >= 2){
		if ((in[0] == 0xfe) && (in[1] == 0xff)){
			return CHAR_CODE_UTF16B;
		}else if ((in[0] == 0xff) & (in[1] == 0xfe)){
			return CHAR_CODE_UTF16L;
		}
	}
	return CHAR_CODE_UNKNOWN;
}

//TODO: 速度と正確性に難あり。
CharCode guessCharCode(const char* in, int size){
	//BOMで確定させる。BOMはSJISではありえるが、実際上ないから無視する。
	CharCode bom = getBomType(in, size);
	if (bom == CHAR_CODE_UTF8){
		return CHAR_CODE_UTF8;
	}else if (bom == CHAR_CODE_UTF16L){
		return CHAR_CODE_UTF16L;
	}else if (bom == CHAR_CODE_UTF16B){
		return CHAR_CODE_UTF16B;
	}
	bool utf8 = isUtf8(in, size);
	bool utf16l = isUtf16L(in, size);
	bool utf16b = isUtf16B(in, size);
	bool sjis = isSjis(in, size);
	if (utf8){ //一番厳しい
		return CHAR_CODE_UTF8;
	}else if (sjis){
		return CHAR_CODE_SJIS;
	}else if (utf16l){
		return CHAR_CODE_UTF16L;
	}else if (utf16b){
		return CHAR_CODE_UTF16B;
	}else{
		return CHAR_CODE_UNKNOWN;
	}
}

} //namespace Internal

void convertToUnicode(Array<wchar_t>* out, const char* in, int inSize, bool outputWithBOM){
	using namespace Internal;
	CharCode cc = guessCharCode(in, inSize);
	if (cc == CHAR_CODE_SJIS){
		convertSjisToUnicode(out, in, inSize, outputWithBOM);
		if (out->size() == 0){ //失敗した！コード判定しくじったな！仕方ないのでutf8とみなす
			convertUtf8ToUnicode(out, in, inSize, outputWithBOM);
		}
	}else if (cc == CHAR_CODE_UTF8){
		convertUtf8ToUnicode(out, in, inSize, outputWithBOM);
	}else if (cc == CHAR_CODE_UTF16L){
		convertUtf16LToUnicode(out, in, inSize, outputWithBOM);
	}else if (cc == CHAR_CODE_UTF16B){
		convertUtf16BToUnicode(out, in, inSize, outputWithBOM);
	}else{
		convertUtf8ToUnicode(out, in, inSize, outputWithBOM); //とりあえずUTF-8とみなす
	}
}

void convertUnicodeToSjis(Array<char>* out, const wchar_t* in, int inSize){
#if 0
	//wcstombsがNULL終端を必要とするので、念のためコピー。この時BOMを除いておく。
	Array<wchar_t> tmpIn;
	if (in[0] == 0xfeff){ //BOMあり
		tmpIn.setSize(inSize + 1 - 1);
		memcpy(tmpIn.pointer(), in + 1, (inSize - 1) * sizeof(wchar_t));
	}else{
		tmpIn.setSize(inSize + 1);
		memcpy(tmpIn.pointer(), in, inSize * sizeof(wchar_t));
	}
	tmpIn[tmpIn.size() - 1] = L'¥0';

	setlocale(LC_ALL, "");
	size_t convertedLength;
	int dstBufferSize = (inSize * 2) + 1; //全部2バイト化して、さらに終端、というのが最大
	int error = 0;
	out->setSize(dstBufferSize);
	error = wcstombs_s( 
		&convertedLength,
		out->pointer(),
		dstBufferSize,
		tmpIn.pointer(),
		_TRUNCATE);
	if (error != 0){
		out->clear();
	}else{
		//正確な長さに
		out->setSize(convertedLength - 1); //NULLを含むので1小さく
	}
#else
    die(__FILE__, __LINE__, "TODO: @convertUnicodeToSjis.\n" );
#endif
}

void convertUnicodeToUtf8( Array<char>* out, const wchar_t* in, int inSize )
{
    // Sunabaは内部が wchar_t に格納された UTF16 で動いている.
    // そのため、ここでは UTF16 , BOM なし でという条件で変換を行う.
    int utf8Length = 0;
    int instrCount = 0;
    const wchar_t* wc = in;
    /* UTF8としての長さをカウント. */
    while( *wc != '\0' && instrCount < inSize ) {
        uint32_t code = static_cast<uint32_t>( *wc );
        if( code < 0x80 ) {
            utf8Length += 1; // ASCII
        } else if( code < 0x800 ) {
            utf8Length += 2;
        } else {
            if( !(code < 0xD800u || code >= 0xE000u) ) {
                /* ここは未サポート */
                printf( "Illegal.\n" );
            }
            utf8Length += 3;
        }
        ++instrCount;
        wc++;
    }

    /* UTF8を格納 */
    out->setSize( utf8Length+1 );
    wc = in;
    (*out)[ utf8Length ] = '\0';
    int n = 0;
    instrCount = 0;
    while( *wc != '\0' && instrCount < inSize ) {
        uint32_t code = static_cast<uint32_t>( *wc );
        if( code < 0x80u ) {
            (*out)[n++] = static_cast<char>( code & 0x7Fu );
        } else if( code < 0x800 ) {
            (*out)[n++] = static_cast<char>( 0xC0u | ((code & 0x7C0u) >> 6) );
            (*out)[n++] = static_cast<char>( 0x80u | (code & 0x3Fu) );
        } else {
            if( !(code < 0xD800u || code >= 0xE000u) ) {
                printf( "Illegal.\n" );
            }
            (*out)[n++] = static_cast<char>( 0xE0u | ((code & 0xF000u) >> 12));
            (*out)[n++] = static_cast<char>( 0x80u | ((code & 0xFC0u) >> 6 ));
            (*out)[n++] = static_cast<char>( 0x80u | ( code & 0x3fu ) );
        }
        wc++;
        instrCount++;
    }
    (*out)[n] = '\0';
}
    
void getWorkingDirectory( char* buf )
{
    if( buf ) {
        NSString* path = NSHomeDirectory();
        strcpy( buf, [path UTF8String ] );
    }
}
void makeWorkingFilePath( Array<wchar_t>* out, const char* fileName )
{
    NSString* path = NSHomeDirectory();
    NSString* file = [[NSString alloc] initWithUTF8String: fileName];
    NSString* concatStr = [NSString stringWithFormat: @"%@/%@", path, file ];
    
    int lengthUtf8 = (int)[concatStr lengthOfBytesUsingEncoding: NSUTF8StringEncoding ];
    lengthUtf8 += 1; // for NULL
    int lengthUtf16 = (int)[concatStr lengthOfBytesUsingEncoding: NSUTF16StringEncoding ];
    lengthUtf16 += 1;
    out->setSize( lengthUtf16 );
    convertToUnicode( out, [concatStr UTF8String], lengthUtf8, false );
}
    
//Mutex
class Mutex::Impl{
public:
	// CRITICAL_SECTION mCs;
    pthread_mutex_t mCs;
};

Mutex::Mutex(){
	mImpl = new Impl;
	//InitializeCriticalSection(&(mImpl->mCs));
    pthread_mutex_init( &(mImpl->mCs), NULL );
}

Mutex::~Mutex(){
	DELETE(mImpl);
}

void Mutex::lock(){
	//EnterCriticalSection(&(mImpl->mCs));
    pthread_mutex_lock( &(mImpl->mCs));
}

void Mutex::unlock(){
	//LeaveCriticalSection(&(mImpl->mCs));
    pthread_mutex_unlock( &(mImpl->mCs));
}

//Event
class Event::Impl{
public:
	// HANDLE mEvent;
    pthread_mutex_t mLock;
    pthread_cond_t  mEvent;
    bool mIsSignal;
};

Event::Event(){
	mImpl = new Impl;
	// mImpl->mEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    pthread_cond_init( &(mImpl->mEvent), NULL );
    pthread_mutex_init( &(mImpl->mLock), NULL );
    mImpl->mIsSignal = false;
}

Event::~Event(){
	ASSERT(mImpl);
	//CloseHandle(mImpl->mEvent);
    pthread_cond_destroy( &(mImpl->mEvent));
    pthread_mutex_destroy( &(mImpl->mLock));
	DELETE(mImpl);
}

void Event::set(){
	ASSERT(mImpl);
	// SetEvent(mImpl->mEvent);
    pthread_mutex_lock( &(mImpl->mLock));
    pthread_cond_signal( &(mImpl->mEvent));
    mImpl->mIsSignal = true;
    pthread_mutex_unlock( &(mImpl->mLock));
}

void Event::reset(){
	ASSERT(mImpl);
	//ResetEvent(mImpl->mEvent);
    pthread_mutex_lock( &(mImpl->mLock));
    mImpl->mIsSignal = false;
    pthread_mutex_unlock( &(mImpl->mLock));
}

bool Event::isSet(){
	ASSERT(mImpl);
    pthread_mutex_lock( &(mImpl->mLock) );
    bool bRet = mImpl->mIsSignal;
    pthread_mutex_unlock( &(mImpl->mLock));
	//return (WaitForSingleObject(mImpl->mEvent, 0) == WAIT_OBJECT_0);
    return bRet;
}

//Thread
class Thread::Impl{
public:
	//static unsigned __stdcall function(void* arg){
    static void* __cdecl function(void* arg){
		Thread* t = static_cast<Thread*>(arg);
		if (t->mTimeCritical){
			// SetThreadPriority(t->mImpl->mThread, THREAD_PRIORITY_TIME_CRITICAL);
		}
		(*t)();
		return 0;
	}
	// HANDLE mThread;
    pthread_t mThread;
};

Thread::Thread(bool timeCritical) : mImpl(0), mTimeCritical(timeCritical){
}

Thread::~Thread(){
	ASSERT(!mImpl && "you must call wait().");
}

void Thread::start(){
	mImpl = new Impl;
//	mImpl->mThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, Impl::function, this, 0, NULL));
    pthread_create( &(mImpl->mThread), NULL, &(Thread::Impl::function), (void*)this );
}

void Thread::wait(){
	ASSERT(mImpl);
    pthread_join( mImpl->mThread, NULL );
	// WaitForSingleObject(mImpl->mThread, INFINITE);
	//CloseHandle(mImpl->mThread);
	DELETE(mImpl);
}


bool fileExist(const wchar_t* filename){
    int len = (int)wcslen( filename ) + 1;
    Array<char> fileUtf8;
    convertUnicodeToUtf8( &fileUtf8, filename, len );
    
    FILE* fp = fopen( fileUtf8.pointer(), "rb" );
    if( fp != NULL ) {
        fclose( fp );
    }
    return (fp != NULL);
}

//InputFile
class InputFile::Impl{
public:
	Array<unsigned char> mData;
};

InputFile::InputFile(const wchar_t* filename) : mImpl(0){
//InputFile::InputFile(const char* filename) : mImpl(0){
#if 0
	HANDLE h = CreateFileW(
		filename, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (h == INVALID_HANDLE_VALUE){
		char buf[1024];
		FormatMessageA( 
			FORMAT_MESSAGE_FROM_SYSTEM |  FORMAT_MESSAGE_IGNORE_INSERTS, 
			NULL, 
			GetLastError(), 
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
			buf, 
			1024, 
			NULL);
		writeLog("FileOpenError( ");
		Array<char> sjisFilename;
		convertUnicodeToSjis(&sjisFilename, filename, getStringSize(filename));
		writeLog(sjisFilename.pointer());
		writeLog(" ) ");
		writeLog(buf);
	}else{
		//読み取り
		DWORD fileSize = GetFileSize(h, NULL);
		Array<unsigned char> tmp(fileSize);
		DWORD readSize;
		BOOL r = ReadFile(h, tmp.pointer(), fileSize, &readSize, NULL);
		if ((r != 0) && (readSize == fileSize)){ //読めた
			mImpl = new Impl;
			tmp.moveTo(&(mImpl->mData));
		}else{
			writeLog("read error.");
		}
		CloseHandle(h); //ファイルは閉じる
	}
#else
    Array<char> utf8FileName;
    convertUnicodeToUtf8( &utf8FileName, filename, getStringSize(filename) );
    
    FILE* fp = fopen( utf8FileName.pointer(), "rb" );
    if( fp == NULL ) {
#if 0
		char buf[1024];
		FormatMessageA(
                       FORMAT_MESSAGE_FROM_SYSTEM |  FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       GetLastError(),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       buf,
                       1024,
                       NULL);
		writeLog("FileOpenError( ");
		Array<char> sjisFilename;
		convertUnicodeToSjis(&sjisFilename, filename, getStringSize(filename));
		writeLog(sjisFilename.pointer());
		writeLog(" ) ");
		writeLog(buf);
#else
        writeLog( "FileOpenError( " );
        writeLog( utf8FileName.pointer() );
        writeLog( " ) " );
#endif
    } else {
        fseek( fp, 0, SEEK_END );
        unsigned int fileSize = (unsigned int)ftell(fp);
        rewind( fp );
        Array<unsigned char> tmp(fileSize );
        unsigned int readSize = 0;
        readSize = (unsigned int)fread( tmp.pointer(), 1, fileSize, fp );
        if( readSize != 0 && readSize == fileSize ) {
            mImpl = new Impl;
            tmp.moveTo( &(mImpl->mData));
        } else {
            writeLog( "read error." );
        }
        
        fclose( fp );
        fp = NULL;
    }
#endif
}

InputFile::~InputFile(){
	DELETE(mImpl);
}

bool InputFile::isError() const{
	return (mImpl == 0);
}

const Array<unsigned char>* InputFile::data() const{
	if (!mImpl){
		return 0;
	}else{
		return &(mImpl->mData);
	}
}

//OutputFile
class OutputFile::Impl{
public:
	~Impl(){
		//CloseHandle(mHandle);
		//mHandle = 0;
        fclose( mHandle );
        mHandle = NULL;
	}
	// HANDLE mHandle;
    FILE* mHandle;
};

OutputFile::OutputFile(const wchar_t*  filename, bool append) : mImpl(0){
#if 0
	HANDLE h = CreateFileW(
		filename, 
		GENERIC_WRITE, 
		FILE_SHARE_DELETE | FILE_SHARE_READ, //消すことと読むことは許可する。
		NULL, 
		(append) ? OPEN_ALWAYS : CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (h != INVALID_HANDLE_VALUE){
		bool error = false;
		if (append){
			DWORD ret = SetFilePointer(h, 0, NULL, FILE_END);
			if (ret == INVALID_SET_FILE_POINTER){
				error = true;
			}
		}
		if (!error){
			mImpl = new Impl;
			mImpl->mHandle = h;
		}
	}
#else
    const char* attr = (append) ? "wb+" : "wb";
    Array<char> utf8FileName;
    convertUnicodeToUtf8( &utf8FileName, filename, getStringSize(filename) );
    FILE* fp = fopen( utf8FileName.pointer(), attr );
    if( fp != NULL ) {
        bool error = false;
        if( append ) {
            fseek(fp, 0, SEEK_END );
        }
        if( !error ) {
            mImpl = new Impl;
            mImpl->mHandle = fp;
        }
    }
#endif
}

OutputFile::~OutputFile(){
	DELETE(mImpl);
}

bool OutputFile::isError() const{
	return (mImpl == 0);
}

void OutputFile::write(const unsigned char* data, int size){
	if (!mImpl){
		return;
	}
	unsigned int writeSize;
#if 0
	BOOL r = WriteFile(mImpl->mHandle, data, size, &writeSize, NULL);
#else
    writeSize = (unsigned int)fwrite( data, 1, size, mImpl->mHandle );
#endif
	if ((static_cast<int>(writeSize) != size)){
		DELETE(mImpl);
	}
}

void OutputFile::flush(){
	if (!mImpl){
		return;
	}
#if 0
	BOOL r = FlushFileBuffers(mImpl->mHandle);
	if (r == 0){ 
		DELETE(mImpl);
	}
#else
    fflush( mImpl->mHandle);
#endif
}

} //namespace Sunaba
