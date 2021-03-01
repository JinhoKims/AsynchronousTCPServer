#include<sdkddkver.h>
#include<iostream>
#include<algorithm>
#include<string>
#include<list>
#include<boost/bind.hpp>
#include<boost/asio.hpp>
using namespace std;

const char UDP_IP[] = "127.0.0.1";
const unsigned short SERVER_PORT_NUMBER = 31400;
const unsigned short CLIENT_PORT_NUMBER = 31401;

class UDP_Server // UDP Echo Server
{
public:
	UDP_Server(boost::asio::io_service& io_service) :m_Socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), SERVER_PORT_NUMBER))
	{
		PostReceive();
	}
	~UDP_Server() {}

	void PostReceive() // 데이터 수신
	{
		memset(&m_ReceiveBuffer, '\0', sizeof(m_ReceiveBuffer)); // 수신용 버퍼 초기화
		m_Socket.async_receive_from(boost::asio::buffer(m_ReceiveBuffer, 128), m_SenderEndpoint, boost::bind(&UDP_Server::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		/*
			UDP의 특성상 연결하지 않은 상태에서 데이터를 주고받기 때문에, 어디서 데이터를 보냈는지 알 수 없다.
			그래서 m_SenderEndpoint에 보낸 측의 주소를 미리 저장해놔야 한다.

			async_receive_from는 UDP 상에서 데이터를 수신할 때 사용되는 함수다.
			인자는 수신 버퍼, 발신자 IP(저장용), 랩핑 함수로 구성된다
		*/
	}

private:
	boost::asio::ip::udp::socket m_Socket; // UDP 소켓
	boost::asio::ip::udp::endpoint m_SenderEndpoint; // 클라이언트의 주소
	/* 
		UDP의 특성상 연결하지 않은 상태에서 데이터를 주고받기 때문에 어디서 데이터를 보냈는지 알 수 없다. 
		그래서 접속자의 주소가 반드시 필요하다. 
	*/
	
	std::string m_WriteMessage; // 전송 메시지
	std::array<char, 128> m_ReceiveBuffer; // 수신용 버퍼
	int m_nSeqNumber;

	void handle_write(const boost::system::error_code&, size_t)
	{

	}

	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred)
	{
		if (error)
		{
			cout << "error No : " << error.value() << "error Message : " << error.message() << endl;
		}
		else
		{
			const std::string strRecvMessage = m_ReceiveBuffer.data(); // 수신 데이터 복사
			cout << "클라이언트에서 받은 메시지 : " << strRecvMessage << ", 받은 크기 : " << bytes_transferred << ", From : " << m_SenderEndpoint.address().to_string().c_str() << " " << m_SenderEndpoint.port() << endl;

			char szMessage[128] = { 0, }; // 에코 메시지
			sprintf_s(szMessage, 128 - 1, "Re:%s", strRecvMessage.c_str());
			m_WriteMessage = szMessage; // 에코(발신) 메시지로 전환

			// 데이터 발신(에코)
			m_Socket.async_send_to(boost::asio::buffer(m_WriteMessage), boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(UDP_IP), CLIENT_PORT_NUMBER), boost::bind(&UDP_Server::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			/* 
				UDP는 연결(동기화)되지 않은 상태에서 데이터를 주고받기 때문에, 보낼 곳의 주소(_to)를 알고 있어야 한다.
				async_send_to의 인자는 발신용 버퍼, 보낼 주소(endpoint), 바인딩 함수로 구성된다. 
			*/
		}
		PostReceive(); // 에코 후 다음 데이터를 받음
	}

};

int main()
{

	cout << "UDP 서버 실행" << endl;

	boost::asio::io_context io_service;
	UDP_Server server(io_service);
	io_service.run();

	cout << "네트워크 종료" << endl;

	// UDP 프로토콜 특성상 서버의 accept와 클라이언트의 connect가 필요 없고, UDP용 송수신 함수를 사용하여 write와 read를 구현한다는 점이 특징이다.

}