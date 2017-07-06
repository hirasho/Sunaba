#ifndef INCLUDED_SUNABA_BASE_TEXTFILE_H
#define INCLUDED_SUNABA_BASE_TEXTFILE_H

namespace Sunaba{
template<class T> class Array;
class OutputFile;

class InputTextFile{
public:
	InputTextFile(const wchar_t* filename);
	bool isError() const;
	const Array<wchar_t>* text() const;
private:
	Array<wchar_t> mText;
};

class OutputTextFile{
public:
	OutputTextFile(const wchar_t* filename);
	~OutputTextFile();
	bool isError() const;
	void write(const wchar_t* text, int size);
	void flush();
private:
	OutputFile* mFile;
};

} //namespace Sunaba

#include "Base/inc/TextFile.inc.h"

#endif
