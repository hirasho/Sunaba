using System.Collections.Generic;
using System;
using System.Text;
using System.Diagnostics;

namespace Sunaba
{
	public class Assembler
	{
		public static bool Process(
			List<uint> instructionsOut,
			StringBuilder messageStream,
			IList<char> compiled,
			Localization localization,
			bool outputIntermediates)
		{
			//タブ処理
			var tabProcessed = new List<char>();
			TabProcessor.Process(tabProcessed, compiled);
			if (outputIntermediates)
			{
				Utility.WriteDebugFile("assemblerTabProcessed.txt", new string(tabProcessed.ToArray()));
			}
			//読めるテキストが来たケースに備えて念のため文字置換
			var replaced = new List<char>();
			CharacterReplacer.Process(replaced, tabProcessed, localization);
			if (outputIntermediates)
			{
				Utility.WriteDebugFile("assemblerReplaced.txt", new string(replaced.ToArray()));
			}
			//コメントを削除する。行数は変えない。
			var commentRemoved = new List<char>();
			CommentRemover.Process(commentRemoved, replaced);
			if (outputIntermediates)
			{
				Utility.WriteDebugFile("assemblerCommentRemoved.txt", new string(commentRemoved.ToArray()));
			}
			//まずトークン分解
			var tokens = new List<Token>();
			var assembler = new Assembler(messageStream);
			assembler.Tokenize(tokens, commentRemoved.ToArray());
			if (outputIntermediates)
			{
				var sb = new System.Text.StringBuilder();
				foreach (var token in tokens)
				{
					sb.AppendFormat("{0}: {1} {2} {3} {4}\n", token.line, token.type, token.instruction, token.number, token.str);
				}
				Utility.WriteDebugFile("assemblerTokenized.txt", sb.ToString());
			}
			//ラベルだけ処理します。
			assembler.CollectLabel(tokens);
			//パースします。
			if (!assembler.Parse(instructionsOut, tokens))
			{
				return false;
			}

			//分岐先アドレス解決
			if (!assembler.ResolveLabelAddress(instructionsOut))
			{
				return false;
			}

			if (outputIntermediates)
			{
				var sb = new System.Text.StringBuilder();
				for (var i = 0; i < instructionsOut.Count; i++)
				{
					sb.AppendFormat("{0}\t{1:x8}\n", i, instructionsOut[i]);
				}
				Utility.WriteDebugFile("assembled.txt", sb.ToString());
			}
			return true;
		}

		// non public --------
		enum TokenType
		{
			Instruction,
			Identifier,
			Number,
			NewLine,
			LabelEnd,
			Unknown,
		};

		enum Mode
		{
			Space, //スペース上
			String, //何らかの文字列。それが何であるかはToken生成時に自動判別する。
		}

		class Label
		{
			public int id = -1; //ラベルID
			public int address = -1; //ラベルのアドレス
		}

		class Token
		{
			public Token(int line) // Unknownのまま。最後の番兵データ
			{
				this.line = line;
			}

			public Token(ReadOnlySpan<char> s, int line)
			{
				this.line = line;
				if (s.Length == 0)
				{ //長さ0なら改行トークン
					type = TokenType.NewLine;
				}
				else if ((s.Length == 1) && (s[0] == ':'))
				{
					type = TokenType.LabelEnd;
				}
				else if (Utility.ConvertNumber(out number, s))
				{
					type = TokenType.Number;
				}
				else
				{
					instruction = InstructionUtil.NameToInstruction(s);
					if (instruction != Instruction.Unknown)
					{
						type = TokenType.Instruction;
					}
					else if (Utility.IsAsmName(s))
					{
						type = TokenType.Identifier;
						str = new string(s);
					}
				}
			}

			public string str;
			public TokenType type = TokenType.Unknown;
			public Instruction instruction = Instruction.Unknown;
			public int line = 0;
			public int number = 0; //数値ならここで変換してしまう。
		}

		Dictionary<string, Label> labelNameMap; //ラベル名->ラベル
		Dictionary<int, Label> labelIdMap; //ラベルID->ラベル
		StringBuilder messageStream;

		Assembler(StringBuilder messageStream)
		{
			this.messageStream = messageStream;
			labelNameMap = new Dictionary<string, Label>();
			labelIdMap = new Dictionary<int, Label>();
		}

