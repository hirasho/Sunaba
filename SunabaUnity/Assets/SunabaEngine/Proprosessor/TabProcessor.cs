using System.Collections.Generic;

namespace Sunaba
{
	//タブ及び全角スペースを半角スペースに置換
	public class TabProcessor
	{
		public static void Process(
			List<char> output,
			List<char> input,
			int tabWidth = 8)
		{
			var col = 0; //行内位置
			var p = 0; //位置
			var l = input.Count;
			while (p < l)
			{
				if (input[p] == '\t')
				{ //タブ発見
					var spaceCount = tabWidth - (col % tabWidth); //丁度割り切れれば丸々、最小1。
					for (var i = 0; i < spaceCount; ++i)
					{
						output.Add(' ');
					}
					col += spaceCount;
				}
				else if(input[p] == '　')
				{ //全角スペースは半角2個に置換
					output.Add(' ');
					output.Add(' ');
					col += 2;
				}
				else
				{
					output.Add(input[p]);
					if (input[p] == '\n')
					{ //改行発見
						col = 0;
					}
					else
					{
						++col;
					}
				}
				++p;
			}
		}
	}
} //namespace Sunaba
