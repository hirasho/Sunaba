#include "Compiler/Node.h"
#include "Base/MemoryPool.h"

namespace Sunaba{

inline Node* Parser::process(
const Array<Token>& in,
std::wostringstream* messageStream,
MemoryPool* memoryPool,
bool english,
const Localization& loc){
	Parser parser(messageStream, memoryPool, english, loc);
	return parser.parseProgram(in);
}

inline Parser::Parser(
std::wostringstream* messageStream, 
MemoryPool* memoryPool, 
bool english,
const Localization& loc) : 
mMemoryPool(memoryPool),
mMessageStream(messageStream),
mEnglish(english),
mLocalization(&loc){
}

inline Parser::~Parser(){
	mMemoryPool = 0;
	mMessageStream = 0;
}

//Program : ( Const | FuncDef | Statement )*
inline Node* Parser::parseProgram(const Array<Token>& in){
	//定数マップに「メモリ」と「memory」を登録
	const wchar_t* memoryWord = mLocalization->memoryWord;
	std::pair<ConstMap::iterator, bool> result;
	result = mConstMap.insert(ConstMap::value_type(memoryWord, 0));
	ASSERT(result.second);
	result = mConstMap.insert(ConstMap::value_type(L"memory", 0));
	ASSERT(result.second);

	//Programノードを確保
	AutoNode node(mMemoryPool->create<Node>());
	node->mType = NODE_PROGRAM;
	int pos = 0;

	//定数全部処理
	//このループを消して、後ろのループのparseConstのtrueを消せば、定数定義を前に限定できる
	while (in[pos].mType != TOKEN_END){
		if (in[pos].mType == TOKEN_CONST){
			if (!parseConst(in, &pos, false)){ //ノードを返さない。
				return 0;
			}
		}else{
			++pos;
		}
	}

	pos = 0;
	Node* lastChild = 0;
	while (in[pos].mType != TOKEN_END){
		StatementType type = getStatementType(in, pos);
		Node* child = 0;
		if (type == STATEMENT_UNKNOWN){
			return 0;
		}else if (type == STATEMENT_CONST){
			if (!parseConst(in, &pos, true)){ //ノードを返さない。
				return 0;
			}
		}else{
			if (type == STATEMENT_DEF){
				child = parseFunctionDefinition(in, &pos);
			}else{
				child = parseStatement(in, &pos);
			}
			if (!child){
				return 0;
			}else if (!lastChild){
				node->mChild = child;
			}else{
				lastChild->mBrother = child;
			}
			lastChild = child;
		}
	}
	return node.get();
}

//Const : const name '->' Expression STATEMENT_END
inline bool Parser::parseConst(const Array<Token>& in, int* pos, bool skip){
	if (in[*pos].mType != TOKEN_CONST){
		beginError(in[*pos]);
		*mMessageStream << L"定数行のはずだが解釈できない。上の行からつながってないか。" << std::endl;
		return false;
	}
	++(*pos);

	//名前
	if (in[*pos].mType != TOKEN_NAME){
		beginError(in[*pos]);
		if (mEnglish){
			*mMessageStream << L"constの次は定数名。\"" << in[*pos].toString() << L"\"は定数名と解釈できない。" << std::endl;
		}else{
			*mMessageStream << L"\"定数\"の次は定数名。\"" << in[*pos].toString() << L"\"は定数名と解釈できない。" << std::endl;
		}
		return false;
	}
	std::wstring constName(in[*pos].mString.pointer(), in[*pos].mString.size());
	++(*pos);

	//->
	if (in[*pos].mType != TOKEN_SUBSTITUTION){
		beginError(in[*pos]);
		if (mEnglish){
			*mMessageStream << L"const [名前]、と来たら次は\"->\"のはずだが\"" << in[*pos].toString() << L"\"がある。" << std::endl;
		}else{
			*mMessageStream << L"定数 [名前]、と来たら次は\"→\"のはずだが\"" << in[*pos].toString() << L"\"がある。" << std::endl;
		}
		return false;
	}
	++(*pos);

	Node* expression = parseExpression(in, pos);
	if (!expression){
		return false;
	}
	if (expression->mType != NODE_NUMBER){ //数字に解決されていなければ駄目。
		beginError(in[*pos]);
		*mMessageStream << L"定数の右辺の計算に失敗した。メモリや名前つきメモリ、部分プログラム参照などが入っていないか？" << std::endl;
		return false;
	}
	int constValue = expression->mNumber;
	//;
	if (in[*pos].mType != TOKEN_STATEMENT_END){
		beginError(in[*pos]);
		*mMessageStream << L"定数作成の後に\"" << in[*pos].toString() << L"\"がある。改行してくれ。" << std::endl;
		return false;
	}
	++(*pos);
	if (!skip){
		//定数マップに登録
		std::pair<ConstMap::iterator, bool> result = 
			mConstMap.insert(ConstMap::value_type(constName, constValue));
		if (!result.second){
			beginError(in[*pos]);
			*mMessageStream << L"すでに同じ名前の定数がある。" << std::endl;
			return false;
		}
	}
	return true;
}

//FunctionDefinition : id '(' id? [ ',' id ]* ')' 'とは' [BLOCK_BEGIN statement... BLOCK_END]
//FunctionDefinition : 'def' id '(' id? [ ',' id ]* ')' [BLOCK_BEGIN statement... BLOCK_END]
inline Node* Parser::parseFunctionDefinition(const Array<Token>& in, int* pos){
	//defスキップ
	bool defFound = false;
	if (in[*pos].mType == TOKEN_DEF_PRE){
		++(*pos);
		defFound = true;
	}
	ASSERT(in[*pos].mType == TOKEN_NAME);
	//自分ノード
	AutoNode node(mMemoryPool->create<Node>());
	node->mType = NODE_FUNCTION_DEFINITION;
	node->mToken = &in[*pos]; //関数名トークン
	++(*pos);

	//(
	if (in[*pos].mType != TOKEN_LEFT_BRACKET){
		beginError(in[*pos]);
		*mMessageStream << L"ここで入力リスト開始の\"(\"があるはずだが、\"" << in[*pos].toString() << L"\"がある。これ、本当に部分プログラム？" << std::endl;
		return 0;
	}
	++(*pos);

	//次がIDなら引数が一つはある
	Node* lastChild = 0;
	if (in[*pos].mType == TOKEN_NAME){
		Node* arg = parseVariable(in, pos);
		if (!arg){
			return 0;
		}
		node->mChild = arg;
		lastChild = arg;

		//第二引数以降を処理
		while (in[*pos].mType == TOKEN_COMMA){ //コンマがある！
			++(*pos);
			if (in[*pos].mType != TOKEN_NAME){
				beginError(in[*pos]);
				if (mEnglish){
					*mMessageStream << L"入力リスト中で\",\"があるということは、まだ入力があるはずだ。しかし、\"" << in[*pos].toString() << L"\"は入力と解釈できない。" << std::endl;
				}else{
					*mMessageStream << L"入力リスト中で\"、\"があるということは、まだ入力があるはずだ。しかし、\"" << in[*pos].toString() << L"\"は入力と解釈できない。" << std::endl;
				}
				return 0;
			}
			arg = parseVariable(in, pos);
			if (!arg){
				return 0;
			}
			//引数名が定数だったら不許可
			if (arg->mType == NODE_NUMBER){
				beginError(in[*pos]);
				*mMessageStream << L"入力名はすでに定数に使われている。" << std::endl;
				return 0;
			}
			lastChild->mBrother = arg;
			lastChild = arg;
		}
	}
	//)
	if (in[*pos].mType != TOKEN_RIGHT_BRACKET){ 
		beginError(in[*pos]);
		*mMessageStream << L"入力リストの後には\")\"があるはずだが、\"" << in[*pos].toString() << L"\"がある。\",\"を書き忘れてないか？" << std::endl;
		return 0;
	}
	++(*pos);

	if (in[*pos].mType == TOKEN_DEF_POST){ //「とは」がある場合、スキップ
		if (defFound){
			beginError(in[*pos]);
			*mMessageStream << L"\"def\"と\"とは\"が両方ある。片方にしてほしい。" << std::endl;
		}
		++(*pos);
	}

	//次のトークンがBLOCK_BEGINの場合、中を処理。
	if (in[*pos].mType == TOKEN_BLOCK_BEGIN){
		++(*pos);
		//文をパース。複数ある可能性があり、1つもなくてもかまわない
		while (true){
			Node* child = 0;
			if (in[*pos].mType == TOKEN_BLOCK_END){ //抜ける
				++(*pos);
				break;
			}else if (in[*pos].mType == TOKEN_CONST){ //定数。これはエラーです。
				beginError(in[*pos]);
				*mMessageStream << L"部分プログラム内で定数は作れない。" << std::endl;
				return 0;
			}else{
				child = parseStatement(in, pos);
				if (!child){
					return 0;
				}
			}
			if (lastChild){
				lastChild->mBrother = child;
			}else{
				node->mChild = child;
			}
			lastChild = child;
		}
	}else if (in[*pos].mType == TOKEN_STATEMENT_END){ //中身がない
		++(*pos);
	}else{
		beginError(in[*pos]);
		*mMessageStream << L"部分プログラムの最初の行の行末に\"" << in[*pos].toString() << L"\"が続いている。ここで改行しよう。" << std::endl;
		return 0;
	}
	return node.get();
}

//Statement : ( While | If | Return | FuncDef | Func | Substitution )
inline Node* Parser::parseStatement(const Array<Token>& in, int* pos){
	Node* node = 0;
	StatementType type = getStatementType(in, *pos);
	if (type == STATEMENT_WHILE_OR_IF){
		node = parseWhileOrIfStatement(in, pos);
	}else if (type == STATEMENT_DEF){ //関数定義はありえない
		beginError(in[*pos]);
		*mMessageStream << L"部分プログラムの中で部分プログラムは作れない。" << std::endl;
		return 0;
	}else if (type == STATEMENT_CONST){
		ASSERT(false); //これは上でチェックしているはず
	}else if (type == STATEMENT_FUNCTION){ //関数呼び出し文
		node = parseFunction(in, pos);
		if (!node){
			return 0;
		}
		if (in[*pos].mType != TOKEN_STATEMENT_END){
			beginError(in[*pos]);
			if (in[*pos].mType == TOKEN_BLOCK_BEGIN){
				if (mEnglish){
					*mMessageStream << L"部分プログラムを作る気なら「def」が必要。あるいは、次の行の字下げが多すぎる。" << std::endl;
				}else{
					*mMessageStream << L"部分プログラムを作る気なら「とは」が必要。あるいは、次の行の字下げが多すぎる。" << std::endl;
				}
			}else{
				if (mEnglish){
					*mMessageStream << L"部分プログラム参照の後ろに、変なもの\"" << in[*pos].toString() << L"\"がある。部分プログラムを作るなら「def」を置くこと。" << std::endl;
				}else{
					*mMessageStream << L"部分プログラム参照の後ろに、変なもの\"" << in[*pos].toString() << L"\"がある。部分プログラムを作るなら「とは」を置くこと。" << std::endl;
				}
			}
			return 0;
		}
		++(*pos);
	}else if (type == STATEMENT_SUBSTITUTION){ //代入文
		node = parseSubstitutionStatement(in, pos);
	}else{
		ASSERT(type == STATEMENT_UNKNOWN); 
		return 0;//エラーメッセージはもう出してある。
	}
	return node;
}

//関数定義、関数呼び出し、while, if, 代入文のどれらしいかを判定する。
inline StatementType Parser::getStatementType(const Array<Token>& in, int pos) const{
	//文頭トークンからまず判断
	//いきなり範囲開始がある場合、インデントが狂っている。
	TokenType t = in[pos].mType;
	if (t == TOKEN_BLOCK_BEGIN){
		beginError(in[pos]);
		*mMessageStream << L"字下げを間違っているはず。上の行より多くなっていないか。" << std::endl;
		return STATEMENT_UNKNOWN;
	}else if ((t == TOKEN_WHILE_PRE) || (t == TOKEN_IF_PRE)){
		return STATEMENT_WHILE_OR_IF;
	}else if (t == TOKEN_DEF_PRE){
		return STATEMENT_DEF;
	}else if (t == TOKEN_CONST){
		return STATEMENT_CONST;
	}else if (t == TOKEN_OUT){
		return STATEMENT_SUBSTITUTION;
	}
	//文末までスキャンする。
	int endPos = pos;
	while ((in[endPos].mType != TOKEN_STATEMENT_END) && (in[endPos].mType != TOKEN_BLOCK_BEGIN)){
		ASSERT(in[endPos].mType != TOKEN_END); //ENDの前に文末があるはずなんだよ。本当か？TODO:
		++endPos;
	}
	if (endPos > pos){
		t = in[endPos - 1].mType;
		if ((t == TOKEN_WHILE_POST) || (t == TOKEN_IF_POST)){
			return STATEMENT_WHILE_OR_IF;
		}else if (t == TOKEN_DEF_POST){
			return STATEMENT_DEF;
		}
	}
	//代入を探す
	for (int p = pos; p < endPos; ++p){
		t = in[p].mType;
		if (t == TOKEN_SUBSTITUTION){
			return STATEMENT_SUBSTITUTION;
		}
	}
	//となると関数コールくさい。
	if (in[pos].mType == TOKEN_NAME){ //関数呼び出しの可能性 名前 ( とつながれば関数呼び出し文
		if (in[pos + 1].mType == TOKEN_LEFT_BRACKET){
			return STATEMENT_FUNCTION;
		}
	}
	//ここまで来るとダメ。何なのか全然わからない。しかしありがちなのが「なかぎり」と「なら」の左に空白がないケース
	beginError(in[pos]);//TODO:なら、なかぎりを検索してあれば助言を出す
	if (mEnglish){
		*mMessageStream << L"この行を解釈できない。注釈は//ではなく#だが大丈夫か？";
	}else{
		*mMessageStream << L"この行を解釈できない。「なかぎり」や「なら」の左側には空白が必要だが、大丈夫か？また、注釈は//ではなく#だが大丈夫か？";
	}
	//
	for (int p = pos; p < endPos; ++p){
		if (in[p].mOperator == OPERATOR_EQ){
			*mMessageStream << L"=があるが、→や->と間違えてないか？";
			break;
		}
	}
	*mMessageStream << std::endl;
	return STATEMENT_UNKNOWN;
}

//Substitution : [Out | Memory | id | ArrayElement ] '=' Expression STATEMENT_END
inline Node* Parser::parseSubstitutionStatement(const Array<Token>& in, int* pos){
	//以下は引っかかったらおかしい
	if ((in[*pos].mType != TOKEN_NAME) && (in[*pos].mType != TOKEN_OUT)){ 
		beginError(in[*pos]);
		if (mEnglish){
			*mMessageStream << L"->があるのでメモリセット行だと思うが、そうなら最初に「memory」や「out」、名前付きメモリがあるはずだ。 " << std::endl;
		}else{
			*mMessageStream << L"→があるのでメモリセット行だと思うが、そうなら最初に「メモリ」や「出力」、名前付きメモリがあるはずだ。" << std::endl;
		}
		return 0;
	}
	//自分ノード
	AutoNode node(mMemoryPool->create<Node>());
	node->mType = NODE_SUBSTITUTION_STATEMENT;
	node->mToken = &in[*pos];
	//出力か、メモリ要素か、変数か、配列要素
	Node* left = 0;
	if (in[*pos].mType == TOKEN_OUT){
		left = mMemoryPool->create<Node>();
		left->mType = NODE_OUT;
		left->mToken = &in[*pos];
		++(*pos);
	}else if (in[*pos + 1].mType == TOKEN_INDEX_BEGIN){
		left = parseArrayElement(in, pos);
	}else{
		left = parseVariable(in, pos);
		if (left->mType == NODE_NUMBER){ //定数じゃねえか！
			beginError(in[*pos]);
			*mMessageStream << L"定数 " << in[*pos].toString() << "は変えられない。" << std::endl;
			return 0;
		}
	}
	if (!left){
		return 0;
	}
	node->mChild = left;

	//→が必要
	if (in[*pos].mType != TOKEN_SUBSTITUTION){
		beginError(in[*pos]);
		if (mEnglish){
			*mMessageStream << L"メモリセット行だと思ったのだが、\"->\"があるべき場所に、\"" << in[*pos].toString() << L"\"がある。もしかして「if」か「while」？" << std::endl;
		}else{
			*mMessageStream << L"メモリセット行だと思ったのだが、\"→\"があるべき場所に、\"" << in[*pos].toString() << L"\"がある。もしかして「なら」か「なかぎり」？" << std::endl;
		}
		return 0;
	}
	++(*pos);

	//右辺は式
	Node* expression = parseExpression(in, pos);
	if (!expression){
		return 0;
	}
	left->mBrother = expression;

	//STATEMENT_ENDが必要
	if (in[*pos].mType != TOKEN_STATEMENT_END){
		beginError(in[*pos]);
		if (in[*pos].mType == TOKEN_BLOCK_BEGIN){
			*mMessageStream << L"次の行の字下げが多すぎる。行頭の空白は同じであるはずだ。" << std::endl;
		}else{
			*mMessageStream << L"メモリ変更行が終わるべき場所に\"" << in[*pos].toString() << L"\"がある。改行してね。" << std::endl;
		}
		return 0;
	}
	++(*pos);
	return node.get();
}

inline Node* Parser::parseWhileOrIfStatement(const Array<Token>& in, int* pos){
	//自分ノード
	const wchar_t* whileOrIf = 0;
	AutoNode node(mMemoryPool->create<Node>());
	//英語版ならすぐ決まる。
	if (in[*pos].mType == TOKEN_WHILE_PRE){
		node->mType = NODE_WHILE_STATEMENT;
		whileOrIf = L"while";
		node->mToken = &in[*pos];
		++(*pos);
	}else if (in[*pos].mType == TOKEN_IF_PRE){
		node->mType = NODE_IF_STATEMENT;
		whileOrIf = L"if";
		node->mToken = &in[*pos];
		++(*pos);
	}

	//式を解釈
	Node* expression = parseExpression(in, pos);
	if (!expression){
		return 0;
	}
	node->mChild = expression;

	//日本語版ならここにキーワードがあるはず
	if (!whileOrIf){ //まだ確定してない
		if (in[*pos].mType == TOKEN_WHILE_POST){
			node->mType = NODE_WHILE_STATEMENT;
			whileOrIf = L"\"なかぎり\"";
		}else if (in[*pos].mType == TOKEN_IF_POST){
			node->mType = NODE_IF_STATEMENT;
			whileOrIf = L"\"なら\"";
		}
		node->mToken = &in[*pos];
		++(*pos);
	}

	//次のトークンがBLOCK_BEGINであれば、中身があるので処理。
	if (in[*pos].mType == TOKEN_BLOCK_BEGIN){
		++(*pos);

		//文をパース。複数ある可能性があり、1つもなくてもかまわない
		Node* lastChild = expression;
		while (true){
			Node* child = 0;
			if (in[*pos].mType == TOKEN_BLOCK_END){ //抜ける
				++(*pos);
				break;
			}else if (in[*pos].mType == TOKEN_CONST){ //定数。これはエラーです。
				beginError(in[*pos]);
				*mMessageStream << L"繰り返しや条件実行内で定数は作れない。" << std::endl;
				return 0;
			}else{
				child = parseStatement(in, pos);
				if (!child){
					return 0;
				}
			}
			lastChild->mBrother = child;
			lastChild = child;
		}
	}else if (in[*pos].mType == TOKEN_STATEMENT_END){ //中身がない
		++(*pos);
	}else{
		beginError(in[*pos]);
		*mMessageStream << L"条件行は条件の終わりで改行しよう。\"" << in[*pos].toString() << L"\"が続いている。" << std::endl;
		return 0;
	}
	return node.get();
}

//ArrayElement : id '[' Expression ']'
inline Node* Parser::parseArrayElement(const Array<Token>& in, int* pos){
	//自分ノード
	Node* node = parseVariable(in, pos);
	if (!node){
		return 0;
	}
	node->mType = NODE_ARRAY_ELEMENT;
	//[
	ASSERT(in[*pos].mType == TOKEN_INDEX_BEGIN); //getTermTypeで判定済み。
	++(*pos);

	//Expression
	Node* expression = parseExpression(in, pos);
	if (!expression){
		return 0;
	}
	node->mChild = expression;
	//expressionが数値であれば、アドレス計算はここでやる
	if (expression->mType == NODE_NUMBER){
		node->mNumber += expression->mNumber;
		node->mChild = 0; //子のExpressionを破棄
	}
	//]
	if (in[*pos].mType != TOKEN_INDEX_END){
		beginError(in[*pos]);
		*mMessageStream << L"名前つきメモリ[番号]の\"]\"の代わりに\"" << in[*pos].toString() << L"\"がある。" << std::endl;
		return 0;
	}
	++(*pos);
	return node;
}

//Variable : id
inline Node* Parser::parseVariable(const Array<Token>& in, int* pos){
	ASSERT(in[*pos].mType == TOKEN_NAME);
	//自分ノード
	AutoNode node(mMemoryPool->create<Node>());
	//定数か調べる
	std::wstring s(in[*pos].mString.pointer(), in[*pos].mString.size());
	std::map<std::wstring, int>::iterator it = mConstMap.find(s);
	if (it != mConstMap.end()){ //ある場合
		node->mType = NODE_NUMBER;
		node->mNumber = it->second;
	}else{
		node->mType = NODE_VARIABLE;
		node->mToken = &in[*pos];
	}
	++(*pos);
	return node.get();
}

inline Node* Parser::parseOut(const Array<Token>& in, int* pos){
	ASSERT(in[*pos].mType == TOKEN_OUT);
	//自分ノード
	AutoNode node(mMemoryPool->create<Node>());
	node->mType = NODE_OUT;
	node->mToken = &in[*pos];
	++(*pos);
	return node.get();
}

//Expression : Expression (+,-,*,/,<,>,!=,== ) Expression
//左結合の木になる。途中で回転が行われることがある。
inline Node* Parser::parseExpression(const Array<Token>& in, int* pos){
	//ボトムアップ構築して、左結合の木を作る。
	//最初の左ノードを生成
	Node* left = parseTerm(in, pos);
	if (!left){
		return 0;
	}
	//演算子がある場合
	while (in[*pos].mType == TOKEN_OPERATOR){
		//ノードを生成
		Node* node = mMemoryPool->create<Node>();
		node->mType = NODE_EXPRESSION;
		node->mToken = &in[*pos];
		//演算子を設定
		node->mOperator = in[*pos].mOperator;
		ASSERT(node->mOperator != OPERATOR_UNKNOWN);
		++(*pos);
		//連続して演算子なら親切にエラーを吐いてやる。
		if ((in[*pos].mOperator != OPERATOR_UNKNOWN) && (in[*pos].mOperator != OPERATOR_MINUS)){
			beginError(in[*pos]);
			*mMessageStream << L"演算子が連続している。==,++,--はないし、=<,=>あたりは<=,>=の間違いだろう。" << std::endl;
			return 0;
		}
		//右の子を生成
		Node* right = parseTerm(in, pos);
		if (!right){
			return 0;
		}
		//GTとGEなら左右を交換
		if ((node->mOperator == OPERATOR_GT) || (node->mOperator == OPERATOR_GE)){
			Node* tmp = left;
			left = right;
			right = tmp;
			if (node->mOperator == OPERATOR_GT){
				node->mOperator = OPERATOR_LT;
			}else{
				node->mOperator = OPERATOR_LE;
			}
		}
		//最適化。左右ノードが両方数値なら計算をここでやる
		int preComputedValue = 0;
		bool preComputed = false;
		if ((left->mType == NODE_NUMBER) && (right->mType == NODE_NUMBER)){
			int valueA = left->mNumber;
			int valueB = right->mNumber;
			switch (node->mOperator){
				case OPERATOR_PLUS: preComputedValue = valueA + valueB; break;
				case OPERATOR_MINUS: preComputedValue = valueA - valueB; break;
				case OPERATOR_MUL: preComputedValue = valueA * valueB; break;
				case OPERATOR_DIV: preComputedValue = valueA / valueB; break;
				case OPERATOR_EQ: preComputedValue = (valueA == valueB) ? 1 : 0; break;
				case OPERATOR_NE: preComputedValue = (valueA != valueB) ? 1 : 0; break;
				case OPERATOR_LT: preComputedValue = (valueA < valueB) ? 1 : 0; break;
				case OPERATOR_LE: preComputedValue = (valueA <= valueB) ? 1 : 0; break;
				default: ASSERT(false); //LE,GEは上で変換されてなくなっていることに注意
			}
			if (abs(preComputedValue) <= getMaxImmS(IMM_BIT_COUNT_I)){ //即値に収まりません
				preComputed = true;
			}
		}
		if (preComputed){ //事前計算によるノードマージ
			node->mType = NODE_NUMBER;
			node->mNumber = preComputedValue;
			left->~Node(); //デストラクタ呼び出し
			right->~Node();
		}else{ //マージされない。左右を子に持つ
			node->mChild = left;
			left->mBrother = right;
		}
		//左の子を現ノードに変更
		left = node;
	}
	return left;
}

inline TermType Parser::getTermType(const Array<Token>& in, int pos) const{
	TermType r = TERM_UNKNOWN;
	if (in[pos].mType == TOKEN_LEFT_BRACKET){
		r = TERM_EXPRESSION;
	}else if (in[pos].mType == TOKEN_NUMBER){
		r = TERM_NUMBER;
	}else if (in[pos].mType == TOKEN_NAME){
		++pos;
		if (in[pos].mType == TOKEN_LEFT_BRACKET){
			r = TERM_FUNCTION;
		}else if (in[pos].mType == TOKEN_INDEX_BEGIN){
			r = TERM_ARRAY_ELEMENT;
		}else{
			r = TERM_VARIABLE;
		}
	}else if (in[pos].mType == TOKEN_OUT){
		r = TERM_OUT;
	}
	return r;
}

//Term :[ - ] Function | Variable | Out | ArrayElement | number | '(' Expression ')' | Allocate
inline Node* Parser::parseTerm(const Array<Token>& in, int* pos){
	bool minus = false;
	if (in[*pos].mOperator == OPERATOR_MINUS){
		minus = true;
		++(*pos);
	}
	TermType type = getTermType(in, *pos);
	Node* node = 0;
	if (type == TERM_EXPRESSION){
		ASSERT(in[*pos].mType == TOKEN_LEFT_BRACKET); //getTermTypeで判定済み
		++(*pos);
		node = parseExpression(in, pos);
		if (in[*pos].mType != TOKEN_RIGHT_BRACKET){ //)が必要
			beginError(in[*pos]);
			*mMessageStream << L"()で囲まれた式がありそうなのだが、終わりの\")\"の代わりに、\"" << in[*pos].toString() << L"\"がある。\")\"を忘れていないか？" << std::endl;
			return 0;
		}
		++(*pos);
	}else if (type == TERM_NUMBER){
		node = mMemoryPool->create<Node>();
		node->mType = NODE_NUMBER;
		node->mToken = &in[*pos];
		node->mNumber = node->mToken->mNumber;
		++(*pos);
	}else if (type == TERM_FUNCTION){
		node = parseFunction(in, pos);
	}else if (type == TERM_ARRAY_ELEMENT){
		node = parseArrayElement(in, pos);
	}else if (type == TERM_VARIABLE){
		node = parseVariable(in, pos);
	}else if (type == TERM_OUT){
		node = parseOut(in, pos);
	}else{
		beginError(in[*pos]);
		if (mEnglish){
			*mMessageStream << L"()で囲まれた式、memory[]、数、名前つきメモリ、部分プログラム参照のどれかがあるはずの場所に\"" << in[*pos].toString() << L"\"がある。" << std::endl;
		}else{
			*mMessageStream << L"()で囲まれた式、メモリ[]、数、名前つきメモリ、部分プログラム参照のどれかがあるはずの場所に\"" << in[*pos].toString() << L"\"がある。" << std::endl;
		}
	}
	if (node && minus){
		//数字ノードになっていれば、マイナスはこの場で処理
		if (node->mType == NODE_NUMBER){
			node->mNumber *= -1;
		}else{
			node->mNegation = !node->mNegation;
		}
	}
	return node;
}

//Function : id '(' [ Expression [ ',' Expression ]* ] ')'
inline Node* Parser::parseFunction(const Array<Token>& in, int* pos){
	ASSERT(in[*pos].mType == TOKEN_NAME);
	//以下はバグ
	AutoNode node(mMemoryPool->create<Node>());
	node->mType = NODE_FUNCTION;
	node->mToken = &in[*pos];
	++(*pos);

	//(
	ASSERT(in[*pos].mType == TOKEN_LEFT_BRACKET); //getTermTypeかgetStatementTypeで判定済み。
	++(*pos);

	//最初のExpressionがあるかどうかは、次が右括弧かどうかでわかる
	if (in[*pos].mType != TOKEN_RIGHT_BRACKET){ //括弧が出たら抜ける
		Node* expression = parseExpression(in, pos);
		if (!expression){
			return 0;
		}
		node->mChild = expression;

		//2個目以降。, Expressionが連続する限り取り込む
		Node* lastChild = expression;
		while (true){
			if (in[*pos].mType != TOKEN_COMMA){
				break; //コンマでないなら抜ける
			}
			++(*pos);
			expression = parseExpression(in, pos);
			if (!expression){
				return 0;
			}
			lastChild->mBrother = expression;
			lastChild = expression;
		}
	}
	//)
	if (in[*pos].mType != TOKEN_RIGHT_BRACKET){
		beginError(in[*pos]);
		if (mEnglish){
			*mMessageStream << L"部分プログラムの入力は\")\"で終わるはず。だが、\"" << in[*pos].toString() << L"\"がある。\",\"の書き忘れはないか？" << std::endl;
		}else{
			*mMessageStream << L"部分プログラムの入力は\")\"で終わるはず。だが、\"" << in[*pos].toString() << L"\"がある。\"、\"の書き忘れはないか？" << std::endl;
		}
		return 0;
	}
	++(*pos);
	return node.get();
}

inline void Parser::beginError(const Token& token) const{
	mMessageStream->write(token.mFilename.pointer(), token.mFilename.size());
	*mMessageStream << L'(' << token.mLine << L") ";
}

} //namespace Sunaba
