#include<stdlib.h> 
#include<stddef.h>
#include<assert.h>
#include<string.h>
#include<stdio.h>

#include<unistd.h>
#include<time.h>
#include<errno.h>
#include<sched.h>
#include<pthread.h>
#include<sys/mman.h>

#define INACTIVO 0
#define ACTIVO 1

#define PERIODO_TMP_SEC 0
#define PERIODO_TMP_NSEC 500000000
#define PRIORIDAD_TMP 22
#define TMP_UMBRAL_SUP 100
#define TMP_UMBRAL_INF 90

#define PERIODO_STMP_SEC 0
#define PERIODO_STMP_NSEC 400000000
#define PRIORIDAD_STMP 24
#define STMP_VALOR_INI 80
#define STMP_INC 1
#define STMP_DEC 2

#define PERIODO_PRS_SEC 0
#define PERIODO_PRS_NSEC 350000000
#define PRIORIDAD_PRS 26
#define PRS_UMBRAL_SUP 1000
#define PRS_UMBRAL_INF 900

#define PERIODO_SPRS_SEC 0
#define PERIODO_SPRS_NSEC 350000000
#define PRIORIDAD_SPRS 28
#define SPRS_VALOR_INI 800
#define SPRS_INC 10
#define SPRS_DEC 20

#define PERIODO_MTR_SEC 1
#define PERIODO_MTR_NSEC 0
#define PRIORIDAD_MTR 20

struct Data_Tmp{
    pthread_mutex_t mutex;
    int estado;
    int val;
}

struct Data_Prs{
    pthread_mutex_t mutex;
    int estado;
    int val;
}

struct Data_Mtr{
    struct Data_Tmp tmp;
    struct Data_Prs prs;
}

/*----------------------------*/
/*
* Nota: los "printf" están con el objetivo de depuración, para que el
* alumno pueda analizar el comportamiento de las tareas.
*/
/*----------------------------*/
/*--Soporte-------------------*/
/*----------------------------*/
const char *get_time(char *buf){
    time_t t = time(0); //Obtener el tiempo actual
    char *f = ctime_r(&t, buf); //Esta función convierte el tiempo en una cadena y la almacena en buf /* buf de longitud minima 26 */
    f[strlen(f)-1] = '\0'; //Eliminar el salto de línea
    return f; //Retornar la cadena
}
/*----------------------------*/
void addtime(struct timespec *tm, const struct timespec *val){
    tm->tv_sec += val->tv_sec; //Sumar los segundos
    tm->tv_nsec += val->tv_nsec; //Sumar los nanosegundos
    if (tm->tv_nsec >= 1000000000L){ //Si los nanosegundos son mayores o iguales a 1000000000, es decir, un segundo
        tm->tv_sec += (tm->tv_nsec / 1000000000L); //Sumar los segundos
        tm->tv_nsec = tm->tv_nsec % 1000000000L; //Obtener el residuo de los nanosegundos
    }
}
/*----------------------------*/
int clock_nanosleep_intr(clockid_t clk_id, int flags, const struct timespec *request, struct timespec *remain){
    int err;
    while((err = clock_nanosleep(clk_id, flags, request, remain)) == EINTR){
        /*Repetir si ha sido interrumpido*/
    }
    return err;
}
/*----------------------------*/
/*--Tareas--------------------*/
/*----------------------------*/
void ini_tmp(struct Data_Tmp *data){
    data->estado = INACTIVO;
    data->val = STMP_VALOR_INI;
    pthread_mutex_init(&data->mutex, NULL);
}
/*----------------------------*/
void *tarea_stmp(void *arg){
    const struct timespec periodo = {PERIODO_STMP_SEC, PERIODO_STMP_NSEC}; //Periodo de la tarea
    struct timespec next; //Siguiente tiempo de ejecución
    struct Data_Tmp *data = arg; //Datos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int policy; //Política de planificación
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación
    printf("# Tarea Sensor de Temperatura [%s:%d]\n", pol, param.sched_priority); //Mensaje de depuración
    clock_gettime(CLOCK_MONOTONIC, &next); //Obtener el tiempo actual
    while(1){ //Ciclo infinito
        clock_nanosleep_intr(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL); //Dormir hasta next
        addtime(&next, &periodo); //Sumar el periodo a next
        pthread_mutex_lock(&data->mutex); //Bloquear el mutex
        switch(data->estado){
            CASE INACTIVO:
                data->val += STMP_INC;
                break;
            CASE ACTIVO:
                data->val -= STMP_DEC;
                break;
        }
        pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
    }
    return NULL; //Retornar NULL
}
/*----------------------------*/
void *tarea_tmp(void *arg){
    const struct timespec periodo = {PERIODO_STMP_SEC, PERIODO_STMP_NSEC}; //Periodo de la tarea
    struct timespec next; //Siguiente tiempo de ejecución
    char buf[30]; //Buffer para la hora
    struct Data_Tmp *data = arg; //Datos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int policy; //Política de planificación
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación
    printf("# Tarea Control de Temperatura [%s:%d]\n", pol, param.sched_priority); //Mensaje de depuración
    clock_gettime(CLOCK_MONOTONIC, &next); //Obtener el tiempo actual
    while(1){ //Ciclo infinito
        clock_nanosleep_intr(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL); //Dormir hasta next
        addtime(&next, &periodo); //Sumar el periodo a next
        pthread_mutex_lock(&data->mutex); //Bloquear el mutex
        switch(data->estado){
            CASE INACTIVO:
                if(data->val > TMP_UMBRAL_SUP){
                    data->estado = ACTIVO;
                    printf("# [%s] Activar inyección de aire frío\n", get_time(buf));
                }
                break;
            CASE ACTIVO:
                if(data->val < TMP_UMBRAL_INF){
                    data->estado = INACTIVO;
                    printf("# [%s] Desactivar inyección de aire frío\n", get_time(buf));
                }
                break;
        }
        pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
    }
    return NULL; //Retornar NULL
}
/*----------------------------*/
void ini_prs(struct Data_Prs *data){
    data->estado = INACTIVO;
    data->val = SPRS_VALOR_INI;
    pthread_mutex_init(&data->mutex, NULL);
}
/*----------------------------*/
void *tarea_sprs(void *arg){
    const struct timespec periodo = {PERIODO_STMP_SEC, PERIODO_STMP_NSEC}; //Periodo de la tarea
    struct timespec next; //Siguiente tiempo de ejecución
    struct Data_Tmp *data = arg; //Datos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int policy; //Política de planificación
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación
    printf("# Tarea Sensor de Presión [%s:%d]\n", pol, param.sched_priority); //Mensaje de depuración
    clock_gettime(CLOCK_MONOTONIC, &next); //Obtener el tiempo actual
    while(1){ //Ciclo infinito
        clock_nanosleep_intr(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL); //Dormir hasta next
        addtime(&next, &periodo); //Sumar el periodo a next
        pthread_mutex_lock(&data->mutex); //Bloquear el mutex
        switch(data->estado){
            CASE INACTIVO:
                data->val += SPRS_INC;
                break;
            CASE ACTIVO:
                data->val -= SPRS_DEC;
                break;
        }
        pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
    }
    return NULL; //Retornar NULL
}

