// これは メイン DLL ファイルです。

#include "SunabaLib.h"

#include "Base/Array.h"
#include "Base/Base.h"
#include "Compiler/Compiler.h"
#include "Compiler/Assembler.h"
#include "System.h"
#include "Localization.h"

namespace SunabaLib{

//SunabaSystem
SunabaSystem::SunabaSystem(IntPtr windowHandle, System::String^ langName) : mSystem(0){
	Sunaba::Array<wchar_t> tmpLangName(langName->Length + 1);
	for (int i = 0; i < langName->Length; ++i){
		tmpLangName[i] = langName[i];
	}
	tmpLangName[tmpLangName.size() - 1] = '\0';
	mSystem = new Sunaba::System(static_cast<void*>(windowHandle), tmpLangName.pointer());
}

SunabaSystem::~SunabaSystem(){
	ASSERT(!mSystem);
}

void SunabaSystem::end(){
	DELETE(mSystem);
}

void SunabaSystem::setPictureBoxHandle(IntPtr pictureBoxHandle, int screenWidth, int screenHeight){
	mSystem->setPictureBoxHandle(static_cast<void*>(pictureBoxHandle), screenWidth, screenHeight);
}

bool SunabaSystem::bootProgram(array<System::Byte>^ objectCode){
	Sunaba::Array<unsigned char> o(objectCode->Length);
	for (int i = 0; i < o.size(); ++i){
		o[i] = objectCode[i];
	}
	return mSystem->bootProgram(o.pointer(), o.size());
}

bool SunabaSystem::bootProgram(System::String^ filename){
	Sunaba::Array<wchar_t> tmpFilename(filename->Length + 1);
	for (int i = 0; i < filename->Length; ++i){
		tmpFilename[i] = filename[i];
	}
	tmpFilename[tmpFilename.size() - 1] = '\0';
	return mSystem->bootProgram(tmpFilename.pointer());
}

void SunabaSystem::update(array<wchar_t>^% messageOut, int pointerX, int pointerY, array<System::SByte>^ keys){
	Sunaba::Array<char> k;
	k.setSize(keys->Length);
	for (int i = 0; i < k.size(); ++i){
		k[i] = keys[i];
	}
	Sunaba::Array<unsigned char> messageOutTmp;
	mSystem->update(&messageOutTmp, pointerX, pointerY, k.pointer());
	int c = messageOutTmp.size() / 2;
	messageOut = gcnew array<wchar_t>(c);
	for (int i = 0; i < c; ++i){
		messageOut[i] = messageOutTmp[(i * 2) + 0] << 8;
		messageOut[i] |= messageOutTmp[(i * 2) + 1] << 0;
	}
}

int SunabaSystem::screenWidth(){
	return mSystem->screenWidth();
}

int SunabaSystem::screenHeight(){
	return mSystem->screenHeight();
}

int SunabaSystem::framePerSecond(){
	return mSystem->framePerSecond();
}

int SunabaSystem::calculationTimePercent(){
	return mSystem->calculationTimePercent();
}

void SunabaSystem::requestAutoSync(bool f){
	mSystem->requestAutoSync(f);
}

bool SunabaSystem::requestAutoSyncFinished(){
	return mSystem->requestAutoSyncFinished();
}

bool SunabaSystem::autoSync(){
	return mSystem->autoSync();
}

void SunabaSystem::requestMemoryWrite(int address, int value){
	mSystem->requestMemoryWrite(address, value);
}

bool SunabaSystem::requestMemoryFinished(){
	return mSystem->requestMemoryFinished();
}

int SunabaSystem::memoryValue(int address){
	return mSystem->memoryValue(address);
}

//Compiler
bool Compiler::compile(
array<unsigned>^% instructionsOut,
array<wchar_t>^% messageOut,
System::String^ filename,
System::String^ langName){
	Sunaba::Array<unsigned> tmpInstructionsOut;
	std::wostringstream messageStream;
	Sunaba::Array<wchar_t> tmpFilename(filename->Length + 1);
	for (int i = 0; i < filename->Length; ++i){
		tmpFilename[i] = filename[i];
	}
	tmpFilename[tmpFilename.size() - 1] = '\0';

	Sunaba::Array<wchar_t> tmpLangName(langName->Length + 1);
	for (int i = 0; i < langName->Length; ++i){
		tmpLangName[i] = langName[i];
	}
	tmpLangName[tmpLangName.size() - 1] = '\0';

	Sunaba::Localization loc;
	loc.init(tmpLangName.pointer());
	Sunaba::Array<wchar_t> compiled;
	bool ret = Sunaba::Compiler::process(
		&compiled,
		&messageStream,
		tmpFilename.pointer(),
		loc);
	if (ret){
		Sunaba::Assembler::process(
			&tmpInstructionsOut,
			&messageStream,
			compiled,
			loc);
		if (ret){
			System::Array::Resize(instructionsOut, tmpInstructionsOut.size());
			for (int i = 0; i < tmpInstructionsOut.size(); ++i){
				instructionsOut[i] = tmpInstructionsOut[i];
			}
		}
	}
	std::wstring ws = messageStream.str();
	System::Array::Resize(messageOut, ws.size());
	for (unsigned i = 0; i < ws.size(); ++i){
		messageOut[i] = ws[i];
	}
	return ret;
}


} //namespace SunabaLib
