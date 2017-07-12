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

#include <sstream> // for test

namespace Sunaba{

void writeToConsole(const char* s){
    printf( "%s", s );
}
    
    
    
void writeToConsole(const wchar_t* s){
    Sunaba::Array<char> strUtf8;
    int len = (int)wcslen( s );
    Sunaba::convertUnicodeToUtf8( &strUtf8, s, len );
    writeToConsole( strUtf8.pointer() );
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
    convertToUnicode( out, [concatStr UTF8String], lengthUtf8);
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
