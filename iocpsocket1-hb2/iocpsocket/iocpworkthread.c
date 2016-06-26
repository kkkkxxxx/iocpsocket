#include "stdafx.h"

int iocpworkrecvhead(unsigned int NumberOfBytes,POVERLAPPEDPLUS pol)
{
	int r_eax,NumberOfBytesRecvd,Flags;
	UINT tmp,crc;
	tmp=((UINT)pol->olbuf+pol->wsaseek);
	RtlMoveMemory((void*)tmp,pol->wsabuf.buf,NumberOfBytes);
	pol->wsaseek+=NumberOfBytes;
	if (pol->wsaseek==pol->ollen)
	{
		tmp=*((UINT*)pol->olbuf+2);
		crc=crc32(0,pol->olbuf,8);
		if (tmp==crc)
		{
			pol->tag=*((UINT*)pol->olbuf);
			pol->ollen=*((UINT*)pol->olbuf+1);
			pol->wsabuf.len=pol->ollen;
			pol->wsaseek=0;
			pol->wsabuf.buf=GlobalReAlloc(pol->wsabuf.buf,pol->ollen,GMEM_ZEROINIT | GMEM_MOVEABLE);
			pol->olbuf=GlobalReAlloc(pol->olbuf,pol->ollen,GMEM_ZEROINIT | GMEM_MOVEABLE);
			WaitForSingleObject(iocpsocket_hflagsmutex,INFINITE);
			pol->flags &= ~IOCPFLAGS_RECVHEAD;
			pol->flags |= IOCPFLAGS_RECVDATA;
			ReleaseMutex(iocpsocket_hflagsmutex);
			GOTORECVHEAD:
			RtlZeroMemory(&pol->ol,sizeof(OVERLAPPED));
			NumberOfBytesRecvd=0;
			Flags=0;
			if ((r_eax=WSARecv(pol->hsocket,&pol->wsabuf,1,&NumberOfBytesRecvd,&Flags,(LPWSAOVERLAPPED)pol,NULL))==SOCKET_ERROR)
			{
				if ((r_eax=WSAGetLastError())!=WSA_IO_PENDING)
				{
					if (r_eax==WSAENOBUFS)
					{
						if (pol->wsabuf.len>1048576)
						{
							pol->wsabuf.len=1048576;
						}
						goto GOTORECVHEAD;
					}
					else
					{
						pol->flags|=IOCPFLAGS_SOCKERROR;
						return -1;
					}
				}
			}
			return 0;
		}
		else
		{
			printf("crc32error flags=%d,len=%d\n",pol->flags,pol->ollen);
			pol->flags|=IOCPFLAGS_SOCKERROR;
			return -1;
		}
	}
	else
	{
		pol->wsabuf.len=pol->ollen-pol->wsaseek;
		RtlZeroMemory(&pol->ol,sizeof(OVERLAPPED));
		NumberOfBytesRecvd=0;
		Flags=0;
		if ((r_eax=WSARecv(pol->hsocket,&pol->wsabuf,1,&NumberOfBytesRecvd,&Flags,(LPWSAOVERLAPPED)pol,NULL))==SOCKET_ERROR)
		{
			if ((r_eax=WSAGetLastError())!=WSA_IO_PENDING)
			{
				pol->flags|=IOCPFLAGS_SOCKERROR;
				return -1;
			}
		}
		return 0;
	}
}

