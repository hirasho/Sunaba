#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <Shlwapi.h>
#include <MMSystem.h>
#undef DELETE
#include <cstdlib>
#include <locale.h>
#include <sstream>
#include <cstring>

#include "Base/Os.h"
#include "Base/Utility.h"

#ifndef SUNABA_USE_CLI
extern HWND gWindowHandle;
#endif

namespace Sunaba{

void writeToConsole(const wchar_t* s){
#ifdef SUNABA_USE_CLI
	System::String^ refS = gcnew System::String(s);
	System::Diagnostics::Debug::Write(refS);
#else
	OutputDebugStringW(s);
#endif
}

void writeToConsole(const char* s){
	Array<wchar_t> converted;
	convertSjisToUnicode(&converted, s, std::strlen(s));
	writeToConsole(converted.pointer());
}

void die(const char* filename, int line, const char* message){
	writeLog(filename, line, message);

#ifdef SUNABA_USE_CLI
	System::Diagnostics::Debug::Assert(false);
#else
	if (gWindowHandle){
		MessageBoxA(gWindowHandle, s.c_str(), "致命的エラー", MB_OK | MB_ICONERROR);
	}
	::DebugBreak();
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
	bool append = false;
	if (fileExist(L"SunabaSystemError.txt")){
		append = true;
	}
	OutputFile f(L"SunabaSystemError.txt", true);
	if (!(f.isError())){
		if (append){
			const char* newLine = "----begin appending---\n";
			int l = std::strlen(newLine);
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
	timeBeginPeriod(1);
}

unsigned getTimeInMilliSecond(){
	return timeGetTime();
}

void endTimer(){
	timeEndPeriod(1);
}

void sleepMilliSecond(int t){
	Sleep(t);
}

void convertUnicodeToSjis(Array<char>* out, const wchar_t* in, int inSize){
	//wcstombsがNULL終端を必要とするので、念のためコピー。この時BOMを除いておく。
	Array<wchar_t> tmpIn;
	if (in[0] == 0xfeff){ //BOMあり
		tmpIn.setSize(inSize + 1 - 1);
		memcpy(tmpIn.pointer(), in + 1, (inSize - 1) * sizeof(wchar_t));
	}else{
		tmpIn.setSize(inSize + 1);
		memcpy(tmpIn.pointer(), in, inSize * sizeof(wchar_t));
	}
	tmpIn[tmpIn.size() - 1] = L'\0';

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
}

//Mutex
class Mutex::Impl{
public:
	CRITICAL_SECTION mCs;
};

Mutex::Mutex(){
	mImpl = new Impl;
	InitializeCriticalSection(&(mImpl->mCs));
}

Mutex::~Mutex(){
	DELETE(mImpl);
}

void Mutex::lock(){
	EnterCriticalSection(&(mImpl->mCs));
}

void Mutex::unlock(){
	LeaveCriticalSection(&(mImpl->mCs));
}

//Event
class Event::Impl{
public:
	HANDLE mEvent;
};

Event::Event(){
	mImpl = new Impl;
	mImpl->mEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

Event::~Event(){
	ASSERT(mImpl);
	CloseHandle(mImpl->mEvent);
	DELETE(mImpl);
}

void Event::set(){
	ASSERT(mImpl);
	SetEvent(mImpl->mEvent);
}

void Event::reset(){
	ASSERT(mImpl);
	ResetEvent(mImpl->mEvent);
}

bool Event::isSet(){
	ASSERT(mImpl);
	return (WaitForSingleObject(mImpl->mEvent, 0) == WAIT_OBJECT_0);
}

//Thread
class Thread::Impl{
public:
	static unsigned __stdcall function(void* arg){
		Thread* t = static_cast<Thread*>(arg);
		if (t->mTimeCritical){
			SetThreadPriority(t->mImpl->mThread, THREAD_PRIORITY_TIME_CRITICAL);
		}
		(*t)();
		return 0;
	}
	HANDLE mThread;
};

Thread::Thread(bool timeCritical) : mImpl(0), mTimeCritical(timeCritical){
}

Thread::~Thread(){
	ASSERT(!mImpl && "you must call wait().");
}

void Thread::start(){
	mImpl = new Impl;
	mImpl->mThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, Impl::function, this, 0, NULL));
}

void Thread::wait(){
	ASSERT(mImpl);
	WaitForSingleObject(mImpl->mThread, INFINITE);
	CloseHandle(mImpl->mThread);
	DELETE(mImpl);
}


bool fileExist(const wchar_t* filename){
	BOOL ret = PathFileExistsW(filename);
	return (ret == TRUE);
}

//InputFile
class InputFile::Impl{
public:
	Array<unsigned char> mData;
};

InputFile::InputFile(const wchar_t* filename) : mImpl(0){
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
		CloseHandle(mHandle);
		mHandle = 0;
	}
	HANDLE mHandle;
};

OutputFile::OutputFile(const wchar_t* filename, bool append) : mImpl(0){
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
	DWORD writeSize;
	BOOL r = WriteFile(mImpl->mHandle, data, size, &writeSize, NULL);
	if ((r == 0) || (static_cast<int>(writeSize) != size)){
		DELETE(mImpl);
	}
}

void OutputFile::flush(){
	if (!mImpl){
		return;
	}
	BOOL r = FlushFileBuffers(mImpl->mHandle);
	if (r == 0){ 
		DELETE(mImpl);
	}
}

} //namespace Sunaba
