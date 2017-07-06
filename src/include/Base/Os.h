#ifndef INCLUDED_SUNABA_OS_H
#define INCLUDED_SUNABA_OS_H

namespace Sunaba{

template<class T> class Array;

void writeToConsole(const wchar_t* s);

void beginTimer();
unsigned getTimeInMilliSecond();
void endTimer();
void sleepMilliSecond(int);
bool fileExist(const wchar_t* filename);

class Mutex{
public:
	Mutex();
	~Mutex();
	void lock();
	void unlock();
private:
	class Impl;
	Impl* mImpl;
};

class Event{
public:
	Event();
	~Event();
	void set();
	void reset();
	bool isSet();
private:
	class Impl;
	Impl* mImpl;
};

class Thread{
public:
	virtual void operator()() = 0;
	void start();
	void wait();
protected:
	Thread(bool timeCritical = false);
	virtual ~Thread();
private:
	class Impl;
	Impl* mImpl;
	bool mTimeCritical;
};

//コンストラクトと同時に全量読みます。
class InputFile{
public:
	InputFile(const wchar_t* filename);
	~InputFile();
	bool isError() const;
	const Array<unsigned char>* data() const;
private:
	class Impl;
	Impl* mImpl;
};

class OutputFile{
public:
	OutputFile(const wchar_t* filename, bool append = false);
	~OutputFile();
	bool isError() const;
	void write(const unsigned char* data, int size);
	void flush();
private:
	class Impl;
	Impl* mImpl;
};

} //namespace Sunaba

#endif