int iocpworkrecvdata(unsigned int NumberOfBytes,POVERLAPPEDPLUS pol)
{
	int r_eax,NumberOfBytesRecvd,Flags;
	UINT tmp;
	tmp=((UINT)pol->olbuf+pol->wsaseek);
	RtlMoveMemory((void*)tmp,pol->wsabuf.buf,NumberOfBytes);
	pol->wsaseek+=NumberOfBytes;
	if (pol->wsaseek==pol->ollen)
	{
		//////////////////////////////////////////////////////////////////////////
		if(pol->tag==0)
		{
			InterlockedIncrement(&pol->heartbeatcount);
		}
		else
		{
			piocpsocket_recvqueueinfo precvinfo;
			precvinfo=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,sizeof(iocpsocket_recvqueueinfo));
			precvinfo->hsocket=pol->hsocket;
			precvinfo->tag=pol->tag;
			precvinfo->ollen=pol->ollen;
			precvinfo->olbuf=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,pol->ollen);
			RtlMoveMemory(precvinfo->olbuf,pol->olbuf,pol->ollen);
			precvinfo->extptr=pol->extptr;
			precvinfo->ipstr=pol->ipstr;
			precvinfo->recvcall=pol->recvcall;
			precvinfo->pol=pol;
			WaitForSingleObject(iocpsocket_hrecvmutex,INFINITE);
			InsertTailList(&iocpsocket_recvqueuelisthead,&precvinfo->recvqueuelist);
			ReleaseMutex(iocpsocket_hrecvmutex);
			ReleaseSemaphore(iocpsocket_hrecvsem,1,NULL);
		}
		//////////////////////////////////////////////////////////////////
		//pol->recvcall(pol->tag,pol->olbuf,pol->ollen,pol->extptr,pol->ipstr,pol->hsocket);
		//////////////////////////////////////////////////////////////////
		pol->wsaseek=0;
		pol->wsabuf.len=12;
		pol->wsabuf.buf=GlobalReAlloc(pol->wsabuf.buf,12,GMEM_ZEROINIT | GMEM_MOVEABLE);
		pol->ollen=12;
		pol->olbuf=GlobalReAlloc(pol->olbuf,12,GMEM_ZEROINIT | GMEM_MOVEABLE);
		WaitForSingleObject(iocpsocket_hflagsmutex,INFINITE);
		pol->flags &= ~IOCPFLAGS_RECVDATA;
		pol->flags |= IOCPFLAGS_RECVHEAD;
		ReleaseMutex(iocpsocket_hflagsmutex);
		RtlZeroMemory(&pol->ol,sizeof(OVERLAPPED));
		NumberOfBytesRecvd=0;
		Flags=0;
		if ((r_eax=WSARecv(pol->hsocket,&pol->wsabuf,1,&NumberOfBytesRecvd,&Flags,(LPWSAOVERLAPPED)pol,NULL))==SOCKET_ERROR)
		{
			if ((r_eax=WSAGetLastError())!=WSA_IO_PENDING)
			{
				pol->flags|=IOCPFLAGS_SOCKERROR;
				return -1;
			}
		}
		return 0;
	}
	else
	{
		pol->wsabuf.len=pol->ollen-pol->wsaseek;
		GOTORECVDATA:
		RtlZeroMemory(&pol->ol,sizeof(OVERLAPPED));
		NumberOfBytesRecvd=0;
		Flags=0;
		if ((r_eax=WSARecv(pol->hsocket,&pol->wsabuf,1,&NumberOfBytesRecvd,&Flags,(LPWSAOVERLAPPED)pol,NULL))==SOCKET_ERROR)
		{
			if ((r_eax=WSAGetLastError())!=WSA_IO_PENDING)
			{
				if (r_eax==WSAENOBUFS)
				{
					if (pol->wsabuf.len>1048576)
					{
						pol->wsabuf.len=1048576;
					}
					goto GOTORECVDATA;
				}
				else
				{
					pol->flags|=IOCPFLAGS_SOCKERROR;
					return -1;
				}
			}
		}
		return 0;
	}
}

int iocpworksenddata(unsigned int NumberOfBytes,POVERLAPPEDPLUS pol)
{
	int r_eax,NumberOfBytesSent,Flags;
	UINT tmp;
	pol->wsaseek+=NumberOfBytes;
	if (pol->wsaseek==pol->ollen)
	{
		return -1;
	}
	else
	{
		pol->wsabuf.len=pol->ollen-pol->wsaseek;
		tmp=((UINT)pol->olbuf+pol->wsaseek);
		RtlMoveMemory(pol->wsabuf.buf,(void*)tmp,pol->wsabuf.len);
		GOTOSENDDATA:
		RtlZeroMemory(&pol->ol,sizeof(OVERLAPPED));
		NumberOfBytesSent=0;
		Flags=0;
		if ((r_eax=WSASend(pol->hsocket,&pol->wsabuf,1,&NumberOfBytesSent,Flags,(LPWSAOVERLAPPED)pol,NULL))==SOCKET_ERROR)
		{
			if ((r_eax=WSAGetLastError())!=WSA_IO_PENDING)
			{
				if (r_eax==WSAENOBUFS)
				{
					if (pol->wsabuf.len>1048576)
					{
						pol->wsabuf.len=1048576;
					}
					goto GOTOSENDDATA;
				}
				else
				{
					return -1;
				}
			}
		}
		return 0;
	}
}

