
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT   8080

int main()
{
	// 创建套接字
	int sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_fd < 0)
	{
		std::cerr << "[ERROR] Socket cannot be created!\n";
		return -2;
	}
	std::cout << "[INFO] Socket has been created.\n";

	// 设置套接字地址
	sockaddr_in server_addr{};
	std::memset((char*)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	std::memcpy((char*)SERVER_IP, (char*)&server_addr.sin_addr, strlen(SERVER_IP));

	// 连接服务器
	if (connect(sock_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		std::cerr << "Connection cannot be established!\n";
		return -6;
	}
	std::cout << "[INFO] Connection established.\n";

	static const char MESSAGE[] = "Hello Server!";

	char buf[4096]{ "test!" };
	std::string temp;
	for (;;)
	{
		memcpy(buf, MESSAGE, sizeof(MESSAGE));
		// 发送数据
		ssize_t bytes_send = send(sock_fd, &buf, (size_t)strlen(buf), 0);
		if (bytes_send < 0)
		{
			std::cerr << "[ERROR] Message cannot be sent!\n";
			break;
		}

		// 接收数据
		ssize_t bytes_recv = recv(sock_fd, &buf, 4096, 0);
		if (bytes_recv < 0)
		{
			std::cerr << "[ERROR] Message cannot be recieved!\n";
		}
		else if (bytes_recv == 0)
		{
			std::cout << "[INFO] Server is disconnected.\n";
		}
		else
		{
			std::cout << "server> " << std::string(buf, 0, bytes_recv) << "\n";
		}
	}

	close(sock_fd);
	std::cout << "[INFO] Socket is closed.\n";

	return 0;
}
