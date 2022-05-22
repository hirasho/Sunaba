using System.Collections;
using System.Text;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using Sunaba;

public class Main : MonoBehaviour
{
	[SerializeField] RawImage appWindow;
	[SerializeField] Text messageWindow;
	[SerializeField] Button rebootButton;
	[SerializeField] InputField filenameInput;
	[SerializeField] bool readCompiled;
	[SerializeField] bool outputIntermediates;
	[SerializeField] bool inMainThread;

	void Start()
	{
		keys = new int[(int)IoState.Key.Count];
		rebootButton.onClick.AddListener(OnClickReboot);
		pixels = new Texture2D(100, 100);
		appWindow.texture = pixels;
		messageStream = new StringBuilder();
	}

	void Update()
	{
		if (machine != null)
		{
//			UpdateMachine();
		}

		// ログ吐き出し
		if (messageStream.Length > 0)
		{
			messageWindow.text += messageStream.ToString();
			messageStream.Clear();
		}
	}

	// non public --------
	Texture2D pixels;
	Machine machine;

	StringBuilder messageStream;
	int pointerX;
	int pointerY;
	int[] keys;

	void UpdateMachine()
	{
		var io = machine.BeginSync();
		if (machine.IsTerminated())
		{
			if (machine.Error)
			{
				messageStream.AppendLine("プログラムが異常終了した。間違いがある");
			}
			else
			{
				messageStream.AppendLine("プログラムが最後まで実行された");
				var ret = machine.OutputValue();
				if (ret != 0)
				{
					messageStream.AppendFormat("(出力:{0})\n", machine.OutputValue());
				}
				else
				{
					messageStream.AppendLine("。");
				}
			}
		}
		else
		{
			io.Update(pointerX, pointerY, keys);
			UpdateScreen(io);
		}
		machine.EndSync();

		if (machine.IsTerminated())
		{
			machine.Dispose();
			machine = null;
		}
	}

	void UpdateScreen(IoState io)
	{
		var memory = io.MemoryCopy;
		var w = memory[(int)Machine.VmMemory.GetScreenWidth];
		var h = memory[(int)Machine.VmMemory.GetScreenHeight];
		for (var y = 0; y < h; y++)
		{
			for (var x = 0; x < w; x++)
			{
				var rawColor = memory[io.VramOffset + (y * w) + x];
				var r = rawColor / 10000;
				rawColor -= r * 10000;
				var g = rawColor / 100;
				rawColor -= g * 100;
				var b = rawColor;
				var color = new Color(
					(float)r / 99f,
					(float)g / 99f,
					(float)b / 99f,
					1f);
				pixels.SetPixel(x, y, color);
			}
		}
		pixels.Apply();
	}

	void OnClickReboot()
	{
		// ファイル名取って
		var filename = filenameInput.text;
		if (string.IsNullOrEmpty(filename))
		{
			return;
		}
		var path = System.IO.Path.GetFullPath(filename);
		var exists = System.IO.File.Exists(path);
		if (!exists)
		{
			messageStream.AppendFormat("ファイルが見つからない: {0}\n", path);
			return;
		}

		// 言語確定
		var localization = new Localization("japanese"); // TODO: 他の言語

		// コンパイラに渡して
		var compiled = new System.Text.StringBuilder();
		var succeeded = true;
		if (readCompiled)
		{
			var sourceCompiled = System.IO.File.ReadAllText(path);
			compiled.Append(sourceCompiled);
		}
		else
		{
			succeeded = Compiler.Process(
				compiled,
				messageStream,
				path,
				localization,
				outputIntermediates);
		}

		// アセンブラ
		if (succeeded)
		{
			var instructions = new List<uint>();
			succeeded = Assembler.Process(
				instructions,
				messageStream,
				compiled.ToString().ToCharArray(),
				localization,
				outputIntermediates);

			if (succeeded) 
			{
				// TODO: 同プログラムかどうかでメッセージ変える

				// 動いていれば止める
				if (machine != null)
				{
					machine.Dispose(); // 止める
					machine = null;
				}

				var program = ConvertToBytes(instructions);
				machine = new Machine(messageStream, program, inMainThread, outputIntermediates);
				if (machine.Error)
				{
					messageStream.AppendLine("指定されたプログラムを開始できなかった。");
					machine.Dispose();
					machine = null;
				}
				else
				{
					messageStream.AppendLine("プログラムを開始");
				}
			}
		}
	}

	byte[] ConvertToBytes(IList<uint> instructions)
	{
		var ret = new byte[(instructions.Count * 4)];
		for (int i = 0; i < instructions.Count; ++i)
		{
			var inst = instructions[i];
			ret[(i * 4) + 0] = (byte)((inst >> 24) & 0xff);
			ret[(i * 4) + 1] = (byte)((inst >> 16) & 0xff);
			ret[(i * 4) + 2] = (byte)((inst >> 8) & 0xff);
			ret[(i * 4) + 3] = (byte)((inst >> 0) & 0xff);
		}
		return ret;
	}
}
