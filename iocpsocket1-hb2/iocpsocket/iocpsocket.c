
#include "stdafx.h"
#include "iocpsocket.h"
#include "iocpworkthread.h"

HANDLE iocpsocket_hiocp=NULL;
HANDLE iocpsocket_hflagsmutex;
HANDLE iocpsocket_hrecvmutex;
HANDLE iocpsocket_hsendmutex;
HANDLE iocpsocket_hrecvsem;
LIST_ENTRY iocpsocket_recvqueuelisthead;
HANDLE iocpsocket_heartbeatmutex;
LIST_ENTRY iocpsocket_heartbeatlisthead;

int iocpsocketsend(UINT tag,void *buf,UINT bufsize,PVOID psocket)
{
	POVERLAPPEDPLUS ppol;
	POVERLAPPEDPLUS pol;
	void *sendbuf,*sendbuf0;
	do 
	{
		if (tag==0 || bufsize==0 || psocket==NULL)break;
		ppol=(POVERLAPPEDPLUS)psocket;
		if (ppol->flags & IOCPFLAGS_SOCKERROR)break;
		if ((sendbuf=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,bufsize+12))==NULL)break;
		if((sendbuf0=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,bufsize+12))==NULL){GlobalFree(sendbuf);break;}
		*((UINT*)sendbuf)=tag;
		*((UINT*)sendbuf+1)=bufsize;
		*((UINT*)sendbuf+2)=crc32(0,sendbuf,8);
		RtlMoveMemory((void*)((UINT*)sendbuf+3),buf,bufsize);
		RtlMoveMemory(sendbuf0,sendbuf,bufsize+12);
		if((pol=(POVERLAPPEDPLUS)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,sizeof(OVERLAPPEDPLUS)))==NULL){GlobalFree(sendbuf);GlobalFree(sendbuf0);break;}
		pol->hsocket=ppol->hsocket;
		pol->wsaseek=0;
		pol->flags=IOCPFLAGS_SENDDATA;
		pol->wsabuf.len=bufsize+12;
		pol->wsabuf.buf=sendbuf;
		pol->ollen=bufsize+12;
		pol->olbuf=sendbuf0;
		pol->ppol=ppol;
		WaitForSingleObject(iocpsocket_hsendmutex,INFINITE);
		if (ppol->flags & IOCPFLAGS_SOCKERROR)
		{
			GlobalFree(sendbuf0);
			GlobalFree(sendbuf);
			GlobalFree(pol);
			ReleaseMutex(iocpsocket_hsendmutex);
			return -1;
		}
		else
		{
			InsertTailList(&ppol->sendlisthead,&pol->sendlisthead);
			if (!(ppol->sendstatus & IOCPFLAGS_SENDSTATUS))
			{
				ppol->sendstatus|=IOCPFLAGS_SENDSTATUS;
				PostQueuedCompletionStatus(iocpsocket_hiocp,0,IOCPSENDKEY,(LPOVERLAPPED)pol);
			}
			ReleaseMutex(iocpsocket_hsendmutex);
			return 0;
		}
	} while (FALSE);
	return -1;
}

int iocpsocketclose(PVOID psocket)
{
	do 
	{
		POVERLAPPEDPLUS pol;
		if (psocket==NULL)break;
		pol=(POVERLAPPEDPLUS)psocket;
		WaitForSingleObject(iocpsocket_hflagsmutex,INFINITE);
		pol->flags|=IOCPFLAGS_SOCKERROR;
		ReleaseMutex(iocpsocket_hflagsmutex);
		return 0;
	} while (FALSE);
	return -1;
}

PVOID iocpsocketconnect(void *addrstr,unsigned short port,unsigned int extsize,PVOID *ppsocket,
					  void (*recvcall)(unsigned int tag,void *buf,unsigned int bufsize,void *extptr,void*ipstr,PVOID psocket),
					  void (*disconncall)(void*extptr,void*ipstr))
{
	int r_eax,NumberOfBytesRecvd,Flags;
	struct sockaddr_in sa;
	SOCKET hsocket;
	struct hostent *hs;
	char **addrlist;
	POVERLAPPEDPLUS pol;
	do 
	{
		if (extsize==0){break;}
		if((hs=gethostbyname(addrstr))==NULL)break;
		addrlist=hs->h_addr_list;
		RtlZeroMemory(&sa,sizeof(struct sockaddr_in));
		sa.sin_family=AF_INET;
		sa.sin_port=htons(port);
		sa.sin_addr.s_addr=*(ULONG*)*addrlist;
		if((hsocket=socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)break;
		if((r_eax=connect(hsocket,(struct sockaddr*)&sa,sizeof(struct sockaddr_in)))==SOCKET_ERROR){closesocket(hsocket);break;}
		pol=(POVERLAPPEDPLUS)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,sizeof(OVERLAPPEDPLUS));
		pol->hsocket=hsocket;
		pol->wsaseek=0;
		pol->heartbeatcount=2;
		pol->flags=IOCPFLAGS_RECVHEAD;
		pol->wsabuf.len=12;
		pol->wsabuf.buf=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,12);
		pol->ollen=12;
		pol->olbuf=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,12);
		pol->ipstr=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,32);
		pol->extptr=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,extsize);
		InitializeListHead(&pol->sendlisthead);
		pol->recvcall=recvcall;
		pol->disconncall=disconncall;
		CreateIoCompletionPort((HANDLE)hsocket,iocpsocket_hiocp,0,0);
		WaitForSingleObject(iocpsocket_heartbeatmutex,INFINITE);
		InsertTailList(&iocpsocket_heartbeatlisthead,&pol->heartbeatlist);
		ReleaseMutex(iocpsocket_heartbeatmutex);
		NumberOfBytesRecvd=0;
		Flags=0;
		if ((r_eax=WSARecv(hsocket,&pol->wsabuf,1,&NumberOfBytesRecvd,&Flags,(LPWSAOVERLAPPED)pol,NULL))==SOCKET_ERROR)
		{
			if ((r_eax=WSAGetLastError())!=WSA_IO_PENDING)
			{
				pol->flags|=IOCPFLAGS_SOCKERROR;
				PostQueuedCompletionStatus(iocpsocket_hiocp,0,0,(LPOVERLAPPED)pol);
			}
		}
		*ppsocket=pol;
		return pol->extptr;
	} while (FALSE);
	*ppsocket=NULL;
	return NULL;
}

