#include "stdinc.h"
#include "event.h"

static int g_efd;          /* epoll_create返回的句柄 */

// 初始化epoll
bool init_epoll()
{
	g_efd = epoll_create(1);
	if (g_efd < 0)
	{
		printf("epoll_create error, exit\n");
		return false;
	}
	return true;
}

//
bool set_epoll(int efd, int fd, void* data, uint32_t events, int op)
{
	struct epoll_event epv = { events, { nullptr }};
	if (data == nullptr)
	{
		epv.data.fd = fd;
	}
	else
	{
		epv.data.ptr = data;
	}
	int rc = epoll_ctl(efd, op, fd, &epv);
	if (rc < 0)
	{
		printf("epoll_ctl error, exit\n");
		return false;
	}
	return true;
}

void event_add(int events, struct event_s* ev)
{
	int op;
	if (ev->status == 1)
	{
		op = EPOLL_CTL_MOD;
	}
	else
	{
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}
	ev->events = events;

	set_epoll(g_efd, ev->fd, ev, events, op);
}

void event_del(struct event_s* ev)
{
	if (ev->status != 1)
	{
		return;
	}
	ev->status = 0;

	set_epoll(g_efd, ev->fd, ev, 0, EPOLL_CTL_DEL);
}

// 派发事件
bool dispatch_events()
{
	static struct epoll_event events[MAX_EVENTS + 1];

	// 接收事件
	int nfd = epoll_wait(g_efd, events, MAX_EVENTS + 1, 1000);
	if (nfd < 0)
	{
		printf("epoll_wait error, exit\n");
		return false;
	}

	// 处理事件
	for (int i = 0; i < nfd; i++)
	{
		auto* ev = (struct event_s*)events[i].data.ptr;
		if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
		{
			ev->call_back(ev->fd, events[i].events, ev->arg);
		}
		if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
		{
			ev->call_back(ev->fd, events[i].events, ev->arg);
		}
	}

	return true;
}
