using System.Collections;
using System.Collections.Generic;
using System.Threading;
using System.Diagnostics;
using System.Text;

namespace Sunaba
{
	public class Machine
	{
		//設定定数。ただし、いじるとIOメモリの番号が変わるので、ソースコードが非互換になる。
		public const int FreeAndProgramSize = 40000;
		public const int StackSize = 10000;
		public const int IoMemorySize = 10000;
		public const int IoWritableOffset = 5000;

		public const int ExecutionUnit = 10000; //これだけの命令実行したらウィンドウ状態を見に行く。
		public const int MinimumFreeSize = 1000; //最低これだけのメモリは0から空いている。プログラムサイズが制約される。
		public const int VmMemoryStackBase = FreeAndProgramSize;
		public const int VmMemoryStackEnd = VmMemoryStackBase + StackSize;
		public const int VmMemoryIoBase = VmMemoryStackEnd;
		public const int VmMemoryIoEnd = VmMemoryIoBase + IoMemorySize;
		public const int VmMemoryVramBase = VmMemoryIoEnd; //解像度に応じてメモリの総量は変化する。

		public enum VmMemory
		{
			//READ
			IoReadableBegin = VmMemoryIoBase,
			PointerX = IoReadableBegin,
			PointerY,
			PointerButtonLeft,
			PointerButtonRight,
			KeyUp,
			KeyDown,
			KeyLeft,
			KeyRight,
			KeySpace,
			KeyEnter,
			FreeRegionEnd,
			GetScreenWidth,
			GetScreenHeight,
			ScreenBegin,
			//終了番号
			IoReadableEnd,
			//WRITE
			IoWritableBegin = VmMemoryIoBase + IoWritableOffset,
			Sync = IoWritableBegin,
			DisableAutoSync,
			DrawChar,
			Break,
			SetScreenWidth,
			SetScreenHeight,
			SetSoundFrequency0,
			SetSoundFrequency1,
			SetSoundFrequency2,
			SetSoundDumping0,
			SetSoundDumping1,
			SetSoundDumping2,
			SetSoundAmplitude0,
			SetSoundAmplitude1,
			SetSoundAmplitude2,
			IoWritableEnd,
		};

		public bool Error { get; private set; }
		public int ScreenWidth { get; private set; }
		public int ScreenHeight { get; private set; }

