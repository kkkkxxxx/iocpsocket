
#ifndef COMMON_H
#define	COMMON_H

#pragma comment(lib,"ws2_32.lib")

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <Windows.h>
#include "crc32.h"
#include "listentry.h"

#define IOCPFLAGS_RECVHEAD		0x00000001
#define IOCPFLAGS_RECVDATA		0x00000002
#define IOCPFLAGS_SENDDATA		0x00000004
#define IOCPFLAGS_SENDSTATUS	0x00000008
#define IOCPFLAGS_SOCKERROR		0x00000010
#define IOCPFLAGS_SOCKFREE		0x00000020

#define IOCPSENDKEY	0x77

extern HANDLE iocpsocket_hiocp;
extern HANDLE iocpsocket_hflagsmutex;
extern HANDLE iocpsocket_hrecvmutex;
extern HANDLE iocpsocket_hsendmutex;
extern HANDLE iocpsocket_hrecvsem;
extern LIST_ENTRY iocpsocket_recvqueuelisthead;
extern HANDLE iocpsocket_heartbeatmutex;
extern LIST_ENTRY iocpsocket_heartbeatlisthead;

typedef struct _OVERLAPPEDPLUS
{
	OVERLAPPED	ol;
	WSABUF	wsabuf;
	unsigned int wsaseek;
	SOCKET	hsocket;
	LIST_ENTRY heartbeatlist;
	int heartbeatcount;
	unsigned int flags;
	unsigned int sendstatus;
	unsigned int tag;
	void *olbuf;
	unsigned int ollen;
	void *ipstr;
	void *extptr;
	LIST_ENTRY sendlisthead;
	struct _OVERLAPPEDPLUS *ppol;
	void (*recvcall)(unsigned int tag,void *buf,unsigned int bufsize,void *extptr,void*ipstr,PVOID psocket);
	void (*disconncall)(void*extptr,void*ipstr);
}OVERLAPPEDPLUS,*POVERLAPPEDPLUS;

typedef struct _iocpsocket_recvqueueinfo
{
	LIST_ENTRY recvqueuelist;
	SOCKET hsocket;
	unsigned int tag;
	void *olbuf;
	unsigned int ollen;
	void *ipstr;
	void *extptr;
	void (*recvcall)(unsigned int tag,void *buf,unsigned int bufsize,void *extptr,void*ipstr,PVOID psocket);
	POVERLAPPEDPLUS pol;
}iocpsocket_recvqueueinfo,*piocpsocket_recvqueueinfo;

#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

