#ifndef INCLUDED_SUNABA_COMPILER_PARSER_H
#define INCLUDED_SUNABA_COMPILER_PARSER_H

#include <map>
#include <sstream>
#include "Base/MemoryPool.h"
#include "Compiler/Node.h"

namespace Sunaba{
template<class T> class Array;
struct Node;
struct Token;
struct Localization;

class Parser{
public:
	static Node* process(
		const Array<Token>& in,
		std::wostringstream* messageStream,
		MemoryPool*,
		bool english,
		const Localization&);
private:
	typedef MemoryPool::AutoDestroyer<Node> AutoNode;
	typedef std::map<std::wstring, int> ConstMap;

	Parser(std::wostringstream*, MemoryPool*, bool english, const Localization&);
	~Parser();
	StatementType getStatementType(const Array<Token>& in, int pos) const;
	TermType getTermType(const Array<Token>& in, int pos) const;

	Node* parseProgram(const Array<Token>& in);
	bool parseConst(const Array<Token>& in, int* pos, bool skip); //2パス目はスキップ
	Node* parseFunctionDefinition(const Array<Token>& in, int* pos);

	Node* parseStatement(const Array<Token>& in, int* pos);
	Node* parseSubstitutionStatement(const Array<Token>& in, int* pos);
	Node* parseWhileOrIfStatement(const Array<Token>& in, int* pos);
	Node* parseReturnStatement(const Array<Token>& in, int* pos);
	Node* parseFunction(const Array<Token>& in, int* pos);

	Node* parseArrayElement(const Array<Token>& in, int* pos);
	Node* parseVariable(const Array<Token>& in, int* pos);
	Node* parseOut(const Array<Token>& in, int* pos);
	Node* parseExpression(const Array<Token>& in, int* pos);
	Node* parseTerm(const Array<Token>& in, int* pos);

	void beginError(const Token& token) const;

	MemoryPool* mMemoryPool;
	ConstMap mConstMap; //定数マップ。定数のスコープは全体なのでこういうマネができる。
	std::wostringstream* mMessageStream;
	bool mEnglish;
	const Localization* mLocalization;
};

} //namespace Sunaba

#include "Compiler/inc/Parser.inc.h"

#endif
