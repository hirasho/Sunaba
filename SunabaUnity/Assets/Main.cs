using System.Collections;
using System.Text;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using Sunaba;

public class Main : BaseRaycaster, IPointerDownHandler, IPointerUpHandler
{
	[SerializeField] new Camera camera;
	[SerializeField] RawImage appWindow;
	[SerializeField] Text messageWindow;
	[SerializeField] Button rebootButton;
	[SerializeField] InputField filenameInput;
	[SerializeField] SoundSynthesizer soundSynthesizer;
	[SerializeField] bool readCompiled;
	[SerializeField] bool outputIntermediates;
	[SerializeField] bool inMainThread;
	[SerializeField] int messageLineCapacity;

	public override Camera eventCamera { get => camera; }

	protected override void Start()
	{
		base.Start();
		Application.targetFrameRate = 60;
		soundSynthesizer.ManualStart(IoState.SoundChannelCount);

		keys = new int[(int)IoState.Key.Count];
		rebootButton.onClick.AddListener(OnClickReboot);

		pixels = new Texture2D(100, 100);
		pixels.filterMode = FilterMode.Point;
		pixels.wrapMode = TextureWrapMode.Clamp;

		appWindow.texture = pixels;
		messageStream = new StringBuilder();
		messageLines = new string[messageLineCapacity];
		
		try
		{
			dropHandler = new DropHandlerWrapper(OnDrop);
		}
		catch (System.Exception e)
		{
			Debug.LogException(e);
		}
	}

	void Update()
	{
		if (machine != null)
		{
			UpdateMachine();
		}

		// ログ吐き出し
		if (messageStream.Length > 0)
		{
			var append = messageStream.ToString();
			var lines = append.Split('\n', System.StringSplitOptions.RemoveEmptyEntries);
			for (var i = 0; i < lines.Length; i++)
			{
				messageLines[nextMessageLine] = lines[i];
				nextMessageLine++;
				if (nextMessageLine == messageLines.Length)
				{
					nextMessageLine = 0;
				}
			}

			messageStream.Clear(); // 使い回してるだけ
			for (var i = 0; i < messageLines.Length; i++)
			{
				var idx = (nextMessageLine + i) % messageLines.Length;
				messageStream.AppendLine(messageLines[idx]);
			}
			messageWindow.text = messageStream.ToString();
			messageStream.Clear();
		}
	}

	public override void Raycast(PointerEventData eventData, List<RaycastResult> resultAppendList)
	{
		// 範囲外なら抜ける(エディタで範囲外クリックしても来るので)
		if (
		(eventData.position.x < 0f) ||
		(eventData.position.y < 0f) ||
		(eventData.position.x > Screen.width) ||
		(eventData.position.y > Screen.height))
		{
			return;
		}

		var cameraTransform = camera.transform;
		var result = new RaycastResult
		{
			gameObject = this.gameObject,
			module = this,
			distance = 1000f,
			worldPosition = cameraTransform.position + (cameraTransform.forward * 1000f),
			worldNormal = -cameraTransform.forward,
			screenPosition = eventData.position,
			index = resultAppendList.Count,
			sortingLayer = 0,
			sortingOrder = 0
		};
		resultAppendList.Add(result);
		pointerPosition = eventData.position;
	}

	public void OnPointerDown(PointerEventData eventData)
	{
		if (pointerId == int.MaxValue)
		{
			pointerId = eventData.pointerId;
			pointerDown = true;
		}
	}

	public void OnPointerUp(PointerEventData eventData)
	{
		if (pointerId == eventData.pointerId)
		{
			pointerId = int.MaxValue;
			pointerDown = false;
		}
	}

	// non public --------
	Texture2D pixels;
	Machine machine;
	StringBuilder messageStream;
	int[] keys;
	Vector3 pointerPosition; // 絶対スクリーン座標
	bool pointerDown;
	int pointerId = int.MaxValue;
	DropHandlerWrapper dropHandler;
	string[] messageLines;
	int nextMessageLine;

	void OnDrop(string path)
	{
		filenameInput.text = path;
		OnClickReboot();
	}

	void UpdateMachine()
	{
		var io = machine.BeginSync();
		if (machine.IsTerminated())
		{
			soundSynthesizer.StopAll();
			if (machine.Error)
			{
				messageStream.AppendLine("プログラムが異常終了した。間違いがある");
			}
			else
			{
				messageStream.Append("プログラムが最後まで実行された");
				var ret = machine.OutputValue();
				if (ret != 0)
				{
					messageStream.AppendFormat("(出力:{0})\n", machine.OutputValue());
				}
				else
				{
					messageStream.Append("\n");
				}
			}
		}
		else
		{
			Vector2 localPoint;
			RectTransformUtility.ScreenPointToLocalPointInRectangle(
				appWindow.rectTransform as RectTransform,
				pointerPosition,
				camera,
				out localPoint);
			
			var w = io.MemoryCopy[(int)Machine.VmMemory.GetScreenWidth];
			var h = io.MemoryCopy[(int)Machine.VmMemory.GetScreenHeight];
			var size = appWindow.rectTransform.sizeDelta;
			var pointerX = Mathf.RoundToInt((float)w * (localPoint.x + (size.x * 0.5f)) / size.x);
			var pointerY = Mathf.RoundToInt((float)h * (localPoint.y + (size.y * 0.5f)) / size.y);
			keys[(int)IoState.Key.LButton] = pointerDown ? 1 : 0;
			keys[(int)IoState.Key.RButton] = Input.GetKey(KeyCode.Mouse1) ? 1 : 0;
			keys[(int)IoState.Key.Up] = Input.GetKey(KeyCode.UpArrow) ? 1 : 0;
			keys[(int)IoState.Key.Down] = Input.GetKey(KeyCode.DownArrow) ? 1 : 0;
			keys[(int)IoState.Key.Left] = Input.GetKey(KeyCode.LeftArrow) ? 1 : 0;
			keys[(int)IoState.Key.Right] = Input.GetKey(KeyCode.RightArrow) ? 1 : 0;
			keys[(int)IoState.Key.Space] = Input.GetKey(KeyCode.Space) ? 1 : 0;
			keys[(int)IoState.Key.Enter] = Input.GetKey(KeyCode.Return) ? 1 : 0;
			io.Update(pointerX, pointerY, keys);
			UpdateSound(io);
		}
		UpdateScreen(io);
		machine.EndSync();

		if (machine.IsTerminated())
		{
			machine.Dispose();
			machine = null;
		}
	}

	void UpdateSound(IoState io)
	{
		for (var i = 0; i < IoState.SoundChannelCount; i++)
		{
			var amp = (float)io.GetAndResetSoundRequest(i) * 0.00001f;
			if (amp > 0f)
			{
				var f = (float)io.MemoryCopy[(int)Machine.VmMemory.SetSoundFrequency0 + i];
				var d = (float)io.MemoryCopy[(int)Machine.VmMemory.SetSoundDumping0 + i] * 0.00001f;
				soundSynthesizer.Play(i, f, d, amp);
			}
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

		string path;
		if (System.IO.Path.IsPathFullyQualified(filename))
		{
			path = filename;
		}
		else
		{
			path = System.IO.Path.GetFullPath(filename);
		}

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
