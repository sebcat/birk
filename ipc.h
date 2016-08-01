#ifndef __BIRK_IPC_H
#define __BIRK_IPC_H

#include <stdint.h>

#define IPC_MSGSZ	768
#define IPC_HDRSZ	(sizeof(uint16_t)*2)
#define IPC_VALSZ	(IPC_MSGSZ-IPC_HDRSZ)

#define IPC_CREADY		0
#define IPC_CCONNECT	1

#define IPC_MREADY(msg) \
	do { \
		(msg)->type = IPC_CREADY; \
		(msg)->length = 0; \
	} while(0)

/* XXX: responsibility of caller to check for overflows */
#define IPC_MCONNECT(msg, host, hostlen, port) \
	do { \
		(msg)->type = IPC_CCONNECT; \
		(msg)->length = sizeof(int) + (hostlen); \
		*(int*)(msg)->value = (int)port; \
		memcpy((msg)->value+sizeof(int), host, hostlen); \
	} while(0)

struct ipcmsg {
	uint16_t type;
	uint16_t length; /* 0 <= length <= IPC_VALSZ */
	uint8_t value[IPC_VALSZ];
} __attribute__((packed));

ssize_t ipc_sendmsg(int fd, const struct ipcmsg *msg);
ssize_t ipc_recvmsg(int fd, struct ipcmsg *msg);
#endif