		public Machine(
			StringBuilder messageStream,
			byte[] objectCode,
			bool inMainThread,
			bool outputDecoded)
		{
			Error = true; // 最初trueにしておき、正常終了したらfalseにする
			ScreenWidth = ScreenHeight = 100;
			debugInfo = new DebugInfo();

			this.messageStream = messageStream;

			var vramSize = ScreenWidth * ScreenHeight;
			memory = new int[VmMemoryVramBase + vramSize];

			ioState = new IoState();
			ioState.AllocateMemoryCopy(
				memory.Length, 
				VmMemoryIoBase,
				VmMemoryVramBase);
			endRequestEvent = new ManualResetEvent(false);
			terminatedEvent = new ManualResetEvent(false);
				
			//オブジェクトコードを命令列に変換
			//すぐできるエラーチェック
			var instructionCount = objectCode.Length  / 4;
			if ((objectCode.Length % 4) != 0)
			{
				messageStream.Append("不正なコード(32bit単位でない)。\n");
				return;
			}
			else if (instructionCount >= (FreeAndProgramSize - MinimumFreeSize))
			{
				messageStream.AppendFormat(
					"プログラムが大きすぎる({0}命令)。最大{1}命令。\n",
					instructionCount, 
					(FreeAndProgramSize - MinimumFreeSize));
				return;
			}
			//0番地からプログラムをコピー
			programBegin = FreeAndProgramSize - instructionCount; //ここから実行開始
			for (var i = 0; i < instructionCount; ++i){
				memory[programBegin + i] = (objectCode[(i * 4) + 0] << 24);
				memory[programBegin + i] += (objectCode[(i * 4) + 1] << 16);
				memory[programBegin + i] += (objectCode[(i * 4) + 2] << 8);
				memory[programBegin + i] += (objectCode[(i * 4) + 3] << 0);
			}
			memory[(int)VmMemory.FreeRegionEnd] = programBegin; //プログラム開始位置を入れる
			memory[(int)VmMemory.GetScreenWidth] = memory[(int)VmMemory.SetScreenWidth] = ScreenWidth;
			memory[(int)VmMemory.GetScreenHeight] = memory[(int)VmMemory.SetScreenHeight] = ScreenHeight;
			memory[(int)VmMemory.ScreenBegin] = VmMemoryVramBase;
			for (var i = 0; i < IoState.SoundChannelCount; ++i)
			{
				memory[(int)VmMemory.SetSoundDumping0 + i] = 1000; //減衰初期値。これでノイズを殺す。
				memory[(int)VmMemory.SetSoundFrequency0 + i] = 20; 
			}

			//事前デコード
			if (Decode())
			{
				Error = false;
				//実行開始
				programCounter = programBegin;
				if (!inMainThread)
				{
					executionThread = new Thread(ThreadFunc);
				}

				if (outputDecoded)
				{
					var sb = new System.Text.StringBuilder();
					for (var i = 0; i < this.decodedInsts.Length; i++)
					{
						var inst = decodedInsts[i];
						sb.AppendFormat("{0}\t{1}\t{2}\n", i, inst.inst, inst.imm);
					}
					Utility.WriteDebugFile("decoded.txt", sb.ToString());
				}
			}
		}

		public void Dispose()
		{
			endRequestEvent.Set();
			if (executionThread != null)
			{
				executionThread.Join();
			}
			messageStream = null;
			//endTimer(); // これなんだ?
		}

		public bool IsTerminated() //プログラム終了してますか
		{
			return terminatedEvent.WaitOne(0);
		}

		public int OutputValue() //終わってからしか呼べない
		{
			if (!IsTerminated())
			{
				throw new System.Exception("Machine.OutputValue called before termination.");
			}
			return memory[VmMemoryStackBase];
		}

		public IoState BeginSync()
		{
			if (executionThread == null) // 実行
			{
				var end = false;
				Execute(out end);
				if (end)
				{
					terminatedEvent.Set();
				}
			}

			ioState.Lock();
			return ioState;
		}

		public void EndSync()
		{
			ioState.Unlock();
		}

		// non public ------------
		//デバグ関連
		struct CallInfo
		{
			public void Set(int pc, int fp)
			{
				programCounter = pc;
				framePointer = fp;
			}
			
			public void Reset()
			{
				programCounter = -0x7fffffff;
				framePointer = -0x7fffffff;
			}

			public int programCounter;
			public int framePointer;
		}

		class DebugInfo
		{
			public DebugInfo()
			{
				callHistory = new CallInfo[CallHistorySize];
				foreach (var item in callHistory)
				{
					item.Reset();
				}
			}

			public bool Push(int pc, int fp)
			{
				if (callHistoryPosition >= CallHistorySize)
				{
					return false;
				}
				callHistory[callHistoryPosition].Set(pc, fp);
				callHistoryPosition++;
				return true;
			}

			public bool Pop()
			{
				if (callHistoryPosition <= 0)
				{
					return false;
				}
				callHistoryPosition--;
				callHistory[callHistoryPosition].Reset();
				return true;
			}

			public CallInfo[] callHistory;
			public int callHistoryPosition;
			public int maxStackPointer;

			// non public ------
			const int CallHistorySize = 256;
		}

		//最適化実行
		struct DecodedInst
		{
			public void Set(Instruction i, int imm)
			{
				this.inst = i;
				this.imm = imm;
			}
			public Instruction inst;
			public int imm;
		}

