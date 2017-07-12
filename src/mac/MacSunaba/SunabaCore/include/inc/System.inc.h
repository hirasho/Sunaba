#include "System.h"
#include "Graphics/Graphics.h"
#include "Base/Os.h"
#include "Machine/Machine.h"
#include "Compiler/Compiler.h"
#include "Compiler/Assembler.h"
#include "Base/Array.h"
#include "Sound/Sound.h"

namespace Sunaba{

inline System::System(void* windowHandle, const wchar_t* langName) :
mGraphics(0),
mMachine(0),
mWindowHandle(0),
mPictureBoxHandle(0),
mPictureBoxWidth(1),
mPictureBoxHeight(1),
mError(false),
mTerminationMessageDrawn(false),
mFramePerSecond(0),
mCalculationTimePercent(0),
mMemoryRequestAddress(0),
mMemoryRequestValue(0),
mMemoryRequestState(IoState::MEMORY_REQUEST_NONE),
mSound(0){
	STRONG_ASSERT(windowHandle);
	mLocalization.init(langName);
	beginTimer();
	mSound = new Sound(IoState::SOUND_CHANNEL_COUNT, windowHandle); 
}

inline System::~System(){
	DELETE(mMachine);
	DELETE(mGraphics);
	DELETE(mSound);
	endTimer();
}

inline bool System::bootProgram(const unsigned char* objectCode, int objectCodeSize){
#ifdef _WIN32
	STRONG_ASSERT(mPictureBoxHandle);
#endif
	STRONG_ASSERT(objectCode);
	DELETE(mMachine);
	mTerminationMessageDrawn = false;
	mMachine = new Machine(
		&mMessageStream,
		objectCode,
		objectCodeSize);
	if (mMachine->isError()){
		mMessageStream << L"指定されたプログラムを開始できなかった。" << std::endl;
		DELETE(mMachine);
	}else{
		mMessageStream << L"プログラムを開始！" << std::endl;
	}
	restartGraphics();
	return (mMachine != 0);
}

inline bool System::bootProgram(const wchar_t* filename){
	STRONG_ASSERT(filename);
	Array<wchar_t> compiled;
	bool ret = false;
	if (Compiler::process(&compiled, &mMessageStream, filename, mLocalization)){
		Array<unsigned> instructions;
		if (Assembler::process(&instructions, &mMessageStream, compiled, mLocalization)){
			//BEに変換
			Array<unsigned char> objectCode(instructions.size() * 4);
			for (int i = 0; i < instructions.size(); ++i){
				objectCode[(i * 4) + 0] = static_cast<unsigned char>((instructions[i] >> 24) & 0xff);
				objectCode[(i * 4) + 1] = static_cast<unsigned char>((instructions[i] >> 16) & 0xff);
				objectCode[(i * 4) + 2] = static_cast<unsigned char>((instructions[i] >> 8) & 0xff);
				objectCode[(i * 4) + 3] = static_cast<unsigned char>((instructions[i] >> 0) & 0xff);
			}
			ret = bootProgram(objectCode.pointer(), objectCode.size());
		}
	}
	return ret;
}

inline void System::update(Array<unsigned char>* messageOut, int pointerX, int pointerY, const char* keys){
	std::wstring ws = mMessageStream.str();
	if (ws.size() > 0){
		messageOut->setSize(static_cast<int>(ws.size()) * 2);
		for (int i = 0; i < static_cast<int>(ws.size()); ++i){
			(*messageOut)[(2 * i) + 0] = static_cast<unsigned char>((ws[i] >> 8) & 0xff);
			(*messageOut)[(2 * i) + 1] = static_cast<unsigned char>((ws[i] >> 0) & 0xff);
		}
		//TODO:メッセージ引き渡し
		writeToConsole(ws.c_str());
		mMessageStream.str(L"");
	}
	//IOメモリを読み書きする間はロック
	if (mMachine){
		IoState* io = mMachine->beginSync();
		if (mMachine->isTerminated()){
			if (!mTerminationMessageDrawn){
				mTerminationMessageDrawn = true;
				//終了メッセージ
				if (mMachine->isError()){ //エラーなら
					mMessageStream << L"プログラムが異常終了した。間違いがある。" << std::endl;
				}else{
					mMessageStream << L"プログラムが最後まで実行された";
					int out = mMachine->outputValue();
					if (out != 0){
						mMessageStream << L"(出力:" << mMachine->outputValue() << L")" << std::endl;
					}else{
						mMessageStream << L"。" << std::endl;
					}
				}
			}
		}else{
			//入力取得
			io->update(pointerX, pointerY, keys);
			//実行情報取得
			const int* mem = io->memory();
			int avgSyncInt = mem[Machine::VM_MEMORY_IO_AVERAGE_SYNC_INTERVAL];
			if (avgSyncInt == 0){
				mFramePerSecond = 0;
				mCalculationTimePercent = 0;
			}else{
				int avgSyncWait = mem[Machine::VM_MEMORY_IO_AVERAGE_SYNC_WAIT];
				mFramePerSecond = 1000 * 1000 / avgSyncInt;
				mCalculationTimePercent = (avgSyncInt - avgSyncWait) * 100 / avgSyncInt;
			}
			//メモリ
			if (mMemoryRequestState == IoState::MEMORY_REQUEST_WRITE){
				io->mMemoryRequestState = static_cast<IoState::MemoryRequestState>(mMemoryRequestState);
				io->mMemoryRequestAddress = mMemoryRequestAddress;
				io->mMemoryRequestValue = mMemoryRequestValue;
				mMemoryRequestState = IoState::MEMORY_REQUEST_WAITING;
				mMachine->requestSync();
			}else if (mMemoryRequestState == IoState::MEMORY_REQUEST_WAITING){
				if (io->mMemoryRequestState == IoState::MEMORY_REQUEST_NONE){
					mMemoryRequestState = IoState::MEMORY_REQUEST_NONE;
				}
			}
			//音制御
			for (int i = 0; i < IoState::SOUND_CHANNEL_COUNT; ++i){
				float f = static_cast<float>(mem[Machine::VM_MEMORY_SET_SOUND_FREQUENCY0 + i]);
				mSound->setFrequency(i, f);
				float d = static_cast<float>(mem[Machine::VM_MEMORY_SET_SOUND_DUMPING0 + i]) * 0.00001f;
				if (d < 0.f){
					d = 0.f;
				}else if (d > 10000.f){
					d = 1.f;
				}
				mSound->setDumping(i, d);
				if (io->mSoundRequests[i] != 0){
					float s = static_cast<float>(io->mSoundRequests[i]) * 0.00001f;
					if (s < 0.f){
						s = 0.f;
					}else if (s > 10000.f){
						s = 1.f;
					}
					if (s != 0.f){
						mSound->play(i, s);
					}
					io->mSoundRequests[i] = 0;
				}
			}
			if (io->mScreenSizeChanged){
				io->mScreenSizeChanged = false;
				restartGraphics(); //この場でGraphics再起動
			}
		}
		if (mGraphics){
			mGraphics->getScreenData(io); //GPUへの画像転送はメインスレッド。終わってても毎回やる必要がある。でないと画面が砂嵐だ。
		}
		mMachine->endSync(&io);
	}
	//フレーム終了
	if (mGraphics){
		mGraphics->endFrame();
	}
}

inline int System::screenWidth() const{
	if (mMachine){
		return mMachine->screenWidth();
	}else{
		return 1;
	}
}

inline int System::screenHeight() const{
	if (mMachine){
		return mMachine->screenHeight();
	}else{
		return 1;
	}
}

inline int System::framePerSecond() const{
	return mFramePerSecond;
}

inline int System::calculationTimePercent() const{
	return mCalculationTimePercent;
}

inline void System::requestAutoSync(bool f){
	requestMemoryWrite(Machine::VM_MEMORY_DISABLE_AUTO_SYNC, (f) ? 0 : 1);
}

inline bool System::requestAutoSyncFinished() const{
	return requestMemoryFinished();
}

inline bool System::autoSync() const{
	return (memoryValue(Machine::VM_MEMORY_DISABLE_AUTO_SYNC) == 0);
}

inline void System::requestMemoryWrite(int address, int value){
	mMemoryRequestAddress = address;
	mMemoryRequestValue = value;
	mMemoryRequestState = IoState::MEMORY_REQUEST_WRITE;
}

inline bool System::requestMemoryFinished() const{
	return (mMemoryRequestState == IoState::MEMORY_REQUEST_NONE);
}

inline int System::memoryValue(int address) const{
	int r = 0;
	if (mMachine){
		IoState* io = mMachine->beginSync();
		if ((address >= 0) && (address < mMachine->memorySize())){
			r = io->memory()[address];
		}
		mMachine->endSync(&io);
	}
	return r;
}

inline bool System::restartGraphics(){
	STRONG_ASSERT(mMachine);
#ifdef _WIN32
    STRONG_ASSERT(mPictureBoxHandle);
#endif
	DELETE(mGraphics);
	int w = mMachine->screenWidth();
	int h = mMachine->screenHeight();
	mGraphics = new Graphics(mPictureBoxHandle, w, h, mPictureBoxWidth, mPictureBoxHeight);
	if (mGraphics->isError()){
		DELETE(mGraphics);
	}
	return (mGraphics != 0);
}

inline void System::setPictureBoxHandle(void* pictureBoxHandle, int pbw, int pbh){
	STRONG_ASSERT(pictureBoxHandle);
	mPictureBoxHandle = pictureBoxHandle;
	mPictureBoxWidth = pbw;
	mPictureBoxHeight = pbh;
	if (mMachine){
		restartGraphics();
	}
}

inline const Localization* System::localization() const{
	return &mLocalization;
}

}
