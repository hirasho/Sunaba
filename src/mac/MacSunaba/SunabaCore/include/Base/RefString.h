#ifndef INCLUDED_SUNABA_BASE_REFSTRING_H
#define INCLUDED_SUNABA_BASE_REFSTRING_H

namespace Sunaba{

//NULL終端を保証しない！参照先が解放されるとおかしくなる！
class RefString{
public:
	RefString();
#ifndef __APPLE__
	explicit RefString(const RefString&);
#else
	RefString(const RefString&);
#endif
	explicit RefString(const wchar_t*); //NULL終端
	explicit RefString(const wchar_t* str, int size);
	void operator=(const wchar_t*); //NULL終端
	void operator=(const RefString&);
	void set(const wchar_t* str, int size);
	bool operator!=(const RefString&) const;
	bool operator!=(const wchar_t*) const;
	bool operator==(const RefString&) const;
	bool operator==(const wchar_t*) const;
	bool operator<(const RefString&) const;
	wchar_t operator[](int i) const;
	const wchar_t* pointer() const;
	int size() const;
private:
	const wchar_t* mString;
	int mSize;
};

} //namespace Sunaba

#include "Base/inc/RefString.inc.h"

#endif
