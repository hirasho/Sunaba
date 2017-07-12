#include "Base/Base.h"
#include "Base/Utility.h"

namespace Sunaba{

inline String::String(){
}

inline String::String(const String& a) : mString(a.mString){
}

inline String::String(const wchar_t* str){
	set(str);
}

inline String::String(const wchar_t* str, int size){
	set(str, size);
}

inline void String::set(const wchar_t* str){
	int size = (str) ? getStringSize(str) : 0;
	set(str, size);
}

inline void String::set(const wchar_t* str, int size){
	//自分からのセットに備えてテンポラリへ一旦格納
	Array<wchar_t> tmp(size + 1); //NULL
	for (int i = 0 ; i < size; ++i){
		tmp[i] = str[i];
	}
	tmp[size] = L'\0';
	//コピー後に移動
	tmp.moveTo(&mString);
}

inline bool String::operator!=(const String& a) const{
	return !operator==(a);
}

inline bool String::operator!=(const wchar_t* a) const{
	return !operator==(a);
}

inline bool String::operator==(const String& a) const{
	return isEqualString(mString.pointer(), mString.size(), a.mString.pointer(), a.mString.size());
}

inline bool String::operator==(const wchar_t* a) const{
	return isEqualString(mString.pointer(), mString.size(), a);
}

inline bool String::operator<(const String& a) const{
	return isLessString(mString.pointer(), mString.size(), a.mString.pointer(), a.mString.size());
}

inline wchar_t String::operator[](int i) const{
	return mString[i];
}

inline wchar_t& String::operator[](int i){
	return mString[i];
}

inline const wchar_t* String::pointer() const{
	return mString.pointer();
}

inline int String::size() const{
	return mString.size() - 1; //NULL終端分減らす
}

inline void String::moveTo(String* a){
	mString.moveTo(&(a->mString));
}

inline bool StringPointerLess::operator()(const String* a, const String* b) const{
	return (*a < *b);
}

} //namespace Sunaba
