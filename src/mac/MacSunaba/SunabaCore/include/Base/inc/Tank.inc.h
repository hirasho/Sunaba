#include "Base/Base.h"
#include "Base/Array.h"

namespace Sunaba{

template<class T> inline Tank<T>::Tank(int blockSize) :
mHead(blockSize),
mTail(&mHead), 
mSize(0), 
mBlockSize(blockSize){
}

template<class T> inline Tank<T>::~Tank(){
	clear();
}

template<class T> inline void Tank<T>::clear(){
	//headブロックはかってに消える。
	Block* b = mHead.mNext;
	while (b){
		Block* next = b->mNext;
		DELETE(b);
		b = next;
	}
	mSize = 0;
	mTail = &mHead;
	//headブロックはサイズを0にして、次のポインタを0にする
	mHead.mNext = 0;
	mHead.mSize = 0;
}

template<class T> inline int Tank<T>::size() const{
	return mSize;
}

template<class T> inline void Tank<T>::add(const T* valueArray, int arraySize){ //TODO:最適化できる
	for (int i = 0; i < arraySize; ++i){
		add(valueArray[i]);
	}
}

template<class T> inline void Tank<T>::addString(const T* s){ 
	int i = 0;
	while (s[i] != 0){
		add(s[i]);
		++i;
	}
}

template<class T> inline void Tank<T>::add(const T& value){
	if (mTail->mSize == mBlockSize){
		mTail->mNext = new Block(mBlockSize);
		mTail = mTail->mNext;
	}
	mTail->mElements[mTail->mSize] = value;
	++(mTail->mSize);
	++mSize;
}

template<class T> inline T* Tank<T>::add(){
	if (mTail->mSize == mBlockSize){
		mTail->mNext = new Block(mBlockSize);
		mTail = mTail->mNext;
	}
	T* r = mTail->mElements + mTail->mSize;
	new (r) T;
	++(mTail->mSize);
	++mSize;
	return r;
}

template<class T> inline void Tank<T>::copyTo(Array<T>* o) const{
	o->setSize(mSize);
	const Block* b = &mHead;
	int j = 0;
	while (b){
		for (int i = 0; i < b->mSize; ++i){
			(*o)[j] = b->mElements[i];
			++j;
		}
		b = b->mNext;
	}
}

template<class T> inline void Tank<T>::moveTo(Array<T>* o) const{
	o->setSize(mSize);
	const Block* b = &mHead;
	int j = 0;
	while (b){
		for (int i = 0; i < b->mSize; ++i){
			(b->mElements[i]).moveTo(&((*o)[j]));
			++j;
		}
		b = b->mNext;
	}
}

template<class T> inline Tank<T>::Block::Block(int blockSize) : mNext(0), mElements(0), mSize(0){
	mElements = new T[blockSize];
}

template<class T> inline Tank<T>::Block::~Block(){
	DELETE_ARRAY(mElements);
}

} //namespace Sunaba