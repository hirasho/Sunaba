#include "Network/Connection.h"

namespace Sunaba{

inline Server::Server(int port) : mSocket(port){
}

inline Server::~Server(){
}

inline Connection* Server::accept(){
	Socket tmpSocket;
	mSocket.accept(&tmpSocket);
	Connection* r = 0;
	if (!(tmpSocket.isEmpty())){
		r = new Connection();
		tmpSocket.moveTo(&(r->mSocket)); //移動
	}
	return r;
}

} //namespace Sunaba
