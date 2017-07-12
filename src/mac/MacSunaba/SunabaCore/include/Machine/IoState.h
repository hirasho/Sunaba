#ifndef INCLUDED_SUNABA_MACHINE_IOSTATE_H
#define INCLUDED_SUNABA_MACHINE_IOSTATE_H

namespace Sunaba{

class IoState{
public:
	enum{
		SOUND_CHANNEL_COUNT = 3,
	};
	enum Key{
		KEY_UNKNOWN = -1,
		KEY_LBUTTON = 0,
		KEY_RBUTTON,
		KEY_UP,
		KEY_DOWN,
		KEY_LEFT,
		KEY_RIGHT,
		KEY_SPACE,
		KEY_ENTER,

		KEY_COUNT,
	};
	//メモリデバグ
	enum MemoryRequestState{
		MEMORY_REQUEST_NONE,
		MEMORY_REQUEST_WRITE,
		MEMORY_REQUEST_WAITING,
	};
	IoState();
	~IoState();
	void allocateMemoryCopy(int memorySize, int ioOffset, int vramOffset);
	//外から1フレームに1回呼ぶ関数。以下lock()中だけ呼ぼうね
	void update(int pointerX, int pointerY, const char* keys);
	int frameCount() const;
	const int* vramReadPointer() const;
	const int* memory() const;

	//開発用
	int mMemoryRequestAddress;
	int mMemoryRequestValue;
	MemoryRequestState mMemoryRequestState;
	bool mScreenSizeChanged;
	int mSoundRequests[SOUND_CHANNEL_COUNT];
private:
	friend class Machine;
	//Machineからしか呼ばない
	void lock();
	void unlock();

	Mutex mLock;

	//Machine->外
	int* mMemoryCopy;
	int mMemorySize;
	int mIoOffset;
	int mVramOffset;

	//外->Machine
	int mFrameCount;
	int mPointerX;
	int mPointerY;
	char mKeys[KEY_COUNT];
};

} //namespace Sunaba

#include "Machine/inc/IoState.inc.h"

#endif
