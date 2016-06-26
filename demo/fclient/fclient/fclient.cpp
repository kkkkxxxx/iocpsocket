// fclient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "iocpsocket.h"

typedef struct _userext
{
	unsigned int filesize;
	unsigned int seek;
	void *filebuf;

}USEREXT,*PUSEREXT;

unsigned int tmpcount=0;
void recvcall(unsigned int tag,void *buf,unsigned int bufsize,void *extptr,void*ipstr,PVOID psocket)
{
	char tmpbuf[512];
	PUSEREXT pext;
	unsigned int tmp;
	pext=(PUSEREXT)extptr;
	switch(tag)
	{
	case 2:
		if (pext->filesize>8192)
		{
			tmp=(unsigned int)pext->filebuf+pext->seek;
			iocpsocketsend(2,(PVOID)tmp,8192,psocket);
			pext->seek+=8192;
			pext->filesize-=8192;
		}
		else if (pext->filesize>0)
		{
			tmp=(unsigned int)pext->filebuf+pext->seek;
			iocpsocketsend(2,(PVOID)tmp,pext->filesize,psocket);
			pext->seek+=pext->filesize;
			pext->filesize-=pext->filesize;
		}
		else
		{
			GlobalFree(pext->filebuf);
			iocpsocketsend(3,buf,4,psocket);
		}
		break;
	case 10:
		//printf("bufsize=%d,buf=%s\n",bufsize,buf);
GOTORAND:
		srand(time(NULL));
		tmp=rand()%510;
		if (tmp==0)
		{
			goto GOTORAND;
		}
		iocpsocketsend(10,tmpbuf,tmp,psocket);
		break;
	default:
		break;
	}

}

int discount=0;
void disconncall(void*extptr,void*ipstr)
{
	printf("disconn%d\n",discount);
	discount++;
}

DWORD WINAPI MyThread(LPVOID lpParam) 
{
	void *buf;
	buf=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,512);
	lstrcat((LPSTR)buf,"vvvv");
	while (true)
	{
		iocpsocketsend(10,buf,512,lpParam);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	PVOID psocket,psocket0,pext;
	HANDLE hfile;
	int r_eax;
	DWORD tmp=0;
	void *buf;
	char tmpbuf[512];
	r_eax=iocpsocketinit();
	//hfile=CreateFile("d:\\1.pdf",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	//r_eax=GetFileSize(hfile,&tmp);
	//buf=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,r_eax);
	//ReadFile(hfile,buf,r_eax,&tmp,NULL);
	//CloseHandle(hfile);
	//pext=iocpsocketconnect("192.168.127.159",17777,100,&psocket,recvcall,disconncall);
	//((PUSEREXT)pext)->filebuf=buf;
	//((PUSEREXT)pext)->filesize=r_eax;
	//iocpsocketsend(1,"1.pdf",5,psocket);

	buf=GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,512);
	lstrcat((LPSTR)buf,"vvvv");
	//tmp=0;
	//while (tmp<500)
	//{
	//	pext=iocpsocketconnect("192.168.127.156",17777,100,&psocket,recvcall,disconncall);
	//	r_eax=iocpsocketsend(10,buf,512,psocket);
	//	if (r_eax==-1)
	//	{
	//		printf("ssssssssss\n");
	//	}
	//	tmp++;
	//}

	pext=iocpsocketconnect("127.0.0.1",17777,100,&psocket,recvcall,disconncall);

	//pext=iocpsocketconnect("192.168.127.156",17777,100,&psocket,recvcall,disconncall);
	//pext=iocpsocketconnect("192.168.127.156",17777,100,&psocket0,recvcall,disconncall);
	//CreateThread(NULL,0,MyThread,psocket,0,	(LPDWORD)&r_eax);
	//CreateThread(NULL,0,MyThread,psocket0,0,(LPDWORD)&r_eax);
	//Sleep(INFINITE);
	iocpsocketclose(psocket);
	GlobalFree(buf);
	Sleep(INFINITE);
	return 0;
}