void iocpworkfree(POVERLAPPEDPLUS pol)
{
	pol->disconncall(pol->extptr,pol->ipstr);
	GlobalFree(pol->ipstr);
	GlobalFree(pol->extptr);
	GlobalFree(pol->wsabuf.buf);
	GlobalFree(pol->olbuf);
	//GlobalFree(pol);
}


DWORD WINAPI iocprecvqueuethread(PVOID param)
{
	PLIST_ENTRY plist=NULL;
	piocpsocket_recvqueueinfo precvinfo;
	while (TRUE)
	{
		WaitForSingleObject(iocpsocket_hrecvsem,INFINITE);
		plist=iocpsocket_recvqueuelisthead.Flink;
		if (plist!=&iocpsocket_recvqueuelisthead)
		{
			precvinfo=(piocpsocket_recvqueueinfo)plist;
			if (precvinfo->tag!=0)
			{
				precvinfo->recvcall(precvinfo->tag,precvinfo->olbuf,precvinfo->ollen,precvinfo->extptr,precvinfo->ipstr,precvinfo->pol);
				WaitForSingleObject(iocpsocket_hrecvmutex,INFINITE);
				RemoveEntryList(plist);
				ReleaseMutex(iocpsocket_hrecvmutex);
				GlobalFree(precvinfo->olbuf);
				GlobalFree(precvinfo);
			}
			else
			{				
				iocpworkfree(precvinfo->pol);
				WaitForSingleObject(iocpsocket_hrecvmutex,INFINITE);
				RemoveEntryList(plist);
				ReleaseMutex(iocpsocket_hrecvmutex);
				GlobalFree(precvinfo);
			}
		}
	}
	return 0;
}

