namespace Sunaba
{
	public enum Instruction
	{
		//bit31=1
		I = 0x80, //immはSint31
		//bit30=1 算術命令 //現状immはないが、あれば27bit
		Add = 0x78,
		Sub = 0x70,
		Mul = 0x68,
		Div = 0x60,
		Lt = 0x58,
		Le = 0x50,
		Eq = 0x48,
		Ne = 0x40,
		//bit29=1 ロードストア命令
		Ld = 0x38, //immはSint27
		St = 0x30, //immはSint27
		Fld = 0x28, //immはSint27
		Fst = 0x20, //immはSint27
		//フロー制御命令
		J = 0x1c, //immはUint26 無条件ジャンプ
		Bz = 0x18, //immはUint26 topが0ならジャンプ
		Call = 0x14, //immはUint26 push(FP),push(CP),FP=SP,CP=imm
		Ret = 0x10, //immはSint26 pop(imm), CP=pop(), FP=pop() 
		Pop = 0x0c, //immはSint26

		Unknown = -1,
	}

	public static class InstructionUtil
	{
		public static Instruction NameToInstruction(string s, int l)
		{
			if (infoTable == null)
			{
				MakeInfoTable();
			}
			var r = Instruction.Unknown;
			for (var i = 0; i < infoTable.Length; i++)
			{
				if (s == infoTable[i].name)
				{
					r = infoTable[i].instruction;
					break;
				}
			}
			return r;
		}

		// non public -------
		struct Info
		{
			public Info(string name, Instruction instruction)
			{
				this.name = name;
				this.instruction = instruction;
			}
			public string name;
			public Instruction instruction;
		}

		static void MakeInfoTable()
		{
			infoTable = new Info[]
			{
				new Info("i", Instruction.I),
				new Info("add", Instruction.Add),
				new Info("sub", Instruction.Sub),
				new Info("mul", Instruction.Mul),
				new Info("div", Instruction.Div),
				new Info("lt", Instruction.Lt),
				new Info("le", Instruction.Le),
				new Info("eq", Instruction.Eq),
				new Info("ne", Instruction.Ne),
				new Info("ld", Instruction.Ld),
				new Info("st", Instruction.St),
				new Info("fld", Instruction.Fld),
				new Info("fst", Instruction.Fst),
				new Info("j", Instruction.J),
				new Info("bz", Instruction.Bz),
				new Info("call", Instruction.Call),
				new Info("ret", Instruction.Ret),
				new Info("pop", Instruction.Pop),
			};		
		}
		static Info[] infoTable;
	}

	public static class ImmUtil
	{
		public static int GetU(uint inst, int bitCount)
		{
			var immU = inst & ((1U << bitCount) - 1); //下位31bit
			return (int)immU;

		}

		public static int GetS(uint inst, int bitCount)
		{
			var immU = GetU(inst, bitCount);
			var immS = (immU >= (1 << (bitCount - 1))) ? (immU - (1 << bitCount)) : immU;
			return immS;
		}

		public static int GetMaxU(int bitCount)
		{
			return (1 << bitCount) - 1;
		}

		public static int GetMaxS(int bitCount)
		{
			return (1 << (bitCount - 1)) - 1;
		}

		public static int GetMask(int bitCount)
		{
			return (1 << bitCount) - 1;
		}
	}

	public static class HeadBit
	{
		public const int I = 31;
		public const int Alu = 30;
		public const int Ls = 29;
	}

	public static class ImmBitCount
	{
		public const int I = 31;
		public const int Ls = 27;
		public const int Flow = 26;
	}
}