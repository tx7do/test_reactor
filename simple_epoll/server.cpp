
#include "stdinc.h"
#include "server.h"
#include "event.h"

bool set_non_block(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) {
		printf("fcntl get failed");
		return false;
	}
	int r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (r < 0) {
		printf("fcntl set failed");
		return false;
	}
	return true;
}

void handle_accept(int fd, uint32_t events, void* arg)
{
	struct sockaddr_in cin{};
	socklen_t len = sizeof(cin);

	int cfd, i;
	if ((cfd = accept(fd, (struct sockaddr*)&cin, &len)) == -1)
	{
		if (errno != EAGAIN && errno != EINTR)
		{
			/* 暂时不做出错处理 */
		}
		printf("%s: accept, %s\n", __func__, strerror(errno));
		return;
	}

	do
	{
		for (i = 0; i < MAX_EVENTS; i++)
		{
			if (g_events[i].status == 0)
				break;
		}

		if (i == MAX_EVENTS)
		{
			printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
			break;
		}

		if (!set_non_block(cfd)) {
			printf("%s: set nonblock failed\n", __func__);
			break;
		}

		set_event(&g_events[i], cfd, handle_recv, &g_events[i]);
		event_add(EPOLLIN, &g_events[i]);
	} while (false);

	printf("new connect [%s:%d][time:%ld], pos[%d]\n",
		inet_ntoa(cin.sin_addr),
		ntohs(cin.sin_port),
		g_events[i].last_active,
		i);
}

void handle_recv(int fd, uint32_t events, void* arg)
{
	auto* ev = (struct event_s*)arg;

	ssize_t len = recv(fd, ev->buf, sizeof(ev->buf), 0);
	event_del(ev);

	if (len > 0)
	{
		ev->len = len;
		ev->buf[len] = '\0';
		printf("C[%d]:%s\n", fd, ev->buf);
		/* 转换为发送事件 */
		set_event(ev, fd, handle_send, ev);
		event_add(EPOLLOUT, ev);
	}
	else if (len == 0)
	{
		close(ev->fd);
		printf("[fd=%d] pos[%d], closed\n", fd, (int)(ev - g_events));
	}
	else
	{
		close(ev->fd);
		printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
	}
}

void handle_send(int fd, uint32_t events, void* arg)
{
	auto* ev = (struct event_s*)arg;
	ssize_t len;

	len = send(fd, ev->buf, ev->len, 0);

	event_del(ev);
	if (len > 0)
	{
		printf("send[fd=%d], [%d]%s\n", fd, len, ev->buf);
		set_event(ev, fd, handle_recv, ev);
		event_add(EPOLLIN, ev);
	}
	else
	{
		close(ev->fd);
		printf("send[fd=%d] error %s\n", fd, strerror(errno));
	}
}

struct sockaddr_in make_addr(short port)
{
	struct sockaddr_in sin{};
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	return sin;
}

// 监听
int init_listen_socket(short port)
{
	// 创建监听套接字
	int listenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 设置套接字
	set_non_block(listenFd);// 设置为非阻塞

	auto sin = make_addr(port);

	// 绑定端口
	int ret = bind(listenFd, (struct sockaddr*)&sin, sizeof(sin));
	if (ret < 0)
	{
		printf("bind failed, %s\n", strerror(errno));
		return -1;
	}

	// 开始监听
	listen(listenFd, 20);

	return listenFd;
}

void run()
{
	printf("server running: port[%d]\n", g_port);

	for (;;)
	{
		if (!dispatch_events())
		{
			break;
		}
	}
}

int start_server()
{
	// 初始化监听套接字
	int listenFd = init_listen_socket(g_port);
	if (listenFd < 0)
	{
		return -1;
	}

	// 初始化epoll
	if (!init_epoll())
	{
		return -1;
	}

	// 初始化epoll事件
	set_event(&g_events[MAX_EVENTS], listenFd, handle_accept, &g_events[MAX_EVENTS]);
	event_add(EPOLLIN, &g_events[MAX_EVENTS]);

	// 启动服务
	run();

	return 0;
}
