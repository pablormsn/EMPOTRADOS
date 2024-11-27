#include <stdlib.h> //Librería estándar de C
#include <stddef.h> //Librería que incluye definiciones como NULL
#include <assert.h> //Librería que incluye la macro assert
#include <string.h> //Librería que incluye funciones para manipular cadenas de caracteres
#include <stdio.h> //Librería que incluye funciones de entrada y salida estándar

#include <unistd.h> //Librería que incluye funciones para el sistema operativo Unix
#include <time.h> //Librería que incluye funciones para manipular el tiempo
#include <errno.h> //Librería que incluye la variable errno
#include <sched.h> //Librería que incluye funciones para manipular la planificación de tareas
#include <pthread.h> //Librería que incluye funciones para manipular hilos
#include <sys/mman.h> //Librería que incluye funciones para manipular la memoria
#include <signal.h> //Librería que incluye funciones para manejar señales

#define PERIODO_A_SEC 90 //Periodo de la tarea A en segundos
#define PERIODO_A_NSEC  200000000 //Periodo de la tarea A en nanosegundos
#define ITER_A 40 //Número de iteraciones de la tarea A
#define INC_A 100 //Incremento de la tarea A
#define PRIORIDAD_A 24 //Prioridad de la tarea A

#define PERIODO_B_SEC 100 //Periodo de la tarea B en segundos
#define PERIODO_B_NSEC 300000000 //Periodo de la tarea B en nanosegundos
#define ITER_B 40 //Número de iteraciones de la tarea B
#define INC_B 1 //Incremento de la tarea B
#define PRIORIDAD_B 26 //Prioridad de la tarea B

struct Data{
    pthread_mutex_t mutex; //Mutex para sincronizar la salida
    int cnt; //Contador de la tarea
};

/*--------------------------*/
/*
* Nota: los "printf" están con el objetivo de depuración, para que el
* alumno pueda analizar el comportamiento de las tareas.
*/
/*--------------------------*/
/*--Soporte----------------*/
/*--------------------------*/
void espera_activa(time_t seg){
    volatile time_t t = time(0) + seg; //Tiempo actual más seg
    while (time(0) < t){ /*Esperar hasta que el tiempo actual sea mayor que t*/}
}
/*---------------------------*/
const char *get_time (char *buf){
    time_t t = time(0); //Tiempo actual
    char *f = ctime_r(&t, buf); //Convertir el tiempo en una cadena y almacenarla en buf
    f[strlen(f)-1] = '\0'; //Eliminar el salto de línea
    return f; //Retornar la cadena
}
/*---------------------------*/
// int sigwait_int (const sigset_t, int *signum){
//     int err; //Variable para el resultado
//     while ((err = sigwait(&set, signum)) == EINTR){ /*Repetir si ha sido interrumpido*/}
//     return err; //Retornar el resultado
// }
/*---------------------------*/
/*---Tareas------------------*/
/*---------------------------*/
void *tarea_a (void *arg){
    const struct timespec periodo = {PERIODO_A_SEC, PERIODO_A_NSEC}; //Periodo de la tarea A
    timer_t timerid; //Identificador del temporizador
    struct sigevent sgev; //Evento de señal
    struct itimerspec its; //Estructura de tiempo del temporizador
    sigset_t sigset; //Conjunto de señales
    char buf[30]; //Buffer para la hora
    struct Data *data = arg; //Datos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int i, policy, signum; //Contador, política de planificación y número de señal
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación

    sgev.sigev_notify = SIGEV_SIGNAL; //Notificar con señal
    sgev.sigev_signo = SIGUSR1; //Señal a enviar
    sgev.sigev_value.sival_ptr = &timerid; //Identificador del temporizador
    timer_create(CLOCK_MONOTONIC, &sgev, &timerid); //Crear el temporizador
    its.it_interval = periodo; //Periodo del temporizador
    its.it_value.tv_sec = 0; //Tiempo inicial
    its.it_value.tv_nsec = 1; //Tiempo inicial
    timer_settime(timerid, 0, &its, NULL); //Configurar el temporizador

    sigemptyset(&sigset); //Limpiar el conjunto de señales
    sigaddset(&sigset, SIGUSR1); //Añadir la señal SIGUSR1 al conjunto

    while(1){
        sigwait(&sigset, &signum); //Esperar la señal
        printf("Tarea A [%s]\n", get_time(buf)); //Mensaje de depuración
        for(i = 0; i<ITER_A; i++){
            pthread_mutex_lock(&data->mutex); //Bloquear el mutex
            data->cnt += INC_A; //Incrementar el contador
            printf("Tarea A [%s:%d]: %d\n", pol, param.sched_priority, data->cnt); //Mensaje de depuración
            pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
            espera_activa(1); //Esperar un segundo
        }
    }
    timer_delete(timerid); //Eliminar el temporizador
    return NULL; //Retornar NULL
}

