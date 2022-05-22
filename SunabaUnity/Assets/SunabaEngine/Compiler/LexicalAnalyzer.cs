using System.Collections.Generic;
using System;
using System.Text;
using System.Diagnostics;

namespace Sunaba
{
	public class LexicalAnalyzer
	{
		//最後に改行があると仮定して動作する。前段で改行文字を後ろにつけること。
		public static bool Process(
			List<Token> output,
			StringBuilder messageStream,
			char[] input,
			string filename,
			int lineStart,
			Localization localization)
		{
			var analyzer = new LexicalAnalyzer(
				messageStream, 
				filename, 
				lineStart, 
				localization);
			return analyzer.Process(output, input);
		}

		// non public -------
		string filename; //処理中のファイル名
		int line;
		int lineStart;
		StringBuilder messageStream;
		Localization localization; //借り物

		enum Mode
		{
			None, //出力直後
			LineBegin, //行頭
			Minus, //-の上 (次が>かもしれない)
			Exclamation, //!の上(次が=かもしれない)
			Lt, //<の上(次が=かも)
			Gt, //>の上(次が=かも)
			Name, //名前[a-zA-Z0-9_全角]の上
			StringLiteral, //"文字列"の上
		}

		LexicalAnalyzer(
			StringBuilder messageStream, 
			string filename,
			int lineStart, 
			Localization localization)
		{
			this.messageStream = messageStream;
			this.filename = filename;
			this.lineStart = lineStart;
			this.localization = localization; 
		}

