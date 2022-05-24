using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DropHandlerWrapper
{
	public DropHandlerWrapper(System.Action<string> onDrop)
	{
		this.onDrop = onDrop;

#if !UNITY_EDITOR
#if UNITY_STANDALONE_OSX
		Shibuya24.Utility.UniDragAndDrop.Initialize();
		Shibuya24.Utility.UniDragAndDrop.onDragAndDropFilePath = OnDropFile;
#elif UNITY_STANDALONE_WIN
		B83.Win32.UnityDragAndDropHook.InstallHook();
        B83.Win32.UnityDragAndDropHook.OnDroppedFiles += OnDropFiles;
#endif 
#endif
	}

	// non public -----
	System.Action<string> onDrop;

	void OnDropFile(string path)
	{
		onDrop?.Invoke(path);
	}

	void OnDropFiles(List<string> paths, B83.Win32.POINT pos)
	{
		foreach (var path in paths)
		{
			onDrop?.Invoke(path);
		}
	}
}
