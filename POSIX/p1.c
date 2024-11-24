/*---------------------------------*/
/* Tareas con prioridades y planificador*/
/*---------------------------------*/
#include <stdlib.h> // Para usar exit
#include <stddef.h> // Para usar NULL
#include <assert.h> // Para usar assert
#include <string.h> // Para usar strcmp y strcpy
#include <stdio.h> // Para usar printf

#include <unistd.h> // Para usar fork
#include <time.h> // Para usar time
#include <errno.h> // Para usar errno
#include <sched.h> // Para usar sched_yield
#include <pthread.h> // Para usar pthread_create, pthread_join, pthread_exit
#include <sys/mmam.h> // Para usar mmap, munmap. Da error porque solo funciona en sistemas operativos de tiempo real

#define PRIORIDAD_A 24 // Prioridad de la tarea A. Debe ser mayor que la prioridad de la tarea B
#define ITER_A 40 // Número de iteraciones de la tarea A
#define INC_A 1 // Incremento de la tarea A

#define PRIORIDAD_B 26 // Prioridad de la tarea B. Debe ser menor que la prioridad de la tarea A
#define ITER_B 40 // Número de iteraciones de la tarea B
#define INC_B 1 // Incremento de la tarea B

struct Data{ // Estructura de datos para pasar a las tareas
    pthread_mutex_t mutex; // Mutex para sincronizar la salida
    int cnt; // Contador de la tarea
};
/*---------------------------------*/
/*
* Nota: los "printf" están con el objetivo de depuración, para que el
* alumno pueda analizar el comportamiento de las tareas.
*/
/*---------------------------------*/
/*--Soporte------------------------*/
/*---------------------------------*/
#define CHKN(syscall) // Macro para verificar errores en llamadas al sistema
    do{
        int err = syscall; // Llamada al sistema
        if (err != 0){ //Si hay error
            fprintf(stderr, "%s:%d: SysCall Error: %s\n", __FILE__, __LINE__, strerror(errno)); // Mensaje de error. strerror convierte el código de error en una cadena. errno es una variable global que contiene el código de error. __FILE__ y __LINE__ son macros que contienen el nombre del archivo y el número de línea respectivamente. stderr es el flujo de error estándar. %s es un marcador de posición para una cadena y %d para un entero.
            exit(EXIT_FAILURE); // Salir con error
        }
    } while (0) // Hacer mientras 0. Se usa para que la macro se comporte como una función.
/*---------------------------------*/
#define CHKE(syscall) // Macro para verificar errores en llamadas a funciones de la biblioteca estándar
    do{
        int err = syscall; // Llamada a la función
        if (err != 0){ // Si hay error
            fprintf(stderr, "%s:%d: Error: %s\n", __FILE__, __LINE__, strerror(err)); // Mensaje de error. strerror convierte el código de error en una cadena. __FILE__ y __LINE__ son macros que contienen el nombre del archivo y el número de línea respectivamente. stderr es el flujo de error estándar. %s es un marcador de posición para una cadena y %d para un entero. err es el código de error.
            exit(EXIT_FAILURE); // Salir con error
        }
    }
    
    
