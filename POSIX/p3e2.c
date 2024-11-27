#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>

#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>

#define SIG_FIN     (SIGRTMIN) // Señal para finalizar la tarea
#define SIG_BAT     (SIGRTMIN+1) // Señal para el control de la batería
#define SIG_EVNT    (SIGRTMIN+2) // Señal para la llegada de eventos del exterior
#define SIG_LLM     (SIGRTMIN+3) // Señal para la petición de llamada
#define SIG_SMS     (SIGRTMIN+4) // Señal para la petición de SMS
#define SIG_MTR     (SIGRTMIN+5) // Señal para la monitorización de la carga de la batería

#define PERIODO_BAT_SEC 1 // Periodo de la tarea
#define PERIODO_BAT_NSEC 0 // Periodo de la tarea
#define PRIORIDAD_BAT 28 // Prioridad de la tarea
#define BAT_VALOR_INI 100 // Valor inicial de la batería (100%)
#define BAT_DEC 1 // Decremento de la batería

#define PRIORIDAD_TFN 26 // Prioridad de la tarea

#define PERIODO_EVN_SEC 1 // Periodo de la tarea
#define PERIODO_EVN_NSEC 0 // Periodo de la tarea
#define PRIORIDAD_EVN 24 // Prioridad de la tarea

#define PERIODO_MTR_SEC 5 // Periodo de la tarea
#define PERIODO_MTR_NSEC 0 // Periodo de la tarea
#define PRIORIDAD_MTR 22 // Prioridad de la tarea

struct Data_Bat{
    pthread_mutex_t mutex;
    int val;
};

/*--------------------------*/
/* Nota: los "printf" están con el objetivo de depuración, para que el
 * alumno pueda analizar el comportamiento de las tareas.
 */
/*--------------------------*/
/*--Soporte----------------*/
/*--------------------------*/
// #define CHKN(syscall) // Macro para verificar errores en llamadas al sistema
//     do{
//         int err = syscall; // Llamada al sistema
//         if (err != 0){ // Si hay error
//             fprintf(stderr, "%s:%d: SysCall Error: %s\n", __FILE__, __LINE__, strerror(errno)); // Mensaje de error. strerror convierte el código de error en una cadena. errno es una variable global que contiene el código de error. __FILE__ y __LINE__ son macros que contienen el nombre del archivo y el número de línea respectivamente. stderr es el flujo de error estándar. %s es un marcador de posición para una cadena y %d para un entero.
//             exit(EXIT_FAILURE); // Salir con error
//         }
//     } while (0) // Hacer mientras 0. Se usa para que la macro se comporte como una función.

// #define CHKE(syscall) // Macro para verificar errores en llamadas a funciones de la biblioteca estándar
//     do{
//         int err = syscall; // Llamada a la función
//         if (err != 0){ // Si hay error
//             fprintf(stderr, "%s:%d: Error: %s\n", __FILE__, __LINE__, strerror(err)); // Mensaje de error. strerror convierte el código de error en una cadena. __FILE__ y __LINE__ son macros que contienen el nombre del archivo y el número de línea respectivamente. stderr es el flujo de error estándar. %s es un marcador de posición para una cadena y %d para un entero. err es el código de error.
//             exit(EXIT_FAILURE); // Salir con error
//         }
//     } while (0) // Hacer mientras 0. Se usa para que la macro se comporte como una función.

/*--------------------------*/
const char *get_time (char *buf){
    time_t t = time(0); // Tiempo actual
    char *f = ctime_r(&t, buf); // Convertir el tiempo en una cadena y almacenarla en buf
    f[strlen(f)-1] = '\0'; // Eliminar el salto de línea
    return f; // Retornar la cadena
}
/*--------------------------*/
void ini_aleatorio(){
    struct timeval tv; // Estructura para el tiempo
    long seed; // Semilla
    if(gettimeofday(&tv, NULL) == 0){ // Obtener el tiempo actual
        seed = tv.tv_sec + tv.tv_usec; // Calcular la semilla
    }else{
        seed = time(0); // Si no se puede obtener el tiempo, usar el tiempo actual
    }
    srand(seed); // Inicializar el generador de números aleatorios
}


