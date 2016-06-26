
#ifndef IOCPWORKTHREAD_H
#define	IOCPWORKTHREAD_H

#ifdef	__cplusplus
extern "C" {
#endif

	DWORD WINAPI iocprecvqueuethread(PVOID param);

	DWORD WINAPI iocpworkthread(PVOID param);

	DWORD WINAPI iocpheartbeatthread(PVOID param);

#ifdef	__cplusplus
}
#endif

#endif	/* IOCPWORKTHREAD_H */
