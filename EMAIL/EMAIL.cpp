#define _XOPEN_SOURCE
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <iomanip>
#include <string>
#include <filesystem> 
#include <sstream>
#include <thread>
#include <ctime>
#include <fstream>
#pragma comment(lib, "Ws2_32.lib")


class Client
{
public:

	Client(const std::string& ip, const int& port)
	{
		WSADATA wsData;
		if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0)
		{
			std::cerr << "Failed to initialize Winsock\n";
			return;
		}

		socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (socket_ == INVALID_SOCKET)
		{
			std::cerr << "Failed to create socket, error: " << WSAGetLastError() << std::endl;
			WSACleanup();
			return;
		}
		if (inet_pton(AF_INET, ip.c_str(), &servData_.sin_addr) <= 0)
		{
			std::cerr << "Invalid address, error: " << WSAGetLastError() << std::endl;
			WSACleanup();
			return;
		}

		servData_.sin_family = AF_INET;
		servData_.sin_port = htons(port);
	}

	~Client()
	{
		closesocket(socket_);
		WSACleanup();
	}


	// Метод для установки соединения с сервером
	bool connectToServer()
	{
		if (connect(socket_, reinterpret_cast<sockaddr*>(&servData_), sizeof(servData_)) == SOCKET_ERROR)
		{
			std::cerr << "Failed to connect to server, error: " << WSAGetLastError() << std::endl;
			return false;
		}
		return true;
	}


	bool sendString(std::string msg) const
	{
		int bytes{};
		std::string m{ msg };
		m += "\r\n";
		bytes = send(socket_, m.data(), m.size(), 0);
		if (bytes == SOCKET_ERROR)
		{
			return false;
		}
		return true;
	}

	std::string recvString()
	{
		int bytes;
		std::vector<char> buffer(1024);

		bytes = recv(socket_, buffer.data(), buffer.size(), 0);
		if (bytes == SOCKET_ERROR)
		{
			std::cerr << "Received from server is failed, error:" << WSAGetLastError() << std::endl;
			return std::string();
		}
		else
		{
			std::string recp{ buffer.begin(), buffer.begin() + bytes };
			return recp;
		}
	}


private:
	SOCKET socket_;
	sockaddr_in servData_;
};

// Функция для преобразования строки с датой в time_t
time_t convertDateToTimeT(const std::string& date) {
	std::tm tm = {};
	std::istringstream ss(date);
	ss >> std::get_time(&tm, "%a, %d %b %Y");
	return mktime(&tm);
}

int main()
{
	std::vector<char> buffer(1024);

	bool quit = false;
	while (!quit)
	{

		std::string msg{};

		int choice{};
		std::cout << "1 - smtp\n2 - pop3\n3 - task\n";
		std::cin >> choice;

		std::cin.ignore();
		switch (choice)
		{
		case 1:
		{
			Client smtp("192.168.0.105", 25);
			smtp.connectToServer();
			std::cout << smtp.recvString() << std::endl;

			bool exit = false;
			while (!exit)
			{
				std::getline(std::cin, msg);
				smtp.sendString(msg);

				std::cout << smtp.recvString() << std::endl;

				if (msg.find("quit") != std::string::npos || msg.find("QUIT") != std::string::npos)
					exit = true;
				else if (msg.find("data") != std::string::npos || msg.find("DATA") != std::string::npos)
				{
					while (true)
					{
						std::getline(std::cin, msg);
						smtp.sendString(msg);
						if (msg == ".")
							break;
					}
					std::cout << smtp.recvString() << std::endl;
				}
			}
			break;
		}
		case 2:
		{
			Client pop3("192.168.0.105", 110);
			pop3.connectToServer();
			std::cout << pop3.recvString() << std::endl;

			bool exit = false;
			while (!exit)
			{
				std::getline(std::cin, msg);
				pop3.sendString(msg);

				std::cout << pop3.recvString() << std::endl;

				if (msg.find("quit") != std::string::npos || msg.find("QUIT") != std::string::npos)
					exit = true;
				if (msg.find("retr") != std::string::npos || msg.find("RETR") != std::string::npos)
					std::cout << pop3.recvString() << std::endl;
			}
			break;
		}
		case 3:
		{
			Client pop3("192.168.0.105", 110);
			std::string target_date = "Wed, 18 May 2024"; // Замените на дату, которую вы хотите проверить
			std::string mess;
			pop3.connectToServer();
			std::cout << pop3.recvString() << std::endl;
			mess = "user client@morozov.ru";
			pop3.sendString(mess);
			std::cout << pop3.recvString() << std::endl;
			mess = "pass 1234";
			pop3.sendString(mess);
			std::cout << pop3.recvString() << std::endl;
			mess = "list";
			pop3.sendString(mess);
			std::cout << pop3.recvString() << std::endl;
			pop3.sendString(mess);
			mess = pop3.recvString();
			std::string count = "";
			size_t first_space_pos = mess.find(' ');
			if (first_space_pos != std::string::npos)
			{
				size_t second_space_pos = mess.find(' ', first_space_pos + 1);
				if (second_space_pos != std::string::npos) {
					count = mess.substr(first_space_pos + 1, second_space_pos - first_space_pos - 1);
				}
				else {
					count = mess.substr(first_space_pos + 1);
				}
			}

			int number;

			try
			{
				number = std::stoi(count);
			}
			catch (const std::invalid_argument& e) {
				std::cerr << "Ошибка: невозможно преобразовать строку в число." << std::endl;
			}
			catch (const std::out_of_range& e) {
				std::cerr << "Ошибка: значение вне диапазона типа int." << std::endl;
			}
			for (int i = 1; i <= number; i++)
			{
				mess = "RETR ";
				count = std::to_string(i);
				mess += count;
				pop3.sendString(mess);
				pop3.recvString();
				mess = pop3.recvString();
				size_t date_pos = mess.find(" MailEnable ESMTP; ");
				if (date_pos != std::string::npos) {
					size_t first_space_pos = mess.find(' ', date_pos + 18);
					std::string email_date = mess.substr(first_space_pos + 1, 16);
					time_t email_time = convertDateToTimeT(email_date);
					time_t target_time = convertDateToTimeT(target_date);
					if (email_time <= target_time) // Сравниваем даты
					{
						msg = "DELE ";
						msg += std::to_string(i);
						pop3.sendString(msg);
						std::cout << pop3.recvString() << std::endl;
						std::cout << "message number " << i << " deleted" << std::endl;
					}
				}
			}
			msg = "QUIT";
			pop3.sendString(msg);
			std::cout << pop3.recvString() << std::endl;

		}

		}

	}

	return 0;
}