#ifndef INCLUDED_SUNABA_BASE_MEMORYPOOL_H
#define INCLUDED_SUNABA_BASE_MEMORYPOOL_H

#include "Base/Array.h"

namespace Sunaba{

class MemoryPool{
public:
	//自動デストラクタ呼び機
	template<class T> class AutoDestroyer{
	public:
		explicit AutoDestroyer(T* p);
		~AutoDestroyer();
		T* get();
		T* operator->();
	private:
		T* mP;
	};
	explicit MemoryPool(int blockSize);
	~MemoryPool();
	///n個分確保
	template<class T> inline T* create(int n = 1);
	template<class T> inline void destroy(T* a, int n = 1);
	void clear();
private:
	int roundUp(int n);
	///nバイト確保
	void* allocate(int n);
	struct Block{
		Block(Block* prevBlock, int size);

		Block* mPrev; //次のブロック
		int mUsingSize; //使われた数
		Array<char> mMemory;
	};
	void operator=(const MemoryPool&); //代入禁止
	MemoryPool(const MemoryPool&); //コピーコンストラクタ禁止

	int mBlockSize; //単位量
	Block* mTail;
};

} //namespace Sunaba

#include "Base/inc/MemoryPool.inc.h"

#endif
