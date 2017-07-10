#include "Base/Base.h"
#include "Base/Array.h"

namespace Sunaba{

template<class T> inline MemoryPool::AutoDestroyer<T>::AutoDestroyer(T* p) : mP(p){
}

template<class T> inline MemoryPool::AutoDestroyer<T>::~AutoDestroyer(){
	if (mP){
		mP->~T();
		mP = 0;
	}
}
template<class T> inline T* MemoryPool::AutoDestroyer<T>::get(){ 
	T* r = mP;
	mP = 0; //管理権委譲
	return r;
}

template<class T> inline T* MemoryPool::AutoDestroyer<T>::operator->(){
	return mP;
}

inline MemoryPool::MemoryPool(int blockSize) : 
mBlockSize(0),
mTail(0){
	mBlockSize = roundUp(blockSize); //4または8の倍数に切り上げ
}

inline MemoryPool::~MemoryPool(){
	clear();
}

///n個分確保
template<class T> inline T* MemoryPool::create(int n){
	T* t = static_cast<T*>(allocate(static_cast<int>(static_cast<int>(sizeof(T))) * n));
	for (int i = 0; i < n; ++i){
		new (t + i) T;
	}
	return t;
}

template<class T> inline void MemoryPool::destroy(T* a, int n){
	//TODO:メモリが範囲内かチェックできたらした方がいい。
	for (int i = 0; i < n; ++i){
		a[i].~T();
	}
}

inline void MemoryPool::clear(){
	Block* b = mTail;
	while (b){
		Block* prev = b->mPrev;
		b->mPrev = 0;
		DELETE(b);
		b = prev;
	}
	mTail = 0;
}

inline int MemoryPool::roundUp(int n){
	int a = static_cast<int>(sizeof(void*));
	return (n + a - 1) & ~(a - 1);
}

///nバイト確保
inline void* MemoryPool::allocate(int n){
	n = roundUp(n); //4または8の倍数に切り上げ
	if (n == 0){
		return 0;
	}
	if (!mTail || (n > (mTail->mMemory.size() - mTail->mUsingSize))){ //足りない
		//標準ブロックサイズで足りないなら必要数分だけアロケート
		int blockSize = (n > mBlockSize) ? n : mBlockSize;
		mTail = new Block(mTail, blockSize);
	}
	char* r = &mTail->mMemory[mTail->mUsingSize];
	mTail->mUsingSize += n;
	return r;
}

inline MemoryPool::Block::Block(Block* prevBlock, int size) : 
mPrev(prevBlock), 
mUsingSize(0),
mMemory(size){
}

} //namespace Sunaba
