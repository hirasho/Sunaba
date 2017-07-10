#ifndef INCLUDED_SUNABA_NETWORK_CONNECTION_H
#define INCLUDED_SUNABA_NETWORK_CONNECTION_H

#include <list>
#include "Network/Socket.h"
#include "Base/Os.h"
#include "Base/Array.h"

namespace Sunaba{

class Connection{
public:
	//論理転送単位として書き込む
	void write(const unsigned char* data, int size);
	void write(const wchar_t* unicodeString, int charCount = -1); //-1なら文字列長は自動判別(NULL終端)
	//一つの論理転送単位を読み込む
	void read(Array<unsigned char>* data);
	void read(Array<wchar_t>* data);

	//エラーですか？これが立ったら破棄してやり直して欲しい(TODO:復帰)
	bool isError() const;
private:
	friend class NetworkManager;
	friend class Server;
	Connection();
	Connection(const char* hostname, int port);
	~Connection();
	void update(); //保留中の転送を試す
	Connection(const Connection&); //コピーコンストラクタ封印
	void operator=(const Connection&); //代入封印

	Socket mSocket;
	std::list<Connection*>::iterator mIterator;
	std::list<Array<unsigned char> > mWritingList; //書き込み保留リスト
	Array<unsigned char> mReadingBuffer;
	int mReadingBufferPos;
	unsigned char mReadingSizeBuffer[4];
	int mReadingSizeBufferPos;

	Mutex mLock;
};

} //namespace Sunaba

#include "Network/inc/Connection.inc.h"

#endif
