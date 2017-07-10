#ifndef INCLUDED_SUNABA_BASE_TANK_H
#define INCLUDED_SUNABA_BASE_TANK_H

#include "Base/Array.h"

namespace Sunaba{

//数不定のものを溜めるだけ溜めてから配列に移すためのコンテナ
template<class T> class Tank{
public:
	explicit Tank(int blockSize = 1024);
	~Tank();
	void clear();
	int size() const;
	void add(const T* valueArray, int arraySize);
	void addString(const T*); //0終端
	void add(const T&);
	T* add();
	void copyTo(Array<T>*) const;
	void moveTo(Array<T>*) const;
private:
	struct Block{
		Block(int blockSize);
		~Block();

		Block* mNext;
		T* mElements;
		int mSize;
	};
	Block mHead;
	Block* mTail;
	int mSize;
	int mBlockSize;
};

} //namespace Sunaba

#include "Base/inc/Tank.inc.h"

#endif