DWORD WINAPI iocpworkthread(PVOID param)
{
	unsigned int NumberOfBytes,CompletionKey;
	POVERLAPPEDPLUS pol;
	int r_eax;
	while (TRUE)
	{
		NumberOfBytes=0;
		CompletionKey=0;
		pol=NULL;
		r_eax=GetQueuedCompletionStatus(iocpsocket_hiocp,&NumberOfBytes,&CompletionKey,(LPOVERLAPPED*)&pol,INFINITE);
		if(r_eax==0 && pol==NULL){continue;}
		if (r_eax==0 && pol!=NULL)
		{
			pol->flags|=IOCPFLAGS_SOCKERROR;
		}
		if (r_eax>0 && NumberOfBytes==0 && CompletionKey!=IOCPSENDKEY)
		{
			pol->flags|=IOCPFLAGS_SOCKERROR;
		}
		if ((pol->flags & IOCPFLAGS_RECVHEAD) && !(pol->flags & IOCPFLAGS_SOCKERROR))
		{
			if ((r_eax=iocpworkrecvhead(NumberOfBytes,pol))==0)continue;
		}
		else if((pol->flags & IOCPFLAGS_RECVDATA) && !(pol->flags & IOCPFLAGS_SOCKERROR))
		{
			if ((r_eax=iocpworkrecvdata(NumberOfBytes,pol))==0)continue;
		}
		else if ((pol->flags & IOCPFLAGS_SENDDATA) && !(pol->flags & IOCPFLAGS_SOCKERROR))
		{
			PLIST_ENTRY plist=NULL;
			POVERLAPPEDPLUS ppol;
			POVERLAPPEDPLUS pcol;
			ppol=pol->ppol;
			GOTOSENDLIST:
			WaitForSingleObject(iocpsocket_hsendmutex,INFINITE);
			plist=ppol->sendlisthead.Flink;
			if (plist!=&ppol->sendlisthead)
			{
				pcol=CONTAINING_RECORD(plist,OVERLAPPEDPLUS,sendlisthead);
				if (ppol->flags & IOCPFLAGS_SOCKERROR){r_eax=-1;} 
				else
				{
					r_eax=iocpworksenddata(NumberOfBytes,pcol);
				}
				if (r_eax==-1)
				{
					RemoveEntryList(plist);
					GlobalFree(pcol->wsabuf.buf);
					GlobalFree(pcol->olbuf);
					GlobalFree(pcol);
					ReleaseMutex(iocpsocket_hsendmutex);
					NumberOfBytes=0;
					goto GOTOSENDLIST;
				}
				else
				{
					ReleaseMutex(iocpsocket_hsendmutex);
					continue;
				}
			} 
			else
			{
				ppol->sendstatus &=~IOCPFLAGS_SENDSTATUS;
				ReleaseMutex(iocpsocket_hsendmutex);
				continue;
			}
		}
		if ((pol->flags & IOCPFLAGS_SOCKERROR) && !(pol->flags & IOCPFLAGS_SOCKFREE))
		{
			WaitForSingleObject(iocpsocket_hflagsmutex,INFINITE);
			pol->flags|=IOCPFLAGS_SOCKFREE;
			ReleaseMutex(iocpsocket_hflagsmutex);
			if ((pol->flags & IOCPFLAGS_RECVHEAD) || (pol->flags & IOCPFLAGS_RECVDATA))
			{
				piocpsocket_recvqueueinfo precvinfo;
				WaitForSingleObject(iocpsocket_heartbeatmutex,INFINITE);
				RemoveEntryList(&pol->heartbeatlist);
				ReleaseMutex(iocpsocket_heartbeatmutex);
				shutdown(pol->hsocket,0x02);
				closesocket(pol->hsocket);
				precvinfo=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,sizeof(iocpsocket_recvqueueinfo));
				precvinfo->tag=0;
				precvinfo->pol=pol;
				WaitForSingleObject(iocpsocket_hrecvmutex,INFINITE);
				InsertTailList(&iocpsocket_recvqueuelisthead,&precvinfo->recvqueuelist);
				ReleaseMutex(iocpsocket_hrecvmutex);
				ReleaseSemaphore(iocpsocket_hrecvsem,1,NULL);
			}
			else if (pol->flags & IOCPFLAGS_SENDDATA)
			{
				POVERLAPPEDPLUS ppol;
				POVERLAPPEDPLUS pcol;
				PLIST_ENTRY plist = NULL;
				ppol=pol->ppol;
				WaitForSingleObject(iocpsocket_hsendmutex,INFINITE);
				plist=ppol->sendlisthead.Flink;
				while (plist!=&ppol->sendlisthead)
				{
					pcol=CONTAINING_RECORD(plist,OVERLAPPEDPLUS,sendlisthead);
					GlobalFree(pcol->wsabuf.buf);
					GlobalFree(pcol->olbuf);
					RemoveEntryList(plist);
					plist=plist->Flink;
					GlobalFree(pcol);
				}
				ppol->sendstatus &=~IOCPFLAGS_SENDSTATUS;
				ReleaseMutex(iocpsocket_hsendmutex);
			}
			else
			{
				printf("free error pol->ollen=%d,pol->flags=%d",pol->ollen,pol->flags);
			}
		}
	}
	return 0;
}

int iocpheartbeatsend(UINT tag,void *buf,UINT bufsize,PVOID psocket)
{
	POVERLAPPEDPLUS ppol;
	POVERLAPPEDPLUS pol;
	void *sendbuf,*sendbuf0;
	do 
	{
		if (bufsize==0 || psocket==NULL)break;
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

DWORD WINAPI iocpheartbeatthread(PVOID param)
{
	PLIST_ENTRY plist;
	POVERLAPPEDPLUS pol;
	char buf[32];
	while (TRUE)
	{
		Sleep(300000);
		WaitForSingleObject(iocpsocket_heartbeatmutex,INFINITE);
		plist=iocpsocket_heartbeatlisthead.Flink;
		while (plist!=&iocpsocket_heartbeatlisthead)
		{
			pol=(POVERLAPPEDPLUS)CONTAINING_RECORD(plist,OVERLAPPEDPLUS,heartbeatlist);
			if (!(pol->flags & IOCPFLAGS_SOCKERROR))
			{
				if (pol->heartbeatcount>0)
				{
					InterlockedDecrement(&pol->heartbeatcount);
					iocpheartbeatsend(0,buf,4,pol);
				}
				else
				{
					PostQueuedCompletionStatus(iocpsocket_hiocp,0,0,(LPOVERLAPPED)pol);
				}
			}
			plist=plist->Flink;
		}
		ReleaseMutex(iocpsocket_heartbeatmutex);
	}
	return 0;
}