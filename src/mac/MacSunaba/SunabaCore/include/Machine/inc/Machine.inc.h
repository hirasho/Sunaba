#include "Machine/Instruction.h"
#include "Base/Os.h"

namespace Sunaba{

inline Machine::CallInfo::CallInfo() : mProgramCounter(-0x7fffffff), mFramePointer(-0x7fffffff){
}

inline void Machine::CallInfo::set(int pc, int fp){
	mProgramCounter = pc;
	mFramePointer = fp;
}

inline void Machine::CallInfo::reset(){
	mProgramCounter = -0x7fffffff;
	mFramePointer = -0x7fffffff;
}

inline Machine::DebugInfo::DebugInfo() : mCallHistoryPosition(0), mMaxStackPointer(0){
}

inline bool Machine::DebugInfo::push(int pc, int fp){
	if (mCallHistoryPosition >= CALL_HISTORY_SIZE){
		return false;
	}
	mCallHistory[mCallHistoryPosition].set(pc, fp);
	++mCallHistoryPosition;
	return true;
}

inline bool Machine::DebugInfo::pop(){
	if (mCallHistoryPosition <= 0){
		return false;
	}
	--mCallHistoryPosition;
	mCallHistory[mCallHistoryPosition].reset();
	return true;
}

inline Machine::ExecutionThread::ExecutionThread(Machine* machine) : mMachine(machine){
	start();
}

inline Machine::ExecutionThread::~ExecutionThread(){
	wait();
	mMachine = 0;
}

inline void Machine::ExecutionThread::operator()(){
	mMachine->threadFunc();
}

//Machine
inline Machine::Machine(
std::wostringstream* messageStream,
const unsigned char* objectCode,
int objectCodeSize) :
mExecutionThread(0),
mFrameCount(0),
mScreenWidth(100),
mScreenHeight(100),
mProgramBegin(0),
mProgramCounter(0),
mStackPointer(VM_MEMORY_STACK_BASE),
mFramePointer(VM_MEMORY_STACK_BASE),
mError(true), //無事初期化が終わったらfalseにする
mCharDrawn(false),
mMessageStream(messageStream),
mExecutedInstructionCount(0.0),
mTimeArrayPosition(0),
mTimeArrayValidCount(0),
mLastSyncEndTime(0){
	for (int i = 0; i < TIME_ARRAY_COUNT; ++i){
		mTimeArray[(i * 2) + 0] = mTimeArray[(i * 2) + 1] = 0;
	}
	int vramSize = mScreenWidth * mScreenHeight;
	mMemory.setSize(VM_MEMORY_VRAM_BASE + vramSize); //メモリサイズはIO_BASE+vram
	memset(mMemory.pointer(), 0, mMemory.size() * 4);
	mIoState.allocateMemoryCopy(mMemory.size(), VM_MEMORY_IO_BASE, VM_MEMORY_VRAM_BASE);

	//オブジェクトコードを命令列に変換
	//すぐできるエラーチェック
	int instructionCount = objectCodeSize  / 4;
	if ((objectCodeSize % 4) != 0){
		*mMessageStream << L"不正なコード(32bit単位でない)。" << std::endl;
		return;
	}else if (instructionCount >= (FREE_AND_PROGRAM_SIZE - MINIMUM_FREE_SIZE)){
		*mMessageStream << L"プログラムが大きすぎる(" << instructionCount << L"命令)。最大" << (FREE_AND_PROGRAM_SIZE - MINIMUM_FREE_SIZE) << L"命令。" << std::endl;
		return;
	}
	//0番地からプログラムをコピー
	mProgramBegin = FREE_AND_PROGRAM_SIZE - instructionCount; //ここから実行開始
	for (int i = 0; i < instructionCount; ++i){
		mMemory[mProgramBegin + i] = (objectCode[(i * 4) + 0] << 24);
		mMemory[mProgramBegin + i] += (objectCode[(i * 4) + 1] << 16);
		mMemory[mProgramBegin + i] += (objectCode[(i * 4) + 2] << 8);
		mMemory[mProgramBegin + i] += (objectCode[(i * 4) + 3] << 0);
	}
	mMemory[VM_MEMORY_FREE_REGION_END] = mProgramBegin; //プログラム開始位置を入れる
	mMemory[VM_MEMORY_GET_SCREEN_WIDTH] = mMemory[VM_MEMORY_SET_SCREEN_WIDTH] = mScreenWidth;
	mMemory[VM_MEMORY_GET_SCREEN_HEIGHT] = mMemory[VM_MEMORY_SET_SCREEN_HEIGHT] = mScreenHeight;
	mMemory[VM_MEMORY_SCREEN_BEGIN] = VM_MEMORY_VRAM_BASE;
	for (int i = 0; i < IoState::SOUND_CHANNEL_COUNT; ++i){
		mMemory[VM_MEMORY_SET_SOUND_DUMPING0 + i] = 1000; //減衰初期値。これでノイズを殺す。
		mMemory[VM_MEMORY_SET_SOUND_FREQUENCY0 + i] = 20; 
	}
	//事前デコード
	if (!decode()){
		mError = true;
	}
	mError = false; //無事終了
	//実行開始
	mProgramCounter = mProgramBegin;
	mExecutionThread = new ExecutionThread(this);
}

inline Machine::~Machine(){
	mEndRequestEvent.set();
	DELETE(mExecutionThread);
	mMessageStream = 0;
	endTimer();
}

inline int Machine::memorySize() const{
	return mMemory.size();
}

inline int Machine::screenWidth() const{
	return mScreenWidth;
}

inline int Machine::screenHeight() const{
	return mScreenHeight;
}

inline void Machine::requestSync(){
	mSyncRequestEvent.set();
}

inline IoState* Machine::beginSync(){
	mIoState.lock();
	return &mIoState;
}

inline void Machine::endSync(IoState** p){
	ASSERT(*p == &mIoState);
	mIoState.unlock();
	*p = 0;
}

inline bool Machine::isError() const{
	return mError;
}

inline bool Machine::isTerminated(){ //プログラム終了してますか
	return mTerminatedEvent.isSet();
}

inline int Machine::outputValue(){
	ASSERT(mTerminatedEvent.isSet());
	return mMemory[VM_MEMORY_STACK_BASE];
}

inline void Machine::threadFunc(){
	mLastSyncEndTime = getTimeInMilliSecond();
	sync(true); //一回sync。起動直後に押されたキーを反映させる。vsyncしないとUIからの入力が来ない
	bool end = false;
	while (!end){
		if (mEndRequestEvent.isSet()){
			end = true;
		}else{
			if (mSyncRequestEvent.isSet()){
				sync(false);
				mSyncRequestEvent.reset();
			}
			for (int i = 0; i < EXECUTION_UNIT; ++i){
				if (mMemory[VM_MEMORY_BREAK] != 0){ //ブレークポイント実行により実行中断
					sleepMilliSecond(1); //1ms寝る
					break;
				}
				if (mError || (mProgramCounter == FREE_AND_PROGRAM_SIZE)){ //終わりまで来た
					end = true; //終わります
					//正常終了時デバグコード
					if (!mError){
						ASSERT(mProgramCounter == VM_MEMORY_STACK_BASE);
						ASSERT(mStackPointer == VM_MEMORY_STACK_BASE);
						ASSERT(mFramePointer == VM_MEMORY_STACK_BASE);
					}
					break;
				}
				executeDecoded(); //最適化版
			}
			//無意味な演算でCPUをフル活用するのを止めるための強制スリープ
			//VSYnc待ちsyncがないまま一定以上時間が経つと無理矢理寝かせる。
			unsigned time = getTimeInMilliSecond();
			if ((time - mLastSyncEndTime) >= BUSY_SLEEP_THRESHOLD){
				sleepMilliSecond(BUSY_SLEEP_THRESHOLD);
				mLastSyncEndTime = time;
			}
		}
	}
	sync(false); //一回sync。最後のIO命令を反映。
	mTerminatedEvent.set();
}

inline int Machine::pop(){
	if (mStackPointer <= VM_MEMORY_STACK_BASE){
		beginError();
		*mMessageStream << L"ポップしすぎてスタック領域をはみ出した。" << std::endl;
		return 0; //とりあえず0を返し、後で抜ける
	}else{
		--mStackPointer;
		return mMemory[mStackPointer];
	}
}

inline void Machine::pop(int n){ //マイナスならプッシュ
	if ((mStackPointer - n) < VM_MEMORY_STACK_BASE){
		beginError();
		*mMessageStream << L"ポップしすぎてスタック領域をはみ出した。" << std::endl;
	}else if ((mStackPointer - n) >= VM_MEMORY_STACK_END){
		beginError();
		*mMessageStream << L"スタックを使い切った。名前つきメモリを使いすぎ。" << std::endl;
	}else{
		mStackPointer -= n;
		if (mDebugInfo.mMaxStackPointer < mStackPointer){
			mDebugInfo.mMaxStackPointer = mStackPointer;
		}
	}
}

inline void Machine::push(int a){
	if (mStackPointer >= VM_MEMORY_STACK_END){
		beginError();
		*mMessageStream << L"スタックを使い切った。名前つきメモリを使いすぎ。" << std::endl;
	}else{
		mMemory[mStackPointer] = a;
		++mStackPointer;
		if (mDebugInfo.mMaxStackPointer < mStackPointer){
			mDebugInfo.mMaxStackPointer = mStackPointer;
		}
	}
}

//メインスレッドとの同期
inline void Machine::sync(bool waitVSync){
	//時刻測定
	if (waitVSync){
		int newFrame = mFrameCount;
		unsigned syncBeginTime = getTimeInMilliSecond();
		//vsync待ち。同じフレームに二度は呼ばない
		while (true){
			if (mEndRequestEvent.isSet()){
				return;
			}
			mIoState.lock();
			newFrame = mIoState.frameCount();
			mIoState.unlock();
			if (newFrame != mFrameCount){
				break;
			}
			sleepMilliSecond(1);
		}
		int averageSyncWait = 0;
		int averageSyncInterval = 0;
		unsigned syncEndTime = getTimeInMilliSecond();
		mLastSyncEndTime = syncEndTime;
		if (mTimeArrayValidCount >= 1){
			//平均フレーム時間を計算
			int oldestIndex = mTimeArrayPosition - mTimeArrayValidCount;
			if (oldestIndex < 0){
				oldestIndex += TIME_ARRAY_COUNT;
			}
			unsigned oldest = mTimeArray[(oldestIndex * 2) + 0];
			averageSyncInterval = (syncBeginTime - oldest) * 1000 / mTimeArrayValidCount;
			//平均待ち時間計算
			int sum = 0;
			for (int i = 0; i < mTimeArrayValidCount; ++i){ //60個埋まった後は全部だし、埋まるまでは0番からなので、0から回してかまわない。
				sum += mTimeArray[(i * 2) + 1] - mTimeArray[(i * 2) + 0];
			}
			averageSyncWait = sum * 1000 / mTimeArrayValidCount;
		}
		//現在位置に記録
		mTimeArray[(mTimeArrayPosition * 2) + 0] = syncBeginTime;
		mTimeArray[(mTimeArrayPosition * 2) + 1] = syncEndTime;
		//ポインタ進める
		++mTimeArrayPosition;
		if (mTimeArrayValidCount < TIME_ARRAY_COUNT){
			++mTimeArrayValidCount;
		}
		if (mTimeArrayPosition == TIME_ARRAY_COUNT){
			mTimeArrayPosition = 0;
		}
		mFrameCount = newFrame;
		//デバグ情報
		mMemory[VM_MEMORY_IO_AVERAGE_SYNC_INTERVAL] = averageSyncInterval;
		mMemory[VM_MEMORY_IO_AVERAGE_SYNC_WAIT] = averageSyncWait;
	}
	//メモリ
	if (mIoState.mMemoryRequestState == IoState::MEMORY_REQUEST_WRITE){
		int addr = mIoState.mMemoryRequestAddress;
		if ((addr >= 0) && (addr < mMemory.size())){
			mMemory[mIoState.mMemoryRequestAddress] = mIoState.mMemoryRequestValue;
		}
		mIoState.mMemoryRequestState = IoState::MEMORY_REQUEST_NONE;
	}
	//デバグ情報格納
	mMemory[VM_MEMORY_IO_FRAME_COUNT] = mFrameCount;
	mMemory[VM_MEMORY_IO_PROGRAM_COUNTER] = mProgramCounter;
	mMemory[VM_MEMORY_IO_STACK_POINTER] = mStackPointer;
	mMemory[VM_MEMORY_IO_FRAME_POINTER] = mFramePointer;
	int high31 = static_cast<int>(mExecutedInstructionCount /  static_cast<double>(1 << 31)) & 0x7fffffff;
	int low31 = static_cast<int>(mExecutedInstructionCount - (static_cast<double>(1 << 31) * static_cast<double>(high31))) & 0x7fffffff;
	mMemory[VM_MEMORY_IO_EXECUTED_INSTRUCTION_COUNT_LOW31] = low31;
	mMemory[VM_MEMORY_IO_EXECUTED_INSTRUCTION_COUNT_HIGH31] = high31;

	mIoState.lock();
	//外->Machine
	mMemory[VM_MEMORY_POINTER_X] = mIoState.mPointerX;
	mMemory[VM_MEMORY_POINTER_Y] = mIoState.mPointerY;
	mMemory[VM_MEMORY_POINTER_BUTTON_LEFT] = mIoState.mKeys[IoState::KEY_LBUTTON];
	mMemory[VM_MEMORY_POINTER_BUTTON_RIGHT] = mIoState.mKeys[IoState::KEY_RBUTTON];
	mMemory[VM_MEMORY_KEY_UP] = mIoState.mKeys[IoState::KEY_UP];
	mMemory[VM_MEMORY_KEY_DOWN] = mIoState.mKeys[IoState::KEY_DOWN];
	mMemory[VM_MEMORY_KEY_LEFT] = mIoState.mKeys[IoState::KEY_LEFT];
	mMemory[VM_MEMORY_KEY_RIGHT] = mIoState.mKeys[IoState::KEY_RIGHT];
	mMemory[VM_MEMORY_KEY_SPACE] = mIoState.mKeys[IoState::KEY_SPACE];
	mMemory[VM_MEMORY_KEY_ENTER] = mIoState.mKeys[IoState::KEY_ENTER];

	//Machine->外(全メモリコピー)
	if ( //解像度変更
	(mMemory[VM_MEMORY_SET_SCREEN_WIDTH] != mScreenWidth) ||
	(mMemory[VM_MEMORY_SET_SCREEN_HEIGHT] != mScreenHeight)){
		mScreenWidth = mMemory[VM_MEMORY_SET_SCREEN_WIDTH];
		mScreenHeight = mMemory[VM_MEMORY_SET_SCREEN_HEIGHT];
		mMemory[VM_MEMORY_GET_SCREEN_WIDTH] = mScreenWidth;
		mMemory[VM_MEMORY_GET_SCREEN_HEIGHT] = mScreenHeight;
		mIoState.mScreenSizeChanged = true;
		//VRAM取り直し
		int vramSize = mScreenWidth * mScreenHeight;
		Array<int> tmpMemory(VM_MEMORY_VRAM_BASE + vramSize);
		memcpy(tmpMemory.pointer(), mMemory.pointer(), VM_MEMORY_VRAM_BASE * sizeof(int));
		memset(tmpMemory.pointer() + VM_MEMORY_VRAM_BASE, 0, vramSize * sizeof(int));
		mMemory.clear();
		tmpMemory.moveTo(&mMemory);
		mIoState.allocateMemoryCopy(mMemory.size(), VM_MEMORY_IO_BASE, VM_MEMORY_VRAM_BASE);
	}
	memcpy(mIoState.mMemoryCopy, mMemory.pointer(), mMemory.size() * sizeof(int));
	//発音命令はリセット
	for (int i = 0; i < IoState::SOUND_CHANNEL_COUNT; ++i){
		if (mMemory[VM_MEMORY_SET_SOUND_AMPLITUDE0 + i] != 0){
			mIoState.mSoundRequests[i] = mMemory[VM_MEMORY_SET_SOUND_AMPLITUDE0 + i];
			mMemory[VM_MEMORY_SET_SOUND_AMPLITUDE0 + i] = 0;
		}
	}
	//終了
	mIoState.unlock();
}

inline void Machine::syncInput(){
	mIoState.lock();
	//外->Machine
	mMemory[VM_MEMORY_POINTER_X] = mIoState.mPointerX;
	mMemory[VM_MEMORY_POINTER_Y] = mIoState.mPointerY;
	mMemory[VM_MEMORY_POINTER_BUTTON_LEFT] = mIoState.mKeys[IoState::KEY_LBUTTON];
	mMemory[VM_MEMORY_POINTER_BUTTON_RIGHT] = mIoState.mKeys[IoState::KEY_RBUTTON];
	mMemory[VM_MEMORY_KEY_UP] = mIoState.mKeys[IoState::KEY_UP];
	mMemory[VM_MEMORY_KEY_DOWN] = mIoState.mKeys[IoState::KEY_DOWN];
	mMemory[VM_MEMORY_KEY_LEFT] = mIoState.mKeys[IoState::KEY_LEFT];
	mMemory[VM_MEMORY_KEY_RIGHT] = mIoState.mKeys[IoState::KEY_RIGHT];
	mMemory[VM_MEMORY_KEY_SPACE] = mIoState.mKeys[IoState::KEY_SPACE];
	mMemory[VM_MEMORY_KEY_ENTER] = mIoState.mKeys[IoState::KEY_ENTER];
	mIoState.unlock();
}

inline void Machine::beginError(){
	if (mCharDrawn){
		*mMessageStream << std::endl;
		mCharDrawn = false; 
	}
 	mError = true;
}

inline bool Machine::decode(){
	using namespace std; //hex,decのため
	int instCount = FREE_AND_PROGRAM_SIZE - mProgramBegin;
	mDecodedInsts.setSize(instCount);
	for (int i = 0; i < instCount; ++i){
		unsigned inst = static_cast<unsigned>(mMemory[mProgramBegin + i]);
		int inst8 = (inst & 0xff000000) >> 24;
		int imm = 0;
		if (inst & (1 << INSTRUCTION_HEAD_BIT_I)){ //即値ロード
			inst8 = 0x80;
			imm = getImmS(inst, IMM_BIT_COUNT_I);
		}else if (inst & (1 << INSTRUCTION_HEAD_BIT_ALU)){ //算術命令
			inst8 &= 0x78;
			if (
			(inst8 != INSTRUCTION_ADD) &&
			(inst8 != INSTRUCTION_SUB) &&
			(inst8 != INSTRUCTION_MUL) &&
			(inst8 != INSTRUCTION_DIV) &&
			(inst8 != INSTRUCTION_LT) &&
			(inst8 != INSTRUCTION_LE) &&
			(inst8 != INSTRUCTION_EQ) &&
			(inst8 != INSTRUCTION_NE)){
				beginError();
				*mMessageStream << L"存在しない算術命令。おそらく壊れている(コード:" << hex << inst << dec << L")" << std::endl;
				return false;
			}
		}else if (inst & (1 << INSTRUCTION_HEAD_BIT_LS)){ //ロードストア
			inst8 &= 0x38;
			if (
			(inst8 != INSTRUCTION_LD) &&
			(inst8 != INSTRUCTION_ST) &&
			(inst8 != INSTRUCTION_FLD) &&
			(inst8 != INSTRUCTION_FST)){
				beginError();
				*mMessageStream << L"存在しないロードストア命令。おそらく壊れている(コード:" << hex << inst << dec << L")" << std::endl;
				return false;
			}
			imm = getImmS(inst, IMM_BIT_COUNT_LS);
		}else{ //分岐、関数コール、スタック操作系
			inst8 &= 0x1c;
			if (
			(inst8 != INSTRUCTION_J) &&
			(inst8 != INSTRUCTION_BZ) &&
			(inst8 != INSTRUCTION_CALL) &&
			(inst8 != INSTRUCTION_RET) &&
			(inst8 != INSTRUCTION_POP)){
				beginError();
				*mMessageStream << L"存在しないフロー制御命令。おそらく壊れている(コード:" << hex << inst << dec << L")" << std::endl;
				return false;
			}
			imm;
			if (inst8 == INSTRUCTION_POP){//popだけ符号つき
				imm = getImmS(inst, IMM_BIT_COUNT_FLOW);
			}else{
				imm = getImmU(inst, IMM_BIT_COUNT_FLOW);
			}
		}
		mDecodedInsts[i].set(static_cast<Instruction>(inst8), imm);
	}
	return true;
}

inline void Machine::executeDecoded(){
	using namespace std; //hex,decのため

	const DecodedInst& decoded = mDecodedInsts[mProgramCounter - mProgramBegin];
	int inst = decoded.mInst;
	int imm = decoded.mImm;
	if (inst > INSTRUCTION_LD){
		if (inst > INSTRUCTION_LT){
			if (inst > INSTRUCTION_MUL){
				if (inst == INSTRUCTION_I){
					push(imm);
				}else if (inst == INSTRUCTION_ADD){
					int op1 = pop(); //先に入れた方が後から出てくるので1,0の順
					int op0 = pop();
					push(op0 + op1);
				}else{ //INSTRUCTION_SUB
					int op1 = pop(); //先に入れた方が後から出てくるので1,0の順
					int op0 = pop();
					push(op0 - op1);
				}
			}else if (inst == INSTRUCTION_MUL){
				int op1 = pop(); //先に入れた方が後から出てくるので1,0の順
				int op0 = pop();
				push(op0 * op1);
			}else{ //INSTRUCTION_DIV
				int op1 = pop(); //先に入れた方が後から出てくるので1,0の順
				if (op1 == 0){
					beginError();
					*mMessageStream<< L"0では割れない。" << std::endl;
				}else{
					int op0 = pop();
					push(op0 / op1);
				}
			}
		}else if (inst > INSTRUCTION_EQ){
			if (inst == INSTRUCTION_LT){
				int op1 = pop(); //先に入れた方が後から出てくるので1,0の順
				int op0 = pop();
				push((op0 < op1) ? 1 : 0);
			}else{ //INSTRUCTION_LE
				int op1 = pop(); //先に入れた方が後から出てくるので1,0の順
				int op0 = pop();
				push((op0 <= op1) ? 1 : 0);
			}
		}else if (inst == INSTRUCTION_EQ){
			int op1 = pop(); //先に入れた方が後から出てくるので1,0の順
			int op0 = pop();
			push((op0 == op1) ? 1 : 0);
		}else{ //INSTRUCTION_NE
			int op1 = pop(); //先に入れた方が後から出てくるので1,0の順
			int op0 = pop();
			push((op0 != op1) ? 1 : 0);
		}
	}else if (inst > INSTRUCTION_J){
		if ((inst == INSTRUCTION_LD) || (inst == INSTRUCTION_FLD)){
			int op0;
			if (inst > INSTRUCTION_FLD){
				op0 = pop();
			}else{
				op0 = mFramePointer;
			}
			op0 += imm;
			if (op0 < 0){
				beginError();
				*mMessageStream << L"マイナスの番号のメモリを読みとろうとした(番号:" << op0 << L")" << std::endl;
			}else if (op0 < mProgramBegin){ //正常実行
				push(mMemory[op0]);
			}else if (op0 < VM_MEMORY_STACK_BASE){ //プログラム領域
				beginError();
				*mMessageStream << L"プログラムが入っているメモリを読みとろうとした(番号:" << op0 << L")" << std::endl;
			}else if (op0 < VM_MEMORY_IO_BASE){ //スタック。正常実行
				push(mMemory[op0]);
			}else if (op0 < VM_MEMORY_IO_READABLE_END){ //読み取り可能メモリ
				syncInput(); //入力だけ
				push(mMemory[op0]);
			}else if (op0 < VM_MEMORY_VRAM_BASE){ //書きこみ専用メモリ
				beginError();
				*mMessageStream << L"このあたりのメモリはセットはできるが読み取ることはできない(番号:" << op0 << L")" << std::endl;
			}else if (op0 < (VM_MEMORY_VRAM_BASE + (mScreenWidth * mScreenHeight))){
				beginError();
				*mMessageStream << L"画面メモリは読み取れない(番号:" << op0 << L")" << std::endl;
			}else{
				beginError();
				*mMessageStream << L"メモリ範囲外を読みとろうとした(番号:" << op0 << L")" << std::endl;
			}
		}else{ //store
			int op1 = pop();
			int op0;
			if (inst > INSTRUCTION_FLD){
				op0 = pop();
			}else{
				op0 = mFramePointer;
			}
			op0 += imm;
			if (op0 < 0){ //負アドレス
				beginError();
				*mMessageStream << L"マイナスの番号のメモリを変えようとした(番号:" << op0 << L")" << std::endl;
			}else if (op0 < mProgramBegin){ //正常実行
				mMemory[op0] = op1;
			}else if (op0 < VM_MEMORY_STACK_BASE){ //プログラム領域
				beginError();
				*mMessageStream << L"プログラムが入っているメモリに" << op1 << L"をセットしようとした(番号:" << op0 << L")" << std::endl;
			}else if (op0 < VM_MEMORY_IO_BASE){ //スタック。正常実行
				mMemory[op0] = op1;
			}else if (op0 < VM_MEMORY_IO_WRITABLE_BEGIN){ //IOのうち書きこみ不可能領域
				beginError();
				*mMessageStream << L"このあたりのメモリは読み取れはするが、セットはできない(番号:" << op0 << L")" << std::endl;
			}else if (op0 == VM_MEMORY_SYNC){ //vsync待ち検出
				sync(true); //op1は見ていない。なんでもいい。
			}else if (op0 == VM_MEMORY_DISABLE_AUTO_SYNC){ //自動SYNCモード
				mMemory[op0] = (op1 == 0) ? 0 : 1;
			}else if (op0 == VM_MEMORY_DRAW_CHAR){ //デバグ文字出力
				if (op1 == L'\n'){
					*mMessageStream << std::endl;
				}else{
					*mMessageStream << static_cast<wchar_t>(op1);
					mCharDrawn = true;
				}
			}else if (op0 == VM_MEMORY_BREAK){ //ブレークポイント
				sync(false); //同期を走らせて
				mMemory[op0] = op1; //値書きかえ
			}else if (op0 == VM_MEMORY_SET_SCREEN_WIDTH){
				if (op1 <= 0){
					beginError();
					*mMessageStream << L"横解像度として0以下の値を設定した" << std::endl;
				}else if (op1 > 1280){
					beginError();
					*mMessageStream << L"横解像度の最大は1280" << std::endl;
				}
				mMemory[op0] = op1;
			}else if (op0 == VM_MEMORY_SET_SCREEN_HEIGHT){
				if (op1 <= 0){
					beginError();
					*mMessageStream << L"縦解像度として0以下の値を設定した" << std::endl;
				}else if (op1 > 720){
					beginError();
					*mMessageStream << L"縦解像度の最大は720" << std::endl;
				}
				mMemory[op0] = op1;
			}else if ((op0 >= VM_MEMORY_SET_SOUND_FREQUENCY0) && (op0 <= VM_MEMORY_SET_SOUND_AMPLITUDE2)){
				//自動sync
				if (mMemory[VM_MEMORY_DISABLE_AUTO_SYNC] == 0){ 
					sync(true);
				}
				mMemory[op0] = op1;
			}else if (op0 < VM_MEMORY_VRAM_BASE){ //IO書き込み領域からVRAMまで
				beginError();
				*mMessageStream << L"このあたりのメモリは使えない(番号:" << op0 << L")" << std::endl;
			}else if (op0 < (VM_MEMORY_VRAM_BASE + (mScreenWidth * mScreenHeight))){ //VRAM書き込み
				mMemory[op0] = op1;
				//自動sync
				if (mMemory[VM_MEMORY_DISABLE_AUTO_SYNC] == 0){ 
					sync(true);
				}
			}else{
				beginError();
				*mMessageStream << L"メモリ範囲外をいじろうとした(番号:" << op0 << L")" << std::endl;
			}
		}
	}else if (inst > INSTRUCTION_CALL){
		bool jump = true;
		if (inst == INSTRUCTION_BZ){
			int op0 = pop();
			if (op0 != 0){
				jump = false;
			}
		}
		if (jump){
			mProgramCounter = imm - 1 + mProgramBegin; //絶対アドレス。後で足されるので1引いておく
		}
	}else if (inst > INSTRUCTION_POP){
		if (inst == INSTRUCTION_CALL){
			push(mFramePointer);
			push(mProgramCounter);
			mFramePointer = mStackPointer;
			mProgramCounter = imm - 1 + mProgramBegin; //後で足されるので1引いておく
			//デバグ情報入れる
			if (!mDebugInfo.push(imm, mFramePointer)){
				beginError();
				*mMessageStream << L"部分プログラムを激しく呼びすぎ。たぶん再帰に間違いがある(命令:" << mProgramCounter << L")" << std::endl;
			}
		}else{ //INSTRUCTION_RET
			pop(imm);
			mProgramCounter = pop(); //絶対アドレス。後で+1されるので、それで飛んだアドレスの次になる
			mFramePointer = pop();
			if (!mDebugInfo.pop()){
				beginError();
				*mMessageStream << L"ret命令が多すぎている。絶対おかしい(命令:" << mProgramCounter << L")" << std::endl;
			}
		}
	}else{ //INSTRUCTION_POP
		pop(imm);
	}
#if 1 //TODO: これでいいよなあ？エラーこいたら上流で止めるわけで、そこでPCが1多くても困らないだろ？
	++mProgramCounter;
#else
	if (!mError){
		++mProgramCounter;
	}
#endif
	mExecutedInstructionCount += 1.0;
}

} //namespce Sunaba
