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

	void PostReceive() // ������ ����
	{
		memset(&m_ReceiveBuffer, '\0', sizeof(m_ReceiveBuffer)); // ���ſ� ���� �ʱ�ȭ
		m_Socket.async_receive_from(boost::asio::buffer(m_ReceiveBuffer, 128), m_SenderEndpoint, boost::bind(&UDP_Server::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		/*
			UDP�� Ư���� �������� ���� ���¿��� �����͸� �ְ�ޱ� ������, ��� �����͸� ���´��� �� �� ����.
			�׷��� m_SenderEndpoint�� ���� ���� �ּҸ� �̸� �����س��� �Ѵ�.

			async_receive_from�� UDP �󿡼� �����͸� ������ �� ���Ǵ� �Լ���.
			���ڴ� ���� ����, �߽��� IP(�����), ���� �Լ��� �����ȴ�
		*/
	}

private:
	boost::asio::ip::udp::socket m_Socket; // UDP ����
	boost::asio::ip::udp::endpoint m_SenderEndpoint; // Ŭ���̾�Ʈ�� �ּ�
	/* 
		UDP�� Ư���� �������� ���� ���¿��� �����͸� �ְ�ޱ� ������ ��� �����͸� ���´��� �� �� ����. 
		�׷��� �������� �ּҰ� �ݵ�� �ʿ��ϴ�. 
	*/
	
	std::string m_WriteMessage; // ���� �޽���
	std::array<char, 128> m_ReceiveBuffer; // ���ſ� ����
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
			const std::string strRecvMessage = m_ReceiveBuffer.data(); // ���� ������ ����
			cout << "Ŭ���̾�Ʈ���� ���� �޽��� : " << strRecvMessage << ", ���� ũ�� : " << bytes_transferred << ", From : " << m_SenderEndpoint.address().to_string().c_str() << " " << m_SenderEndpoint.port() << endl;

			char szMessage[128] = { 0, }; // ���� �޽���
			sprintf_s(szMessage, 128 - 1, "Re:%s", strRecvMessage.c_str());
			m_WriteMessage = szMessage; // ����(�߽�) �޽����� ��ȯ

			// ������ �߽�(����)
			m_Socket.async_send_to(boost::asio::buffer(m_WriteMessage), boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(UDP_IP), CLIENT_PORT_NUMBER), boost::bind(&UDP_Server::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			/* 
				UDP�� ����(����ȭ)���� ���� ���¿��� �����͸� �ְ�ޱ� ������, ���� ���� �ּ�(_to)�� �˰� �־�� �Ѵ�.
				async_send_to�� ���ڴ� �߽ſ� ����, ���� �ּ�(endpoint), ���ε� �Լ��� �����ȴ�. 
			*/
		}
		PostReceive(); // ���� �� ���� �����͸� ����
	}

};

int main()
{

	cout << "UDP ���� ����" << endl;

	boost::asio::io_context io_service;
	UDP_Server server(io_service);
	io_service.run();

	cout << "��Ʈ��ũ ����" << endl;

	// UDP �������� Ư���� ������ accept�� Ŭ���̾�Ʈ�� connect�� �ʿ� ����, UDP�� �ۼ��� �Լ��� ����Ͽ� write�� read�� �����Ѵٴ� ���� Ư¡�̴�.

}