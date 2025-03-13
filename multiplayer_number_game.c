#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define BUF_SIZE 16

void usage(char *name);
void sigchld_handler(int sig);
void child_work(int in,int out,int m,int id);
void parent_work(int in[],int out[],int n,int m);
int sethandler(void (*f)(int), int sigNo);
void create_children_and_pipes(int n,int m,int *pipes_cp,int *pipes_pc);
int find_xth_card(int x,int cards[]);

int main(int argc, char **argv)
{
    if(argc != 3)
        usage(argv[0]);
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    if(n<2 || n>5 || m<5 || m>10)
        usage(argv[0]);
    if (sethandler(sigchld_handler, SIGCHLD))
        ERR("Seting parent SIGCHLD:");
    if(sethandler(SIG_IGN,SIGPIPE))
        ERR("setting sigpipe");

    //int i,j;
    int* pipes_cp;
    int* pipes_pc;


    if (NULL == (pipes_cp = (int *)malloc(sizeof(int) * n)))
        ERR("malloc");
    if (NULL == (pipes_pc = (int *)malloc(sizeof(int) * n)))
        ERR("malloc");

    create_children_and_pipes(n,m,pipes_cp,pipes_pc);


    parent_work(pipes_cp,pipes_pc,n,m);

    while (n--) {
        if (pipes_pc[n] && TEMP_FAILURE_RETRY(close(pipes_pc[n])))
            ERR("close");
        if (pipes_cp[n] && TEMP_FAILURE_RETRY(close(pipes_cp[n])))
            ERR("close");
    }

    free(pipes_cp);
    free(pipes_pc);


    return EXIT_SUCCESS;
}

void parent_work(int in[],int out[],int n,int m)
{
    /*sleep(1);
    fprintf(stderr,"Parent [%d]\n",getpid());
    for(int i = 0;i<n;i++)
    {
        fprintf(stderr,"Child [%d] in: %d; out: %d\n",i,in[i],out[i]);
    }*/


    int x,j,i,max_counter,max_x,player_points,status;
    char buf[BUF_SIZE];
    int round_results[n];
    int points[n];
    int active_players[n];
    for(i = 0;i<n;i++) {
        points[i] = 0;
        active_players[i] = 1;
    }
    /*for(j = 0;j<n;j++) {
        if (TEMP_FAILURE_RETRY(read(in[j], buf, BUF_SIZE)) < BUF_SIZE)
            ERR("read");
        x = atoi(buf);
        fprintf(stderr, "Got number %d from player %d\n", x, j);
    }*/

    for(i = 0;i<m;i++)
    {
        max_counter = 0;
        max_x = 0;

        fprintf(stderr,"--------------- NEW ROUND ---------------\n");
        sleep(1);
        memset(buf,0,BUF_SIZE);
        sprintf(buf,"%s","new_round");
        for(j = 0;j<n;j++)
        {
            if(active_players[j] == 1) {
                //fprintf(stderr,"Message P%d: %s\n",j+1,buf);
                status = TEMP_FAILURE_RETRY(write(out[j], buf, BUF_SIZE));
                //fprintf(stderr,"%d %d\n",j+1,status);
                if (status < 0) {
                    if (errno == EPIPE) {
                        //fprintf(stderr,"dezaktywacja %d\n",j+1);
                        active_players[j] = 0;
                        round_results[j] = -1;
                    } else
                        ERR("write");
                }
            }
        }

        for(j = 0;j<n;j++) {
            if(active_players[j] == 1 ) {
                status = TEMP_FAILURE_RETRY(read(in[j], buf, BUF_SIZE));
                if (status == 0)
                    active_players[j] = 0;
                else if(status<BUF_SIZE)
                    ERR("read");
                x = atoi(buf);
                fprintf(stderr, "Got number %d from player %d\n", x, j + 1);
                round_results[j] = x;
                if (max_counter == 0 || round_results[j] > max_x) {
                    max_counter = 1;
                    max_x = round_results[j];
                } else if (max_x == round_results[j])
                    max_counter++;
            }
        }
        fprintf(stderr,"--------------- ROUND WINNERS ---------------\n");
        for(j = 0;j<n;j++)
        {
            if(active_players[j] == 1) {
                if (round_results[j] == max_x) {
                    player_points = n / max_counter;
                    points[j] += player_points;
                    fprintf(stderr, "Player [%d] got %d points\n", j + 1, player_points);
                } else {
                    player_points = 0;
                }
                memset(buf, 0, BUF_SIZE);
                sprintf(buf, "%d", player_points);

                if (TEMP_FAILURE_RETRY(write(out[j], buf, BUF_SIZE)) < 0)
                    ERR("write");
            }
        }
        fprintf(stderr,"------------------------------------------------\n");
    }
    fprintf(stderr,"-------------------- FINAL RESULTS --------------------\n");
    for(j = 0;j<n;j++)
    {
        fprintf(stderr,"Player [%d] got %d points\n",j+1,points[j]);
    }
}

