#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_MESSAGES 10
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

typedef struct {
    pid_t pid;
    uint32_t value1;
    uint32_t value2;
} Message;

void usage(void)
{
    fprintf(stderr, "Argument to musi byc nazwa kolejki z id wypisanym przez serwer na przyklad: 5754_s albo 10253_m\n");
    exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sig_handler(int sig) { last_signal = sig; }

int main(int argc, char** argv) {
    if (argc != 2)
        usage();

    char name[10];
    char name2[10];
    char line[100];
    sprintf(name,"/%d",getpid());
    mqd_t pid,out;
    uint32_t a,b,l;

    sethandler(sig_handler,SIGINT);

    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = sizeof(uint32_t);

    struct mq_attr attr2;
    attr2.mq_maxmsg = MAX_MESSAGES;
    attr2.mq_msgsize = sizeof(Message);

    if((pid = TEMP_FAILURE_RETRY(mq_open(name,O_CREAT | O_RDONLY,0600,&attr))) == (mqd_t)-1)
        ERR("mq_open");

    sprintf(name2,"/%s",argv[1]);

    if((out = TEMP_FAILURE_RETRY(mq_open(name2,O_WRONLY,0600,&attr2)))== (mqd_t)-1)
        ERR("mq_open");
    fprintf(stderr,"Podaj liczby\n");
    while (fgets(line, sizeof(line), stdin) != NULL && last_signal!=SIGINT) {
        if (strcmp(line, "\n") == 0)
            break;

        if (sscanf(line, "%d %d", &a, &b) != 2) {
            printf("Niepoprawny format danych. Wprowadź dwie liczby oddzielone spacją.\n");
            continue;
        }


        Message m;
        m.pid = getpid();
        m.value1 = a;
        m.value2 = b;

        if(TEMP_FAILURE_RETRY(mq_send(out,(char*)&m,sizeof(m),0)))
            ERR("mq_send");
        struct timespec timeout;
        if (clock_gettime(CLOCK_REALTIME, &timeout) < 0)
            ERR("clock_gettime");

        timeout.tv_nsec += 100 * 1000000;

        if (timeout.tv_nsec >= 1000000000) {
            timeout.tv_sec++;
            timeout.tv_nsec -= 1000000000;
        }

        if(TEMP_FAILURE_RETRY(mq_timedreceive(pid,(char*)&l,sizeof(uint32_t),NULL,&timeout))==-1) {
            if(errno == ETIMEDOUT)
                break;
            else
                ERR("mq_timedrecieve");
        }
        fprintf(stderr,"wynik: %d\n",l);
        fprintf(stderr,"\nPodaj liczby\n");
    }

    if(mq_unlink(name))
        ERR("mq_unlink");
    return EXIT_SUCCESS;

}