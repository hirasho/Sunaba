using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace Sunaba
{
	public enum TokenType
	{
		//特定の文字列と対応する種類
		WhilePre,
		WhilePost, //なかぎり
		IfPre,
		IfPost, //なら
		DefPre,
		DefPost, //とは
		Const,
		Include,
		LeftBracket,
		RightBracket,
		Comma,
		IndexBegin,
		IndexEnd,
		Substitution, //->か→か⇒
		Out,
		//実際の内容が様々であるような種類
		Name,
		StringLiteral, //文字列リテラル
		Number,
		Operator,
		//Structurizerで自動挿入される種類
		StatementEnd,
		BlockBegin,
		BlockEnd,
		LineBegin,
		//終わり
		End,

		Unknown,
		LargeNumber, //エラーメッセージ用
	}

	public enum Operator{
		Plus,
		Minus,
		Mul,
		Div,
		Eq, //==
		Ne, //!=
		Lt, //<
		Gt, //>
		Le, //<=
		Ge, //>=

		Unknown,
	}

	public class Token
	{
		public string str;
		public int number;
		public int spaceCount;
		public TokenType type = TokenType.Unknown;
		public Operator opType = Operator.Unknown;
		public int line; //デバグ用行番号
		public string filename; //デバグ用ファイル名(他のTokenのmStringのコピー)

		public Token()
		{			
		}

		public Token(Operator opType, int line)
		{
			this.type = TokenType.Operator;
			this.opType = opType;
			this.line = line;
		}

		public Token(TokenType type, int line)
		{
			this.line = line;
			this.type = type;
		}

		public Token(ReadOnlySpan<char> s, TokenType type, int line, Localization loc)
		{
			str = new string(s);

			this.line = line;

			if ( //自動挿入トークン(Structurizerからしか呼ばれない)
			(type == TokenType.BlockBegin) ||  //自動挿入トークン(Structurizerからしか呼ばれない)
			(type == TokenType.BlockEnd) ||  //自動挿入トークン(Structurizerからしか呼ばれない)
			(type == TokenType.StatementEnd) || //自動挿入トークン(Structurizerからしか呼ばれない)
			(type == TokenType.End) || //ファイル終端ダミー
			(type == TokenType.Substitution))
			{ //->から来る代入
				this.type = type;
			}
			else if (type == TokenType.Unknown)
			{
				var tokenType = TokenType.Unknown;
				if (s.Length == 1)
				{ //1文字識別トークンなら試す
					Operator operatorType;
					GetOneCharacterDefinedType(out tokenType, out operatorType, s[0]);
					if (tokenType != TokenType.Unknown)
					{
						this.type = tokenType;
						this.opType = operatorType;
					}
				}

				if (tokenType == TokenType.Unknown)
				{ //その他文字列
					if (Utility.ConvertNumber(out number, s))
					{ //数字か？
						if (Math.Abs(number) <= ImmUtil.GetMaxS(ImmBitCount.I))
						{
							this.type = TokenType.Number;
						}
						else
						{
							this.type = TokenType.LargeNumber;
						}
					}
					else
					{ //数字じゃない場合、
						this.type = GetKeywordType(s, loc);
						if (this.type == TokenType.Unknown)
						{
							this.type = TokenType.Name; //わからないので名前
						}
					}
				}
			}
			else if (type == TokenType.LineBegin)
			{
				this.type = TokenType.LineBegin;
				this.spaceCount = s.Length; //スペースの数
			}
			else if (type == TokenType.StringLiteral)
			{
				this.type = TokenType.StringLiteral;
			}
			else
			{
				Debug.Assert(false); //ここはバグ。
			}
		}

		public static bool IsOneCharacterDefinedType(char c)
		{
			TokenType tokenType;
			Operator operatorType;
			GetOneCharacterDefinedType(out tokenType, out operatorType, c);
			return (tokenType != TokenType.Unknown);
		}

		public static TokenType GetKeywordType(ReadOnlySpan<char> s, Localization loc)
		{
			var t = new string(s);
			var r = TokenType.Unknown;
			if (t == "while")
			{
				r = TokenType.WhilePre;
			}
			else if ((t == loc.whileWord0) || ((loc.whileWord1 != null) && (t == loc.whileWord1)))
			{ //whileWord1はないことがある
				if (loc.whileAtHead)
				{
					r = TokenType.WhilePre;
				}
				else
				{
					r = TokenType.WhilePost;
				}
			}
			else if (t == "if")
			{
				r = TokenType.IfPre;
			}
			else if (t == loc.ifWord)
			{
				if (loc.ifAtHead)
				{
					r = TokenType.IfPre;
				}
				else
				{
					r = TokenType.IfPost;
				}
			}
			else if (t == "def")
			{
				r = TokenType.DefPre;
			}
			else if (t == loc.defWord)
			{
				if (loc.defAtHead)
				{
					r = TokenType.DefPre;
				}
				else
				{
					r = TokenType.DefPost;
				}
			}
			else if ((t == "const") || (t == loc.constWord))
			{
				r = TokenType.Const;
			}
			else if ((t == "include") || (t == loc.includeWord))
			{
				r = TokenType.Include;
			}
			else if ((t == "out") || (t == loc.outWord))
			{
				r = TokenType.Out;
			}
			return r;
		}

		//デバグ機能
		public static string ToString(IList<Token> tokens)
		{
			var sb = new System.Text.StringBuilder();
			var level = 0;
			for (var i = 0; i < tokens.Count; ++i)
			{
				if (tokens[i].type == TokenType.BlockEnd)
				{
					--level;
				}

				for (var j = 0; j < level; ++j)
				{
					sb.Append('\t');
				}
				tokens[i].ToString(sb);
				sb.AppendLine("");
				if (tokens[i].type == TokenType.BlockBegin)
				{
					++level;
				}
			}			
			return sb.ToString();
		}

		public override string ToString()
		{
			var sb = new System.Text.StringBuilder();
			ToString(sb);
			return sb.ToString();
		}

		public void ToString(System.Text.StringBuilder sb)
		{
			if (type == TokenType.Operator)
			{
				switch (opType)
				{
					case Operator.Plus: sb.Append("+"); break;
					case Operator.Minus: sb.Append("-"); break;
					case Operator.Mul: sb.Append("×"); break;
					case Operator.Div: sb.Append("÷"); break;
					case Operator.Eq: sb.Append("="); break;
					case Operator.Ne: sb.Append("≠"); break;
					case Operator.Lt: sb.Append("<"); break;
					case Operator.Gt: sb.Append(">"); break;
					case Operator.Le: sb.Append("≦"); break;
					case Operator.Ge: sb.Append("≧"); break;
					default: Debug.Assert(false); break;
				}
			}
			else if (type == TokenType.LineBegin)
			{
				sb.AppendFormat("行開始({0})", spaceCount);
			}
			else if ((type == TokenType.Name) || (type == TokenType.StringLiteral) || (type == TokenType.Number))
			{
				sb.Append(str);
			}
			else
			{
				switch (type)
				{
					case TokenType.WhilePre: sb.Append("while"); break;
					case TokenType.IfPre: sb.Append("if"); break;
					case TokenType.WhilePost: sb.Append("なかぎり"); break;
					case TokenType.IfPost: sb.Append("なら"); break;
					case TokenType.DefPre: sb.Append("def"); break;
					case TokenType.DefPost: sb.Append("とは"); break;
					case TokenType.Const: sb.Append("定数"); break;
					case TokenType.Include: sb.Append("挿入"); break;
					case TokenType.StatementEnd: sb.Append("行末"); break;
					case TokenType.LeftBracket: sb.Append("("); break;
					case TokenType.RightBracket: sb.Append(")"); break;
					case TokenType.BlockBegin: sb.Append("範囲開始"); break;
					case TokenType.BlockEnd: sb.Append("範囲終了"); break;
					case TokenType.IndexBegin: sb.Append("["); break;
					case TokenType.IndexEnd: sb.Append("]"); break;
					case TokenType.Comma: sb.Append(","); break;
					case TokenType.End: sb.Append("ファイル終了"); break;
					case TokenType.Substitution: sb.Append("→"); break;
					case TokenType.Out: sb.Append("出力"); break;
					default: Debug.Assert(false); break;
				}
			}
		}

		// non public ------

		//1文字で判定出来るタイプ
		static void GetOneCharacterDefinedType(out TokenType tokenType, out Operator operatorType, char c)
		{
			tokenType = TokenType.Unknown;
			operatorType = Operator.Unknown;
			switch (c)
			{
				case '(': tokenType = TokenType.LeftBracket; break;
				case ')': tokenType = TokenType.RightBracket; break;
				case '[': tokenType = TokenType.IndexBegin; break;
				case ']': tokenType = TokenType.IndexEnd; break;
				case ',': tokenType = TokenType.Comma; break;

				case '+': tokenType = TokenType.Operator; operatorType = Operator.Plus; break;
				case '*': tokenType = TokenType.Operator; operatorType = Operator.Mul; break;
				case '/': tokenType = TokenType.Operator; operatorType = Operator.Div; break;
				case '=': tokenType = TokenType.Operator; operatorType = Operator.Eq; break;
			}
		}
	}
} //namespace Sunaba