		void Tokenize(List<Token> output, char[] input)
		{
			var line = 0;
			var s = input.Length;
			var mode = Mode.Space;
			var begin = 0; //tokenの開始点
			int l;
			for (var i = 0; i < s; ++i)
			{
				var c = input[i];
				l = i - begin; //現トークンの文字列長
				switch (mode)
				{
					case Mode.Space:
						if (c == '\n')
						{ //改行が来たら
							//改行トークンを追加
							output.Add(new Token(new ReadOnlySpan<char>(input, 0, 0), line)); //改行
							++line;
						}
						else if (c == ' ')
						{ //スペースが来たら、
							; //何もしない
						}
						else if (c == ':')
						{ //ラベル終わり
							output.Add(new Token(new ReadOnlySpan<char>(input, i, 1), line));
						}
						else
						{ //スペースでも:でもない文字が来たら、何かの始まり
							mode = Mode.String;
							begin = i; //この文字から始まり
						}
						break;
					case Mode.String:
						if (c == '\n')
						{ //改行が来たら
							//ここまでを出力
							Debug.Assert(l > 0);
							output.Add(new Token(new ReadOnlySpan<char>(input, begin, l), line));
							//改行トークンを追加
							output.Add(new Token(new ReadOnlySpan<char>(input, 0, 0), line)); //改行
							mode = Mode.Space;
							++line;
						}
						else if (c == ' ')
						{ //スペースが来たら
							//ここまでを出力
							Debug.Assert(l > 0);
							output.Add(new Token(new ReadOnlySpan<char>(input, begin, l), line));
							mode = Mode.Space;
						}
						else if (c == ':')
						{ //:が来たら
							//ここまでを出力
							Debug.Assert(l > 0);
							output.Add(new Token(new ReadOnlySpan<char>(input, begin, l), line));
							//加えて:を出力
							output.Add(new Token(new ReadOnlySpan<char>(input, i, 1), line));
							mode = Mode.Space;
						} //else 文字列を継続
						break;
					default: Debug.Assert(false); 
						break;
				}
			}
			//最後のトークンを書き込み
			l = s - begin;
			if ((l > 0) && (mode == Mode.String))
			{
				output.Add(new Token(new ReadOnlySpan<char>(input, begin, l), line));
			}
			output.Add(new Token(line)); //ダミートークンを書き込み
		}

		void CollectLabel(List<Token> input)
		{
			var tokenCount = input.Count;
			var labelId = 0;
			for (var i = 1; i < tokenCount; ++i)
			{
				//トークンを取り出して
				var t0 = input[i - 1];
				var t1 = input[i];
				if ((t0.type == TokenType.Identifier) && (t1.type == TokenType.LabelEnd))
				{ //ラベルだったら
					//ラベルmapにLabelを放り込む
					var label = new Label();
					label.id = labelId;
					++labelId;
					labelNameMap.Add(t0.str, label);
					labelIdMap.Add(label.id, label);
					//次のトークンが存在しないか、改行である必要がある。ラベルに続けて書くのは遺法
					Debug.Assert((i == (tokenCount - 1)) || (input[i + 1].type == TokenType.NewLine));
				}
			}
		}

		bool Parse(List<uint> output, IList<Token> input)
		{
			var pos = 0;
			while (input[pos].type != TokenType.Unknown)
			{
				//トークンを取り出してみる
				var t = input[pos].type;
				//タイプに応じて構文解析
				if (t == TokenType.Identifier)
				{ //ラベル発見
					if (!ParseLabel(output, input, ref pos))
					{
						return false;
					}
				}
				else if (t == TokenType.Instruction)
				{ //命令発見
					if (!ParseInstruction(output, input, ref pos))
					{
						return false;
					}
				}
				else if (t == TokenType.NewLine)
				{ //無視
					++pos;
				}
				else
				{ //後はエラー
					messageStream.AppendFormat("{0} : 文頭にラベルでも命令でもないものがあるか、オペランドを取らない命令にオペランドがついていた。\n", input[pos].line);
					return false;
				}
			}
			return true;
		}
		
		bool ParseLabel(List<uint> output, IList<Token> input, ref int pos)
		{
			//トークンを取り出して
			var t = input[pos];
			Debug.Assert(t.type == TokenType.Identifier);
			//ラベルmapから検索して、アドレスを放り込む
			Label label;
			if (!labelNameMap.TryGetValue(t.str, out label))
			{
				messageStream.AppendFormat(
					"{0} : 文字列\"{1}\"は命令でもなく、ラベルでもないようだ。たぶん書き間違え。\n",
					input[pos].line,
					t.str);
				return false;
			}
			Debug.Assert(label != null);
			label.address = output.Count;
			++pos;
			//ラベル終了の:があるはず
			if (input[pos].type != TokenType.LabelEnd)
			{
				messageStream.AppendFormat("{0} : ラベルは\':\'で終わらないといけない。\n", input[pos].line);
				return false;
			}
			++pos;
			return true;
		}

