/*------------------------------------------------------------------------*/
/* Tareas periódicas con relojes de tiempo real, prioridades y planif.
*/
/*------------------------------------------------------------------------*/
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

#define PERIODO_A_SEC 2
#define PERIODO_A_NSEC 0
#define PRIORIDAD_A 24


#define PERIODO_B_SEC 3
#define PERIODO_B_NSEC 0
#define PRIORIDAD_B 22


#define PERIODO_C_SEC 4
#define PERIODO_C_NSEC 0
#define PRIORIDAD_C 20

#define MIN 0
#define MAX 100

struct Data {
	pthread_mutex_t mutexA;
    int tempA;
    pthread_mutex_t mutexB;
    int tempB;
};

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


/*------------------------------------------------------------------------*/
void addtime(struct timespec* tm, const struct timespec* val)
{
	tm->tv_sec += val->tv_sec;
	tm->tv_nsec += val->tv_nsec;
	if (tm->tv_nsec >= 1000000000L) {
		tm->tv_sec += (tm->tv_nsec / 1000000000L);
		tm->tv_nsec = (tm->tv_nsec % 1000000000L);
	}
}
/*------------------------------------------------------------------------*/
/*-- Tareas --------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
void* tarea_a(void* arg)
{
	const struct timespec periodo = { PERIODO_A_SEC, PERIODO_A_NSEC };
	struct timespec next;
	struct Data* data = arg;

	CHKN(clock_gettime(CLOCK_MONOTONIC, &next));

	while (1) {
		CHKE(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL));

        pthread_mutex_lock(&data->mutexA);

        float temprand = MIN + (rand() % (MAX - MIN + 1));
		printf("D1-TEMP RANDOM: %f\n", temprand);

        data->tempA = temprand;

        if(data->tempA < 0.8 * MAX){
            printf("D1-TEMPERATURA OPTIMA\n");
        }
        if(data->tempA > 0.8 * MAX && data->tempA < 0.9 * MAX){
            printf("D1-TEMPERATURA ALTA. PUEDE AFECTAR A LA CALIDAD DEL PRODUCTO\n");
        }
        if(data->tempA > 0.9 * MAX){
            printf("D1-Esta Temperatura afecta a la calidad del producto\n");
        }

        pthread_mutex_unlock(&data->mutexA);
        addtime(&next, &periodo);
	}
	return NULL;
}
/*------------------------------------------------------------------------*/
void* tarea_b(void* arg)
{
	const struct timespec periodo = { PERIODO_B_SEC, PERIODO_B_NSEC };
	struct timespec next;
	struct Data* data = arg;

	CHKN(clock_gettime(CLOCK_MONOTONIC, &next));

	while (1) {
		CHKE(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL));

        pthread_mutex_lock(&data->mutexB);

        float temprand = MIN + (rand() % (MAX - MIN + 1));
		printf("D2-TEMP RANDOM: %f\n", temprand);

        data->tempB = temprand;

        if(data->tempB < 0.8 * MAX){
            printf("D2-TEMPERATURA OPTIMA\n");
        }
        if(data->tempB > 0.8 * MAX && data->tempB < 0.9 * MAX){
            printf("D2-TEMPERATURA ALTA. PUEDE AFECTAR A LA CALIDAD DEL PRODUCTO\n");
        }
        if(data->tempB > 0.9 * MAX){
            printf("D2-Esta Temperatura afecta a la calidad del producto\n");
        }

        pthread_mutex_unlock(&data->mutexB);
        addtime(&next, &periodo);
	}
	return NULL;
}

void* tarea_c(void* arg)
{
	const struct timespec periodo = { PERIODO_C_SEC, PERIODO_C_NSEC };
	struct timespec next;
	struct Data* data = arg;

	CHKN(clock_gettime(CLOCK_MONOTONIC, &next));
    	
    while (1) {
		CHKE(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL));
		pthread_mutex_lock(&data->mutexA);
        pthread_mutex_lock(&data->mutexB);
        printf("Temperatura de A - %d\n", data->tempA);
        printf("Temperatura de B - %d\n", data->tempB);
        pthread_mutex_unlock(&data->mutexB);
        pthread_mutex_unlock(&data->mutexA);
        addtime(&next, &periodo);
	}
	return NULL;
}

/*------------------------------------------------------------------------*/
/*-- Programa Principal --------------------------------------------------*/
/*------------------------------------------------------------------------*/
int main(int argc, const char* argv[])
{
	struct Data data;
	pthread_attr_t attr;
	struct sched_param param;
	pthread_t t1, t2, t3;
	
	CHKN(mlockall(MCL_CURRENT | MCL_FUTURE));
	data.tempA = 0;
	data.tempB = 0;
	srand(time(NULL));
	
	CHKE(pthread_mutex_init(&data.mutexA, NULL));
    CHKE(pthread_mutex_init(&data.mutexB, NULL));

	CHKE(pthread_attr_init(&attr));
	CHKE(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
	CHKE(pthread_attr_setschedpolicy(&attr, SCHED_FIFO));

	param.sched_priority = PRIORIDAD_A;	
	CHKE(pthread_attr_setschedparam(&attr, &param));
	CHKE(pthread_create(&t1, &attr, tarea_a, &data));

	param.sched_priority = PRIORIDAD_B;
	CHKE(pthread_attr_setschedparam(&attr, &param));
	CHKE(pthread_create(&t2, &attr, tarea_b, &data));

	param.sched_priority = PRIORIDAD_C;
	CHKE(pthread_attr_setschedparam(&attr, &param));
	CHKE(pthread_create(&t3, &attr, tarea_c, &data));
	

	CHKE(pthread_join(t1, NULL));
	CHKE(pthread_join(t2, NULL));
	CHKE(pthread_join(t3, NULL));

	CHKE(pthread_attr_destroy(&attr));

	CHKE(pthread_mutex_destroy(&data.mutexA));
    CHKE(pthread_mutex_destroy(&data.mutexB));
	return 0;
}
/*------------------------------------------------------------------------*/
