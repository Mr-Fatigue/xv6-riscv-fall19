#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "user/user.h"

void main(int argc, char *argv[]) {
	long time = 0;
	if(argc > 1){
		for(int i = 0 ; argv[1][i]!= 0 ; i++){
			time = 10*time+argv[1][i]-'0';
		}
	}
	else
	{
		time = 10;
	}
	printf("%d",time);
	sleep(time);
	exit();
}