void *tarea_b (void *arg){
    const struct timespec periodo = {PERIODO_A_SEC, PERIODO_A_NSEC}; //Periodo de la tarea A
    timer_t timerid; //Identificador del temporizador
    struct sigevent sgev; //Evento de señal
    struct itimerspec its; //Estructura de tiempo del temporizador
    sigset_t sigset; //Conjunto de señales
    char buf[30]; //Buffer para la hora
    struct Data *data = arg; //Datos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int i, policy, signum; //Contador, política de planificación y número de señal
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación

    sgev.sigev_notify = SIGEV_SIGNAL; //Notificar con señal
    sgev.sigev_signo = SIGUSR1; //Señal a enviar
    sgev.sigev_value.sival_ptr = &timerid; //Identificador del temporizador
    timer_create(CLOCK_MONOTONIC, &sgev, &timerid); //Crear el temporizador
    its.it_interval = periodo; //Periodo del temporizador
    its.it_value.tv_sec = 0; //Tiempo inicial
    its.it_value.tv_nsec = 1; //Tiempo inicial
    timer_settime(timerid, 0, &its, NULL); //Configurar el temporizador

    sigemptyset(&sigset); //Limpiar el conjunto de señales
    sigaddset(&sigset, SIGUSR1); //Añadir la señal SIGUSR1 al conjunto

    while(1){
        sigwait(&sigset, &signum); //Esperar la señal
        printf("Tarea B [%s]\n", get_time(buf)); //Mensaje de depuración
        for(i = 0; i<ITER_B; i++){
            pthread_mutex_lock(&data->mutex); //Bloquear el mutex
            data->cnt += INC_B; //Incrementar el contador
            printf("Tarea B [%s:%d]: %d\n", pol, param.sched_priority, data->cnt); //Mensaje de depuración
            pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
            espera_activa(1); //Esperar un segundo
        }
    }
    timer_delete(timerid); //Eliminar el temporizador
    return NULL; //Retornar NULL
}

/*--------------------------*/
/*--Principal---------------*/
/*--------------------------*/
void usage (const char *nm){
    fprintf(stderr, "usage: %s [-h] [-ff] [-rr] [-p1] [-p2]\n", nm); //Imprimir mensaje de uso
    exit(EXIT_FAILURE); //Salir con error
}

void get_args (int argc, const char *argv[], int *policy, int *prio1, int *prio2){
    int i; //Contador
    if (argc < 2){
        usage(argv[0]); //Imprimir mensaje de uso
    }else{
        for(i = 1; i<argc; i++){
            if (strcmp(argv[i], "-h") == 0){
                usage(argv[0]); //Imprimir mensaje de uso
            }else if (strcmp(argv[i], "-ff") == 0){
                *policy = SCHED_FIFO; //Asignar la política de planificación FIFO
            }else if (strcmp(argv[i], "-rr") == 0){
                *policy = SCHED_RR; //Asignar la política de planificación RR
            }else if (strcmp(argv[i], "-p1") == 0){
                *prio1 = PRIORIDAD_A; //Asignar la prioridad de la tarea A
                *prio2 = PRIORIDAD_B; //Asignar la prioridad de la tarea B
            }else if (strcmp(argv[i], "-p2") == 0){
                *prio1 = PRIORIDAD_B; //Asignar la prioridad de la tarea B
                *prio2 = PRIORIDAD_A; //Asignar la prioridad de la tarea A
            }else{
                usage(argv[0]); //Imprimir mensaje de uso
            }
        }
    }
}

int main(int argc, const char *argv[]){
    sigset_t sigset; //Conjunto de señales
    struct Data shared_data; //Datos compartidos
    pthread_attr_t attr; //Atributos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int pcy, policy = SCHED_FIFO; //Política de planificación
    int prio0 = 1, prio1 = 1, prio2 = 1; //Prioridades de las tareas
    pthread_t t1, t2; //Hilos
    /*
    * La tarea principal debe tener la mayor prioridad, para poder
    * crear todas las tareas necesarias.    
    */
    get_args(argc, argv, &policy, &prio1, &prio2); //Obtener los argumentos
    prio0 = (prio1 > prio2 ? prio1 : prio2) + 1; //Asignar la mayor prioridad más 1 a la tarea principal

    mlockall(MCL_CURRENT | MCL_FUTURE); //Bloquear la memoria
    pthread_getschedparam(pthread_self(), &pcy, &param); //Obtener la política de planificación y los parámetros
    param.sched_priority = prio0; //Establecer la prioridad
    pthread_setschedparam(pthread_self(), policy, &param); //Establecer la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación

    /*
    * La tarea principal debe bloquear todas las señales con las
    * que se trabaja. Las tareas hijas heredarán la máscara de bloqueo.
    */
    sigemptyset(&sigset); //Limpiar el conjunto de señales
    sigaddset(&sigset, SIGUSR1); //Añadir la señal SIGUSR1 al conjunto
    sigaddset(&sigset, SIGUSR2); //Añadir la señal SIGUSR2 al conjunto
    pthread_sigmask(SIG_BLOCK, &sigset, NULL); //Bloquear las señales

    shared.data.cnt = 0; //Inicializar el contador
    pthread_mutex_init(&shared_data.mutex, NULL); //Inicializar el mutex

    pthread_attr_init(&attr); //Inicializar los atributos de la tarea
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); //Establecer la herencia de la planificación
    pthread_attr_setschedpolicy(&attr, policy); //Establecer la política de planificación
    param.sched_priority = prio1; //Establecer la prioridad
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de planificación
    pthread_create(&t1, &attr, tarea_a, &shared_data); //Crear la tarea A
    param.sched_priority = prio2; //Establecer la prioridad
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de planificación
    pthread_create(&t2, &attr, tarea_b, &shared_data); //Crear la tarea B
    pthread_attr_destroy(&attr); //Destruir los atributos de la tarea

    printf("Tarea principal [%s:%d]\n", pol, prio0); //Mensaje de depuración

    pthread_join(t1, NULL); //Esperar a la tarea A
    pthread_join(t2, NULL); //Esperar a la tarea B
    pthread_mutex_destroy(&shared_data.mutex); //Destruir el mutex
    return 0;
}
    