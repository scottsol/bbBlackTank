#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#define SIGF 20
#define SIGB 21 
#define SIGL 22
#define SIGR 23
#define SIG_STOP 24


void init() {
	system("echo am33xx_pwm > /sys/devices/bone_capemgr.9/slots");
	system("echo bone_pwm_P9_14 > /sys/devices/bone_capemgr.9/slots");
	system("echo bone_pwm_P9_16 > /sys/devices/bone_capemgr.9/slots");
	system("echo 100000 > /sys/devices/ocp.3/pwm_test_P9_14.15/duty");
	system("echo 100000 > /sys/devices/ocp.3/pwm_test_P9_16.16/duty");

	system("echo 112 > /sys/class/gpio/export");
	system("echo 115 > /sys/class/gpio/export");
	system("echo 48 > /sys/class/gpio/export");
	system("echo 60 > /sys/class/gpio/export");
	system("echo 49 > /sys/class/gpio/export");

	system("echo out > /sys/class/gpio/gpio112/direction");
	system("echo out > /sys/class/gpio/gpio115/direction");
	system("echo out > /sys/class/gpio/gpio48/direction");
	system("echo out > /sys/class/gpio/gpio60/direction");
	system("echo out > /sys/class/gpio/gpio49/direction");
}

void start_motor(int start) {
	FILE* standby;
	standby = fopen("/sys/class/gpio/gpio49/value", "w");
	fseek(standby, 0, SEEK_SET);
	fprintf(standby, "%d", start);
	fflush(standby);
	fclose(standby);
}

void direction(int controla, int controlb) {
	int ain1, ain2;
	ain1 = controla & 1;
	ain2 = (controla & 2) >> 1;

	int bin1, bin2;
	bin1 = controlb & 1;
	bin2 = (controlb & 2) >> 1;

	FILE* a1;
	FILE* a2;
	FILE* b1;
	FILE* b2;

	a1 = fopen("/sys/class/gpio/gpio112/value", "w");
	a2 = fopen("/sys/class/gpio/gpio115/value", "w");
	b1 = fopen("/sys/class/gpio/gpio48/value", "w");
	b2 = fopen("/sys/class/gpio/gpio60/value", "w");

	fseek(a1, 0, SEEK_SET);
	fseek(a2, 0, SEEK_SET);
	fseek(b1, 0, SEEK_SET);
	fseek(b2, 0, SEEK_SET);

	fprintf(a1, "%d", ain1);
	fprintf(a2, "%d", ain2);
	fprintf(b1, "%d", bin1);
	fprintf(b2, "%d", bin2);

	fflush(a1);
	fflush(a2);
	fflush(b1);
	fflush(b2);

	fclose(a1);
	fclose(a2);
	fclose(b1);
	fclose(b2);

}

void forward(int signo) {
	start_motor(1);
	direction(2, 2);
}

void backward(int signo) {
	start_motor(1);
	direction(1, 1);
}

void left(int signo) {
	start_motor(1);
	direction(1, 2);
}

void right(int signo) {
	start_motor(1);
	direction(2, 1);
}

void stop() {
	start_motor(0);
}

int main(int argc, char* argv[]) {
	init();
	start_motor(1);
	direction(2, 2);

	while (1) {
		if (signal(SIGF, forward) == SIG_ERR) {
			printf("\ncan't catch SIGF\n");
		}

		if (signal(SIGB, backward) == SIG_ERR) {
			printf("\ncan't catch SIGB\n");
		}
		if (signal(SIGL, left) == SIG_ERR) {
			printf("\ncan't catch SIGL\n");
		}
		if (signal(SIGR, right) == SIG_ERR) {
			printf("\ncan't catch SIGR\n");
		}
		if (signal(SIG_STOP, stop) == SIG_ERR) {
			printf("\ncan't catch SIGSTOP\n");
		}
		while (1) {
			sleep(1);
		}
	}

	start_motor(0);
}

