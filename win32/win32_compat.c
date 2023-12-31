/*
Copyright (c) 2006 by Dan Kennedy.
Copyright (c) 2006 by Juliusz Chroboczek.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "config.h"
#include "win32_compat.h"
#include <errno.h>
#include <stdio.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#undef poll
#undef socket
#undef connect
#undef accept
#undef shutdown
#undef getpeername
#undef sleep
#undef inet_aton
#undef gettimeofday
#undef stat
#define assert(a)

/* Windows needs this header file for the implementation of inet_aton() */
#include <ctype.h>

int win32_inet_pton(int af, const char *src, void *dst)
{
	struct sockaddr_in sa;
	int len = sizeof(SOCKADDR);
	int ret = -1;
#ifdef UNICODE
	int strLen = strlen(src) + 1;
	wchar_t *srcNonConst = (wchar_t *)malloc(strLen * sizeof(wchar_t));
	memset(srcNonConst, 0, strLen);
	MultiByteToWideChar(CP_ACP, 0, src, -1, srcNonConst, strLen);
#else
	char *srcNonConst = strdup(src);
#endif

	if (WSAStringToAddress(srcNonConst, af, NULL, (LPSOCKADDR)&sa, &len) ==
	    0) {
		ret = 1;
	} else {
		if (WSAGetLastError() == WSAEINVAL) {
			ret = -1;
		}
	}
	free(srcNonConst);
	memcpy(dst, &sa.sin_addr, sizeof(struct in_addr));
	return ret;
}

int win32_poll(struct pollfd *fds, unsigned int nfds, int timo)
{
	SELECT_TIMEVAL_TYPE timeout, *toptr;
	fd_set ifds, ofds, efds, *ip, *op;
	unsigned int i;
	int rc;

	// Set up the file-descriptor sets in ifds, ofds and efds.
	FD_ZERO(&ifds);
	FD_ZERO(&ofds);
	FD_ZERO(&efds);
	for (i = 0, op = ip = 0; i < nfds; ++i) {
		fds[i].revents = 0;
		if (fds[i].events & (POLLIN | POLLPRI)) {
			ip = &ifds;
			FD_SET(fds[i].fd, ip);
		}
		if (fds[i].events & POLLOUT) {
			op = &ofds;
			FD_SET(fds[i].fd, op);
		}
		FD_SET(fds[i].fd, &efds);
	}

	// Set up the timeval structure for the timeout parameter
	if (timo < 0) {
		toptr = 0;
	} else {
		toptr = &timeout;
		timeout.tv_sec = timo / 1000;
		timeout.tv_usec = (timo - timeout.tv_sec * 1000) * 1000;
	}

#ifdef DEBUG_POLL
	printf("Entering select() sec=%ld usec=%ld ip=%lx op=%lx\n",
	       (long)timeout.tv_sec, (long)timeout.tv_usec, (long)ip, (long)op);
#endif
	rc = select(0, ip, op, &efds, toptr);
#ifdef DEBUG_POLL
	printf("Exiting select rc=%d\n", rc);
#endif

	if (rc <= 0)
		return rc;

	if (rc > 0) {
		for (i = 0; i < nfds; ++i) {
			int fd = fds[i].fd;
			if (fds[i].events & (POLLIN | POLLPRI) &&
			    FD_ISSET(fd, &ifds))
				fds[i].revents |= POLLIN;
			if (fds[i].events & POLLOUT && FD_ISSET(fd, &ofds))
				fds[i].revents |= POLLOUT;
			if (FD_ISSET(fd,
				     &efds)) // Some error was detected ... should be some way to know.
				fds[i].revents |= POLLHUP;
#ifdef DEBUG_POLL
			printf("%d %d %d revent = %x\n", FD_ISSET(fd, &ifds),
			       FD_ISSET(fd, &ofds), FD_ISSET(fd, &efds),
			       fds[i].revents);
#endif
		}
	}
	return rc;
}

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL
#endif

#ifndef HAVE_TIMEZONE_TZ_DSTTIME
struct timezone {
	int tz_minuteswest; /* minutes W of Greenwich */
	int tz_dsttime; /* type of dst correction */
};
#endif

int win32_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL != tv) {
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tmpres /= 10; /*convert into microseconds*/
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz) {
		if (!tzflag) {
#ifdef HAVE_TZSET
                        tzset();
#else
			_tzset();
#endif
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}

ssize_t win32_readv(int fd, const struct iovec *iov, int iovcnt)
{
	return recv(fd, iov[0].iov_base, iov[0].iov_len, 0);
}

ssize_t win32_writev(int fd, const struct iovec *iov, int iovcnt)
{
	return send(fd, iov[0].iov_base, iov[0].iov_len, 0);
}

/* Something is broken with how I use dup2 for windows and I do not know
 * win32 API well enought to figure out why, thus our dup2 for win32 will be
 * a NO-OP.
 * The only consequence of this is that the filedescriptor for libiscsi may
 * now suddenly change under win32. As long as the application just calls
 * iscsi_get_fd() before poll or its equivalent every time ot goes through
 * its event loop, everything should work.
 * IF the app tries to be clever/optimized and only calls iscsi_get_fd()
 * one single time and re-uses that file descriptor number throughout
 * any future iterations in the event loop, then things will break :-(
 */
int win32_dup2(int oldfd, int newfd)
{
	return 0;
}
