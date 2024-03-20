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

typedef struct {
    pid_t pid;
    uint32_t value1;
    uint32_t value2;
} Message;

volatile sig_atomic_t last_signal;

int sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

void sigint_handler(int sig ){last_signal = sig;}

void func_s(union sigval sv)
{
    mqd_t out;
     mqd_t *id = (mqd_t*)sv.sival_ptr;
     struct sigevent sev_s;
    sev_s.sigev_notify = SIGEV_THREAD;
    sev_s.sigev_value.sival_ptr = id;
    sev_s.sigev_notify_attributes = NULL;
    sev_s.sigev_notify_function = func_s;
    if(mq_notify(*id,&sev_s)<0)
    {
        ERR("mq_notify");
    }
     Message m;
     uint32_t l;
     char name[10];

     if(TEMP_FAILURE_RETRY(mq_receive(*id, (char*)&m,sizeof(Message),NULL))<0)
     {
         ERR("mq_receive");
     }
     memset(name,0,10);
     sprintf(name,"/%d",m.pid);
     if((out = TEMP_FAILURE_RETRY(mq_open(name,O_WRONLY|O_NONBLOCK,0600,NULL)))==(mqd_t)-1)
        ERR("mq_open");
     l = m.value1+m.value2;
     if(TEMP_FAILURE_RETRY(mq_send(out,(char*)&l,sizeof(uint32_t),0)))
        ERR("mq_send");
     mq_close(out);
}

void func_d(union sigval sv)
{
    mqd_t out;
    mqd_t *id = (mqd_t*)sv.sival_ptr;
    struct sigevent sev_d;
    sev_d.sigev_notify = SIGEV_THREAD;
    sev_d.sigev_value.sival_ptr = id;
    sev_d.sigev_notify_attributes = NULL;
    sev_d.sigev_notify_function = func_d;
    if(mq_notify(*id,&sev_d)<0)
    {
        ERR("mq_notify");
    }
    Message m;
    uint32_t l;
    char name[10];

    if(TEMP_FAILURE_RETRY(mq_receive(*id, (char*)&m,sizeof(Message),NULL))<0)
    {
        ERR("mq_receive");
    }
    memset(name,0,10);
    sprintf(name,"/%d",m.pid);
    if((out = TEMP_FAILURE_RETRY(mq_open(name,O_WRONLY|O_NONBLOCK,0600,NULL)))==(mqd_t)-1)
        ERR("mq_open");
    l = m.value1/m.value2;
    if(TEMP_FAILURE_RETRY(mq_send(out,(char*)&l,sizeof(uint32_t),0)))
        ERR("mq_send");
    mq_close(out);
}

void func_m(union sigval sv)
{
    mqd_t out;
    mqd_t *id = (mqd_t*)sv.sival_ptr;
    struct sigevent sev_m;
    sev_m.sigev_notify = SIGEV_THREAD;
    sev_m.sigev_value.sival_ptr = id;
    sev_m.sigev_notify_attributes = NULL;
    sev_m.sigev_notify_function = func_m;
    if(mq_notify(*id,&sev_m)<0)
    {
        ERR("mq_notify");
    }
    Message m;
    uint32_t l;
    char name[10];

    if(TEMP_FAILURE_RETRY(mq_receive(*id, (char*)&m,sizeof(Message),NULL))<0)
    {
        ERR("mq_receive");
    }
    memset(name,0,10);
    sprintf(name,"/%d",m.pid);
    if((out = TEMP_FAILURE_RETRY(mq_open(name,O_WRONLY|O_NONBLOCK,0600,NULL)))==(mqd_t)-1)
        ERR("mq_open");
    l = m.value1%m.value2;
    if(TEMP_FAILURE_RETRY(mq_send(out,(char*)&l,sizeof(uint32_t),0)))
        ERR("mq_send");
    mq_close(out);
}

int main(int argc, char** argv)
{
    mqd_t pid_s,pid_d,pid_m;

    struct sigevent sev_s,sev_d,sev_m;

    char name_s[10];
    char name_d[10];
    char name_m[10];

    sprintf(name_s,"/%d_s",getpid());
    sprintf(name_d,"/%d_d",getpid());
    sprintf(name_m,"/%d_m",getpid());

    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = sizeof(Message);

    sethandler(sigint_handler,SIGINT);
    fprintf(stderr,"Server address: %d\n",getpid());

    if((pid_s = TEMP_FAILURE_RETRY(mq_open(name_s,O_RDWR | O_CREAT |O_NONBLOCK,0600,&attr))) == (mqd_t)-1) {
        ERR("mq_open");
    }

    if((pid_d = TEMP_FAILURE_RETRY(mq_open(name_d,O_RDWR | O_CREAT |O_NONBLOCK,0600,&attr))) == (mqd_t)-1)
        ERR("mq_open");

    if((pid_m = TEMP_FAILURE_RETRY(mq_open(name_m,O_RDWR | O_CREAT |O_NONBLOCK,0600,&attr))) == (mqd_t)-1)
        ERR("mq_open");

    sev_s.sigev_notify = SIGEV_THREAD;
    sev_s.sigev_value.sival_ptr = &pid_s;
    sev_s.sigev_notify_attributes = NULL;
    sev_s.sigev_notify_function = func_s;
    if(mq_notify(pid_s,&sev_s)<0)
        ERR("mq_notify");

    sev_d.sigev_notify = SIGEV_THREAD;
    sev_d.sigev_value.sival_ptr = &pid_d;
    sev_d.sigev_notify_attributes = NULL;
    sev_d.sigev_notify_function = func_d;
    if(mq_notify(pid_d,&sev_d)<0)
        ERR("mq_notify");

    sev_m.sigev_notify = SIGEV_THREAD;
    sev_m.sigev_value.sival_ptr = &pid_m;
    sev_m.sigev_notify_attributes = NULL;
    sev_m.sigev_notify_function = func_m;
    if(mq_notify(pid_m,&sev_m)<0)
        ERR("mq_notify");

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        ERR("sigprocmask");
    }

    while (last_signal != SIGINT) {
        sigsuspend(&mask);
    }

    while(wait(NULL)>0);

    mq_close(pid_s);
    mq_close(pid_d);
    mq_close(pid_m);
    mq_unlink(name_s);
    mq_unlink(name_d);
    mq_unlink(name_m);
    return EXIT_SUCCESS;

}