#ifndef INCLUDED_SUNABA_NETWORK_SOCKET_H
#define INCLUDED_SUNABA_NETWORK_SOCKET_H

namespace Sunaba{

//シングルスレッド前提。ノンブロッキング転送のみ行える。
//接続待ちソケットと、接続そのもののソケットの区別があるが、このレベルではクラスは同一とする。
class Socket{
public:
	static void beginSystem();
	static void endSystem();
	Socket(); //空
	Socket(int port); //サーバソケット生成。listenまで行う。
	Socket(const char* hostname, int port); //クライアントソケット生成。connectまで行う。
	~Socket();
	void accept(Socket* out); //サーバソケットしか実行できない。要求がなければ引数にisEmpty()を呼べばfalseになる
	bool isEmpty() const; //空ですか？エラーが起こると空になるのでこれで判定できる
	bool isServer() const;
	//送ったデータ量を返す。失敗なら負。失敗した場合、DELETEして作りなおす以外にない(TODO)。
	int write(const unsigned char* data, int size);
	//受け取ったデータ量を返す。失敗なら負。失敗した場合、DELETEして作りなおす以外にない(TODO)。
	int read(unsigned char* buffer, int bufferSize);
	///中身を引数に移す。コピーを許さないのでこれが必要。
	void moveTo(Socket*);
private: 
	class Impl;
	Socket(Impl*);
	Socket(const Socket&); //コピーコンストラクタ封印
	void operator=(const Socket&); //代入封印

	Impl* mImpl;
};

} //namespace Sunaba

#endif
