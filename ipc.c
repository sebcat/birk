#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdlib.h>

#include "ipc.h"

ssize_t ipc_sendmsg(int fd, const struct ipcmsg *msg) {
	struct iovec iov;
	struct msghdr msgh;

	assert(fd >= 0);
	assert(msg != NULL);
	assert(msg->length <= IPC_VALSZ);

	iov.iov_base = (void*)msg;
	iov.iov_len = IPC_HDRSZ + msg->length;
	msgh.msg_name = NULL;
	msgh.msg_namelen = 0;
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_control = NULL;
	msgh.msg_controllen=0;
	msgh.msg_flags = 0;
	return sendmsg(fd, &msgh, MSG_NOSIGNAL);
}

ssize_t ipc_recvmsg(int fd, struct ipcmsg *msg) {
	struct iovec iov;
	struct msghdr msgh;

	assert(fd >= 0);
	assert(msg != NULL);

	iov.iov_base = (void*)msg;
	iov.iov_len = IPC_MSGSZ;
	msgh.msg_name = NULL;
	msgh.msg_namelen = 0;
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_control = NULL;
	msgh.msg_controllen=0;
	msgh.msg_flags = 0;
	return recvmsg(fd, &msgh, MSG_NOSIGNAL);
}