		IoState ioState;
		ManualResetEvent endRequestEvent;
		ManualResetEvent terminatedEvent;
		ManualResetEvent syncRequestEvent;
		Thread executionThread;
		int frameCount;
		int programBegin;
		//VMレジスタ
		int programCounter = VmMemoryStackBase;
		int stackPointer = VmMemoryStackBase;
		int framePointer;

		bool charDrawn;
		int[] memory;
		StringBuilder messageStream;
		DebugInfo debugInfo;
		long executedInstructionCount;
		DecodedInst[] decodedInsts;

		void Execute(out bool end)
		{
			end = false;
			for (var i = 0; i < ExecutionUnit; ++i)
			{
				if (Error || (programCounter == FreeAndProgramSize)){ //終わりまで来た
					end = true; //終わります
					//正常終了時デバグコード
					if (!Error)
					{
						Debug.Assert(programCounter == VmMemoryStackBase);
						Debug.Assert(stackPointer == VmMemoryStackBase);
						Debug.Assert(framePointer == VmMemoryStackBase);
					}
					break;
				}
				ExecuteDecoded(); //最適化版
			}
		}

		void ThreadFunc()
		{
			Sync(true); //一回sync。起動直後に押されたキーを反映させる。vsyncしないとUIからの入力が来ない
			var end = false;
			while (!end)
			{
				if (endRequestEvent.WaitOne(0))
				{
					end = true;
				}
				else
				{
					Execute(out end);
				}
			}
			terminatedEvent.Set();
		}

		int Pop()
		{
			if (stackPointer <= VmMemoryStackBase)
			{
				BeginError();
				messageStream.Append("ポップしすぎてスタック領域をはみ出した。\n");
				return 0; //とりあえず0を返し、後で抜ける
			}
			else
			{
				--stackPointer;
				return memory[stackPointer];
			}
		}

		void Pop(int n) //マイナスならプッシュ
		{
			if ((stackPointer - n) < VmMemoryStackBase)
			{
				BeginError();
				messageStream.Append("ポップしすぎてスタック領域をはみ出した。\n");
			}
			else if ((stackPointer - n) >= VmMemoryStackEnd)
			{
				BeginError();
				messageStream.Append("スタックを使い切った。名前つきメモリを使いすぎ。\n");
			}
			else
			{
				stackPointer -= n;
				if (debugInfo.maxStackPointer < stackPointer)
				{
					debugInfo.maxStackPointer = stackPointer;
				}
			}
		}

		void Push(int a)
		{
			if (stackPointer >= VmMemoryStackEnd)
			{
				BeginError();
				messageStream.Append("スタックを使い切った。名前つきメモリを使いすぎ。\n");
			}
			else
			{
				memory[stackPointer] = a;
				++stackPointer;
				if (debugInfo.maxStackPointer < stackPointer)
				{
					debugInfo.maxStackPointer = stackPointer;
				}
			}
		}