void *tarea_prs(void *arg){
    const struct timespec periodo = {PERIODO_STMP_SEC, PERIODO_STMP_NSEC}; //Periodo de la tarea
    struct timespec next; //Siguiente tiempo de ejecución
    char buf[30]; //Buffer para la hora
    struct Data_Tmp *data = arg; //Datos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int policy; //Política de planificación
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación
    printf("# Tarea Control de Presión [%s:%d]\n", pol, param.sched_priority); //Mensaje de depuración
    clock_gettime(CLOCK_MONOTONIC, &next); //Obtener el tiempo actual
    while(1){ //Ciclo infinito
        clock_nanosleep_intr(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL); //Dormir hasta next
        addtime(&next, &periodo); //Sumar el periodo a next
        pthread_mutex_lock(&data->mutex); //Bloquear el mutex
        switch(data->estado){
            CASE INACTIVO:
                if(data->val > PRS_UMBRAL_SUP){
                    data->estado = ACTIVO;
                    printf("# [%s] Abrir válvula de presión\n", get_time(buf));
                }
                break;
            CASE ACTIVO:
                if(data->val < PRS_UMBRAL_INF){
                    data->estado = INACTIVO;
                    printf("# [%s] Cerrar válvula de presión\n", get_time(buf));
                }
                break;
        }
        pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
    }
    return NULL; //Retornar NULL
}

void *tarea_mtr(void *arg){
    const struct timespec periodo = {PERIODO_MTR_SEC, PERIODO_MTR_NSEC}; //Periodo de la tarea
    struct timespec next; //Siguiente tiempo de ejecución
    char buf[30]; //Buffer para la hora
    struct Data_Mtr *data = arg; //Datos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int policy; //Política de planificación
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación
    printf("# Tarea Monitorización [%s:%d]\n", pol, param.sched_priority); //Mensaje de depuración
    clock_gettime(CLOCK_MONOTONIC, &next); //Obtener el tiempo actual
    while(1){ //Ciclo infinito
        clock_nanosleep_intr(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL); //Dormir hasta next  
        addtime(&next, &periodo); //Sumar el periodo a next
        /*----------------*/
        pthread_mutex_lock(&data->tmp.mutex); //Bloquear el mutex
        pthread_mutex_lock(&data->prs.mutex); //Bloquear el mutex
        printf("# [%s] Temperatura: %d %s Presión: %d %s\n", 
            get_time(buf), 
            data->tmp.val, (data->tmp.estado == INACTIVO ? "++" : "--"),
            data->prs.val, (data->prs.estado == INACTIVO ? "++" : "--")); //Mensaje de depuración
        pthread_mutex_unlock(&data->prs.mutex); //Desbloquear el mutex
        pthread_mutex_unlock(&data->tmp.mutex); //Desbloquear el mutex
        /*----------------*/
    }
    return NULL; //Retornar NULL
}
/*----------------------------*/
/*--Principal-----------------*/
/*----------------------------*/
int maximo(int a, int b, int c, int d, int e){
    int m = (a > b ? a : b);
    m = (c > m ? c : m);
    m = (d > m ? d : m);
    m = (e > m ? e : m);
    return m;
}