		bool ParseInstruction(List<uint> output, IList<Token> input, ref int pos)
		{
			//トークンを取り出して
			var t = input[pos];
			Debug.Assert(t.type == TokenType.Instruction);
			++pos;
			//後は命令種に応じて分岐
			var inst = t.instruction;
			var inst24 = (uint)((int)inst << 24);
			if (inst == Instruction.I)
			{
				//オペランドが必要
				var op = input[pos];
				++pos;
				if (op.type != TokenType.Number)
				{
					messageStream.AppendFormat("{0} : 命令iの次に数字が必要。\n", input[pos].line);
					return false;
				}

				if (Math.Abs(op.number) > ImmUtil.GetMaxS(ImmBitCount.I))
				{
					messageStream.AppendFormat("{0} : 命令iが取れる数字はプラスマイナス{1}の範囲。\n", input[pos].line, ImmUtil.GetMaxS(ImmBitCount.I));
					return false;
				}
				output.Add(inst24 | (uint)(op.number & ImmUtil.GetMask(ImmBitCount.I)));
			}
			else if ((inst == Instruction.J) || (inst == Instruction.Bz) || (inst == Instruction.Call))
			{
				//オペランドが必要
				var op = input[pos];
				++pos;
				Debug.Assert(op.type == TokenType.Identifier);
				Label label;
				if (!labelNameMap.TryGetValue(op.str, out label))
				{
					Debug.Assert(false);
				}
				var labelId = label.id;
				if (labelId >= ImmUtil.GetMaxU(ImmBitCount.Flow))
				{
					messageStream.AppendFormat("{0} : プログラムが大きすぎて処理できない(ラベルが{1}個以上ある)。\n", input[pos].line, ImmUtil.GetMaxU(ImmBitCount.Flow));
					return false;
				}
				output.Add(inst24 | (uint)labelId); //ラベルIDを仮アドレスとして入れておく。
			}
			else if (
				(inst == Instruction.Ld) ||
				(inst == Instruction.St) ||
				(inst == Instruction.Fld) ||
				(inst == Instruction.Fst))
			{
				//オペランドが必要
				var op = input[pos];
				++pos;
				if (op.type != TokenType.Number)
				{
					messageStream.AppendFormat("{0} : 命令ld,st,fld,fstの次に数字が必要。\n", input[pos].line);
					return false;
				}

				if (Math.Abs(op.number) > ImmUtil.GetMaxS(ImmBitCount.Ls))
				{
					messageStream.AppendFormat("{0} : 命令popが取れる数字はプラスマイナス{1}の範囲。\n", input[pos].line, ImmUtil.GetMaxS(ImmBitCount.Ls));
					return false;
				}
				output.Add(inst24 | (uint)(op.number & ImmUtil.GetMask(ImmBitCount.Ls)));
			}
			else if ((inst == Instruction.Pop) || (inst == Instruction.Ret))
			{
				//オペランドが必要
				var op = input[pos];
				++pos;
				if (op.type != TokenType.Number)
				{
					messageStream.AppendFormat("{0} : 命令pop/retの次に数字が必要。\n", input[pos].line);
					return false;
				}
				//即値は符号付き
				if (Math.Abs(op.number) > ImmUtil.GetMaxS(ImmBitCount.Flow))
				{
					messageStream.AppendFormat("{0} : 命令pop/retが取れる数字はプラスマイナス{1}の範囲。\n", input[pos].line, (1 << (ImmBitCount.Flow - 1)));
					return false;
				}
				output.Add(inst24 | (uint)(op.number & ImmUtil.GetMask(ImmBitCount.Flow)));
			}
			else if ( //即値なし
				(inst == Instruction.Add) ||
				(inst == Instruction.Sub) || 
				(inst == Instruction.Mul) ||
				(inst == Instruction.Div) || 
				(inst == Instruction.Lt) ||
				(inst == Instruction.Le) ||
				(inst == Instruction.Eq) ||
				(inst == Instruction.Ne))
			{
				output.Add(inst24);
			}
			else
			{
				Debug.Assert(false); //バグ。
			}
			return true;
		}

		bool ResolveLabelAddress(IList<uint> instructions)
		{
			var instructionCount = instructions.Count;
			var mask = (0xffffffff << ImmBitCount.Flow); //命令マスク
			for (var i = 0; i < instructionCount; ++i)
			{
				var inst = instructions[i];
				var inst24 = ((inst & mask) >> 24);
				if (
				(inst24 == (int)Instruction.J) ||
				(inst24 == (int)Instruction.Bz) ||
				(inst24 == (int)Instruction.Call))
				{ 
					var labelId = ImmUtil.GetU(inst, ImmBitCount.Flow);
					Label label;
					if (!labelIdMap.TryGetValue(labelId, out label))
					{
						Debug.Assert(false);
					}
					var address = label.address;
					if ((address > ImmUtil.GetMaxU(ImmBitCount.Flow)) || (address < 0))
					{
						messageStream.AppendFormat("- : j,bz,call命令のメモリ番号を解決できない。メモリ番号がマイナスか、{0}を超えている。\n", ImmUtil.GetMaxU(ImmBitCount.Flow));
						return false;
					}
					instructions[i] = (inst & mask) | (uint)address;
				}
			}
			return true;
		}
	}
} //namespace Sunaba
