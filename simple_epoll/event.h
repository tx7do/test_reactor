#pragma once

#include <cstdint>

#define MAX_EVENTS  1024
#define BUFF_LEN 128

typedef void (* CALL_BACK)(int fd, uint32_t events, void* arg);

struct event_s
{
	int fd;                 // listenfd
	int events;             // EPOLLIN  EPLLOUT
	void* arg;              // 指向自己结构体指针
	CALL_BACK call_back;    // 回调函数
	int status;// 1表示在监听事件中，0表示不在
	char buf[BUFF_LEN];
	int len;
	long last_active;// 记录最后一次响应时间,做超时处理
};

static void set_event(struct event_s* ev, int fd, CALL_BACK call_back, void* arg)
{
	ev->fd = fd;
	ev->call_back = call_back;
	ev->events = 0;
	ev->arg = arg;
	ev->status = 0;
	ev->last_active = time(nullptr);
}

static struct event_s g_events[MAX_EVENTS + 1];// 最后一个用于监听事件

// 初始化epoll
bool init_epoll();

void event_add(int events, struct event_s* ev);
void event_del(struct event_s* ev);

// 派发事件
bool dispatch_events();
