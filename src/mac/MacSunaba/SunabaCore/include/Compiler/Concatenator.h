#ifndef INCLUDED_SUNABA_COMPILER_CONCATENATOR_H
#define INCLUDED_SUNABA_COMPILER_CONCATENATOR_H

#include <set>
#include <sstream>
#include "Base/String.h"
#include "Base/Tank.h"

namespace Sunaba{
template<class T> class Array;
struct Token;
class String;
class StringPointerLess;
class MemoryPool;
struct Localization;

class Concatenator{
public:
	static bool process(
		Array<Token>* tokensOut, 
		std::wostringstream* messageStream,
		const wchar_t* rootFilename,
		Tank<String>* fullPathFilenames,
		MemoryPool*,
		const Localization&);
private:
	typedef std::set<const String*, StringPointerLess> FilenameSet;
	 
	Concatenator(
		Tank<String>* fullPathFilenames, 
		std::wostringstream* messageStream,
		const wchar_t* rootFilename,
		MemoryPool*,
		const Localization&);
	~Concatenator();
	bool process(Array<Token>* tokensOut, const wchar_t* filename);
	bool processFile(const Token& filenameToken, const wchar_t* parentFullPath);
	void beginError(const Token& token) const;

	const wchar_t* mRootFilename;
	int mLine; //吐き出したトークンの最大行
	Tank<String>* mFullPathFilenames; //今まで処理したファイル名をここに保存。フルパス。
	FilenameSet mFilenameSet;
	Tank<Token> mTokens;
	std::wostringstream* mMessageStream; //借り物
	MemoryPool* mMemoryPool; //借り物
	const Localization* mLocalization; //借り物
};

} //namespace Sunaba

#include "Compiler/inc/Concatenator.inc.h"

#endif