		//メインスレッドとの同期
		void Sync(bool waitVSync)
		{
			//時刻測定
			if (waitVSync)
			{
				var newFrame = frameCount;
				var syncBeginTime = System.DateTime.Now;
				//vsync待ち。同じフレームに二度は呼ばない
				while (true)
				{
					if (endRequestEvent.WaitOne(0))
					{
						return;
					}
					ioState.Lock();
					newFrame = ioState.FrameCount;
					ioState.Unlock();
					if (newFrame != frameCount)
					{
						break;
					}
					Thread.Sleep(1);
				}
				frameCount = newFrame;
			}

			ioState.Lock();
			//外->Machine
			memory[(int)VmMemory.PointerX] = ioState.PointerX;
			memory[(int)VmMemory.PointerY] = ioState.PointerY;
			memory[(int)VmMemory.PointerButtonLeft] = ioState.GetKey(IoState.Key.LButton);
			memory[(int)VmMemory.PointerButtonRight] = ioState.GetKey(IoState.Key.RButton);
			memory[(int)VmMemory.KeyUp] = ioState.GetKey(IoState.Key.Up);
			memory[(int)VmMemory.KeyDown] = ioState.GetKey(IoState.Key.Down);
			memory[(int)VmMemory.KeyLeft] = ioState.GetKey(IoState.Key.Left);
			memory[(int)VmMemory.KeyRight] = ioState.GetKey(IoState.Key.Right);
			memory[(int)VmMemory.KeySpace] = ioState.GetKey(IoState.Key.Space);
			memory[(int)VmMemory.KeyEnter] = ioState.GetKey(IoState.Key.Enter);

			//Machine->外(全メモリコピー)
			if ( //解像度変更
			(memory[(int)VmMemory.SetScreenWidth] != ScreenWidth) ||
			(memory[(int)VmMemory.SetScreenHeight] != ScreenHeight)){
				ScreenWidth = memory[(int)VmMemory.SetScreenWidth];
				ScreenHeight = memory[(int)VmMemory.SetScreenHeight];
				memory[(int)VmMemory.GetScreenWidth] = ScreenWidth;
				memory[(int)VmMemory.GetScreenHeight] = ScreenHeight;
				ioState.OnScreenSizeChanged();
				//VRAM取り直し
				var vramSize = ScreenWidth * ScreenHeight;
				var tmpMemory = new int[VmMemoryVramBase + vramSize];
				for (var i = 0; i < VmMemoryVramBase; i++)
				{
					tmpMemory[i] = memory[i];
				}
				memory = tmpMemory;
				ioState.AllocateMemoryCopy(memory.Length, VmMemoryIoBase, VmMemoryVramBase);
			}
			for (var i = 0; i < memory.Length; i++)
			{
				ioState.MemoryCopy[i] = memory[i];
			}
			//発音命令はリセット
			for (var i = 0; i < IoState.SoundChannelCount; ++i)
			{
				if (memory[(int)VmMemory.SetSoundAmplitude0 + i] != 0)
				{
					ioState.RequestSound(i, memory[(int)VmMemory.SetSoundAmplitude0 + i]);
					memory[(int)VmMemory.SetSoundAmplitude0 + i] = 0;
				}
			}
			//終了
			ioState.Unlock();
		}

		void SyncInput()
		{
			ioState.Lock();
			//外->Machine
			memory[(int)VmMemory.PointerX] = ioState.PointerX;
			memory[(int)VmMemory.PointerY] = ioState.PointerY;
			memory[(int)VmMemory.PointerButtonLeft] = ioState.GetKey(IoState.Key.LButton);
			memory[(int)VmMemory.PointerButtonRight] = ioState.GetKey(IoState.Key.RButton);
			memory[(int)VmMemory.KeyUp] = ioState.GetKey(IoState.Key.Up);
			memory[(int)VmMemory.KeyDown] = ioState.GetKey(IoState.Key.Down);
			memory[(int)VmMemory.KeyLeft] = ioState.GetKey(IoState.Key.Left);
			memory[(int)VmMemory.KeyRight] = ioState.GetKey(IoState.Key.Right);
			memory[(int)VmMemory.KeySpace] = ioState.GetKey(IoState.Key.Space);
			memory[(int)VmMemory.KeyEnter] = ioState.GetKey(IoState.Key.Enter);
			ioState.Unlock();
		}

		void BeginError()
		{
			if (charDrawn)
			{
				messageStream.Append('\n');
				charDrawn = false;
			}
			Error = true;
		}

