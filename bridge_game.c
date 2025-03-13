#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include<unistd.h>


#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define UNUSED(x) ((void)(x))

#define DECK_SIZE (4 * 13)
#define HAND_SIZE (7)

typedef unsigned int UINT;
typedef struct timespec timespec_t;
typedef struct
{
    int id;
    int* cards;
    int* passed_cards;
    pthread_barrier_t *barrier;
    sem_t* sem;
    unsigned int seed;
    int* players_count;
    int max_players;
} thread_arg;

volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t work = 1;
volatile sig_atomic_t koniec = 1;

void sig_hanler(int sig)
{
    last_signal = sig;
    if(sig==SIGINT)
        work = 0;
}

int set_handler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}
void ReadArguments(int argc, char**argv, int* n);
void *thread_func(void *arg);
void init(pthread_t* thread, thread_arg *targ, int* deck,int* count,int n,pthread_barrier_t*);
bool check_win(thread_arg *targ, int id);
void print_deck(const int *deck, int size);
void shuffle(int *array, size_t n);
void msleep(UINT milisec);

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    srand(time(NULL));
    int n;

    ReadArguments(argc, argv, &n);

    int deck[DECK_SIZE];
    for (int i = 0; i < DECK_SIZE; ++i)
        deck[i] = i;
    shuffle(deck, DECK_SIZE);
    print_deck(deck, DECK_SIZE);
    thread_arg args[n];
    pthread_t thread[n];
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier,NULL,n);
    int count = 0;
    init(thread,args,deck,&count,n,&barrier);
    exit(EXIT_SUCCESS);
}

bool check_win(thread_arg *targ, int id)
{
    int suit = targ->cards[0] %4;
    for(int i = 1; i < HAND_SIZE; i++)
    {
        if(targ->cards[i] %4 != suit)
            return false;
    }

    return true;
}

void *thread_func(void *arg)
{
    thread_arg targ;
    memcpy(&targ,arg,sizeof(targ));
    srand(targ.seed);
    (*targ.players_count)++;
    fprintf(stderr,"Karty gracza %d: ",targ.id);
    print_deck(targ.cards,HAND_SIZE);
    int r = pthread_barrier_wait(targ.barrier);
    koniec = 0;
    for(int runda = 1;koniec==0 && work==1;runda++)
    {
        msleep(10*(targ.id+1));
        if(r==PTHREAD_BARRIER_SERIAL_THREAD)
            printf("Runda %d\n",runda);
        int los = rand()%HAND_SIZE;
        targ.passed_cards[(targ.id+1)%targ.max_players] = targ.cards[los];
        pthread_barrier_wait(targ.barrier);
        targ.cards[los] = targ.passed_cards[targ.id];
        msleep(targ.id+1);
        fprintf(stderr,"Karty gracza %d",targ.id);
        print_deck(targ.cards,HAND_SIZE);
        pthread_barrier_wait(targ.barrier);

        bool win = check_win(&targ,targ.id);
        if(win)
        {
            msleep(10);
            printf("Gracz %d wygrał\n",targ.id);
            koniec = 1;
        }

        pthread_barrier_wait(targ.barrier);
    }

    r = pthread_barrier_wait(targ.barrier);
    if(r == PTHREAD_BARRIER_SERIAL_THREAD)
        printf("koniec gry\n");
    return NULL;
}

void init(pthread_t* thread, thread_arg *targ, int* deck,int* count,int n,pthread_barrier_t *barrier)
{
    set_handler(sig_hanler,SIGUSR1);
    set_handler(sig_hanler,SIGINT);
    sigset_t mask,oldmask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGUSR1);
    //sigaddset(&mask,SIGINT);
    sigprocmask(SIG_BLOCK,&mask,&oldmask);
    int licznik;
    int passed_cards[n];
    while(work) {
        licznik = 0;
        for (int i = 0; i < n; i++) {
            sigsuspend(&oldmask);
            if (last_signal == SIGUSR1) {
                targ[i].seed = rand();
                targ[i].passed_cards = passed_cards;
                targ[i].id = i;
                targ[i].players_count = &licznik;
                targ[i].max_players = n;
                targ[i].barrier = barrier;
                if ((targ[i].cards = (int *) malloc(sizeof(int) * HAND_SIZE)) == NULL)
                    ERR("malloc");
                for (int j = 0; j < HAND_SIZE; j++, (*count)++) {
                    targ[i].cards[j] = deck[*count];
                }
                if (pthread_create(&thread[i], NULL, thread_func, (void *) &targ[i]) != 0)
                    ERR("pthread_create");
            } else {
                work = 0;
            }
        }
        for(int i = 0;i<n;i++)
        {
            if(pthread_join(thread[i],NULL)!=0)
                ERR("pthread_join");
            free(targ[i].cards);
        }
    }

}


void ReadArguments(int argc, char**argv, int* n)
{
    if(argc == 2)
    {
        *n = atoi(argv[1]);
        if(*n < 4 || *n > 7)
            ERR("Invalid table size");
    }
    else
    {
        ERR("Invalid number of arguments");
    }
}

void usage(const char *program_name)
{
    fprintf(stderr, "USAGE: %s n\n", program_name);
    exit(EXIT_FAILURE);
}

void shuffle(int *array, size_t n)
{
    if (n > 1)
    {
        size_t i;
        for (i = 0; i < n - 1; i++)
        {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void print_deck(const int *deck, int size)
{
    const char *suits[] = {" of Hearts", " of Diamonds", " of Clubs", " of Spades"};
    const char *values[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};

    char buffer[1024];
    int offset = 0;

    if (size < 1 || size > DECK_SIZE)
        return;

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[");
    for (int i = 0; i < size; ++i)
    {
        int card = deck[i];
        if (card < 0 || card > DECK_SIZE)
            return;
        int suit = deck[i] % 4;
        int value = deck[i] / 4;

        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", values[value]);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", suits[suit]);
        if (i < size - 1)
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, ", ");
    }
    snprintf(buffer + offset, sizeof(buffer) - offset, "]");

    puts(buffer);
}

void msleep(UINT milisec)
{
    time_t sec = (int)(milisec / 1000);
    milisec = milisec - (sec * 1000);
    timespec_t req = {0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    if (nanosleep(&req, &req))
        ERR("nanosleep");
}