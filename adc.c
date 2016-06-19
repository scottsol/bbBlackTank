#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <termios.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
						} while (0)

#define SIGF 20
#define SIGB 21
#define SIGL 22
#define SIGR 23
#define SIG_STOP 24

struct sigaction sab;
struct termios config;
int pid;
int fd;

int countf = 0;
int countb = 0;
int count = 0;

void adc_init() {
	system("echo cape-bone-iio > /sys/devices/bone_capemgr.9/slots");
	system("echo BB-UART4 > /sys/devices/bone_capemgr.9/slots");
	usleep(1000);
}

static void adc_handler(int sig, siginfo_t *si, void *uc) {
	/* Note: calling printf() from a signal handler is not
	strictly correct, since printf() is not async-signal-safe;
	see signal(7) */
	//printf("get to handler\n");
	FILE* ain1;
	FILE* ain2;
	FILE* ain3;
	FILE* ain4;

	ain1 = fopen("/sys/devices/ocp.3/helper.17/AIN1", "r");
	ain2 = fopen("/sys/devices/ocp.3/helper.17/AIN2", "r");
	ain3 = fopen("/sys/devices/ocp.3/helper.17/AIN3", "r");
	ain4 = fopen("/sys/devices/ocp.3/helper.17/AIN4", "r");

	int i;
	int an1, an2, an3, an4;

	an1 = 0;
	an2 = 0;
	an3 = 0;
	an4 = 0;
	count++;

	for (i = 0; i < 300; i++) {
		int v1;
		int v2;
		int v3;
		int v4;

		fscanf(ain1, "%d", &v1);
		fscanf(ain2, "%d", &v2);
		fscanf(ain3, "%d", &v3);
		fscanf(ain4, "%d", &v4);

		an1 += v1;
		an2 += v2;
		an3 += v3;
		an4 += v4;
	}

	fclose(ain1);
	fclose(ain2);
	fclose(ain3);
	fclose(ain4);

	an1 = an1 / 300;
	an2 = an2 / 300;
	an3 = an3 / 300;
	an4 = an4 / 300;

	if (an1 > 1400 && an1 != 1799) {
		countf += 1;
	} else if (an4 > 1500 && an4 != 1799) {
		countb += 1;
	}

	if (count % 70 == 0) {
		printf("analog 1: %d\n", an1);
		printf("analog 2: %d\n", an2);
		printf("analog 3: %d\n", an3);
		printf("analog 4: %d\n", an4);
	}

	if (countf == 4 && an1 != 1799) {
		printf("turn back\n");
		countf = 0;
		if(kill(pid, SIGF) != 0){ 
    		printf("Can't send msg\n");
    		exit(0);
 		}
	} else if (countb == 4 && an4 != 1799) {
		printf("go forward\n");
		countb = 0;
		if(kill(pid, SIGB) != 0){ 
    		printf("Can't send msg\n");
    		exit(0);
 		}
	}	
}

void handler(int sig) {
	FILE* uart = fopen("/dev/ttyO4", "r");
	char read;
	read = fgetc(uart);
	printf("the following char was received from bluetooth: %c\n", read);

	if (read == 'w') {
		if (kill(pid, SIGF) != 0) {
			printf("Can't send SIG\n");
			exit(0);
		}
	} else if (read == 's') {
		if (kill(pid, SIGB) != 0) {
			printf("Can't send SIG\n");
			exit(0);
		}
	} else if (read == 'a') {
		if (kill(pid, SIGL) != 0) {
			printf("Can't send SIG\n");
			exit(0);
		}
	} else if (read == 'd') {
		if (kill(pid, SIGR) != 0) {
			printf("Can't send SIG\n");
			exit(0);
		}
	} else if (read == 'x') {
		if (kill(pid, SIG_STOP) != 0) {
			printf("Can't send SIG\n");
			exit(0);
		}
	}
	fclose(uart);
}

int main(int argc, char *argv[]) {
 	pid = atoi(argv[2]);
    adc_init();
    fd = open("/dev/ttyO4", O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
    	perror("failed to open port\n");
    	exit(1);
    }

    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    long long freq_nanosecs;
    sigset_t mask;
    struct sigaction sa;

    // BLUETOOTH MODULE INIT
    sab.sa_handler = handler;
    sab.sa_flags = 0;
    sab.sa_restorer = NULL;
    sigaction(SIGIO, &sab, NULL);

    fcntl(fd, F_SETFL, FNDELAY);
    fcntl(fd, F_SETOWN, getpid());
    fcntl(fd, F_SETFL, O_ASYNC);

    tcgetattr(fd, &config);
    cfsetispeed(&config, B115200);
    cfsetospeed(&config, B115200);
    config.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    config.c_cflag |= (CS8 | CLOCAL | CREAD);
    config.c_lflag &= ~(ICANON | ECHO | ECHOE| ISIG);
    config.c_iflag &= ~(IXON | IXOFF| IXANY);
    config.c_oflag &= ~OPOST;
    tcsetattr(fd, TCSANOW, &config);
    printf("UART4 Initialized\n");

    // DISTANCE SENSOR INIT
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <freq_sec> <pid>\n",
               argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Establish handler for timer signal */
    printf("Establishing handler for signal %d\n", SIG);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = adc_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIG, &sa, NULL) == -1)
      errExit("sigaction");

    /* Create the timer */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCKID, &sev, &timerid) == -1)
        errExit("timer_create");

    printf("timer ID is 0x%lx\n", (long) timerid);

    /* Start the timer */
    freq_nanosecs = atoll(argv[1]);
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = freq_nanosecs * 1000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        errExit("timer_settime");
   }

    while (1);
    return 0;
}
