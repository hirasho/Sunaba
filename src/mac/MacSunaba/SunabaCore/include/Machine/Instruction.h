#ifndef INCLUDED_SUNABA_MACHINE_INSTRUCTION_H
#define INCLUDED_SUNABA_MACHINE_INSTRUCTION_H

namespace Sunaba{

//命令32bitの上位8bit
enum Instruction{
	//bit31=1
	INSTRUCTION_I = 0x80, //immはSint31
	//bit30=1 算術命令 //現状immはないが、あれば27bit
	INSTRUCTION_ADD = 0x78,
	INSTRUCTION_SUB = 0x70,
	INSTRUCTION_MUL = 0x68,
	INSTRUCTION_DIV = 0x60,
	INSTRUCTION_LT = 0x58,
	INSTRUCTION_LE = 0x50,
	INSTRUCTION_EQ = 0x48,
	INSTRUCTION_NE = 0x40,
	//bit29=1 ロードストア命令
	INSTRUCTION_LD = 0x38, //immはSint27
	INSTRUCTION_ST = 0x30, //immはSint27
	INSTRUCTION_FLD = 0x28, //immはSint27
	INSTRUCTION_FST = 0x20, //immはSint27
	//フロー制御命令
	INSTRUCTION_J = 0x1c, //immはUint26 無条件ジャンプ
	INSTRUCTION_BZ = 0x18, //immはUint26 topが0ならジャンプ
	INSTRUCTION_CALL = 0x14, //immはUint26 push(FP),push(CP),FP=SP,CP=imm
	INSTRUCTION_RET = 0x10, //immはSint26 pop(imm), CP=pop(), FP=pop() 
	INSTRUCTION_POP = 0x0c, //immはSint26

	INSTRUCTION_UNKNOWN = -1,

	INSTRUCTION_HEAD_BIT_I = 31,
	INSTRUCTION_HEAD_BIT_ALU = 30,
	INSTRUCTION_HEAD_BIT_LS = 29,
	IMM_BIT_COUNT_I = 31,
	IMM_BIT_COUNT_LS = 27,
	IMM_BIT_COUNT_FLOW = 26,
};
 
Instruction nameToInstruction(const wchar_t* s, int l);
int getImmU(unsigned inst, int bitCount);
int getImmS(unsigned inst, int bitCount);
int getMaxImmU(int bitCount);
int getMaxImmS(int bitCount);
int getImmMask(int bitCount);

} //namespace Sunaba

#include "inc/Instruction.inc.h"

#endif
