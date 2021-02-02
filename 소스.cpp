#include<sdkddkver.h>
#include<iostream>
#include<algorithm>
#include<string>
#include<list>
#include<boost/bind.hpp>
#include<boost/asio.hpp>
using namespace std;

const unsigned short PORT_NUMBER = 31400;

class Session // 서버에 접속한 클라이언트(세션)
{
public:
	boost::asio::ip::tcp::socket m_Socket; // 접속한 클라이언트에 할당할 소켓(제어용)
	string m_WriteMessage;
	array<char, 128> m_ReceiveBuffer;

	Session(boost::asio::io_service& io_service) :m_Socket(io_service) {}
	
	boost::asio::ip::tcp::socket& Socket()
	{
		return m_Socket;
	}

	void PostReceive() // 데이터 수신
	{
		memset(&m_ReceiveBuffer, '\0', sizeof(m_ReceiveBuffer)); // 수신용 버퍼 초기화
		m_Socket.async_read_some // 패킷 수신이 없을 때까지 클라이언트에서 발신한 데이터를 비동기적으로 수신 
		(
			boost::asio::buffer(m_ReceiveBuffer), // 클라이언트가 받을 버퍼
			boost::bind(&Session::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) // async_read_some이 완료되면 호출할 함수 랩핑(handle_receive)
			/* 
				async_xxx 함수는 이벤트 핸들러처럼 해당 함수가 완료(리턴)되면 콜백한다.
				bind(바인딩)의 인자 구조(호출할 함수, 멤버 함수일 경우 어떤 인스턴스인지(멤버 함수의 주인, this), 함수 인자) 
			*/
		);
	}

private:
	void handle_write(const boost::system::error_code&, size_t) 
	{
		cout << "데이터 에코 완료" << endl;
	}

	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred) // 핸들링된 수신 데이터 출력 함수
	{
		if (error) // 에러 발생 시
		{
			if (error == boost::asio::error::eof)
			{
				cout << "클라이언트와 연결이 끊어졌습니다." << endl;
			}
			else
			{
				std::cout << "error No:" << error.value() << "error Message: " << error.message() << endl;
			}
		}
		else // 수신 성공 시
		{
			const string strRecvMessage = m_ReceiveBuffer.data();
			cout << "클라이언트에서 받은 메시지 : " << strRecvMessage << ", 받은 크기 : " << bytes_transferred << endl;

			char szMessage[128] = { 0, };
			sprintf_s(szMessage, 128 - 1, "Re:%s", strRecvMessage.c_str()); // sprintf로 szMessage에 "Re:" + strRecvMessage(수신메시지) 대입, "Re:"는 char을 통해서만 붙일 수 있으니 수신 데이터를 지역 char 변수로 잠시 바꿔준다.
			m_WriteMessage = szMessage; // 발신용 버퍼<string>에 "Re:"가 추가된 수신 데이터<char>를 담음

			// 수신한 데이터를 다시 발신(에코)
			// async_write의 인자 구성(소켓, 발신용 버퍼, 랩핑할 함수)
			boost::asio::async_write(m_Socket, boost::asio::buffer(m_WriteMessage), // 클라이언트 소켓(m_Socket)에 데이터 전송
				boost::bind(&Session::handle_write, this, // 끝나고 호출할 handle_write를 인자와 함께 바인딩하여 랩핑
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

			// 에코 후 다시 데이터 수신 함수 호출(반복)
			PostReceive();
		}
	}
};

class TCP_Server // TCP 서버
{
public:
	int m_nSeqNumber;
	boost::asio::ip::tcp::acceptor m_acceptor; // 클라이언트의 접속을 담당하는 객체
	boost::asio::io_service& m_io_service; // I/O 이벤트를 OS에서 디스패치하는 객체
	Session* m_pSession;

	TCP_Server(boost::asio::io_service& io_service) : m_io_service(io_service), m_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), PORT_NUMBER))
	{
		m_pSession = nullptr;
		StartAccept(); // 클라이언트 접속 받기
	}

	~TCP_Server()
	{
		if (m_pSession != nullptr)
		{
			delete m_pSession;
		}
	}

private:
	void StartAccept() // 클라이언트 접속 받기 (1회 한정)
	{
		cout << "클라이언트 접속 대기..." << endl;
		m_pSession = new Session(m_io_service);
		m_acceptor.async_accept(m_pSession->Socket(), boost::bind(&TCP_Server::handle_accept, this, m_pSession, boost::asio::placeholders::error)); // 클라이언트의 접속을 비동기로 처리
		/*
			첫 번째 인자는 클라이언트에 할당할 소켓 클래스를 뜻하며,
			두 번째 인자는 bind 함수를 통하여 이벤트핸들러처럼 async_accept가 완료되면 호출될 함수(handle_accept)를 랩핑하였다.

			async_accept 함수가 호출되면 접속이 끝날 때까지 대기하지 않고 바로 다음 줄로 넘어간다.(async, 비동기 함수이기 때문)
			async_accept 작업이 끝나면 두 번째 인자인 bind(handle_accept,...) 함수가 호출된다.

			bind 함수는 인수를 포팅할 때 사용되는데 handle_accept 함수를 호출할 땐 인수도 같이 넣어줘야하기 때문에 
			bind 함수로 인수와 호출할 함수를 같이 묶어서 넘겨준다.
		*/
	}

	void handle_accept(Session* pSession, const boost::system::error_code& error)  // async_accept 완료 시 핸들링 됨 (1회 한정)
	{
		if (!error) // 성공 시
		{
			cout << "클라이언트 접속 성공" << endl;
			pSession->PostReceive(); // 클라이언트 접속 성공 시 데이터 수신 함수 호출 (해당 함수는 자가 반복함)
		}
	}

};

int main()
{
	
	/*  
												=== 서버의 동작 구조 ===
		접속 받기 요청(async_accept) -> 데이터 받기 요청(async_read_some) -> 데이터 쓰기(async_write)
		동작을 비동기 함수로 실행하여 데이터를 기다리는 작업 없이 바로 다음 로직을 실행할 수 있다. (받는 대로 데이터를 바로 전송(에코)할 수 있다.)
	*/

	boost::asio::io_service io_service;
	TCP_Server server(io_service); // 생성자 호출 시 바로 StartAccept() 호출
	io_service.run(); // main 종료 방지
	/*
		스레드 프로그래밍처럼 run() 함수를 호출하면 main() 함수가 무한 대기 상태가 되고 
		Boost.Asio의 비동기 함수 작업이 모두 끝날 때까지 무한히 대기하다가 
		작업이 모두 끝나면 run() 함수를 빠져나와서 다음 작업을 진행한다.
	*/
	cout << "네트워크 접속 종료" << endl;
}