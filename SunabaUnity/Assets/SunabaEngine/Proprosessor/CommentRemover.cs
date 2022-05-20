//#define ENABLE_CPP_COMMENT
using System.Collections.Generic;

namespace Sunaba
{
	public class CommentRemover
	{
		//TODO: 末尾に改行を加えるが、本来ここでやるべきことじゃない。
		public static bool Process(
			List<char> output,
			List<char> input)
		{
			var l = input.Count;
			if (l < 2)
			{
				return false;
			}
			var l1 = l - 1;
			var level = 0; //コメントのネストを有効に
			var shortComment = false;

			char c0, c1, c2;

			//最初の文字
			c1 = input[0];
			c2 = input[1];
			if ((c1 == '/') && (c2 == '*'))
			{ //コメント開始
				++level;
			}
			if (level == 0)
			{
#if ENABLE_CPP_COMMENT
				if ((c1 == '/') && (c2 == '/'))
				{
					shortComment = true;
				}
#endif
				if (c1 == '#')
				{
					shortComment = true;
				}
			}

			if (c1 == '\n')
			{
				shortComment = false;
				output.Add('\n');
			}
			else if ((level == 0) && !shortComment)
			{
				output.Add(c1);
			}

			//ループ
			for (var i = 1; i < l1; ++i)
			{
				c0 = input[i - 1];
				c1 = input[i];
				c2 = input[i + 1];
				if ((c1 == '/') && (c2 == '*'))
				{ //コメント開始
					++level;
				}

				if (level == 0)
				{
#if ENABLE_CPP_COMMENT
					if ((c1 == '/') && (c2 == '/'))
					{
						shortComment = true;
					}
#endif
					if (c1 == '#')
					{
						shortComment = true;
					}
				}

				//文字出力
				if (c1 == '\n')
				{
					shortComment = false;
					output.Add('\n');
				}
				else if ((level == 0) && !shortComment)
				{
					output.Add(c1);
				}

				if ((c0 == '*') && (c1 == '/'))
				{ //コメント終了
					if (level > 0)
					{
						--level;
					}
				}
			}
			//最後の文字
			c1 = input[l - 1];
			if (c1 == '\n')
			{
				shortComment = false;
				output.Add('\n');
			}
			else if ((level == 0) && !shortComment)
			{
				output.Add(c1);
			}
			//字句解析の便宜を図るために、最後に改行を追加
			output.Add('\n');
			//出力
			return true;
		}
	}
} //namespace Sunaba
