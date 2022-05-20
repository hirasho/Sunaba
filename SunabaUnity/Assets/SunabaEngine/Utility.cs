using System.Collections;
using System;
using System.Collections.Generic;

namespace Sunaba
{
	public static class Utility
	{
		public static bool IsEqual(ReadOnlySpan<char> s0, ReadOnlySpan<char> s1)
		{
			var l0 = s0.Length;
			var l1 = s1.Length;
			if (l0 != l1)
			{
				return false;
			}

			var l = (l0 < l1) ? l0 : l1;
			for (var i = 0; i < l; ++i)
			{
				if (s0[i] != s1[i])
				{
					return false;
				}
			}
			return true;
		}

		public static bool IsAsmName(ReadOnlySpan<char> s)
		{
			var r = true;
			if (s.Length == 0)
			{
				r = false;
			}
			else
			{
				for (var i = 0; i < s.Length; ++i)
				{
					if (!IsInName(s[i]) && (s[i] != '!'))
					{
						r = false;
						break;
					}
				}
			}
			return r;
		}

		//[?_@$%&a-zA-Z全角0_9]のチェック
		public static bool IsInName(char c)
		{ 
			var r = false;
			if (
			(c == '@') ||
			(c == '$') ||
			(c == '&') ||
			(c == '?') ||
			(c == '_') ||
			(c == '\'') || //y'という名前がつけたくなることはありうるだろうよ "は念のためとっておく
			IsAlphabet(c) ||
			((c >= '0') && (c <= '9')) ||
			(c >= 0x100))
			{ //2バイト文字ならなんでもOK
				r = true;
			}
			return r;
		}

		public static bool IsAlphabet(char c)
		{
			var r = false;
			if ( 
			((c >= 'a') && (c <= 'z')) ||
			((c >= 'A') && (c <= 'Z')))
			{
				r = true;
			}
			return r;
		}

		//文字列->数字(マイナスも認識)
		public static bool ConvertNumber(out int output, ReadOnlySpan<char> s)
		{
			output = 0;
			var minus = false;
			var ret = false;
			var pos = 0;
			var l = s.Length;
			if (s[pos] == '-')
			{ //マイナス付き
				minus = true;
				++pos; //sを1文字進める
				--l; //lを1文字減らす
			}

			if (IsDecNumber(s[pos]))
			{ //最初が数値なら数値か、不正かのどちらか
				if (l >= 3)
				{ //3文字以上で、
					if (s[pos + 0] == '0')
					{ //0から始まり、
						if ((s[pos + 1] == 'x') || (s[pos + 1] == 'X'))
						{ //xなら16進
							ret = ConvertHexNumber(out output, s.Slice(2));
						}
						else if ((s[pos + 1] == 'b') || (s[pos + 1] == 'B'))
						{ //bなら2進
							ret = ConvertBinNumber(out output, s.Slice(2));
						}
					}
				}

				if (!ret)
				{ //2進や16進で引っかからなかったならば、10進を試す
					ret = ConvertDecNumber(out output, s);
				}

				if (ret && minus)
				{
					output = -output;
				}
			}
			return ret;		
		}


		public static bool ConvertHexNumber(out int output, ReadOnlySpan<char> s)
		{
			output = 0;
			var l = s.Length;
			for (var i = 0; i < l; ++i)
			{
				output *= 16;
				var c = s[i];
				if ((c >= '0') && (c <= '9'))
				{
					output += (c - '0');
				}
				else if ((c >= 'a') && (c <= 'f'))
				{
					output += (c - 'a') + 0xA;
				}
				else if ((c >= 'A') && (c <= 'A'))
				{
					output += (c - 'A') + 0xA;
				}
				else
				{
					return false;
				}
			}
			return true;
		}

		public static bool ConvertDecNumber(out int output, ReadOnlySpan<char> s)
		{
			output = 0;
			var l = s.Length;
			for (var i = 0; i < l; ++i)
			{
				output *= 10;
				var c = s[i];
				if ((c >= '0') && (c <= '9'))
				{
					output += (c - '0');
				}
				else
				{
					return false;
				}
			}
			return true;			
		}

		public static bool ConvertBinNumber(out int output, ReadOnlySpan<char> s)
		{
			output = 0;
			var l = s.Length;
			for (var i = 0; i < l; ++i)
			{
				output *= 2;
				var c = s[i];
				if ((c >= '0') && (c <= '1'))
				{
					output += (c - '0');
				}
				else
				{
					return false;
				}
			}
			return true;			
		}
		
		public static bool IsDecNumber(char c)
		{
			return ((c >= '0') && (c <= '9'));
		}
	}
}