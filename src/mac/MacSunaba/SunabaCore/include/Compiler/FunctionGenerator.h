#ifndef INCLUDED_SUNABA_COMPILER_FUNCTIONGENERATOR_H
#define INCLUDED_SUNABA_COMPILER_FUNCTIONGENERATOR_H

#include <map>
#include <sstream>

namespace Sunaba{
template<class T> class Tank;
struct Node;
class RefString;
class FunctionInfo;

class FunctionGenerator{
public:
	static bool process(
		Tank<wchar_t>* result, 
		std::wostringstream* messageStream,
		const Node* root,
		const RefString& funcName,
		const FunctionInfo& func,
		const std::map<RefString, FunctionInfo>&,
		bool englishGrammer);
private:
	class Variable{
	public:
		Variable();
		void set(int address);
		void define();
		void initialize(); //初期化済とする
		bool isDefined() const;
		bool isInitialized() const;
		int offset() const;
	private:
		bool mDefined; //実際の行解析時に左辺に現れたらtrueにする。
		bool mInitialized; //始めて左辺に現れた行を解析し終えたところでtrueにする。
		int mOffset; //FP相対オフセット。
	};
	typedef std::map<RefString, Variable> VariableMap;

	class Block{
	public:
		typedef std::map<RefString, Variable> VariableMap;
		Block(int baseOffset);
		~Block();
		void beginError(std::wostringstream*, const Node* node);
		bool addVariable(const RefString& name, bool isArgument = false);
		void collectVariables(const Node* firstStatement);
		Variable* findVariable(const RefString&);

		Block* mParent;
		int mBaseOffset;
		int mFrameSize;
	private:
		VariableMap mVariables;
	};
	FunctionGenerator(
		std::wostringstream*, 
		const RefString& funcName,
		const FunctionInfo&,
		const std::map<RefString, FunctionInfo>&,
		bool englishGrammer);
	~FunctionGenerator();

	bool process(
		Tank<wchar_t>* result, 
		const Node* root);
	bool collectFunctionDefinitionInformation(const Node*);
	bool generateStatement(Tank<wchar_t>* out, const Node*);
	bool generateReturn(Tank<wchar_t>* out, const Node*);
	bool checkReturnInformation(const Node*);
	bool generateWhile(Tank<wchar_t>* out, const Node*);
	bool generateIf(Tank<wchar_t>* out, const Node*);
	bool generateFunctionStatement(Tank<wchar_t>* out, const Node*);
	bool generateFunction(Tank<wchar_t>* out, const Node*, bool isStatement);
	bool generateSubstitution(Tank<wchar_t>* out, const Node*);
	bool generateExpression(Tank<wchar_t>* out, const Node*);
	bool generateTerm(Tank<wchar_t>* out, const Node*);

	bool pushDynamicOffset(
		Tank<wchar_t>* out,
		int* staticOffset,
		bool* isFpRelative,
		const Node* node);
	void beginError(const Node* node);
	void operator=(const FunctionGenerator&); //封印

	std::wostringstream* mMessageStream; //借り物
	Block* mRootBlock;
	Block* mCurrentBlock;
	int mLabelId;

	const RefString& mName;
	const FunctionInfo& mInfo;
	const std::map<RefString, FunctionInfo>& mFunctionMap;
	bool mEnglish;
	bool mOutputExist;
};

} //namespace Sunaba

#include "Compiler/inc/FunctionGenerator.inc.h"

#endif