		//最適化実行
		bool Decode()
		{
			var instCount = FreeAndProgramSize - programBegin;
			decodedInsts = new DecodedInst[instCount];
			for (var i = 0; i < instCount; ++i)
			{
				var inst = (uint)memory[programBegin + i];
				var inst8 = (inst & 0xff000000U) >> 24;
				int imm = 0;
				if ((inst & (1U << HeadBit.I)) != 0)
				{ //即値ロード
					inst8 = 0x80;
					imm = ImmUtil.GetS(inst, ImmBitCount.I);
				}
				else if ((inst & (1U << HeadBit.Alu)) != 0)
				{ //算術命令
					inst8 &= 0x78;
					if (
						(inst8 != (uint)Instruction.Add) &&
						(inst8 != (uint)Instruction.Sub) &&
						(inst8 != (uint)Instruction.Mul) &&
						(inst8 != (uint)Instruction.Div) &&
						(inst8 != (uint)Instruction.Lt) &&
						(inst8 != (uint)Instruction.Le) &&
						(inst8 != (uint)Instruction.Eq) &&
						(inst8 != (uint)Instruction.Ne))
					{
						BeginError();
						messageStream.AppendFormat("存在しない算術命令。おそらく壊れている(コード:{0:x})\n", inst);
						return false;
					}
				}
				else if ((inst & (1U << HeadBit.Ls)) != 0)
				{ //ロードストア
					inst8 &= 0x38;
					if (
						(inst8 != (int)Instruction.Ld) &&
						(inst8 != (int)Instruction.St) &&
						(inst8 != (int)Instruction.Fld) &&
						(inst8 != (int)Instruction.Fst))
					{
						BeginError();
						messageStream.AppendFormat("存在しないロードストア命令。おそらく壊れている(コード:{0:x})\n", inst);
						return false;
					}
					imm = ImmUtil.GetS(inst, ImmBitCount.Ls);
				}
				else
				{ //分岐、関数コール、スタック操作系
					inst8 &= 0x1c;
					if (
						(inst8 != (int)Instruction.J) &&
						(inst8 != (int)Instruction.Bz) &&
						(inst8 != (int)Instruction.Call) &&
						(inst8 != (int)Instruction.Ret) &&
						(inst8 != (int)Instruction.Pop))
					{
						BeginError();
						messageStream.AppendFormat("存在しないフロー制御命令。おそらく壊れている(コード:{0:x})\n", inst);
						return false;
					}

					if (inst8 == (int)Instruction.Pop)
					{//popだけ符号つき
						imm = ImmUtil.GetS(inst, ImmBitCount.Flow);
					}
					else
					{
						imm = ImmUtil.GetU(inst, ImmBitCount.Flow);
					}
				}
				decodedInsts[i].Set((Instruction)inst8, imm);
			}
			return true;
		}

