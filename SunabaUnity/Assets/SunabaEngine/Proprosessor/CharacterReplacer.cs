using System.Collections.Generic;

namespace Sunaba
{
	//危険な文字を排除し、全角->半角の置き換えをおこなったりする。サイズは減るか維持。
	public class CharacterReplacer
	{
		public static void Process(
			List<char> output,
			IList<char> input,
			Localization localization)
		{
			var size = input.Count;
			char c2; //2文字になる場合
			for (var i = 0; i < size; ++i)
			{
				char c = input[i];
				c2 = '\0';
				if ( //捨てる文字を列挙
				((c < ' ') && (c != '\n')) || //改行以外の制御文字(31以下)
				(c == 0x7f))
				{ //DEL(0x7f)
					; //破棄
				}
				else
				{ //活かす文字は一部置換
					if ((c >= '０') && (c <= '９'))
					{ //全角数字
						c = (char)((c - '０') + '0');
					}
					else if ((c >= 'Ａ') && (c <= 'Ｚ'))
					{ //全角大文字
						c = (char)((c - 'Ａ') + 'A');
					}
					else if ((c >= 'ａ') && (c <= 'ｚ'))
					{ //全角小文字
						c = (char)((c - 'ａ') + 'a');
					}
					else if ((localization.argDelimiter != '\0') && (c == localization.argDelimiter))
					{
						c = ','; //カンマに変換
					}
					else
					{ //その他の全角半角変換
						switch (c)
						{
							//0x20-0x2f
		//					case '　': c = ' '; break; //全角スペースはTabProcessorにて半角に置換済み
							case '！': c = '!'; break;
							case '”': c = '"'; break;
							case '＃': c = '#'; break; 
							case '＄': c = '$'; break; 
							case '％': c = '%'; break; 
							case '＆': c = '&'; break; 
							case '’': c = '\''; break;
							case '（': c = '('; break;
							case '）': c = ')'; break;
							case '＊': c = '*'; break;
							case '＋': c = '+'; break;
							case '，': c = ','; break;
							case '−': c = '-'; break;
							case '．': c = '.'; break;
							case '／': c = '/'; break;
							//0x3a-0x3f
							case '：': c = ':'; break; 
							case '；': c = ';'; break; 
							case '＜': c = '<'; break;
							case '＝': c = '='; break;
							case '＞': c = '>'; break;
							case '？': c = '?'; break;
							case '＠': c = '@'; break;
							case '［': c = '['; break;
							case '¥': c = '\\'; break; 
							case '］': c = ']'; break;
							case '＾': c = '^'; break;
							case '＿': c = '_'; break; 
							//0x4b-0x4f
							case '｛': c = '{'; break;
							case '｜': c = '|'; break;
							case '｝': c = '}'; break;
							case '〜': c = '~'; break;
							//その他
							case '×': c = '*'; break; //×もサポート
							case '÷': c = '/'; break; //÷もサポート
							case '≦': c = '<'; c2 = '='; break;
							case '≠': c = '!'; c2 = '='; break;
							case '≧': c = '>'; c2 = '='; break;
							case '→': c = '-'; c2 = '>'; break;
							case '⇒': c = '-'; c2 = '>'; break;
							case '≤': c = '<'; c2 = '='; break;
							case '≥': c = '>'; c2 = '='; break;
							default: break; //変換なし
						}
					}
					output.Add(c);
					if (c2 != '\0')
					{ //2文字目がある
						output.Add(c2);
					}
				}
			}
		}
	}
} //namespace Sunaba