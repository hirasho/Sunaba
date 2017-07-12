#include "Base/RefString.h"

namespace Sunaba{

struct InstructionInfo{
	const wchar_t* mName;
	Instruction mInstruction;
};

inline const InstructionInfo* getInstructionInfo(int index){
	static InstructionInfo table[] = {
		{ L"i", INSTRUCTION_I},
		{ L"add", INSTRUCTION_ADD},
		{ L"sub", INSTRUCTION_SUB},
		{ L"mul", INSTRUCTION_MUL},
		{ L"div", INSTRUCTION_DIV},
		{ L"lt", INSTRUCTION_LT},
		{ L"le", INSTRUCTION_LE},
		{ L"eq", INSTRUCTION_EQ},
		{ L"ne", INSTRUCTION_NE},
		{ L"ld", INSTRUCTION_LD},
		{ L"st", INSTRUCTION_ST},
		{ L"fld", INSTRUCTION_FLD},
		{ L"fst", INSTRUCTION_FST},
		{ L"j", INSTRUCTION_J},
		{ L"bz", INSTRUCTION_BZ},
		{ L"call", INSTRUCTION_CALL},
		{ L"ret", INSTRUCTION_RET},
		{ L"pop", INSTRUCTION_POP},
	};
	int count = static_cast<int>(sizeof(table) / sizeof(InstructionInfo));
	return (index < count) ? &(table[index]) : 0;
}

inline Instruction nameToInstruction(const wchar_t* s, int l){
	Instruction r = INSTRUCTION_UNKNOWN;
	int index = 0;
	while (getInstructionInfo(index)){
		if (isEqualString(s, l, getInstructionInfo(index)->mName)){
			r = getInstructionInfo(index)->mInstruction;
			break;
		}
		++index;
	}
	return r;
}

inline int getImmU(unsigned inst, int bitCount){
	int immU = inst & ((1U << bitCount) - 1); //下位31bit
	return immU;
}

inline int getImmS(unsigned inst, int bitCount){
	int immU = getImmU(inst, bitCount);
	int immS = (immU >= (1 << (bitCount - 1))) ? (immU - (1 << bitCount)) : immU;
	return immS;
}

inline int getMaxImmU(int bitCount){
	return (1 << bitCount) - 1;
}

inline int getMaxImmS(int bitCount){
	return (1 << (bitCount - 1)) - 1;
}

inline int getImmMask(int bitCount){
	return (1 << bitCount) - 1;
}

} //namespace Sunaba