		//最後に改行があると仮定して動作する。前段で改行文字を後ろにつけること。
		/*
		0:出力直後
		1:行頭
		3:!
		4:<
		5:>
		6:名前
		7:""の中
		8:-

		0,' ',0
		0,'\n',1
		0,=,2
		0,!,3
		0,<,4
		0,>,5
		0,c,1字トークンなら出力して0、その他なら6
		0,",7

		1,'\n,1 トークン出力
		1,' ',1 空白カウント+
		1,*,0 1字戻す

		3,'=',0 !=出力
		3,*,0 1字戻す

		4,'=',0 <=出力
		4,*,0 1字戻す

		5,'=',0 >=出力
		5,*,0 1字戻す

		6,c,6 継続
		6,*,0 出力して1字戻す

		7,",1
		7,*,7

		8,'>', ->出力
		8,*,0 1字戻す

		*/
		bool Process(
			List<Token> output,
			char[] input)
		{
			var s = input.Length;
			Debug.Assert(input[s - 1] == '\n');
			var loc = localization; // 短縮

			var mode = Mode.LineBegin;
			var begin = 0; //tokenの開始点
			var literalBeginLine = 0;
			var i = 0;
			while (i < s)
			{
				var advance = true;
				char c = input[i];
				var l = i - begin; //現トークンの文字列長
				switch (mode)
				{
					case Mode.None:
						if (Token.IsOneCharacterDefinedType(c))
						{
							output.Add(new Token(new ReadOnlySpan<char>(input, i, 1), TokenType.Unknown, line, loc));
						}
						else if (c == '-')
						{ //-が来た
							mode = Mode.Minus; //判断保留
						}
						else if (c == '!')
						{ //!が来た
							mode = Mode.Exclamation; //保留
						}
						else if (c == '<')
						{ //<が来た
							mode = Mode.Lt; //保留
						}
						else if (c == '>')
						{ //>が来た
							mode = Mode.Gt; //保留
						}
						else if (Utility.IsInName(c))
						{ //識別子か数字が始まりそう
							mode = Mode.Name;
							begin = i; //ここから
						}
						else if (c == '"')
						{ //"が来た
							mode = Mode.StringLiteral;
							begin = i + 1; //次の文字から
							literalBeginLine = line;
						}
						else if (c == '\n')
						{
							mode = Mode.LineBegin;
							begin = i + 1;
						}
						else if (c == ' ')
						{
							; //何もしない
						}
						else
						{
							BeginError();
							messageStream.AppendFormat("Sunabaで使うはずのない文字\"{0}\"が現れた。", c);
							if (c == ';')
							{
								messageStream.Append("C言語と違って文末の;は不要。");
							}
							else if ((c == '{') || (c == '}'))
							{
								messageStream.Append("C言語と違って{や}は使わない。");
							}
							messageStream.Append("\n");
							return false;
						}
						break;
					case Mode.LineBegin:
						if (c == ' ')
						{ //スペースが続けば継続
							;
						}
						else if (c == '\n')
						{ //改行されれば仕切り直し。空白しかないまま改行ということは、前の行は無視していい。出力しない。
							begin = i + 1;
						}
						else
						{
							//トークンを出力して、MODE_NONEでもう一度回す。第二引数のlはスペースの数で重要。
							output.Add(new Token(new ReadOnlySpan<char>(input, 0, l), TokenType.LineBegin, line, loc));
							mode = Mode.None;
							advance = false;
						}
						break;
					case Mode.Exclamation:
						if (c == '=')
						{
							output.Add(new Token(Operator.Ne, line));
							mode = Mode.None;
						}
						else
						{
							BeginError();
							messageStream.AppendFormat("\'!\'の後は\'=\'しか来ないはずだが、\'{0}\'がある。\"!=\"と間違えてないか？\n", c);
							return false;
						}
						break;
					case Mode.Lt:
						if (c == '=')
						{
							output.Add(new Token(Operator.Le, line));
							mode = Mode.None;
						}
						else
						{ //その他の場合<を出力して、MODE_NONEでもう一度回す
							output.Add(new Token(Operator.Lt, line));
							mode = Mode.None;
							advance = false;
						}
						break;
					case Mode.Gt:
						if (c == '=')
						{
							output.Add(new Token(Operator.Ge, line));
							mode = Mode.None;
						}
						else
						{ //その他の場合>を出力して、MODE_NONEでもう一度回す
							output.Add(new Token(Operator.Gt, line));
							mode = Mode.None;
							advance = false;
						}
						break;
					case Mode.Name:
						if (Utility.IsInName(c))
						{ //識別子の中身。
							; //継続
						}
						else
						{ //他の場合、全て出力
							var newToken = new Token(new ReadOnlySpan<char>(input, begin, l), TokenType.Unknown, line, loc);
							if (newToken.type == TokenType.Unknown)
							{
								BeginError();
								messageStream.AppendFormat("解釈できない文字列( {0} )が現れた。\n", new string(input, begin, l));
								return false;
							}
							else if (newToken.type == TokenType.LargeNumber)
							{ //大きすぎる数
								BeginError();
								messageStream.AppendFormat("数値はプラスマイナス{0}までしか書けない。\n", ImmUtil.GetMaxS(ImmBitCount.I));
								return false;
							}
							output.Add(newToken);
							//もう一度回す
							mode = Mode.None;
							advance = false;
						}
						break;
					case Mode.StringLiteral:
						if (c == '"')
						{ //終わった
							output.Add(new Token(new ReadOnlySpan<char>(input, begin, l), TokenType.StringLiteral, line, loc));
							mode = Mode.None;
						}
						else
						{
							; //継続
						}
						break;
					case Mode.Minus:
						if (c == '>')
						{
							output.Add(new Token(TokenType.Substitution, line));
							mode = Mode.None;
						}
						else
						{ //その他の場合-を出力して、MODE_NONEでもう一度回す
							output.Add(new Token(Operator.Minus, line));
							mode = Mode.None;
							advance = false;
						}
						break;
					default: Debug.Assert(false); break; //これはバグ
				}

				if (advance)
				{
					if (c == '\n')
					{
						++line;
					}
					++i;
				}
			}

			//文字列が終わっていなければ
			if (mode == Mode.StringLiteral)
			{
				BeginError();
				messageStream.AppendFormat("{0}行目の文字列(\"\"ではさまれたもの)が終わらないままファイルが終わった。\n", literalBeginLine);
				return false;
			}
			return true;
		}

		void BeginError()
		{
			var rawFilename = System.IO.Path.GetFileName(filename);
			messageStream.Append(rawFilename);
			if (line != 0)
			{
				messageStream.AppendFormat("({0}) ", line);
			}
			else
			{
				messageStream.Append(' ');
			}
		}
	}
} //namespace Sunaba
