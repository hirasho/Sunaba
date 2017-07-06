#pragma once

namespace Sunaba{
	class System;
	class NetworkManager;
	class Connection;
};

using namespace System;

namespace SunabaLib{
	//サーバ側の描画、コンパイラを全部隠蔽
	public ref class SunabaSystem
	{
	public:
		enum class Key{
			KEY_UNKNOWN = -1,
			KEY_LBUTTON = 0,
			KEY_RBUTTON,
			KEY_UP,
			KEY_DOWN,
			KEY_LEFT,
			KEY_RIGHT,
			KEY_SPACE,
			KEY_ENTER,

			KEY_COUNT,
		};
		SunabaSystem(IntPtr windowHandle, System::String^ langName);
		~SunabaSystem();
		void setPictureBoxHandle(IntPtr pictureBoxHandle, int screenWidth, int screenHeight);
		void end();
		bool bootProgram(array<System::Byte>^ objectCode);
		bool bootProgram(System::String^ filename);
		void update(array<wchar_t>^% messageOut, int pointerX, int pointerY, array<System::SByte>^ keys);
		int screenWidth();
		int screenHeight();
		//実行情報 適当
		int framePerSecond();
		int calculationTimePercent();
		void requestAutoSync(bool);
		bool requestAutoSyncFinished();
		bool autoSync();
		void requestMemoryWrite(int address, int value);
		bool requestMemoryFinished();
		int memoryValue(int address);
	private:
		Sunaba::System* mSystem;
	};
	public ref class Compiler
	{
	public:
		static bool compile(
			array<unsigned>^% instructionsOut,
			array<wchar_t>^% messageOut,
			System::String^ filename,
			System::String^ langName);
	};
}
