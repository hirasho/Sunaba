#ifndef INCLUDED_SUNABA_COMPILER_FUNCTIONINFO_H
#define INCLUDED_SUNABA_COMPILER_FUNCTIONINFO_H

namespace Sunaba{

class FunctionInfo{	
public:
	FunctionInfo();
	void setHasOutputValue();
	int argCount() const;
	void setArgCount(int);
	bool hasOutputValue() const;
private:
	int mArgCount;
	bool mHasOutputValue;
};

} //namespace Sunaba

#include "Compiler/inc/FunctionInfo.inc.h"

#endif