void child_work(int in,int out,int m,int id)
{
    //fprintf(stderr,"Child [%d]: in: %d; out: %d\n",getpid(),in,out);

    srand(getpid());
    int x,i,random_death;
    int cards_left = m;
    char buf[BUF_SIZE];
    char new_message[BUF_SIZE];
    memset(new_message,0,BUF_SIZE);
    sprintf(new_message,"new_message");
    int cards[m];
    for(i = 0;i<m;i++)
        cards[i] = 1;


    /*x = rand()%m + 1;
    memset(buf,0,BUF_SIZE);
    sprintf(buf,"%d",x);

    if (TEMP_FAILURE_RETRY(write(out, buf, BUF_SIZE)) < 0)
        ERR("write");*/

    for(i = 0;i<m;i++)
    {
        random_death = rand()%20;
        if(random_death == 0)
        {
            fprintf(stderr,"Player [%d] died randomly\n",id);

            return;
        }

        if (TEMP_FAILURE_RETRY(read(in, buf, BUF_SIZE)) < 0)
            ERR("read");

        //fprintf(stderr,"Message C%d: %s\n",id,buf);

        x = rand()%cards_left + 1;
        x = find_xth_card(x,cards);
        memset(buf,0,BUF_SIZE);
        sprintf(buf,"%d",x);

        if (TEMP_FAILURE_RETRY(write(out, buf, BUF_SIZE)) < 0)
            ERR("write");

        if (TEMP_FAILURE_RETRY(read(in, buf, BUF_SIZE)) < 0)
            ERR("read");

        cards_left--;
    }


}

void create_children_and_pipes(int n,int m,int* pipes_cp,int*pipes_pc)
{
    int max = n;
    int id;
    int tmpfd_cp[2];
    int tmpfd_pc[2];

    while (n)
    {
        if (pipe(tmpfd_cp))
            ERR("pipe");
        if (pipe(tmpfd_pc))
            ERR("pipe");
        switch (fork())
        {
            case 0:
                id = n;
                while(n<max)
                {
                    if(pipes_pc[n] && TEMP_FAILURE_RETRY(close(pipes_pc[n])))
                        ERR("close");
                    if(pipes_cp[n] && TEMP_FAILURE_RETRY(close(pipes_cp[n++])))
                        ERR("close");
                }
                free(pipes_pc);
                free(pipes_cp);
                if (TEMP_FAILURE_RETRY(close(tmpfd_pc[1])))
                    ERR("close");
                if (TEMP_FAILURE_RETRY(close(tmpfd_cp[0])))
                    ERR("close");
                child_work(tmpfd_pc[0], tmpfd_cp[1],m,id);
                if (TEMP_FAILURE_RETRY(close(tmpfd_cp[1])))
                    ERR("close");
                if (TEMP_FAILURE_RETRY(close(tmpfd_pc[0])))
                    ERR("close");
                exit(EXIT_SUCCESS);

            case -1:
                ERR("Fork:");
        }

        if (TEMP_FAILURE_RETRY(close(tmpfd_pc[0])))
            ERR("close");
        if (TEMP_FAILURE_RETRY(close(tmpfd_cp[1])))
            ERR("close");
        pipes_pc[--n] = tmpfd_pc[1];
        pipes_cp[n] = tmpfd_cp[0];

    }
}

int find_xth_card(int x,int cards[])
{
    int i = 0;
    int l = 0;
    while(1)
    {
        if(cards[i] == 1)
        {
            l++;
        }
        if(l == x)
        {
            cards[i] = 0;
            return i+1;
        }
        i++;
    }
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "2<=N<=5 - number of players\n");
    fprintf(stderr, "5<=M<=10 - number of rounds\n");
    exit(EXIT_FAILURE);
}

void sigchld_handler(int sig)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (0 == pid)
            return;
        if (0 >= pid)
        {
            if (ECHILD == errno)
                return;
            ERR("waitpid:");
        }
    }
}

int sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

