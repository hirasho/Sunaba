#ifndef INCLUDED_SUNABA_LOCALIZATION_H
#define INCLUDED_SUNABA_LOCALIZATION_H

#include "Base/Utility.h"

namespace Sunaba{

enum ErrorName{
	//結合
	ERROR_CONCATENATOR_CANT_OPEN_FILE,
	ERROR_CONCATENATOR_CANT_READ_TEXT,
	ERROR_CONCATENATOR_INCOMPLETE_INCLUDE,
	ERROR_CONCATENATOR_INVALID_TOKEN_AFTER_INCLUDE,
	ERROR_CONCATENATOR_GARBAGE_AFTER_INCLUDE,
	//字句解析
	//構文解析
	//コード生成
	//マシン

	ERROR_UNKNOWN,
};

struct Localization{
public:
	struct ErrorMessage{
		ErrorName name;
		const wchar_t* preToken;
		const wchar_t* postToken;
	};

	void init(const wchar_t* langName);
	void initJapanese();
	void initChinese();
	void initKorean();

	bool ifAtHead;
	bool whileAtHead;
	bool defAtHead;
	const wchar_t* ifWord;
	const wchar_t* whileWord0;
	const wchar_t* whileWord1;
	const wchar_t* defWord;
	const wchar_t* constWord;
	const wchar_t* includeWord;
	const wchar_t* outWord;
	const wchar_t* memoryWord;
	wchar_t argDelimiter;
	const ErrorMessage* errorMessages;
	int errorMessageCount;
};

//----実装----
void Localization::init(const wchar_t* langName){
	if (isEqualString(langName, L"chinese")){
		initChinese();
	}else if (isEqualString(langName, L"korean")){
		initKorean();
	}else{
		initJapanese();
	}
}

} //namespace Sunaba

#include "inc/Localization.japanese.inc.h"
#include "inc/Localization.chinese.inc.h"
#include "inc/Localization.korean.inc.h"

#endif
