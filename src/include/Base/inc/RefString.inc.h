#include "Base/Base.h"
#include "Base/Utility.h"

namespace Sunaba{

inline RefString::RefString() : mString(0), mSize(0){
}

inline RefString::RefString(const RefString& a) : mString(a.mString), mSize(a.mSize){
}

inline RefString::RefString(const wchar_t* str) : mString(str){
	if (str){
		mSize = getStringSize(str);
	}else{
		mSize = 0;
	}
}

inline RefString::RefString(const wchar_t* str, int size) : mString(str), mSize(size){
}

inline void RefString::set(const wchar_t* str, int size){
	mString = str;
	mSize = size;
}

inline void RefString::operator=(const wchar_t* str){
	mString = str;
	if (str){
		mSize = getStringSize(str);
	}else{
		mSize = 0;
	}
}

inline void RefString::operator=(const RefString& a){
	mString = a.mString;
	mSize = a.mSize;
}

inline bool RefString::operator!=(const RefString& a) const{
	return !operator==(a);
}

inline bool RefString::operator!=(const wchar_t* a) const{
	return !operator==(a);
}

inline bool RefString::operator==(const RefString& a) const{
	return isEqualString(mString, mSize, a.mString, a.mSize);
}

inline bool RefString::operator==(const wchar_t* a) const{
	return isEqualString(mString, mSize, a);
}

inline bool RefString::operator<(const RefString& a) const{
	return isLessString(mString, mSize, a.mString, a.mSize);
}

inline wchar_t RefString::operator[](int i) const{
	ASSERT((i >= 0) && (i < mSize));
	return mString[i];
}

inline const wchar_t* RefString::pointer() const{
	return mString;
}

inline int RefString::size() const{
	return mSize;
}

} //namespace Sunaba