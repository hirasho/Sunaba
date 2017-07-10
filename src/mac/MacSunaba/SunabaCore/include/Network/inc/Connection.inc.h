#include "Network/Socket.h"
#include "Base/Utility.h"

namespace Sunaba{

inline Connection::Connection() : 
mReadingBufferPos(0),
mReadingSizeBufferPos(0){
}

inline Connection::Connection(const char* hostname, int port) : 
mSocket(hostname, port),
mReadingBufferPos(0),
mReadingSizeBufferPos(0){
}

inline Connection::~Connection(){
}

inline void Connection::write(const unsigned char* data, int size){
	ASSERT(size > 0);
	mLock.lock();
	mWritingList.push_back(Array<unsigned char>()); //空で追加
	Array<unsigned char>& buf = mWritingList.back();
	//まず4バイトデカいバッファを用意して、サイズをビッグエンディアンで書き込み、
	buf.setSize(size + 4);
	buf[0] = static_cast<unsigned char>((size >> 24) & 0xff);
	buf[1] = static_cast<unsigned char>((size >> 16) & 0xff);
	buf[2] = static_cast<unsigned char>((size >> 8) & 0xff);
	buf[3] = static_cast<unsigned char>((size >> 0) & 0xff);
	//残りをコピー
	memcpy(buf.pointer() + 4, data, size);
	//実際の転送はupdateまで保留
	mLock.unlock();
}

inline void Connection::write(const wchar_t* data, int charCount){
	mLock.lock();
	if (charCount < 0){
		charCount = getStringSize(data);
	}
	ASSERT(charCount > 0);
	mWritingList.push_back(Array<unsigned char>()); //空で追加
	Array<unsigned char>& buf = mWritingList.back();
	//まず4バイトデカいバッファを用意して、サイズをビッグエンディアンで書き込み、
	int size = charCount * 2;
	buf.setSize(size + 4);
	buf[0] = static_cast<unsigned char>((size >> 24) & 0xff);
	buf[1] = static_cast<unsigned char>((size >> 16) & 0xff);
	buf[2] = static_cast<unsigned char>((size >> 8) & 0xff);
	buf[3] = static_cast<unsigned char>((size >> 0) & 0xff);
	//残りをビッグエンディアンでばらしながらコピー
	for (int i = 0; i < charCount; ++i){
		buf[(i * 2) + 4] = static_cast<unsigned char>((data[i] >> 8) & 0xff);
		buf[(i * 2) + 5] = static_cast<unsigned char>((data[i] >> 0) & 0xff);
	}
	//実際の転送はupdateまで保留
	mLock.unlock();
}

inline void Connection::read(Array<unsigned char>* data){
	mLock.lock();
	if (mReadingBufferPos == mReadingBuffer.size()){ //転送終了(0バイトでも同じ処理)
		mReadingBuffer.moveTo(data);
		mReadingBufferPos = 0;
		mReadingSizeBufferPos = 0;
	}
	mLock.unlock();
}

inline void Connection::read(Array<wchar_t>* data){
	mLock.lock();
	int size = mReadingBuffer.size();
	if (mReadingBufferPos == size){ //転送終了(0バイトでも同じ処理)
		STRONG_ASSERT((size % 2) == 0); //偶数バイトのはずだよ！
		int charCount = size / 2;
		for (int i = 0; i < charCount; ++i){
			(*data)[i] = mReadingBuffer[(i * 2) + 0] << 8;
			(*data)[i] += mReadingBuffer[(i * 2) + 1] << 0;
		}
		mReadingBuffer.clear();
		mReadingBufferPos = 0;
		mReadingSizeBufferPos = 0;
	}
	mLock.unlock();
}

inline void Connection::update(){
	if (isError()){
		return;
	}
	mLock.lock();
	//保留中の書き込みを試す
	std::list<Array<unsigned char> >::iterator it = mWritingList.begin();
	std::list<Array<unsigned char> >::iterator end = mWritingList.end();
	while (it != end){
		std::list<Array<unsigned char> >::iterator next = it;
		++next;

		Array<unsigned char>& buf = *it;
		int dataSize = buf.size();
		int size = mSocket.write(buf.pointer(), dataSize);
		if (size < 0){ //エラー発生。もうダメだ。
			break;
		}else if (size == dataSize){ //全部おくれた
			mWritingList.erase(it); //このデータ捨てていい
		}else if (size < dataSize){ //送りきれなかった。
			int rest = buf.size() - size;
			for (int i = 0; i < rest; ++i){
				buf[i] = buf[size + i];
			}
			buf.setSize(rest);
			break; //ここで終わり
		}else{
			HALT("unexpected"); //ありえないはず
		}
		it = next;
	}
	//読み込みを試行
	if (mReadingBufferPos < mReadingBuffer.size()){ //途中なので継ぎ足す
		int rest = mReadingBuffer.size() - mReadingBufferPos;
		int transfered = mSocket.read(mReadingBuffer.pointer() + mReadingBufferPos, rest);
		if (transfered < 0){ //エラー発生。もうだめだ
			; //でも何もしない
		}else if (transfered == 0){ //まだデータが来てない。
			; //何もしない
		}else if (transfered > rest){ //そんなことはありえない。
			HALT("unexpected");
		}else{ //正常にデータ取得
			mReadingBufferPos += transfered;
		}
	}else if (mReadingSizeBufferPos < 4){ //まだサイズの読み取りが終わってない
		ASSERT(mReadingBuffer.size() == 0); //こうでなかったら何かおかしい
		int rest = 4 - mReadingSizeBufferPos;
		int transfered = mSocket.read(mReadingSizeBuffer + mReadingSizeBufferPos, rest);
		if (transfered < 0){ //エラー発生。もうだめだ
			; //でも何もしない
		}else if (transfered > rest){ //ありえない
			HALT("unexpected");
		}else{ //正常にデータ取得
			mReadingSizeBufferPos += transfered;
		}
		if (mReadingSizeBufferPos == 4){
			//バッファ確保
			int size = mReadingSizeBuffer[0] << 24;
			size += mReadingSizeBuffer[1] << 16;
			size += mReadingSizeBuffer[2] << 8;
			size += mReadingSizeBuffer[3];
			mReadingBuffer.setSize(size);
			mReadingBufferPos = 0;
			update(); //コード長くなるの嫌なのでもう一回実行。それで読み込みが走る。
		}
	}
	mLock.unlock();
}

inline bool Connection::isError() const{
	return mSocket.isEmpty();
}

} //namespace Sunaba
