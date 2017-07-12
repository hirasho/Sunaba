#include "Base/MemoryPool.h"
#include "Compiler/Token.h"

namespace Sunaba{

inline Node::Node() : 
mType(NODE_UNKNOWN), 
mChild(0), 
mBrother(0),
mOperator(OPERATOR_UNKNOWN),
mToken(0),
mNegation(false),
mNumber(0){
}

inline Node::~Node(){
	mChild = 0;
	mBrother = 0;
	mToken = 0;
}

inline void Node::destroyTree(Node* root, MemoryPool* memoryPool){
	root->destroyRecursive(memoryPool);
	memoryPool->destroy(root);
}

//木の破棄。MemoryPoolから取ったため、デストラクタを手動で呼んで回らねばならない。一番上は外で呼ぶこと。
inline void Node::destroyRecursive(MemoryPool* memoryPool){
	//子を破棄
	Node* child = mChild;
	while (child){
		child->destroyRecursive(memoryPool);
		memoryPool->destroy(child);
		child = child->mBrother;
	}
}

inline bool Node::isOutputValueSubstitution() const{
	return (mToken->mType == TOKEN_OUT);
}

} //namespace Sunaba

