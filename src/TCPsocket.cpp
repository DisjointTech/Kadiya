/*
MIT License

Copyright (c) 2016 Abdel Sako

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "TCPsocket.hpp"
#include "SocketException.hpp"

/* Definition of net::TCPsocket::socket */
int net::TCPsocket::socket(void)
{
#ifdef _WIN32
	// Initialize Winsock
	//WSADATA wsaData;
	m_sockResult = WSAStartup(MAKEWORD(2, 2), &m_wsaData);
	if (m_sockResult != 0) {
		throw net::SocketException("net::TCPsocket::socket(): WSAStartup failed with error:", m_sockResult);
	}
#endif
	this->m_sockfd = ::socket(addrFamily, SOCK_STREAM, 0);
	if(!isValid())
		throw net::SocketException("net::TCPsocket::socket()", errno);

#ifdef _WIN32
	const char optval = 1;
#else
	int optval = 1;
#endif
	socklen_t optlen = sizeof optval;

	if(
		(this->addrFamily == AF_INET ?
			::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen) :
			::setsockopt(m_sockfd, IPPROTO_IPV6, SO_REUSEADDR, &optval, optlen)
		) == -1) {
		throw SocketException("::setsockopt() in net::TCPsocket::socket()", errno);
	}
	return m_sockfd;
}


/* Definition of net::TCPsocket::bind */
int net::TCPsocket::bind(const char *bindAddr, uint16_t port)

{
	if(!isValid()) return -1;
	int pton_ret;

	switch (addrFamily) {
		case AF_INET:
			std::memset(&m_localSockAddr, 0, sizeof m_localSockAddr);
			m_localSockAddr.sin_family = addrFamily;
			m_localSockAddr.sin_port = htons(port);
			if(bindAddr)
				pton_ret = ::inet_pton(addrFamily, bindAddr, &m_localSockAddr.sin_addr);
			else
				m_localSockAddr.sin_addr.s_addr = INADDR_ANY;
			break;
		case AF_INET6:
			std::memset(&m_localSockAddr6, 0, sizeof m_localSockAddr6);
			m_localSockAddr6.sin6_flowinfo = 0;
			m_localSockAddr6.sin6_family = addrFamily;
			m_localSockAddr6.sin6_port = htons(port);
			if(bindAddr)
				pton_ret = ::inet_pton(addrFamily, bindAddr, &m_localSockAddr6.sin6_addr);
			else
				m_localSockAddr6.sin6_addr = in6addr_any;
			break;
		default:
			throw SocketException("net::TCPsocket::bind()",
				"Unknown Address Family...not implemented yet.");
			break;
	}

	if(bindAddr) {
		if(pton_ret == 0) {
			throw SocketException("::inet_pton() in net::TCPsocket::bind()", "Bind address is invalid");
		}
		else if(pton_ret == -1) {
			throw SocketException("::inet_pton() in net::TCPsocket::bind()", errno);
		}
	}

	switch (addrFamily) {
	case AF_INET:
		m_sockResult = ::bind(m_sockfd,
			(struct sockaddr *)&m_localSockAddr, sizeof m_localSockAddr);
		break;
	case AF_INET6:
		m_sockResult = ::bind(m_sockfd,
			(struct sockaddr *)&m_localSockAddr6, sizeof m_localSockAddr6);
		break;
	}

	if(m_sockResult == -1) {
		throw SocketException("net::TCPsocket::bind()", errno);
	}
	else return 0;
}

/* POLL METHOD ___________________*/
short net::TCPsocket::poll(short events, int timeout)

