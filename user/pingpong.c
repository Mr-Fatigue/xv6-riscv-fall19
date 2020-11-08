#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "user/user.h"

void main() {
	int fd1[2];
	int fd2[2];
	char buf[17];
	char ask[5] = "ping\0";
	char answer[5] = "pong\0";
	pipe(fd1);
	pipe(fd2);
	int pid = fork();
	if (pid == 0) {
		read(fd1[0], buf, sizeof(buf));
		close(fd1[0]);
		printf("%d: received %s\n",getpid(), buf);
		write(fd2[1], answer, sizeof(answer));
		close(fd2[1]);
	}
	else{
		write(fd1[1], ask, sizeof(ask));
		close(fd1[1]);
		wait();
		read(fd2[0], buf, sizeof(buf));
		close(fd2[0]);
		printf("%d: received %s\n",getpid(),buf);
	}
	exit();
}