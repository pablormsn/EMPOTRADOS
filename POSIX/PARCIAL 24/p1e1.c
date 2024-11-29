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

#define PERIODO_TMP_SEG 1
#define PERIODO_TMP_NSEG 0
#define INC_TMP 1
#define DEC_TMP 1
#define PRIO_TMP 12
#define MIN 20
#define MAX 40

#define PERIODO_CTR_SEG 3
#define PERIODO_CTR_NSEG 0
#define PRIO_CTR 11
#define ACTIVO 1
#define INACTIVO 0

#define PERIODO_MTR_SEG 4
#define PERIODO_MTR_NSEG 0
#define PRIO_MTR 10

struct Data{
  pthread_mutex_t mutex;
  float valTmp;
  int chorroF;
  int chorroC;
  float tmpNatu;
};

void addtime(struct timespec *tm, const struct timespec *val){
  tm->tv_sec += val->tv_sec;
  tm->tv_nsec += val->tv_nsec;
  if(tm->tv_nsec > 1000000000L){
    tm->tv_sec += (tm->tv_nsec / 1000000000L);
    tm->tv_nsec = (tm->tv_nsec % 1000000000L);
  }
}

void iniTmp(struct Data *data){
  data->chorroF = INACTIVO;
  data->chorroC = INACTIVO;
  data->valTmp = 30.0;
  pthread_mutex_init(&data->mutex, NULL);
}

void *tareaTmp(void *arg){
  const struct timespec periodo = {PERIODO_TMP_SEG, PERIODO_TMP_NSEG};
  struct timespec next;
  struct Data *data = arg;
  clock_gettime(CLOCK_MONOTONIC, &next);

  while(1){
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    addtime(&next, &periodo);
    pthread_mutex_lock(&data->mutex);

    data->tmpNatu = rand() % (MAX + 1 - MIN) + MIN;

    if(data->chorroF == ACTIVO){
      data->valTmp -= DEC_TMP;
      printf("Chorro frio activo. Reduciendo temperatura\n");
    }else if(data->chorroC == ACTIVO){
      data->valTmp += INC_TMP;
      printf("Chorro caliente activo. Aumentando temperatura\n");
    }else if(data->valTmp < data->tmpNatu){
      data->valTmp += INC_TMP;
      printf("Aumentando temperatura\n");
    }else if (data->valTmp > data->tmpNatu){
      data->valTmp -= DEC_TMP;
      printf("Reduciendo temperatura\n");
    }
    //COMPROBAR IF

    pthread_mutex_unlock(&data->mutex);
  }
  return NULL;
}

void *tareaCtr(void *arg){
  const struct timespec periodo = {PERIODO_CTR_SEG, PERIODO_CTR_NSEG};
  struct timespec next;
  struct Data *data = arg;
  clock_gettime(CLOCK_MONOTONIC, &next);

  while(1){
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    addtime(&next, &periodo);
    pthread_mutex_lock(&data->mutex);

    if(data->valTmp < 25){
      data->chorroC = ACTIVO;
    }else{
      data->chorroC = INACTIVO;
    }

    if(data->valTmp > 35){
      data->chorroF = ACTIVO;
    }else{
      data->chorroF = INACTIVO;
    }

    pthread_mutex_unlock(&data->mutex);
  }
  return NULL;
}

void *tareaMtr(void *arg){
  const struct timespec periodo = {PERIODO_MTR_SEG, PERIODO_MTR_NSEG};
  struct timespec next;
  struct Data *data = arg;
  clock_gettime(CLOCK_MONOTONIC, &next);

  while(1){
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    addtime(&next, &periodo);
    pthread_mutex_lock(&data->mutex);

    printf("La temperatura actual es: %f\n", data->valTmp);
    printf("La temperatura natural es: %f\n", data->tmpNatu);
    printf("El estado del chorro de agua fria es: %s\n", data->chorroF==0 ? "INACTIVO" : "ACTIVO");
    printf("El estado del chorro de agua caliente es: %s\n", data->chorroC==0 ? "INACTIVO" : "ACTIVO");

    pthread_mutex_unlock(&data->mutex);
  }
  return NULL;
}

int main(int argc, const char *argv[]){
  struct Data data;
  pthread_attr_t attr;
  struct sched_param param;

  int prio0=1, prio1=PRIO_TMP, prio2=PRIO_CTR, prio3=PRIO_MTR;
  pthread_t t1, t2, t3;

  mlockall(MCL_CURRENT | MCL_FUTURE);
  prio0 = prio1 + 1;
  param.sched_priority = prio0;
  pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

  srand(time(NULL));
  iniTmp(&data);

  pthread_attr_init(&attr);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

  param.sched_priority = prio1;
  pthread_attr_setschedparam(&attr, &param);
  pthread_create(&t1, &attr, tareaTmp, &data);

  param.sched_priority = prio2;
  pthread_attr_setschedparam(&attr, &param);
  pthread_create(&t2, &attr, tareaCtr, &data);

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