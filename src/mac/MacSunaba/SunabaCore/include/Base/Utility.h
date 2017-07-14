#ifndef INCLUDED_SUNABA_BASE_UTILITY_H
#define INCLUDED_SUNABA_BASE_UTILITY_H

namespace Sunaba{

template<class T> class Array;

int getStringSize(const wchar_t* str);
bool isEqualString(const wchar_t* s0, int l0, const wchar_t* s1, int l1);
//s0はサイズ指定、s1はサイズ不明NULL終端文字列
bool isEqualString(const wchar_t* s0, int l0, const wchar_t* s1);
//s0,s1両方NULL終端
bool isEqualString(const wchar_t* s0, const wchar_t* s1);
bool isLessString(const wchar_t* s0, int l0, const wchar_t* s1, int l1);
wchar_t toLower(wchar_t);
bool checkExtension(const wchar_t* path, const wchar_t* ext);
bool isAlphabet(wchar_t);
//[a-zA-Z0-9_全角]のチェック
bool isInName(wchar_t);
//識別子([a-zA-Z0-9_全角]+)ですか？
bool isName(const wchar_t* s, int l);
bool isHexNumber(wchar_t);
bool isDecNumber(wchar_t);
bool isBinNumber(wchar_t);
bool isAllHexNumber(const wchar_t* s, int l);
bool isAllDecNumber(const wchar_t* s, int l);
bool isAllBinNumber(const wchar_t* s, int l);
bool convertHexNumber(int* out, const wchar_t* s, int l);
bool convertDecNumber(int* out, const wchar_t* s, int l);
bool convertBinNumber(int* out, const wchar_t* s, int l);
//文字列->数字(マイナスも認識)
bool convertNumber(int* out, const wchar_t* s, int l);
//10進で数値化
void makeIntString(wchar_t* p, int v);
//100進の色から256進の色へ
int convertColor100to256(int a);
//2羃に丸める
int roundUpToPow2(int);

//何か->Unicode
void convertToUnicode(Array<wchar_t>* out, const char* in, int inSize);
//SJIS->Unicode
void convertSjisToUnicode(Array<wchar_t>* out, const char* in, int inSize);
//Unicode->Utf8
void convertUnicodeToUtf8(Array<char>* out, const wchar_t* in, int inSize);
//CR,CRLF -> LF
void convertNewLine(Array<wchar_t>* inOut);

int readS4(const unsigned char*);
int readS2(const unsigned char*);
int packColor(const unsigned char*);

//絶対パスファイル名からディレクトリ名を得る。スラッシュはバックスラッシュに変換。
void getDirectoryNameFromAbsoluteFilename(Array<wchar_t>* out, const wchar_t* filename);
//ディレクトリ名にファイル名を連結。ただし、filenameが絶対パスの場合、そのコピーを返す。スラッシュはバックスラッシュに変換。
void makeAbsoluteFilename(Array<wchar_t>* out, const wchar_t* basePath, const wchar_t* filename);
//ファイル名開始場所を検索(\か/を後ろから探す)
int getFilenameBegin(const wchar_t* filename, int filenameSize);
//絶対ファイル名か調べる
bool isAbsoluteFilename(const wchar_t* filename);

} //namespace Sunaba

#include "Base/inc/Utility.inc.h"

#endif