{
	if(!isValid()) throw net::SocketException("net::TCPsocket::poll(): invalid socket.", EBADF);

#ifdef _WIN32
	WSAPOLLFD pollfds[1];
	int nfds = 1;
	fd_set fdRead, fdWrite, fdExcepts;
	timeval timeVal;

	std::memset((char*)&fdRead, 0, sizeof(fdRead));
	std::memset((char*)&timeVal, 0, sizeof(timeVal));

	fdRead.fd_array[0] = this->m_sockfd;
	fdRead.fd_count = 1;
	timeVal.tv_sec = timeout;
	m_sockResult = select(nfds, &fdRead, 0, &fdRead, &timeVal);
#else
	struct pollfd pollfds[1];
#endif
	std::memset((char *) &pollfds, 0, sizeof(pollfds));
	pollfds[0].fd = m_sockfd;
	pollfds[0].events = events;
#ifdef _WIN32
	//if (WSAPoll(pollfds, 1, timeout) == SOCKET_ERROR) {
	if (m_sockResult == -1) {
		throw net::SocketException("net::TCPsocket::poll():", WSAGetLastError());
#else
	if (::poll(pollfds, 1, timeout) < 0) {
		throw net::SocketException("net::TCPsocket::poll():", errno);
#endif
	}

    return pollfds[0].revents;
}

/* Definition of net::TCPsocket::setNonBlocking */
void net::TCPsocket::setNonBlocking(bool non_block)
{
#ifdef _WIN32
	//Winsock doesn't provide a way to check if blocking or non-blocking is set.
	u_long iMode;
	if (non_block)
		iMode = 1;
	else
		iMode = 0;
	m_sockResult = ioctl(m_sockfd, FIONBIO, &iMode);
	if (m_sockResult != NO_ERROR)
		throw net::SocketException("net::TCPsocket::setNonBlocking", WSAGetLastError());

#else
	const int flags = fcntl(m_sockfd, F_GETFL, 0);
	if(flags == -1)
		throw net::SocketException("GET TCPsocket::fcntl()", errno);

	//Check if socket is already in non-blocking mode
	if( (flags & O_NONBLOCK) && non_block)
		return;

	// Check if socket is already in blocking mode
	if( !(flags & O_NONBLOCK) && !non_block)
		return;

	// Set opt
	if(fcntl(m_sockfd, F_SETFL, non_block ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == -1)
		throw net::SocketException("SET TCPsocket::fcntl()", errno);

	return;
#endif
}

/* Definition of net::TCPsocket::read */
int net::TCPsocket::read(char *inBuffer, uint16_t inBufSize, int timeout)
{
	if(!isValid()) throw net::SocketException("net::TCPsocket::read()", EBADF);

	ssize_t byteRecv;
	size_t totalByteRecv = 0;
	int reio;
	short repoll;
#ifdef _WIN32
	u_long value;
#else
	int value;
#endif

	std::memset(inBuffer, 0, inBufSize);

	do {
        //repoll = net::TCPsocket::poll(POLLIN | POLLERR, timeout);

#ifdef _WIN32
		byteRecv = ::recv(m_sockfd, inBuffer + totalByteRecv,
			inBufSize - totalByteRecv, 0);
#else
        byteRecv = ::read(m_sockfd, inBuffer + totalByteRecv,
            inBufSize - totalByteRecv);
#endif

		switch ((int)byteRecv) {

			case -1:
			    if(errno == EAGAIN) {
                    net::TCPsocket::poll(POLLIN | POLLERR, 300);
                    reio = ::ioctl(m_sockfd, FIONREAD, &value);
                    if(value > 0) continue;
			    }
				return totalByteRecv;

			case 0:
				return totalByteRecv;

			default:
				totalByteRecv += (size_t)byteRecv;
				break;
		}
		timeout = 0;
	} while (inBufSize > totalByteRecv);

	if(totalByteRecv == 0) throw net::SocketException("net::TCPsocket::read()", EPIPE);

	return totalByteRecv;
}

/* NEW RECEIVE METHOD */
int net::TCPsocket::recv(char* inBuffer, uint16_t inBufSize, int timeout)
{
	if (!isValid()) throw net::SocketException("net::TCPsocket::read()", EBADF);

	ssize_t byteRecv;
	size_t totalByteRecv = 0;
	int ioctlResult;
	short pollResult;
	u_long value;

	std::memset(inBuffer, 0, inBufSize);

	do {
		byteRecv = ::recv(m_sockfd, inBuffer + totalByteRecv,
			inBufSize - totalByteRecv, 0);

		switch ((int)byteRecv) {

		case -1:
			//if (errno == EAGAIN) {
			//	ioctlResult = ::ioctl(m_sockfd, FIONREAD, &value);
			//	if (value > 0) continue;
			//}
			return totalByteRecv;

		case 0:
			return totalByteRecv;

		default:
			totalByteRecv += (size_t)byteRecv;
			break;
		}
		timeout = 0;
	} while (inBufSize > totalByteRecv);

	//if (totalByteRecv == 0) throw net::SocketException("net::TCPsocket::read()", EPIPE);

	return totalByteRecv;
}
/* Defition of net::TCPsocket::operator>> */
const net::TCPsocket& net::TCPsocket::operator>>(std::string &raw_data)
{
	uint16_t byteRecv;
	size_t totalByteRecv = 0;
	size_t inBufSize = this->m_flags[GET_TRANS_BUFFER];
	char *inBuffer = new char[inBufSize];
	int timeout = this->m_flags[GET_RECV_TIMEOUT];

	try {
        net::TCPsocket::setNonBlocking(true);
        do {
            byteRecv = TCPsocket::read(inBuffer, inBufSize, timeout);
            timeout = 100;

            totalByteRecv += byteRecv;
            if(byteRecv > 0)
                raw_data.append(inBuffer, byteRecv);

            if(byteRecv < inBufSize) break;

        } while(byteRecv > 0);
        net::TCPsocket::setNonBlocking(false);
    } catch(net::SocketException& e) {
            e.display();
    }

	delete inBuffer;
	inBuffer = nullptr;
	return *this;
}

/* SEND BUFFER TO CONNECTED HOST */
int net::TCPsocket::write(const std::string outBuffer, uint16_t outBufSize, int timeout)

{
	if(!isValid()) throw net::SocketException("net::TCPsocket::write()", EBADF);

	ssize_t byteSent;
	size_t totalByteSent = 0;

	do {
		net::TCPsocket::poll(POLLOUT | POLLERR, timeout);

#ifdef _WIN32
		byteSent = ::send(m_sockfd, outBuffer.data() + totalByteSent,
			outBufSize - totalByteSent, 0);
#else
		byteSent = ::write(m_sockfd, outBuffer.data() + totalByteSent,
			outBufSize - totalByteSent);
#endif

			switch ((int)byteSent) {
				case -1:
				    /* Check if all data was sent */
				    if(errno == EAGAIN) {
                        if(totalByteSent < outBufSize) {
                            timeout = 500;
                            continue;
                        }
				    }
					return totalByteSent;

				case 0:
					errno = EPIPE;
					return totalByteSent;

				default:
					totalByteSent += byteSent;
					break;
			}

			timeout = 0;
	} while (outBufSize > totalByteSent);

	return totalByteSent;
}
/* SEND METHOD*/
int net::TCPsocket::send(const std::string outBuffer, uint16_t outBufSize, int timeout)

{
	if (!isValid()) throw net::SocketException("net::TCPsocket::write()", EBADF);

	ssize_t byteSent;
	size_t totalByteSent = 0;

	do {
		//net::TCPsocket::poll(POLLOUT | POLLERR, timeout);

		byteSent = ::send(m_sockfd, outBuffer.data() + totalByteSent,
			outBufSize - totalByteSent, 0);

		switch ((int)byteSent) {
		case -1:
			/* Check if all data was sent */
			if (errno == EAGAIN) {
				//if (totalByteSent < outBufSize) {
				//	continue;
				//}
			}
			return totalByteSent;

		case 0:
			errno = EPIPE;
			return totalByteSent;

		default:
			totalByteSent += byteSent;
			break;
		}
	} while (outBufSize > totalByteSent);

	return totalByteSent;
}

/* WRITE OPERATOR*/
const net::TCPsocket& net::TCPsocket::operator<<(const std::string raw_data)
{
	uint16_t byteSent;
	size_t totalByteSent = 0;
	size_t data_size = raw_data.length();
	int timeout = this->m_flags[GET_SEND_TIMEOUT];

	size_t chunkBytes = this->m_flags[GET_TRANS_BUFFER], lastBytes = 0, chunks = 0;
	if(data_size > chunkBytes) {
		chunks = data_size / chunkBytes;
		lastBytes = data_size % chunkBytes;
	}

    try {
        setNonBlocking(true);
        if(chunks == 0) {
            byteSent = TCPsocket::write(raw_data, data_size, timeout);
            totalByteSent += byteSent;
        }
        else {
            size_t pos = 0;

            for(int n = 0; n < chunks; n++) {
                byteSent = TCPsocket::write(raw_data.substr(pos, chunkBytes),
                    chunkBytes, timeout);

                if(byteSent == 0) break;

                totalByteSent += byteSent;
                pos += chunkBytes;

                if(n == chunks - 1)
                    if(lastBytes != 0) {
                        byteSent = TCPsocket::write(raw_data.substr(pos, lastBytes),
                            lastBytes, timeout);
                        totalByteSent += byteSent;
                    }
            }
        }

        setNonBlocking(false);

    } catch(net::SocketException& e) {
        e.display();
    }

	if(totalByteSent < data_size) {
		std::cout << "[*] Failed to write all data: " << totalByteSent
				<< "/" << data_size << " bytes\n";
	}
	return *this;
}

int net::TCPsocket::flags(net::flags what)
{
    switch(what)
    {
    case GET_WILL_CLOSE_SOCKET:
        return this->m_flags[what];

    case GET_KEEP_ALIVE:
        return this->m_flags[what];

    case GET_TRANS_BUFFER:
        return this->m_flags[what];

    case GET_RECV_TIMEOUT:
        return this->m_flags[what];

    case GET_SEND_TIMEOUT:
        return this->m_flags[what];

    default:
        return -1;
    }
}

int net::TCPsocket::flags(net::flags what, int value)
{
    switch(what)
    {
    case SET_WILL_CLOSE_SOCKET:
        this->m_flags[what] = value;
        return 0;

    case SET_KEEP_ALIVE:
        if((net::TCPsocket::setKeepAlive((bool)value)) == -1)
            return -1;
        else
            return 0;

    case SET_TRANS_BUFFER:
        if(this->m_flags[what] != value) {
            if(value < 1) return -1;

            this->m_flags[what] = value;
            return 0;
        }
        return 0;

    case SET_RECV_TIMEOUT:
        this->m_flags[what] = value;
        return 0;

    case SET_SEND_TIMEOUT:
        this->m_flags[what] = value;
        return 0;

    default:
        return -1;
    }
}

/* */
int net::TCPsocket::shutdown(int how)
{
	if(isValid()) {
		if(::shutdown(m_sockfd, how) == -1)
			return -1;
		else
			return 0;
	} else
		return -1;
}

/* CLOSE socket(net::SOCKET) */
int net::TCPsocket::close(void)
{
	if (isValid()) {
#ifdef _WIN32
		::closesocket(this->m_sockfd);
		return 0;
#else
		//m_sockResult = ::close(this->m_sockfd);
		if (::close(this->m_sockfd) == 0)
			return 0;
		else
			return -1;
#endif
	}
	else
		return -1;
}

struct net::PeerInfo net::TCPsocket::getPeerInfo(void)
{
    return peerInfo;
}

/* keep alive */
int net::TCPsocket::setKeepAlive(bool keep_alive)
{
#ifdef _WIN32
	const char optval = (int)keep_alive;
#else
	int optval = (int)keep_alive;
#endif
	if (setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
		return -1;

/*
	optval = 2;
	if (setsockopt(m_sockfd, SOL_TCP, TCP_KEEPCNT, &optval, sizeof(optval)) < 0)
		return -1;

	optval = 5;
	if (setsockopt(m_sockfd, SOL_TCP, TCP_KEEPIDLE, &optval, sizeof(optval)) < 0)
		return -1;

	optval = 2;
	if (setsockopt(m_sockfd, SOL_TCP, TCP_KEEPINTVL, &optval, sizeof(optval)) < 0)
		return -1;
*/

	int res;

	keep_alive ? res = 1 : res = 0;
	return res;
}