void usage(const char *nm){
    fprintf(stderr, "usage: %s [-h] [-ff] [-rr]\n", nm);
    exit(EXIT_FAILURE);
}

void get_args(int argc, const char *argv[], int *policy){
    int i;
    for(i = 1; i<argc; i++){
        if (strcmp(argv[i], "-h")==0){
            usage(argv[0]);
        }else if (strcmp(argv[i], "-ff")==0){
            *policy = SCHED_FIFO;
        }else if (strcmp(argv[i], "-rr")==0){
            *policy = SCHED_RR;
        }else{
            usage(argv[0]);
        }
    }
}

int main(int argc, const char *argv[]){
    struct Data_Mtr data; //Datos de la tarea
    pthread_attr_t attr; //Atributos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int pcy, policy = SCHED_FIFO; //Política de planificación
    int prio0 = 1; //Prioridad de la tarea 0
    int prio1 = PRIORIDAD_TMP; //Prioridad de la tarea 1
    int prio2 = PRIORIDAD_PRS; //Prioridad de la tarea 2
    int prio3 = PRIORIDAD_MTR; //Prioridad de la tarea 3
    int prio4 = PRIORIDAD_STMP; //Prioridad de la tarea 4
    int prio5 = PRIORIDAD_SPRS; //Prioridad de la tarea 5
    pthread_t t0, t1, t2, t3, t4, t5; //Hilos
    /*
    * La tarea principal debe tener la mayor prioridad, para poder
    * crear todas las tareas necesarias.
    */
    get_args(argc, argv, &policy); //Obtener los argumentos
    prio0 = maximo(prio1, prio2, prio3, prio4, prio5) + 1; //Establecer la prioridad de la tarea principal

    mlockall(MCL_CURRENT | MCL_FUTURE); //Bloquear la memoria
    pthread_getschedparam(pthread_self(), &pcy, &param); //Obtener la política de planificación y los parámetros
    param.sched_priority = prio0; //Establecer la prioridad
    pthread_setschedparam(pthread_self(), policy, &param); //Establecer la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación

    ini_tmp(&data.tmp); //Inicializar los datos de la tarea
    ini_prs(&data.prs); //Inicializar los datos de la tarea

    pthread_attr_init(&attr); //Inicializar los atributos de la tarea
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); //Establecer la herencia de la planificación
    pthread_attr_setschedpolicy(&attr, policy); //Establecer la política de planificación
    param.sched_priority = prio1; //Establecer la prioridad
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de planificación
    pthread_create(&t1, &attr, tarea_tmp, &data.tmp); //Crear la tarea 1
    param.sched_priority = prio2; //Establecer la prioridad
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de planificación
    pthread_create(&t2, &attr, tarea_prs, &data.prs); //Crear la tarea 2
    param.sched_priority = prio3; //Establecer la prioridad
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de planificación
    pthread_create(&t3, &attr, tarea_mtr, &data); //Crear la tarea 3
    param.sched_priority = prio4; //Establecer la prioridad
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de planificación
    pthread_create(&t4, &attr, tarea_stmp, &data.tmp); //Crear la tarea 4
    param.sched_priority = prio5; //Establecer la prioridad
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de planificación
    pthread_create(&t5, &attr, tarea_sprs, &data.prs); //Crear la tarea 5

    printf("# Tarea principal [%s:%d]\n", pol, prio0); //Mensaje de depuración

    pthread_join(t1, NULL); //Esperar a la tarea 1
    pthread_join(t2, NULL); //Esperar a la tarea 2
    pthread_join(t3, NULL); //Esperar a la tarea 3
    pthread_join(t4, NULL); //Esperar a la tarea 4
    pthread_join(t5, NULL); //Esperar a la tarea 5
    pthread_mutex_destroy(&data.tmp.mutex); //Destruir el mutex
    pthread_mutex_destroy(&data.prs.mutex); //Destruir el mutex
    return 0;


}
