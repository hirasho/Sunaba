#ifndef INCLUDED_SUNABA_BASE_STRING_H
#define INCLUDED_SUNABA_BASE_STRING_H

#include "Base/Array.h"

namespace Sunaba{

class String{
public:
	String();
	explicit String(const String&);
	explicit String(const wchar_t*);
	String(const wchar_t* str, int size);
	void set(const wchar_t* str);
	void set(const wchar_t* str, int size);
	bool operator!=(const String&) const;
	bool operator!=(const wchar_t*) const;
	bool operator==(const String&) const;
	bool operator==(const wchar_t*) const;
	bool operator<(const String&) const;
	wchar_t operator[](int i) const;
	wchar_t& operator[](int i);
	const wchar_t* pointer() const;
	int size() const;
	void moveTo(String*);
private:
	void operator=(String);

	Array<wchar_t> mString;
};

class StringPointerLess{
public:
	bool operator()(const String*, const String*) const;
};

} //namespace Sunaba

#include "Base/inc/String.inc.h"

#endif
