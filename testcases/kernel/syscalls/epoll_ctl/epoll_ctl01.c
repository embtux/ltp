/*
 * Copyright (c) 2016 Fujitsu Ltd.
 * Author: Xiao Yang <yangx.jy@cn.fujitsu.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU General Public License
 * alone with this program.
 */

/*
 * Test Name: epoll_ctl01.c
 *
 * Description:
 *    Testcase to check the basic functionality of the epoll_ctl(2).
 * 1) when epoll_ctl(2) succeeds to register fd on the epoll instance and
 *    associates event with fd, epoll_wait(2) will get registered fd and
 *    event correctly.
 * 2) when epoll_ctl(2) succeeds to chage event which is related to fd,
 *    epoll_wait(2) will get chaged event correctly.
 * 3) when epoll_ctl(2) succeeds to deregister fd from the epoll instance
 *    epoll_wait(2) won't get deregistered fd and event.
 *
 */

#include <sys/epoll.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include "tst_test.h"

static int epfd;
static int fd[2];

static struct epoll_event events[3] = {
	{.events = EPOLLIN},
	{.events = EPOLLOUT},
	{.events = EPOLLIN}
};

static void setup(void)
{
	epfd = epoll_create(2);
	if (epfd == -1)
		tst_brk(TBROK | TERRNO, "fail to create epoll instance");

	SAFE_PIPE(fd);

	events[0].data.fd = fd[0];
	events[1].data.fd = fd[1];
	events[2].data.fd = fd[1];
}

static void cleanup(void)
{
	if (epfd > 0)
		SAFE_CLOSE(epfd);

	if (fd[0] > 0)
		SAFE_CLOSE(fd[0]);

	if (fd[1] > 0)
		SAFE_CLOSE(fd[1]);
}

static int has_event(struct epoll_event *epvs, int len,
	int fd, unsigned int events)
{
	int i;

	for (i = 0; i < len; i++) {
		if ((epvs[i].data.fd == fd) && (epvs[i].events == events))
			return 1;
	}

	return 0;
}

static void check_epoll_ctl(int opt, int exp_num)
{
	int res;

	char write_buf[] = "test";
	char read_buf[sizeof(write_buf)];

	struct epoll_event res_evs[2] = {
		{.events = 0, .data.fd = 0},
		{.events = 0, .data.fd = 0}
	};

	SAFE_WRITE(1, fd[1], write_buf, sizeof(write_buf));

	res = epoll_wait(epfd, res_evs, 2, -1);
	if (res == -1)
		tst_brk(TBROK | TERRNO, "epoll_wait() fails");

	if (res != exp_num) {
		tst_res(TFAIL, "epoll_wait() returns %i, expected %i",
			res, exp_num);
		goto end;
	}

	if (exp_num == 1) {
		if (!has_event(res_evs, 2, fd[0], EPOLLIN) ||
		    !has_event(res_evs, 2, 0, 0)) {
			tst_res(TFAIL, "epoll_wait() fails to "
				"get expected fd and event");
			goto end;
		}
	}

	if (exp_num == 2) {
		if (!has_event(res_evs, 2, fd[0], EPOLLIN) ||
		    !has_event(res_evs, 2, fd[1], EPOLLOUT)) {
			tst_res(TFAIL, "epoll_wait() fails to "
				"get expected fd and event");
			goto end;
		}
	}

	tst_res(TPASS, "epoll_ctl() succeeds with op %i", opt);

end:
	SAFE_READ(1, fd[0], read_buf, sizeof(write_buf));
}

static void opera_epoll_ctl(int opt, int fd, struct epoll_event *epvs)
{
	TEST(epoll_ctl(epfd, opt, fd, epvs));
	if (TEST_RETURN == -1)
		tst_brk(TBROK | TTERRNO, "epoll_ctl() fails with op %i", opt);
}

static void verify_epoll_ctl(void)
{
	opera_epoll_ctl(EPOLL_CTL_ADD, fd[0], &events[0]);
	opera_epoll_ctl(EPOLL_CTL_ADD, fd[1], &events[2]);
	check_epoll_ctl(EPOLL_CTL_ADD, 1);
	opera_epoll_ctl(EPOLL_CTL_MOD, fd[1], &events[1]);
	check_epoll_ctl(EPOLL_CTL_MOD, 2);
	opera_epoll_ctl(EPOLL_CTL_DEL, fd[1], &events[1]);
	check_epoll_ctl(EPOLL_CTL_DEL, 1);
	opera_epoll_ctl(EPOLL_CTL_DEL, fd[0], &events[0]);
}

static struct tst_test test = {
	.tid = "epoll_ctl01",
	.setup = setup,
	.cleanup = cleanup,
	.test_all = verify_epoll_ctl,
};
