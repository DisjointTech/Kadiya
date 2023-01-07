#include "TCPsocket.hpp"

namespace serverDB {
	void socketServer(net::TCPpeer peer) {
		std::string welcome = "Welcome to my socket server.\nWhat's your name?\n";
		std::string greet = "We hope you're having a good day, ";
		std::string data;
		peer.send(welcome, welcome.length(), 0);

		uint16_t inBufSize = 200;
		char* inBuffer = new char[inBufSize];
		int byteRecv = 0, byteSend = 0;
		
		try {
			while (peer.getLastError() == 0) {
				byteRecv = peer.recv(inBuffer, inBufSize, 0);
				std::cout << "Data Recv: " << inBuffer << std::endl;
				data = inBuffer;
				byteSend = peer.send(data, data.length(), 0);
				std::cout << "Data Sent Back.\n";
			}
		}
		catch (net::SocketException& e) {
			std::cout << "serverDB::socketServer() ";
			e.display();
		}
		
	}
}