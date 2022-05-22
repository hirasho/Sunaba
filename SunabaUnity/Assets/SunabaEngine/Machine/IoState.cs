using System;
using System.Threading;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Sunaba
{
	public class IoState
	{
		public const int SoundChannelCount = 3;
		public enum Key
		{
			Unknown = -1,
			LButton = 0,
			RButton,
			Up,
			Down,
			Left,
			Right,
			Space,
			Enter,

			Count
		};

		//Machine->外
		public int[] MemoryCopy{ get; private set; }
		public int MemorySize{ get; private set; }
		public int IoOffset{ get; private set; }
		public int VramOffset{ get; private set; }

		//外->Machine
		public int FrameCount { get; private set; }
		public int PointerX { get; private set; }
		public int PointerY { get; private set; }
		public ReadOnlySpan<int> Keys { get => new ReadOnlySpan<int>(keys); }
		
		//開発用
		public int[] soundRequests;

		public IoState()
		{
			lockObject = new object();
			keys = new int[(int)Key.Count];
			for (int i = 0; i < keys.Length; ++i)
			{
				keys[i] = 0;
			}

			soundRequests = new int[SoundChannelCount];
			for (int i = 0; i < soundRequests.Length; ++i){
				soundRequests[i] = 0;
			}
		}

		public void OnScreenSizeChanged()
		{
			screenSizeChanged = true;
		}

		public void RequestSound(int channel, int amplitude)
		{
			if ((channel >= 0) && (channel < soundRequests.Length))
			{
				soundRequests[channel] = amplitude;
			}
		}

		public int GetKey(Key key)
		{
			var index = (int)key;
			if ((index >= 0) && (index < (int)Key.Count))
			{
				return keys[index];
			}
			else
			{
				return 0;
			}
		}

		public void AllocateMemoryCopy(int memorySize, int ioOffset, int vramOffset)
		{
			MemoryCopy = new int[memorySize];
			MemorySize = memorySize;
			IoOffset = ioOffset;
			VramOffset = vramOffset;
		}

		//外から1フレームに1回呼ぶ関数。以下lock()中だけ呼ぼうね
		public void Update(int pointerX, int pointerY, int[] keys)
		{
			FrameCount++;
			PointerX = pointerX;
			PointerY = pointerY;
			for (var i = 0; i < keys.Length; i++)
			{
				this.keys[i] = keys[i];
			}
		}

		ReadOnlySpan<int> VramReadPointer()
		{
			return new ReadOnlySpan<int>(MemoryCopy, VramOffset, MemoryCopy.Length - VramOffset);
		}

		ReadOnlySpan<int> Memory()
		{
			return new ReadOnlySpan<int>(MemoryCopy);
		}

		//Machineからしか呼ばない
		public void Lock()
		{
			Monitor.Enter(lockObject);
		}

		public void Unlock()
		{
			Monitor.Exit(lockObject);
		}

		// non public -------
		bool screenSizeChanged;
		object lockObject;
		int[] keys;
	}
}