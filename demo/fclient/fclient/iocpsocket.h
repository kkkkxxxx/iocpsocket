
#ifndef IOCPSOCKET_H
#define	IOCPSOCKET_H

#ifdef	__cplusplus
extern "C" {
#endif

int iocpsocketsend(UINT tag,void *buf,UINT bufsize,PVOID psocket);

int iocpsocketclose(PVOID psocket);

PVOID iocpsocketconnect(void *addrstr,unsigned short port,unsigned int extsize,PVOID *ppsocket,
					  void (*recvcall)(unsigned int tag,void *buf,unsigned int bufsize,void *extptr,void*ipstr,PVOID psocket),
					  void (*disconncall)(void*extptr,void*ipstr));

int iocpsocketlisten(unsigned short port,unsigned int extsize,
					 void (*recvcall)(unsigned int tag,void *buf,unsigned int bufsize,void *extptr,void*ipstr,PVOID psocket),
					 void (*disconncall)(void*extptr,void*ipstr));

int iocpsocketinit();

#ifdef	__cplusplus
}
#endif

#endif	/* IOCPSOCKET_H */