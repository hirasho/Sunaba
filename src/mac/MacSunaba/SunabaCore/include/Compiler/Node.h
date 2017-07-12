#ifndef INCLUDED_SUNABA_COMPILER_NODE_H
#define INCLUDED_SUNABA_COMPILER_NODE_H

#include "Compiler/Token.h"

namespace Sunaba{
class MemoryPool;

enum StatementType{
	STATEMENT_WHILE_OR_IF,
	STATEMENT_DEF,
	STATEMENT_CONST,
	STATEMENT_FUNCTION,
	STATEMENT_SUBSTITUTION,

	STATEMENT_UNKNOWN,
};

enum TermType{
	TERM_EXPRESSION,
	TERM_NUMBER,
	TERM_FUNCTION,
	TERM_ARRAY_ELEMENT,
	TERM_VARIABLE,
	TERM_OUT,

	TERM_UNKNOWN,
};

//構文木 
//Statementは明示的に節は作らない。Statement = [While | If | Substitution | Function | Return] main以外ではConstは禁止
enum NodeType{ //コメントは子ノードについて
	NODE_PROGRAM, //[Statement | FunctionDefinition] ...
	//Statement
	NODE_WHILE_STATEMENT, // Expression,Statement...
	NODE_IF_STATEMENT, // Expression,Statement...
	NODE_SUBSTITUTION_STATEMENT, //[ Memory | Variable | ArrayElement ] ,Expression
	NODE_FUNCTION_DEFINITION, //Variable... Statement... [ Return ]
	NODE_EXPRESSION, //Expression, Expression
	NODE_VARIABLE,
	NODE_NUMBER,
	NODE_OUT,
	NODE_ARRAY_ELEMENT, //Expression
	NODE_FUNCTION, //Expression ...

	NODE_UNKNOWN,
};

struct Node{
	Node();
	~Node();
	static void destroyTree(Node* root, MemoryPool*);
	//木の破棄。MemoryPoolから取ったため、デストラクタを手動で呼んで回らねばならない。一番上は外で呼ぶこと。
	void destroyRecursive(MemoryPool*);
	bool isOutputValueSubstitution() const;

	NodeType mType;
	Node* mChild;
	Node* mBrother;
	//ノード特性
	OperatorType mOperator; //Expressionの時のみ二項演算子
	const Token* mToken; //何かしらのトークン
	bool mNegation; //項のときに反転するか。
	int mNumber; //定数値、定数[]のアドレス
};

} //namespace Sunaba

#include "Compiler/inc/Node.inc.h"

#endif
