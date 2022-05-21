using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace Sunaba
{
	public class Main : MonoBehaviour
	{
		[SerializeField] RawImage appWindow;
		[SerializeField] Text messageWindow;
		[SerializeField] Button rebootButton;
		[SerializeField] InputField filenameInput;
		[SerializeField] bool readCompiled;
		[SerializeField] bool outputIntermediates;

		void Start()
		{
			rebootButton.onClick.AddListener(OnClickReboot);
			pixels = new Texture2D(100, 100);
			messageStream = new System.IO.MemoryStream();
			messageStreamWriter = new System.IO.StreamWriter(messageStream);
		}

		void Update()
		{
			// UpdateごとにappWindowのテクスチャを送信する
	//		pixels.
			
			
		}

		// non public --------
		Texture2D pixels;
		Machine machine;
		System.IO.StreamWriter messageStreamWriter;
		System.IO.MemoryStream messageStream;

		void OnClickReboot()
		{
			// ファイル名取って
			var filename = filenameInput.text;
			var path = System.IO.Path.GetFullPath(filename);
			var exists = System.IO.File.Exists(path);
			if (!exists)
			{
				messageStreamWriter.WriteLine(string.Format("ファイルが見つからない: {0}", path));
				return;
			}

			// 言語確定
			var localization = new Localization("japanese"); // TODO: 他の言語

			// コンパイラに渡して
			var compiled = new List<char>();
			if (readCompiled)
			{
				var text = System.IO.File.ReadAllText(path);
				for (var i = 0; i < text.Length; i++)
				{
					compiled.Add(text[i]);
				}
			}
			else
			{
/*
			var ret = Compiler.Process(
				compiled,
				messageStreamWriter,
				path,
				loc);
*/			
			}

			// アセンブラ
			var instructions = new List<uint>();
			var succeeded = Assembler.Process(
				instructions,
				messageStreamWriter,
				compiled,
				localization,
				outputIntermediates);
			if (succeeded) 
			{
				//コード送信
				byte[] objectCode = new byte[(instructions.Count * 4)];
				for (int i = 0; i < instructions.Count; ++i)
				{
					objectCode[(i * 4) + 0] = (byte)(instructions[i] >> 24);
					objectCode[(i * 4) + 1] = (byte)(instructions[i] >> 16);
					objectCode[(i * 4) + 2] = (byte)(instructions[i] >> 8);
					objectCode[(i * 4) + 3] = (byte)(instructions[i] >> 0);
				}
				machine = new Machine(messageStreamWriter, objectCode);
			}
//			var bytes = messageStream.ToArray();
//			messageWindow.text = System.Text.Encoding.UTF8.GetString(bytes);
		}
	}
}