		void ExecuteDecoded()
		{
			var decoded = decodedInsts[programCounter - programBegin];
			var inst = (int)decoded.inst;
			var imm = decoded.imm;
			if (inst > (int)Instruction.Ld)
			{
				if (inst > (int)Instruction.Lt)
				{
					if (inst > (int)Instruction.Mul)
					{
						if (inst == (int)Instruction.I)
						{
							Push(imm);
						}
						else if (inst == (int)Instruction.Add)
						{
							var op1 = Pop(); //先に入れた方が後から出てくるので1,0の順
							var op0 = Pop();
							Push(op0 + op1);
						}
						else
						{ //INSTRUCTION_SUB
							var op1 = Pop(); //先に入れた方が後から出てくるので1,0の順
							var op0 = Pop();
							Push(op0 - op1);
						}
					}
					else if (inst == (int)Instruction.Mul)
					{
						var op1 = Pop(); //先に入れた方が後から出てくるので1,0の順
						var op0 = Pop();
						Push(op0 * op1);
					}
					else
					{ //INSTRUCTION_DIV
						var op1 = Pop(); //先に入れた方が後から出てくるので1,0の順
						if (op1 == 0)
						{
							BeginError();
							messageStream.Append("0では割れない。\n");
						}
						else
						{
							var op0 = Pop();
							Push(op0 / op1);
						}
					}
				}
				else if (inst > (int)Instruction.Eq)
				{
					if (inst == (int)Instruction.Lt)
					{
						var op1 = Pop(); //先に入れた方が後から出てくるので1,0の順
						var op0 = Pop();
						Push((op0 < op1) ? 1 : 0);
					}
					else
					{ //INSTRUCTION_LE
						var op1 = Pop(); //先に入れた方が後から出てくるので1,0の順
						var op0 = Pop();
						Push((op0 <= op1) ? 1 : 0);
					}
				}
				else if (inst == (int)Instruction.Eq)
				{
					var op1 = Pop(); //先に入れた方が後から出てくるので1,0の順
					var op0 = Pop();
					Push((op0 == op1) ? 1 : 0);
				}
				else
				{ //INSTRUCTION_NE
					var op1 = Pop(); //先に入れた方が後から出てくるので1,0の順
					var op0 = Pop();
					Push((op0 != op1) ? 1 : 0);
				}
			}
			else if (inst > (int)Instruction.J)
			{
				if ((inst == (int)Instruction.Ld) || (inst == (int)Instruction.Fld))
				{
					int op0;
					if (inst > (int)Instruction.Fld)
					{
						op0 = Pop();
					}
					else
					{
						op0 = framePointer;
					}
					op0 += imm;
					if (op0 < 0)
					{
						BeginError();
						messageStream.AppendFormat("マイナスの番号のメモリを読みとろうとした(番号:{0})\n", op0);
					}
					else if (op0 < programBegin)
					{ //正常実行
						Push(memory[op0]);
					}
					else if (op0 < VmMemoryStackBase)
					{ //プログラム領域
						BeginError();
						messageStream.AppendFormat("プログラムが入っているメモリを読みとろうとした(番号:{0})\n", op0);
					}
					else if (op0 < VmMemoryIoBase)
					{ //スタック。正常実行
						Push(memory[op0]);
					}
					else if (op0 < (int)VmMemory.IoReadableEnd)
					{ //読み取り可能メモリ
						SyncInput(); //入力だけ
						Push(memory[op0]);
					}
					else if (op0 < VmMemoryVramBase)
					{ //書きこみ専用メモリ
						BeginError();
						messageStream.AppendFormat("このあたりのメモリはセットはできるが読み取ることはできない(番号:{0}\n", op0);
					}
					else if (op0 < (VmMemoryVramBase + (ScreenWidth * ScreenHeight)))
					{
						BeginError();
						messageStream.AppendFormat("画面メモリは読み取れない(番号:{0})\n", op0);
					}
					else
					{
						BeginError();
						messageStream.AppendFormat("メモリ範囲外を読みとろうとした(番号:{0})\n", op0);
					}
				}
				else
				{ //store
					var op1 = Pop();
					int op0;
					if (inst > (int)Instruction.Fld)
					{
						op0 = Pop();
					}
					else
					{
						op0 = framePointer;
					}
					op0 += imm;
					if (op0 < 0)
					{ //負アドレス
						BeginError();
						messageStream.AppendFormat("マイナスの番号のメモリを変えようとした(番号:{0})\n", op0);
					}
					else if (op0 < programBegin)
					{ //正常実行
						memory[op0] = op1;
					}
					else if (op0 < VmMemoryStackBase)
					{ //プログラム領域
						BeginError();
						messageStream.AppendFormat("プログラムが入っているメモリに{0}をセットしようとした(番号:{1})\n", op1, op0);
					}
					else if (op0 < VmMemoryIoBase)
					{ //スタック。正常実行
						memory[op0] = op1;
					}
					else if (op0 < (int)VmMemory.IoWritableBegin)
					{ //IOのうち書きこみ不可能領域
						BeginError();
						messageStream.AppendFormat("このあたりのメモリは読み取れはするが、セットはできない(番号:{0})\n", op0);
					}
					else if (op0 == (int)VmMemory.Sync)
					{ //vsync待ち検出
						Sync(true); //op1は見ていない。なんでもいい。
					}
					else if (op0 == (int)VmMemory.DisableAutoSync)
					{ //自動SYNCモード
						memory[op0] = (op1 == 0) ? 0 : 1;
					}
					else if (op0 == (int)VmMemory.DrawChar)
					{ //デバグ文字出力
						if (op1 == '\n')
						{
							messageStream.Append('\n');
						}
						else
						{
							messageStream.Append((char)op1);
							charDrawn = true;
						}
					}
					else if (op0 == (int)VmMemory.Break)
					{ //ブレークポイント
						Sync(false); //同期を走らせて
						memory[op0] = op1; //値書きかえ
					}
					else if (op0 == (int)VmMemory.SetScreenWidth)
					{
						if (op1 <= 0)
						{
							BeginError();
							messageStream.Append("横解像度として0以下の値を設定した\n");
						}
						else if (op1 > 512)
						{
							BeginError();
							messageStream.Append("横解像度の最大は512\n");
						}
						memory[op0] = op1;
					}
					else if (op0 == (int)VmMemory.SetScreenHeight)
					{
						if (op1 <= 0)
						{
							BeginError();
							messageStream.Append("縦解像度として0以下の値を設定した\n");
						}
						else if (op1 > 512)
						{
							BeginError();
							messageStream.Append("縦解像度の最大は512\n");
						}
						memory[op0] = op1;
					}
					else if ((op0 >= (int)VmMemory.SetSoundFrequency0) && (op0 <= (int)VmMemory.SetSoundAmplitude2))
					{
						//自動sync
						if (memory[(int)VmMemory.DisableAutoSync] == 0)
						{ 
							Sync(true);
						}
						memory[op0] = op1;
					}
					else if (op0 < VmMemoryVramBase)
					{ //IO書き込み領域からVRAMまで
						BeginError();
						messageStream.AppendFormat("このあたりのメモリは使えない(番号:{0})\n");
					}
					else if (op0 < (VmMemoryVramBase + (ScreenWidth * ScreenHeight)))
					{ //VRAM書き込み
						memory[op0] = op1;
						//自動sync
						if (memory[(int)VmMemory.DisableAutoSync] == 0)
						{ 
							Sync(true);
						}
					}
					else
					{
						BeginError();
						messageStream.AppendFormat("メモリ範囲外をいじろうとした(番号:{0})\n", op0);
					}
				}
			}
			else if (inst > (int)Instruction.Call)
			{
				var jump = true;
				if (inst == (int)Instruction.Bz)
				{
					var op0 = Pop();
					if (op0 != 0)
					{
						jump = false;
					}
				}

				if (jump)
				{
					programCounter = imm - 1 + programBegin; //絶対アドレス。後で足されるので1引いておく
				}
			}
			else if (inst > (int)Instruction.Pop)
			{
				if (inst == (int)Instruction.Call)
				{
					Push(framePointer);
					Push(programCounter);
					framePointer = stackPointer;
					programCounter = imm - 1 + programBegin; //後で足されるので1引いておく
					//デバグ情報入れる
					if (!debugInfo.Push(imm, framePointer))
					{
						BeginError();
						messageStream.AppendFormat("部分プログラムを激しく呼びすぎ。たぶん再帰に間違いがある(命令:{0})\n", programCounter);
					}
				}
				else
				{ //INSTRUCTION_RET
					Pop(imm);
					programCounter = Pop(); //絶対アドレス。後で+1されるので、それで飛んだアドレスの次になる
					framePointer = Pop();
					if (!debugInfo.Pop())
					{
						BeginError();
						messageStream.AppendFormat("ret命令が多すぎている。絶対おかしい(命令:{0})\n", programCounter);
					}
				}
			}
			else
			{ //INSTRUCTION_POP
				Pop(imm);
			}
			++programCounter;
			executedInstructionCount += 1;
		}
	}
} //namespce Sunaba