int iocpsocketlisten(unsigned short port,unsigned int extsize,
					 void (*recvcall)(unsigned int tag,void *buf,unsigned int bufsize,void *extptr,void*ipstr,PVOID psocket),
					 void (*disconncall)(void*extptr,void*ipstr))
{
	int r_eax,clilen,NumberOfBytesRecvd,Flags;
	SOCKET hsersocket,hclisocket;
	struct sockaddr_in saser,sacli;
	POVERLAPPEDPLUS pol;
	void *pipstr;
	do 
	{
		if (extsize==0)break;
		RtlZeroMemory(&saser,sizeof(struct sockaddr_in));
		saser.sin_family=AF_INET;
		saser.sin_addr.s_addr=htonl(INADDR_ANY);
		saser.sin_port=htons(port);
		if ((hsersocket=WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED))==INVALID_SOCKET)break;
		if ((r_eax=bind(hsersocket,(struct sockaddr*)&saser,sizeof(struct sockaddr_in)))==SOCKET_ERROR){closesocket(hsersocket);break;}
		if ((r_eax=listen(hsersocket,777))==SOCKET_ERROR){closesocket(hsersocket);break;}
		while (TRUE)
		{
			RtlZeroMemory(&sacli,sizeof(struct sockaddr_in));
			clilen=sizeof(struct sockaddr_in);
			if ((hclisocket=accept(hsersocket,(struct sockaddr*)&sacli,&clilen))==INVALID_SOCKET){closesocket(hsersocket);return -1;}
			pipstr=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,32);
			wsprintf(pipstr,"%s:",inet_ntoa(sacli.sin_addr));
			wsprintf((void*)((UINT)pipstr+lstrlen(pipstr)),"%d",ntohs(sacli.sin_port));
			pol=(POVERLAPPEDPLUS)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,sizeof(OVERLAPPEDPLUS));
			pol->hsocket=hclisocket;
			pol->wsaseek=0;
			pol->heartbeatcount=2;
			pol->flags=IOCPFLAGS_RECVHEAD;
			pol->wsabuf.len=12;
			pol->wsabuf.buf=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,12);
			pol->ollen=12;
			pol->olbuf=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,12);
			pol->ipstr=pipstr;
			pol->extptr=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,extsize);
			InitializeListHead(&pol->sendlisthead);
			pol->recvcall=recvcall;
			pol->disconncall=disconncall;
			CreateIoCompletionPort((HANDLE)hclisocket,iocpsocket_hiocp,0,0);
			WaitForSingleObject(iocpsocket_heartbeatmutex,INFINITE);
			InsertTailList(&iocpsocket_heartbeatlisthead,&pol->heartbeatlist);
			ReleaseMutex(iocpsocket_heartbeatmutex);
			NumberOfBytesRecvd=0;
			Flags=0;
			if ((r_eax=WSARecv(hclisocket,&pol->wsabuf,1,&NumberOfBytesRecvd,&Flags,(LPWSAOVERLAPPED)pol,NULL))==SOCKET_ERROR)
			{
				if ((r_eax=WSAGetLastError())!=WSA_IO_PENDING)
				{
					pol->flags|=IOCPFLAGS_SOCKERROR;
					PostQueuedCompletionStatus(iocpsocket_hiocp,0,0,(LPOVERLAPPED)pol);
				}
			}
		}
	} while (FALSE);
	return -1;
}

int iocpsocketinit()
{
	WSADATA wsadata;
	SYSTEM_INFO sysinfo;
	DWORD r_eax=0;
	char buf[32];
	do 
	{
		if (iocpsocket_hiocp!=NULL)break;
		WSAStartup(MAKEWORD(2,2),&wsadata);
		if ((iocpsocket_hiocp=CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0))==NULL)break;
		if ((iocpsocket_hflagsmutex=CreateMutex(NULL,FALSE,NULL))==NULL)break;
		if ((iocpsocket_hrecvmutex=CreateMutex(NULL,FALSE,NULL))==NULL)break;
		if ((iocpsocket_hsendmutex=CreateMutex(NULL,FALSE,NULL))==NULL)break;
		if ((iocpsocket_heartbeatmutex=CreateMutex(NULL,FALSE,NULL))==NULL)break;
		if ((iocpsocket_hrecvsem=CreateSemaphore(NULL,0,65535,NULL))==NULL)break;
		crc32(0,buf,8);
		InitializeListHead(&iocpsocket_recvqueuelisthead);
		InitializeListHead(&iocpsocket_heartbeatlisthead);
		CreateThread(NULL,0,iocprecvqueuethread,NULL,0,NULL);
		CreateThread(NULL,0,iocpheartbeatthread,NULL,0,NULL);
		GetSystemInfo(&sysinfo);
		while (r_eax<sysinfo.dwNumberOfProcessors)
		{
			r_eax++;
			CreateThread(NULL,0,iocpworkthread,NULL,0,NULL);
		}
		return 0;
	} while (FALSE);
	return -1;
}