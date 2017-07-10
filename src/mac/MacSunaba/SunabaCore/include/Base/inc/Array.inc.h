#include "Base/Base.h"

namespace Sunaba{

template<class T> inline Array<T>::Array() : mElements(0), mSize(0){
}
	
template<class T> inline Array<T>::Array(int size) : mElements(0), mSize(0){ 
	setSize(size); 
}

template<class T> inline Array<T>::Array(const Array& a) : mElements(0), mSize(0){ //コピーコンストラクタは空のときだけ許可
	ASSERT(a.mSize == 0);
	static_cast<void>(a); //releaseでは使いません。
}

template<class T> inline Array<T>::~Array(){ 
	clear();
}

template<class T> inline void Array<T>::clear(){
	DELETE_ARRAY(mElements);
	mSize = 0;
}

template<class T> inline void Array<T>::setSize(int size){
	if (size <= mSize){ //縮小は自由
		mSize = size;
	}else{
		ASSERT(!mElements);
		mElements = new T[size];
		mSize = size;
	}
}

template<class T> inline const T& Array<T>::operator[](int i) const{
	ASSERT((i >= 0) && (i < mSize));
	return mElements[ i ];
}

template<class T> inline T& Array<T>::operator[](int i){
	ASSERT((i >= 0) && (i < mSize));
	return mElements[i];
}

template<class T> inline const T* Array<T>::pointer() const{
	return mElements;
}

template<class T> inline T* Array<T>::pointer(){
	return mElements;
}

template<class T> inline int Array<T>::size() const{
	return mSize;
}

template<class T> inline void Array<T>::moveTo(Array<T>* a){
	a->clear();
	a->mElements = mElements;
	a->mSize = mSize;
	mElements = 0;
	mSize = 0;
}

} //namespace Sunaba

