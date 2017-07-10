#include "Compiler/Token.h"
#include "Compiler/Concatenator.h"
#include "Compiler/Node.h"
#include "Compiler/Parser.h"
#include "Compiler/CodeGenerator.h"
#include "Base/Array.h"
#include "Base/MemoryPool.h"

namespace Sunaba{

//プリプロセス結果を受け取る。複雑なので複数のクラスに分けている。そのせいでほとんど中身は空。
inline bool Compiler::process(
Array<wchar_t>* result,
std::wostringstream* messageOut,
const wchar_t* filename){
	MemoryPool memoryPool(1024);
	//ファイル結合+トークン分解
	Array<Token> tokens;
	Tank<String> fullPathFilenames; //エラー表示用のファイル名を保持するTank。
	if (!Concatenator::process(&tokens, messageOut, filename, &fullPathFilenames, &memoryPool)){
		return false;
	}
	//英語文法主体か日本語文法主体かをここで判別
	int japaneseCount = 0;
	int englishCount = 0;
	for (int i = 0; i < tokens.size(); ++i){
		TokenType t = tokens[i].mType;
		if (
		(t == TOKEN_WHILE_J) ||
		(t == TOKEN_IF_J) ||
		(t == TOKEN_DEF_J)){
			++japaneseCount;
		}else if (
		(t == TOKEN_WHILE) ||
		(t == TOKEN_IF) ||
		(t == TOKEN_DEF)){
			++englishCount;
		}else if (t == TOKEN_OUT){
			if (tokens[i].mString == L"出力"){
				++japaneseCount;
			}else if (tokens[i].mString == L"out"){
				++englishCount;
			}else{
				ASSERT(false);
			}
		}
	}
	bool english = (englishCount >= japaneseCount);
	//パースします。
	Node* rootNode = 0;
	rootNode = Parser::process(tokens, messageOut, &memoryPool, english);
	if (!rootNode){
		return false;
	}
	//コード生成
	if (!CodeGenerator::process(result, messageOut, rootNode, english)){
		return false;
	}
	return true;
}

} //namespace Sunaba
