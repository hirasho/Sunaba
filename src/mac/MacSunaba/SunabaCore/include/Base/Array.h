#ifndef INCLUDED_SUNABA_BASE_ARRAY_H
#define INCLUDED_SUNABA_BASE_ARRAY_H

namespace Sunaba{

template<class T> class Array{
public:
	Array();
	explicit Array(int size);
	explicit Array(const Array&);
	~Array();
	void clear();
	void setSize(int size);
	const T& operator[](int i) const;
	T& operator[](int i);
	const T* pointer() const;
	T* pointer();
	int size() const;
	void moveTo(Array<T>*);
private:
	void operator=(const Array&); //代入禁止

	T* mElements;
	int mSize;
};

} //namespace Sunaba

#include "Base/inc/Array.inc.h"

#endif
