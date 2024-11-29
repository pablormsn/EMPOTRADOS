#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#include <pthread.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>

#define PERIODO_ENT_SEC 1
#define PERIODO_ENT_NSEC 0
#define PRIO_ENT 26

#define PERIODO_SAL_SEC 2
#define PERIODO_SAL_NSEC 0
#define PRIO_SAL 24

#define PRIO_MTR 22

#define PROB_ENTRADA 60
#define PROB_SALIDA 40

#define LENT 1
#define LSAL 2

struct Data{
  pthread_mutex_t mutex;
  int agua;
};

void *tareaEnt (void *arg){
    const struct timespec periodo = {PERIODO_ENT_SEC, PERIODO_ENT_NSEC};
    timer_t timerid;
    struct sigevent sgev;
    struct itimerspec its;
    sigset_t sigset;
    struct Data *data = arg;
    int signum;
    float nRand;
    
    sgev.sigev_notify = SIGEV_SIGNAL;
    sgev.sigev_signo = SIGRTMIN+1;
    sgev.sigev_value.sival_ptr = &timerid;

    timer_create(CLOCK_MONOTONIC, &sgev, &timerid);

    its.it_interval = periodo;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;
    timer_settime(timerid, 0, &its, NULL);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+1);

    while(1){
        sigwait(&sigset, &signum);
        pthread_mutex_lock(&data->mutex);
        
        nRand = rand()%101;

        if(nRand <= 60){
          kill(getpid(), (SIGRTMIN+3));
        } 

        pthread_mutex_unlock(&data->mutex);
    }
    timer_delete(timerid);
    return NULL;
}

void *tareaSal (void *arg){
    const struct timespec periodo = {PERIODO_SAL_SEC, PERIODO_SAL_NSEC};
    timer_t timerid;
    struct sigevent sgev;
    struct itimerspec its;
    sigset_t sigset;
    struct Data *data = arg;
    int signum;
    float nRand;
    
    sgev.sigev_notify = SIGEV_SIGNAL;
    sgev.sigev_signo = SIGRTMIN+2;
    sgev.sigev_value.sival_ptr = &timerid;

    timer_create(CLOCK_MONOTONIC, &sgev, &timerid);

    its.it_interval = periodo;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;
    timer_settime(timerid, 0, &its, NULL);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+2);

    while(1){
        sigwait(&sigset, &signum);
        pthread_mutex_lock(&data->mutex);
        
        nRand = rand()%101;

        if(nRand <= 40){
          kill(getpid(), (SIGRTMIN+4));
        } 

        pthread_mutex_unlock(&data->mutex);
    }
    timer_delete(timerid);
    return NULL;
}

void *tareaMtr (void *arg){
    sigset_t sigset;
    int signum;
    struct Data *data = arg;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+3);
    sigaddset(&sigset, SIGRTMIN+4);

    while(1){
        sigwait(&sigset, &signum);

        pthread_mutex_lock(&data->mutex);

        if(signum == SIGRTMIN+3){
            data->agua += LENT;
            printf("Entrando agua. Nivel de agua actual: %d\n", data->agua);
        }

        if ((signum == SIGRTMIN+4) && ((data->agua) >= 2)){
            data->agua -= LSAL;
            printf("Saliendo agua. Nivel de agua actual: %d\n", data->agua);
        }

        pthread_mutex_unlock(&data->mutex);
    }
    return NULL;
}

int main(int argc, const char *argv){
    sigset_t sigset;
    struct Data data;
    pthread_attr_t attr;
    struct sched_param param;

    int prio0 = 1, prio1 = PRIO_ENT, prio2 = PRIO_SAL, prio3 = PRIO_MTR;
    pthread_t t1, t2, t3;

    mlockall(MCL_CURRENT | MCL_FUTURE);
    prio0 = prio1 + 1;
    param.sched_priority = prio0;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+1);
    sigaddset(&sigset, SIGRTMIN+2);
    sigaddset(&sigset, SIGRTMIN+3);
    sigaddset(&sigset, SIGRTMIN+4);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    data.agua=0;
    pthread_mutex_init(&data.mutex, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    param.sched_priority = prio1;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t1, &attr, tareaEnt, &data);

    param.sched_priority = prio2;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t2, &attr, tareaSal, &data);

    param.sched_priority = prio3;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t3, &attr, tareaMtr, &data);

    pthread_attr_destroy(&attr);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    pthread_mutex_destroy(&data.mutex);
    return 0;
}