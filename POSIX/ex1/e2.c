#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>

#define PERIODO_TMP_SEC 1
#define PERIODO_TMP_NSEC 0
#define PRIO_TMP 28

#define PERIODO_CLI_SEC 4
#define PERIODO_CLI_NSEC 0
#define PRIO_CLI 26

#define PERIODO_MTR_SEC 4
#define PERIODO_MTR_NSEC 0
#define PRIO_MTR 24

#define PRECIO_ENERGIA 10
#define TMP_CONFORT 22

#define N 20
#define M 25

struct Data{
    pthread_mutex_t mutex;
    float valtemp;
    int aire;
    float gasto;
};

/*----------Soporte*/
void espera_activa(time_t seg){
    volatile time_t t = time(0) + seg;
    while (time(0)<t) { /*Esperar activamente*/}
}

// const char *get_time(char *buf){
//     time_t t = time(0); //Obtener el tiempo actual
//     char *f = ctime_r(&t, buf); //Esta función convierte el tiempo en una cadena y la almacena en buf /* buf de longitud minima 26 */
//     f[strlen(f)-1] = '\0'; //Eliminar el salto de línea
//     return f; //Retornar la cadena
// }

void addtime (struct timespec *tm, const struct timespec *val){
    tm->tv_sec += val->tv_sec;
    tm->tv_nsec += val->tv_nsec;
    if(tm->tv_nsec > 1000000000L){
        tm->tv_sec += (tm->tv_nsec / 1000000000L);
        tm->tv_nsec = (tm->tv_nsec % 1000000000L);
    }
}

/*--------------*/
/*--------Tareas*/
/*--------------*/

void *tareaTMP (void *arg){
    const struct timespec periodo = {PERIODO_TMP_SEC, PERIODO_TMP_NSEC};
    struct timespec next;
    struct Data *data = arg;
    float randTmp;
    // struct sched_param param; //Parámetros de planificación
    // const char *pol; //Política de planificación
    // int policy; //Política de planificación
    // pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    // pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación
    // printf("# Tarea Sensor de Temperatura [%s:%d]\n", pol, param.sched_priority); //Mensaje de depuración
    clock_gettime(CLOCK_MONOTONIC, &next);
    while(1){
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        addtime(&next, &periodo);
        pthread_mutex_lock(&data->mutex);

        randTmp = rand() % (M + 1 - N) + N;
        printf("La temperatura aleatoria es [%f]\n", randTmp);

        data->valtemp = (TMP_CONFORT-randTmp)*2.5;

        pthread_mutex_unlock(&data->mutex); 
    }
    return NULL;
}
/*-------------------*/
void *tareaCLI (void *arg){
    const struct timespec periodo = {PERIODO_CLI_SEC, PERIODO_CLI_NSEC};
    struct timespec next;
    struct Data *data = arg;
    // struct sched_param param; //Parámetros de planificación
    // const char *pol; //Política de planificación
    // int policy; //Política de planificación
    // pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    // pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación
    // printf("# Tarea Sensor de Temperatura [%s:%d]\n", pol, param.sched_priority); //Mensaje de depuración
    clock_gettime(CLOCK_MONOTONIC, &next);
    while(1){
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        addtime(&next, &periodo);
        pthread_mutex_lock(&data->mutex);

        if(data->valtemp <= 0.0){
            data->aire = 0;
        }else{
            data->aire = 1;
        }
        data->gasto = fabs(data->valtemp) * PRECIO_ENERGIA;

        pthread_mutex_unlock(&data->mutex);
    }
    return NULL;
}

void *tareaMTR (void *arg){
    const struct timespec periodo = {PERIODO_CLI_SEC, PERIODO_CLI_NSEC};
    struct timespec next;
    struct Data *data = arg;
    // struct sched_param param; //Parámetros de planificación
    // const char *pol; //Política de planificación
    // int policy; //Política de planificación
    // pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    // pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación
    // printf("# Tarea Sensor de Temperatura [%s:%d]\n", pol, param.sched_priority); //Mensaje de depuración
    clock_gettime(CLOCK_MONOTONIC, &next);
    while(1){
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        addtime(&next, &periodo);
        pthread_mutex_lock(&data->mutex);

        printf("Parametro energetico: %f\n", data->valtemp);
        printf("Estado del aire acondicionado: %s\n", data->aire==0 ? "Modo Frio" : "Modo Calefaccion");
        printf("Gasto de la ultima accion: %f\n", data->gasto);

        pthread_mutex_unlock(&data->mutex);
    }
    return NULL;
}

/*-----------------*/
/*--------Principal*/
/*-----------------*/

// void usage(const char *nm){
//     fprintf(stderr, "usage: %s [-h] [-ff] [-rr]\n", nm);
//     exit(EXIT_FAILURE);
// }

// void get_args(int argc, const char *argv[], int *policy){
//     int i;
//     for(i = 1; i<argc; i++){
//         if (strcmp(argv[i], "-h")==0){
//             usage(argv[0]);
//         }else if (strcmp(argv[i], "-ff")==0){
//             *policy = SCHED_FIFO;
//         }else if (strcmp(argv[i], "-rr")==0){
//             *policy = SCHED_RR;
//         }else{
//             usage(argv[0]);
//         }
//     }
// }

int main(int argc, const char *argv[]){
    struct Data shared_data;
    pthread_attr_t attr;
    struct sched_param param;
    int prio0=1, prio1=PRIO_TMP, prio2=PRIO_CLI, prio3=PRIO_MTR;
    pthread_t t1, t2, t3;

    

    mlockall(MCL_CURRENT | MCL_FUTURE);
    prio0 = PRIO_TMP + 1;
    param.sched_priority = prio0;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    srand(time(NULL));
    shared_data.gasto = 0.0;
    shared_data.valtemp = 0.0;
    pthread_mutex_init(&shared_data.mutex, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    param.sched_priority = prio1;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t1, &attr, tareaTMP, &shared_data);

    param.sched_priority = prio2;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t2, &attr, tareaCLI, &shared_data);

    param.sched_priority = prio3;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t3, &attr, tareaMTR, &shared_data);

    pthread_attr_destroy(&attr);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    pthread_mutex_destroy(&shared_data.mutex);

    return 0;
}