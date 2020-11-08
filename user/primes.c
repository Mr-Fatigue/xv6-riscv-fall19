#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "user/user.h"

void primes(int read_fd) {
	int primeNum;
	int fd[2];
	int pid;
	int numbers[33] = { 0 };
	int writeNumbers[33] = { 0 };
	int counter = 0;
	pipe(fd);
	read(read_fd, numbers, sizeof(numbers));
	close(read_fd);
	if (numbers[0])
	{
		primeNum = numbers[0];
		printf("prime %d\n",primeNum);
		for (int index = 1; numbers[index]; index++) {
			if (numbers[index]%primeNum != 0) {
				writeNumbers[counter] = numbers[index];
				counter++;
			}
		}
		write(fd[1], writeNumbers, sizeof(writeNumbers));
		close(fd[1]);
		pid = fork();
		if (pid == 0) {
			primes(fd[0]);
		}
	}
	exit();
	
}

void main() {
	int numbers[33] = { 0 };
	int fd[2];
	int pid;
	pipe(fd);
	for (int i = 0; i <= 32; i++) {
		numbers[i] = i+2;
	}
	write(fd[1], numbers, sizeof(numbers));
	close(fd[1]);
	pid = fork();
	if (pid == 0) {
		primes(fd[0]);
	}
	exit();
	
}