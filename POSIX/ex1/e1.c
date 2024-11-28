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
#define PRIO_A 28
#define INC_A 1

#define PERIODO_B_SEC 2
#define PERIODO_B_NSEC 0
#define PRIO_B 26
#define INC_B 2

#define PRIO_C 24

struct Data{
    int contA;
    pthread_mutex_t mutexA;
    int contB;
    pthread_mutex_t mutexB;
};
/*-----------------*/
/*Soporte*/
/*-----------------*/
// #define CHKN(syscall) \
//     do { \
//         int err = syscall; \
//         if (err != 0) { \
//             fprintf(stderr, "%s: %d: SysCall Error: %s\n", \
//                     __FILE__, __LINE__, strerror(errno)); \
//             exit(EXIT_FAILURE); \
//         } \
//     } while (0)

// #define CHKE(syscall) \
//     do { \
//         int err = syscall; \
//         if (err != 0) { \
//             fprintf(stderr, "%s: %d: SysCall Error: %s\n", \
//                     __FILE__, __LINE__, strerror(err)); \
//             exit(EXIT_FAILURE); \
//         } \
//     } while (0)

// const char *get_time (char *buf){
//     time_t t = time(0); //Tiempo actual
//     char *f = ctime_r(&t, buf); //Convertir el tiempo en una cadena y almacenarla en buf
//     f[strlen(f)-1] = '\0'; //Eliminar el salto de línea
//     return f; //Retornar la cadena
// }

/*----------------*/
/*Tareas*/
/*----------------*/

void *tareaA (void *arg){
    const struct timespec periodo = {PERIODO_A_SEC, PERIODO_A_NSEC};
    timer_t timerid;
    struct sigevent sgev;
    struct itimerspec its;
    sigset_t sigset;
    //char buf[30];
    struct Data *data = arg;
    struct sched_param param;
    // const char *pol;
    int signum;
    //int i, policy;
    //pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    //pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación

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
        //printf("Tarea A [%s]\n", get_time(buf)); //Mensaje de depuración
        pthread_mutex_lock(&data->mutexA);
        data->contA += INC_A;
        printf("Tarea A: [%d]\n", data->contA);

        if(data->contA % 10 == 0){
            kill(getpid(), (SIGRTMIN+3));
        }
        pthread_mutex_unlock(&data->mutexA);
    }
    timer_delete(timerid);
    return NULL;
}

void *tareaB (void *arg){
    const struct timespec periodo = {PERIODO_B_SEC, PERIODO_B_NSEC};
    timer_t timerid; 
    struct sigevent sgev;
    struct itimerspec its;
    sigset_t sigset;
    //char buf[30];
    struct Data *data = arg;
    struct sched_param param;
    //const char *pol;
    int signum;
    //int i, policy;
    //pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    //pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación

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
        //printf("Tarea B [%s]\n", get_time(buf)); //Mensaje de depuración
        pthread_mutex_lock(&data->mutexB);
        data->contB += INC_B;
        printf("Tarea B: [%d]\n", data->contB);

        if(data->contB % 5 == 0){
            kill(getpid(), (SIGRTMIN+4));
        }
        pthread_mutex_unlock(&data->mutexB);
    }
    timer_delete(timerid);
    return NULL;
}

void *tareaC (void *arg){
    sigset_t sigset;
    int signum;

    struct Data *data = arg;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+3);
    sigaddset(&sigset, SIGRTMIN+4);

    while(1){
        sigwait(&sigset, &signum);

        pthread_mutex_lock(&data->mutexA);
        pthread_mutex_lock(&data->mutexB);

        if(signum == SIGRTMIN+3){
            printf("-----Contador A multiplo de diez: [%d]\n", data->contA);
        }

        if (signum == SIGRTMIN+4){
            printf("-----Contador B multiplo de cinco: [%d]\n", data->contB);
        }

        pthread_mutex_unlock(&data->mutexA);
        pthread_mutex_unlock(&data->mutexB);
    }
    return NULL;
}

/*------------------------*/
/*---Principal*/
/*------------------------*/

// void usage (const char *nm){
//     fprintf(stderr, "usage: %s [-h] [-ff] [-rr] [-p1] [-p2]\n", nm); //Imprimir mensaje de uso
//     exit(EXIT_FAILURE); //Salir con error
// }

// void get_args (int argc, const char *argv[], int *policy, int *prio1, int *prio2){
//     int i; //Contador
//     if (argc < 2){
//         usage(argv[0]); //Imprimir mensaje de uso
//     }else{
//         for(i = 1; i<argc; i++){
//             if (strcmp(argv[i], "-h") == 0){
//                 usage(argv[0]); //Imprimir mensaje de uso
//             }else if (strcmp(argv[i], "-ff") == 0){
//                 *policy = SCHED_FIFO; //Asignar la política de planificación FIFO
//             }else if (strcmp(argv[i], "-rr") == 0){
//                 *policy = SCHED_RR; //Asignar la política de planificación RR
//             }else if (strcmp(argv[i], "-p1") == 0){
//                 *prio1 = PRIORIDAD_A; //Asignar la prioridad de la tarea A
//                 *prio2 = PRIORIDAD_B; //Asignar la prioridad de la tarea B
//             }else if (strcmp(argv[i], "-p2") == 0){
//                 *prio1 = PRIORIDAD_B; //Asignar la prioridad de la tarea B
//                 *prio2 = PRIORIDAD_A; //Asignar la prioridad de la tarea A
//             }else{
//                 usage(argv[0]); //Imprimir mensaje de uso
//             }
//         }
//     }
// }

int main(int argc, const char *argv){
    sigset_t sigset;
    struct Data shared_data;
    pthread_attr_t attr;
    struct sched_param param;

    int prio0 = 1, prio1 = PRIO_A, prio2 = PRIO_B, prio3 = PRIO_C;
    pthread_t t1, t2, t3;

    mlockall(MCL_CURRENT | MCL_FUTURE);
    prio0 = PRIO_A + 1;
    param.sched_priority = prio0;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+1);
    sigaddset(&sigset, SIGRTMIN+2);
    sigaddset(&sigset, SIGRTMIN+3);
    sigaddset(&sigset, SIGRTMIN+4);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    shared_data.contA=0;
    shared_data.contB=0;
    pthread_mutex_init(&shared_data.mutexA, NULL);
    pthread_mutex_init(&shared_data.mutexB, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    param.sched_priority = prio1;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t1, &attr, tareaA, &shared_data);

    param.sched_priority = prio2;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t2, &attr, tareaB, &shared_data);

    param.sched_priority = prio3;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t3, &attr, tareaC, &shared_data);

    pthread_attr_destroy(&attr);

    printf("Tarea principal con política FIFO \n");

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    pthread_mutex_destroy(&shared_data.mutexA);
    pthread_mutex_destroy(&shared_data.mutexB);
    return 0;
}