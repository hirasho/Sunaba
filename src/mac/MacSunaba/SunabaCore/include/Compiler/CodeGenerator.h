#ifndef INCLUDED_SUNABA_COMPILER_CODEGENERATOR_H
#define INCLUDED_SUNABA_COMPILER_CODEGENERATOR_H

#include <map>
#include <sstream>

namespace Sunaba{
template<class T> class Array;
template<class T> class Tank;
struct Node;
class RefString;
class FunctionInfo;

class CodeGenerator{
public:
	static bool process(
		Array<wchar_t>* result, 
		std::wostringstream* messageStream,
		const Node* root,
		bool english);
private:
	typedef std::map<RefString, FunctionInfo> FunctionMap;

	CodeGenerator(std::wostringstream*, bool englishGrammer);
	~CodeGenerator();
	bool process(Array<wchar_t>* result, const Node* root);
	void allocateObjects(int* offset, const Node*);
	bool generateProgram(Tank<wchar_t>* out, const Node*);
	bool generateFunctionDefinition(Tank<wchar_t>* out, const Node*);
	bool collectFunctionDefinitionInformation(const Node*);
	void beginError(const Node* node);

	std::wostringstream* mMessageStream; //借り物
	FunctionMap mFunctionMap; //関数マップ。これはグローバル。
	bool mEnglish;
};

} //namespace Sunaba

#include "Compiler/inc/CodeGenerator.inc.h"

#endif
