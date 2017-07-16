
#include <cstring>

namespace Sunaba{

//IoState
inline IoState::IoState() : 
mMemoryRequestAddress(0),
mMemoryRequestValue(0),
mMemoryRequestState(MEMORY_REQUEST_NONE),
mScreenSizeChanged(false),
//Machine->外
mMemoryCopy(0),
mMemorySize(0),
mIoOffset(0),
mVramOffset(0),
//外->Machine
mFrameCount(0),
mPointerX(0), 
mPointerY(0){
	for (int i = 0; i < KEY_COUNT; ++i){
		mKeys[i] = 0;
	}
	for (int i = 0; i < SOUND_CHANNEL_COUNT; ++i){
		mSoundRequests[i] = 0;
	}
}

inline IoState::~IoState(){
	DELETE_ARRAY(mMemoryCopy);
}

inline void IoState::allocateMemoryCopy(int memorySize, int ioOffset, int vramOffset){ 
	DELETE_ARRAY(mMemoryCopy);
	mMemoryCopy = new int[memorySize];
    std::memset(mMemoryCopy, 0, memorySize * sizeof(int));
	mMemorySize = memorySize;
	mIoOffset = ioOffset;
	mVramOffset = vramOffset;
}

inline void IoState::update(int px, int py, const char* keys){
	++mFrameCount;
	mPointerX = px;
	mPointerY = py;
	for (int i = 0; i < KEY_COUNT; ++i){
		mKeys[i] = keys[i];
	}
}

inline int IoState::frameCount() const{
	return mFrameCount;
}

inline const int* IoState::vramReadPointer() const{ 
	return mMemoryCopy + mVramOffset;
}

inline const int* IoState::memory() const{
	return mMemoryCopy;
}

inline void IoState::lock(){
	mLock.lock();
}

inline void IoState::unlock(){
	mLock.unlock();
}

} //namespace Sunaba
