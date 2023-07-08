#include "http/http.hpp"
#include "http/httpServer/httpServerConfig.hpp"
#include "http/httpServer/httpServer.hpp"
#include "http/httpServer/httpServerHelpers.hpp"
#include "helpers.hpp"

std::ifstream configStream(PROJECT_DIR + SERVER_CONFIG_FILE);
HttpServerConfig httpServerConfig(configStream);

const std::string DOC_ROOT_DIR(
	"../../../" + httpServerConfig.getDocument_Root());

/* Function */


void http::httpServer(std::shared_ptr<net::TCPserver> server, net::TCPpeer peer) {
	std::ifstream fileStreamToServe;
	std::string filename;
	std::string pathToDoc(DOC_ROOT_DIR);

	int inStatus, outStatus;
	std::string rawData;
	if (inStatus = http::read(peer, rawData)) {
		/* Dont waste your time if the request is empty. */
		if (rawData.empty()) {
			peer.killConn();
			return;
		}

		http::requestParser reqData(rawData);
		std::cout << "[+] request: " << reqData.url_or_host << std::endl;
		if (reqData.url_or_host == "/") {
			filename = "index.html";
			pathToDoc.append(filename);
		}
		else {
			filename = reqData.url_or_host;
			pathToDoc.append(filename);
		}
		printPath();
		FileHandler fileHandler(pathToDoc);

		/* If the file isn't opened */
		if (!fileHandler.isOpen()) {
			std::string response(documentNotFound());
			http::write(peer, response);
			peer.killConn();
			fileHandler.close();
			return;
		}

		/* File ext*/
		std::string fileExt = filename.substr(filename.rfind(".") + 1);
		
		/* file is Opened */
		HeaderHandler headerHandler;
		/* +4 for  "\r\n\r\n" */
		headerHandler.setContentLength(fileHandler.getSize() + 4);
		headerHandler.setConnection("closed");
		headerHandler.setContentType(fileExt);
		std::string header = headerHandler.getHeader();
		http::write(peer, header);

		std::string tmp;
		while (!fileHandler.eof()) {
			tmp.clear();
			tmp = fileHandler.getLine() + "\n";
			http::write(peer, tmp);
		}
		/* Send these characters to mark the end of the http transfer. */
		http::write(peer, "\r\n\r\n"); 
		fileHandler.close();
	}
	peer.killConn();

}

void prepResponseHeader(std::string& resData, HeaderHandler& headers) {
	resData.clear();

}

/* TO-DOs:
1. Work on eof of files that are read. implement a code a check if the file
ends with "\r\n\r\n" and then add them before sending the response data back
to the client. This is needed to figure out the length of the total response 
data to be sent to the client; so we can set the "content-length" in the 
header.

2. Work on the Max-Host option of the config file. The listen() of the server
class runs before httpServer.hpp reads the config file. 
Socket programming probably provides a way to change your maxhost options 
after launching the server. */