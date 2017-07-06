#ifndef INCLUDED_SUNABA_MACHINE_MACHINE_H
#define INCLUDED_SUNABA_MACHINE_MACHINE_H

#include "Base/Os.h"
#include "Machine/IoState.h"
#include "Machine/Instruction.h"
#include <sstream>

namespace Sunaba{
class Connection;

class Machine{
public:
	//設定定数。ただし、いじるとIOメモリの番号が変わるので、ソースコードが非互換になる。
	enum{
		FREE_AND_PROGRAM_SIZE = 40000,
		STACK_SIZE = 10000,
		IO_MEMORY_SIZE = 10000,
		IO_WRITABLE_OFFSET = 5000,
		IO_DEBUG_OFFSET = 8000,
		EXECUTION_UNIT = 10000, //これだけの命令実行したらウィンドウ状態を見に行く。
		MINIMUM_FREE_SIZE = 1000, //最低これだけのメモリは0から空いている。プログラムサイズが制約される。
		BUSY_SLEEP_THRESHOLD = 100, //syncなしでこれだけ時間が経ったら強制的に待ち
	};
	//メモリマップ
	enum{
		VM_MEMORY_STACK_BASE = FREE_AND_PROGRAM_SIZE,
		VM_MEMORY_STACK_END = VM_MEMORY_STACK_BASE + STACK_SIZE,
		VM_MEMORY_IO_BASE = VM_MEMORY_STACK_END,
		VM_MEMORY_IO_END = VM_MEMORY_IO_BASE + IO_MEMORY_SIZE,
		VM_MEMORY_VRAM_BASE = VM_MEMORY_IO_END, //解像度に応じてメモリの総量は変化する。
	};
	enum VmMemory{
		//READ
		VM_MEMORY_IO_READABLE_BEGIN = VM_MEMORY_IO_BASE,
		VM_MEMORY_POINTER_X = VM_MEMORY_IO_READABLE_BEGIN,
		VM_MEMORY_POINTER_Y,
		VM_MEMORY_POINTER_BUTTON_LEFT,
		VM_MEMORY_POINTER_BUTTON_RIGHT,
		VM_MEMORY_KEY_UP,
		VM_MEMORY_KEY_DOWN,
		VM_MEMORY_KEY_LEFT,
		VM_MEMORY_KEY_RIGHT,
		VM_MEMORY_KEY_SPACE,
		VM_MEMORY_KEY_ENTER,
		VM_MEMORY_FREE_REGION_END,
		VM_MEMORY_GET_SCREEN_WIDTH,
		VM_MEMORY_GET_SCREEN_HEIGHT,
		VM_MEMORY_SCREEN_BEGIN,
		//デバガ専用READ
		VM_MEMORY_IO_FRAME_COUNT,
		VM_MEMORY_IO_PROGRAM_COUNTER, 
		VM_MEMORY_IO_STACK_POINTER,
		VM_MEMORY_IO_FRAME_POINTER,
		VM_MEMORY_IO_AVERAGE_SYNC_INTERVAL,
		VM_MEMORY_IO_AVERAGE_SYNC_WAIT,
		VM_MEMORY_IO_EXECUTED_INSTRUCTION_COUNT_LOW31,
		VM_MEMORY_IO_EXECUTED_INSTRUCTION_COUNT_HIGH31,
		//終了番号
		VM_MEMORY_IO_READABLE_END,
		//WRITE(デバッガからはREAD可能なこともある)
		VM_MEMORY_IO_WRITABLE_BEGIN = VM_MEMORY_IO_BASE + IO_WRITABLE_OFFSET,
		VM_MEMORY_SYNC = VM_MEMORY_IO_WRITABLE_BEGIN,
		VM_MEMORY_DISABLE_AUTO_SYNC, //IOへのアクセスが自動的にsyncを発生させる
		VM_MEMORY_DRAW_CHAR,
		VM_MEMORY_BREAK,
		VM_MEMORY_SET_SCREEN_WIDTH,
		VM_MEMORY_SET_SCREEN_HEIGHT,
		VM_MEMORY_SET_SOUND_FREQUENCY0,
		VM_MEMORY_SET_SOUND_FREQUENCY1,
		VM_MEMORY_SET_SOUND_FREQUENCY2,
		VM_MEMORY_SET_SOUND_DUMPING0,
		VM_MEMORY_SET_SOUND_DUMPING1,
		VM_MEMORY_SET_SOUND_DUMPING2,
		VM_MEMORY_SET_SOUND_AMPLITUDE0,
		VM_MEMORY_SET_SOUND_AMPLITUDE1,
		VM_MEMORY_SET_SOUND_AMPLITUDE2,
		//終了番号
		VM_MEMORY_IO_WRITABLE_END,
	};
	Machine(
		std::wostringstream* messageStream,
		const unsigned char* objectCode,
		int objectCodeSize);
	~Machine();
	bool isError() const;
	bool isTerminated(); //プログラム終了してますか
	int outputValue(); //終わってからしか呼べない
	int memorySize() const;
	void requestSync();
	int screenWidth() const;
	int screenHeight() const;

	IoState* beginSync();
	void endSync(IoState**);
private:
	enum{
		TIME_ARRAY_COUNT = 60,
	};
	class ExecutionThread : public Thread{
	public:
		ExecutionThread(Machine*);
		~ExecutionThread();
		void operator()();

		Machine* mMachine;
	};
	void threadFunc();
//	void execute();
	int pop();
	void pop(int n); //マイナスならプッシュ
	void push(int a);
	//メインスレッドとの同期
	void sync(bool waitVSync);
	void syncInput();
	void beginError();
	//最適化実行
	bool decode();
	void executeDecoded();

	IoState mIoState;
	Event mEndRequestEvent;
	Event mTerminatedEvent;
	Event mSyncRequestEvent;
	ExecutionThread* mExecutionThread;
	int mFrameCount;
	int mScreenWidth;
	int mScreenHeight;
	int mProgramBegin;
	//VMレジスタ
	int mProgramCounter;
	int mStackPointer;
	int mFramePointer;

	bool mError;
	bool mCharDrawn;

	Array<int> mMemory;

	std::wostringstream* mMessageStream;

	//デバグ関連
	struct CallInfo{
		CallInfo(); 
		void set(int pc, int fp);
		void reset();
		int mProgramCounter;
		int mFramePointer;
	};
	struct DebugInfo{
		DebugInfo();
		bool push(int pc, int fp);
		bool pop();
		enum{
			CALL_HISTORY_SIZE = 256,
		};
		CallInfo mCallHistory[CALL_HISTORY_SIZE];
		int mCallHistoryPosition;
		int mMaxStackPointer;
	};
	DebugInfo mDebugInfo;

	double mExecutedInstructionCount;
	unsigned mTimeArray[TIME_ARRAY_COUNT * 2];
	int mTimeArrayPosition;
	int mTimeArrayValidCount;
	unsigned mLastSyncEndTime;

	//最適化実行
	struct DecodedInst{
		void set(Instruction i, int imm){
			mInst = i;
			mImm = imm;
		}
		Instruction mInst;
		int mImm;
	};
	Array<DecodedInst> mDecodedInsts;
	int mDecodedProgramCounter;
};

} //namespce Sunaba

#include "Machine/inc/Machine.inc.h"

#endif
