
#include "stdinc.h"

static const char MESSAGE[] = "Hello Client!";

static const unsigned short SERVER_PORT = 8080;

static void
handle_accept(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_arg)
{
	auto base = (struct event_base*)user_arg;

	// 创建新事件
	auto bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev)
	{
		fprintf(stderr, "Error constructing bufferevent!");
		event_base_loopbreak(base);
		return;
	}

	// 设置回调
	bufferevent_setcb(bev, handle_recv, nullptr, handle_event, nullptr);
	bufferevent_enable(bev, EV_READ | EV_WRITE);

	bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}

static void
handle_send(struct bufferevent* bev, void* ctx)
{
	auto output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0)
	{
		printf("flushed answer\n");
		bufferevent_free(bev);
	}
}

static void
handle_recv(struct bufferevent* bev, void* ctx)
{
	char buf[1024] = {0};
	bufferevent_read(bev, buf, sizeof(buf));

	printf("client> %s\n", buf);

	bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}

static void
handle_event(struct bufferevent* bev, short events, void* ctx)
{
	if (events & BEV_EVENT_EOF)
	{
		printf("Connection closed.\n");
	}
	else if (events & BEV_EVENT_ERROR)
	{
		printf("Got an error on the connection: %s\n",
			strerror(errno));
	}

	bufferevent_free(bev);
}

static void
handle_signal(evutil_socket_t sig, short events, void* user_data)
{
	auto base = (struct event_base*)user_data;
	struct timeval delay = { 2, 0 };

	printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

	event_base_loopexit(base, &delay);
}

struct sockaddr_in make_addr(short port)
{
	struct sockaddr_in sin{};
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	return sin;
}

int main()
{
	// 创建一个实例
	auto base = event_base_new();
	if (!base)
	{
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}

	// 创建一个监听器
	auto sin = make_addr(SERVER_PORT);
	auto listener = evconnlistener_new_bind(base, handle_accept, (void*)base,
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
		(struct sockaddr*)&sin,
		sizeof(sin));
	if (!listener)
	{
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}

	// 创建一个信号处理器
	auto signal_event = evsignal_new(base, SIGINT, handle_signal, (void*)base);
	if (!signal_event || event_add(signal_event, nullptr) < 0)
	{
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}

	printf("server running: port[%d]\n", SERVER_PORT);

	// 开始事件循环
	event_base_dispatch(base);

	// 清理资源
	evconnlistener_free(listener);
	event_free(signal_event);
	event_base_free(base);

	printf("done\n");
	return 0;
}
