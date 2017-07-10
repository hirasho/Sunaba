#ifndef INCLUDED_SUNABA_NETWORK_NETWORKMANAGER_H
#define INCLUDED_SUNABA_NETWORK_NETWORKMANAGER_H

#include <list>

namespace Sunaba{
class Server;
class Connection;

//1個しか作らないでね。敢えてシングルトンにはしないよ。
class NetworkManager{
public:
	NetworkManager();
	~NetworkManager();
	Server* createServer(int port);
	Connection* accept(Server*);
	Connection* createClient(const char* hostname, int port);
	void destroyServer(Server**);
	void destroyConnection(Connection**);
	//一定周期で呼ぶこと。遅延した書き込みや、中途半端な読み込みを処理する。
	void update();
private:
	NetworkManager(const NetworkManager&); //コピーコンストラクタ封印
	void operator=(const NetworkManager&); //代入封印

	std::list<Connection*> mConnections;
};

} //namespace Sunaba

#include "Network/inc/NetworkManager.inc.h"

#endif

