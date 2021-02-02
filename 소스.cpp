#include<sdkddkver.h>
#include<iostream>
#include<algorithm>
#include<string>
#include<list>
#include<boost/bind.hpp>
#include<boost/asio.hpp>
using namespace std;

const unsigned short PORT_NUMBER = 31400;

class Session // ������ ������ Ŭ���̾�Ʈ(����)
{
public:
	boost::asio::ip::tcp::socket m_Socket; // ������ Ŭ���̾�Ʈ�� �Ҵ��� ����(�����)
	string m_WriteMessage;
	array<char, 128> m_ReceiveBuffer;

	Session(boost::asio::io_service& io_service) :m_Socket(io_service) {}
	
	boost::asio::ip::tcp::socket& Socket()
	{
		return m_Socket;
	}

	void PostReceive() // ������ ����
	{
		memset(&m_ReceiveBuffer, '\0', sizeof(m_ReceiveBuffer)); // ���ſ� ���� �ʱ�ȭ
		m_Socket.async_read_some // ��Ŷ ������ ���� ������ Ŭ���̾�Ʈ���� �߽��� �����͸� �񵿱������� ���� 
		(
			boost::asio::buffer(m_ReceiveBuffer), // Ŭ���̾�Ʈ�� ���� ����
			boost::bind(&Session::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) // async_read_some�� �Ϸ�Ǹ� ȣ���� �Լ� ����(handle_receive)
			/* 
				async_xxx �Լ��� �̺�Ʈ �ڵ鷯ó�� �ش� �Լ��� �Ϸ�(����)�Ǹ� �ݹ��Ѵ�.
				bind(���ε�)�� ���� ����(ȣ���� �Լ�, ��� �Լ��� ��� � �ν��Ͻ�����(��� �Լ��� ����, this), �Լ� ����) 
			*/
		);
	}

private:
	void handle_write(const boost::system::error_code&, size_t) 
	{
		cout << "������ ���� �Ϸ�" << endl;
	}

	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred) // �ڵ鸵�� ���� ������ ��� �Լ�
	{
		if (error) // ���� �߻� ��
		{
			if (error == boost::asio::error::eof)
			{
				cout << "Ŭ���̾�Ʈ�� ������ ���������ϴ�." << endl;
			}
			else
			{
				std::cout << "error No:" << error.value() << "error Message: " << error.message() << endl;
			}
		}
		else // ���� ���� ��
		{
			const string strRecvMessage = m_ReceiveBuffer.data();
			cout << "Ŭ���̾�Ʈ���� ���� �޽��� : " << strRecvMessage << ", ���� ũ�� : " << bytes_transferred << endl;

			char szMessage[128] = { 0, };
			sprintf_s(szMessage, 128 - 1, "Re:%s", strRecvMessage.c_str()); // sprintf�� szMessage�� "Re:" + strRecvMessage(���Ÿ޽���) ����, "Re:"�� char�� ���ؼ��� ���� �� ������ ���� �����͸� ���� char ������ ��� �ٲ��ش�.
			m_WriteMessage = szMessage; // �߽ſ� ����<string>�� "Re:"�� �߰��� ���� ������<char>�� ����

			// ������ �����͸� �ٽ� �߽�(����)
			// async_write�� ���� ����(����, �߽ſ� ����, ������ �Լ�)
			boost::asio::async_write(m_Socket, boost::asio::buffer(m_WriteMessage), // Ŭ���̾�Ʈ ����(m_Socket)�� ������ ����
				boost::bind(&Session::handle_write, this, // ������ ȣ���� handle_write�� ���ڿ� �Բ� ���ε��Ͽ� ����
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

			// ���� �� �ٽ� ������ ���� �Լ� ȣ��(�ݺ�)
			PostReceive();
		}
	}
};

class TCP_Server // TCP ����
{
public:
	int m_nSeqNumber;
	boost::asio::ip::tcp::acceptor m_acceptor; // Ŭ���̾�Ʈ�� ������ ����ϴ� ��ü
	boost::asio::io_service& m_io_service; // I/O �̺�Ʈ�� OS���� ����ġ�ϴ� ��ü
	Session* m_pSession;

	TCP_Server(boost::asio::io_service& io_service) : m_io_service(io_service), m_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), PORT_NUMBER))
	{
		m_pSession = nullptr;
		StartAccept(); // Ŭ���̾�Ʈ ���� �ޱ�
	}

	~TCP_Server()
	{
		if (m_pSession != nullptr)
		{
			delete m_pSession;
		}
	}

private:
	void StartAccept() // Ŭ���̾�Ʈ ���� �ޱ� (1ȸ ����)
	{
		cout << "Ŭ���̾�Ʈ ���� ���..." << endl;
		m_pSession = new Session(m_io_service);
		m_acceptor.async_accept(m_pSession->Socket(), boost::bind(&TCP_Server::handle_accept, this, m_pSession, boost::asio::placeholders::error)); // Ŭ���̾�Ʈ�� ������ �񵿱�� ó��
		/*
			ù ��° ���ڴ� Ŭ���̾�Ʈ�� �Ҵ��� ���� Ŭ������ ���ϸ�,
			�� ��° ���ڴ� bind �Լ��� ���Ͽ� �̺�Ʈ�ڵ鷯ó�� async_accept�� �Ϸ�Ǹ� ȣ��� �Լ�(handle_accept)�� �����Ͽ���.

			async_accept �Լ��� ȣ��Ǹ� ������ ���� ������ ������� �ʰ� �ٷ� ���� �ٷ� �Ѿ��.(async, �񵿱� �Լ��̱� ����)
			async_accept �۾��� ������ �� ��° ������ bind(handle_accept,...) �Լ��� ȣ��ȴ�.

			bind �Լ��� �μ��� ������ �� ���Ǵµ� handle_accept �Լ��� ȣ���� �� �μ��� ���� �־�����ϱ� ������ 
			bind �Լ��� �μ��� ȣ���� �Լ��� ���� ��� �Ѱ��ش�.
		*/
	}

	void handle_accept(Session* pSession, const boost::system::error_code& error)  // async_accept �Ϸ� �� �ڵ鸵 �� (1ȸ ����)
	{
		if (!error) // ���� ��
		{
			cout << "Ŭ���̾�Ʈ ���� ����" << endl;
			pSession->PostReceive(); // Ŭ���̾�Ʈ ���� ���� �� ������ ���� �Լ� ȣ�� (�ش� �Լ��� �ڰ� �ݺ���)
		}
	}

};

int main()
{
	
	/*  
												=== ������ ���� ���� ===
		���� �ޱ� ��û(async_accept) -> ������ �ޱ� ��û(async_read_some) -> ������ ����(async_write)
		������ �񵿱� �Լ��� �����Ͽ� �����͸� ��ٸ��� �۾� ���� �ٷ� ���� ������ ������ �� �ִ�. (�޴� ��� �����͸� �ٷ� ����(����)�� �� �ִ�.)
	*/

	boost::asio::io_service io_service;
	TCP_Server server(io_service); // ������ ȣ�� �� �ٷ� StartAccept() ȣ��
	io_service.run(); // main ���� ����
	/*
		������ ���α׷���ó�� run() �Լ��� ȣ���ϸ� main() �Լ��� ���� ��� ���°� �ǰ� 
		Boost.Asio�� �񵿱� �Լ� �۾��� ��� ���� ������ ������ ����ϴٰ� 
		�۾��� ��� ������ run() �Լ��� �������ͼ� ���� �۾��� �����Ѵ�.
	*/
	cout << "��Ʈ��ũ ���� ����" << endl;
}