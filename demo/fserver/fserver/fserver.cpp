// fserver.cpp : 定义控制台应用程序的入口点。
//


#include "stdafx.h"
#include "iocpsocket.h"

void recvcall(unsigned int tag,void *buf,unsigned int bufsize,void *extptr,void*ipstr,PVOID psocket)
{
	HANDLE hfile;
	DWORD tmp;
	switch(tag)
	{
		case 1:
			hfile=CreateFile("d:\\2.pdf",GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
			WriteFile(hfile,buf,bufsize,&tmp,NULL);
			CloseHandle(hfile);
			break;
		case 10:
			iocpsocketsend(10,buf,32,psocket);
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

int _tmain(int argc, _TCHAR* argv[])
{
	int r_eax;
	r_eax=iocpsocketinit();
	iocpsocketlisten(17777,100,recvcall,disconncall);
	Sleep(INFINITE);
	return 0;
}

