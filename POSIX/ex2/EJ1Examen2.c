/*SUPER PARECIDO AL 4*/

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>

#define PERIODO_A_SEC 1
#define PERIODO_A_NSEC 0
#define PRIORIDAD_A 28

#define PERIODO_B_SEC 8
#define PERIODO_B_NSEC 0
#define PRIORIDAD_B 26

#define PRIORIDAD_C 24
#define PRIORIDAD_D 24

#define CHKN(syscall) \
    do { \
        int err = syscall; \
        if (err != 0) { \
            fprintf(stderr, "%s: %d: SysCall Error: %s\n", \
                    __FILE__, __LINE__, strerror(errno)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define CHKE(syscall) \
    do { \
        int err = syscall; \
        if (err != 0) { \
            fprintf(stderr, "%s: %d: SysCall Error: %s\n", \
                    __FILE__, __LINE__, strerror(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)


void *tarea_a(void *arg) {

    const struct timespec periodo = {PERIODO_A_SEC, PERIODO_A_NSEC};
    
    timer_t timerid;
    struct sigevent sgev;
    struct itimerspec its;
    sigset_t sigset;
    int signum;
    
    sgev.sigev_notify = SIGEV_SIGNAL;
    sgev.sigev_signo = SIGRTMIN;
    sgev.sigev_value.sival_ptr = &timerid;

    CHKN(timer_create(CLOCK_MONOTONIC, &sgev, &timerid));

    its.it_interval = periodo;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;
    CHKN(timer_settime(timerid, 0, &its, NULL));

    CHKN(sigemptyset(&sigset));
    CHKN(sigaddset(&sigset, SIGRTMIN));

    while (1) {
        CHKE(sigwait(&sigset, &signum));

        kill(getpid(),(SIGRTMIN + 1));
    }
    CHKN(timer_delete(timerid));
    return NULL;
}

void *tarea_b(void *arg) {
    const struct timespec periodo = {PERIODO_B_SEC, PERIODO_B_NSEC};
    
    timer_t timerid;
    struct sigevent sgev;
    struct itimerspec its;
    sigset_t sigset;
    int signum;
    
    sgev.sigev_notify = SIGEV_SIGNAL;
    sgev.sigev_signo = SIGRTMIN+2;
    sgev.sigev_value.sival_ptr = &timerid;

    CHKN(timer_create(CLOCK_MONOTONIC, &sgev, &timerid));

    its.it_interval = periodo;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;

    CHKN(timer_settime(timerid, 0, &its, NULL));

    CHKN(sigemptyset(&sigset));
    CHKN(sigaddset(&sigset, SIGRTMIN+2));

    while (1) {
        CHKE(sigwait(&sigset, &signum));
        
        kill(getpid(),(SIGRTMIN + 3));

    }
    CHKN(timer_delete(timerid));
    return NULL;
}

void *tarea_c(void *arg) {

    sigset_t sigset;
    int signum;
    
    CHKN(sigemptyset(&sigset));
    CHKN(sigaddset(&sigset, SIGRTMIN+1));
    CHKN(sigaddset(&sigset, SIGRTMIN+3));

    while (1) {
        CHKE(sigwait(&sigset, &signum));

        if(signum == SIGRTMIN+1){
            printf("C - RECIBE UNA COCACOLA\n");
        }
        if(signum == SIGRTMIN+3){
            printf("C - 8 BEBIDAS LISTAS PARA EMPAQUETAR\n");
            kill(getpid(), SIGRTMIN + 4);
        }
    }
    return NULL;
}


void *tarea_d(void *arg) {

    sigset_t sigset;
    int signum;
    
    CHKN(sigemptyset(&sigset));
    CHKN(sigaddset(&sigset, SIGRTMIN+4));

    while (1) {
        CHKE(sigwait(&sigset, &signum));

        if(signum == SIGRTMIN+4){
            printf("D - EMPAQUETANDO\n");
        }
    }
    return NULL;
}


//NO CAMBIA HASTA AQUÍ
int main(int argc, const char *argv[]) {

    sigset_t sigset;

    struct sched_param param;
    pthread_attr_t attr;
    
    int prio0 = 1, prio1 = PRIORIDAD_A, prio2 = PRIORIDAD_B, prio3 = PRIORIDAD_C, prio4 = PRIORIDAD_D;
    pthread_t t1, t2, t3, t4;


    CHKN(mlockall(MCL_CURRENT | MCL_FUTURE));
    prio0 = PRIORIDAD_A + 1;
    param.sched_priority = prio0;
    CHKE(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param));

    CHKN(sigemptyset(&sigset)); //Vacía el conjunto de senales para evitar conflico //CAMBIA DESDE AQUÍ
    CHKN(sigaddset(&sigset, SIGRTMIN));
    CHKN(sigaddset(&sigset, SIGRTMIN+1)); //anade al conjunto de senales la senal sigusr1 
    CHKN(sigaddset(&sigset, SIGRTMIN+2));  //anade al conjunto de senales la senal sigusr2
    CHKN(sigaddset(&sigset, SIGRTMIN+3));
    CHKN(sigaddset(&sigset, SIGRTMIN+4));
    CHKE(pthread_sigmask(SIG_BLOCK, &sigset, NULL)); //CAMBIA HASTA AQUÍ


    CHKE(pthread_attr_init(&attr));
    CHKE(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
    CHKE(pthread_attr_setschedpolicy(&attr, SCHED_FIFO));

    param.sched_priority = prio1;
    CHKE(pthread_attr_setschedparam(&attr, &param));
    CHKE(pthread_create(&t1, &attr, tarea_a, NULL));

    param.sched_priority = prio2;
    CHKE(pthread_attr_setschedparam(&attr, &param));
    CHKE(pthread_create(&t2, &attr, tarea_b, NULL));

    param.sched_priority = prio3;
    CHKE(pthread_attr_setschedparam(&attr, &param));
    CHKE(pthread_create(&t3, &attr, tarea_c, NULL));

    param.sched_priority = prio4;
    CHKE(pthread_attr_setschedparam(&attr, &param));
    CHKE(pthread_create(&t4, &attr, tarea_d, NULL));

    CHKE(pthread_attr_destroy(&attr));

    printf("Tarea principal con política FIFO \n");

    CHKE(pthread_join(t1, NULL));
    CHKE(pthread_join(t2, NULL));
    CHKE(pthread_join(t3, NULL));
    CHKE(pthread_join(t4, NULL));


    CHKE(pthread_detach(t1));
    CHKE(pthread_detach(t2));
    CHKE(pthread_detach(t3));
    CHKE(pthread_detach(t4));
    
    return 0;
}
