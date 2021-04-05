#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#define QUEUESIZE 10
#define PROD_WRK 20
#define num_p 8 //Number of producers to create
#define num_c 8 //Number of consumers to create

int prod_counter = 0;
int cons_counter = 0;

int cntr = 0; //Cycle counter

struct timeval tmr;
double waitTimeSum = 0;

FILE * fp;

void *producer(void *args);
void *consumer(void *args);

typedef struct {
  struct workFunction *buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

//New fifo items
struct workFunction {
  double t0; //Variable to measure time
  void *(*work)(void *);
  void *arg;
};

//void func to be used from struct workFunction
void * w_func(int arg) {
  int exp = 3; //Power of the number to calculate
  int result = 1;
  int base = (rand() % (100 - 1 + 1)) + 1; //Get a random number from 1-100
  while(exp != 0) { //Calculate its power
    result *= base;
    --exp;
  }
  printf("Cycle no.%d completed!\n", arg);
}

queue *queueInit(void);
void queueDelete(queue *q);
void queueAdd(queue *q, struct workFunction *in);
void queueDel(queue *q);

int main() {
  queue *fifo;
  pthread_t *pro = (pthread_t *)malloc(sizeof(pthread_t) * num_p);
  pthread_t *con = (pthread_t *)malloc(sizeof(pthread_t) * num_c);

  fifo = queueInit();
  if (fifo == NULL){
    fprintf(stderr, "main: Queue Init failed.\n");
    exit(1);
  }
  //Creating file to save time data
  char location[] = ""; //Change accordingly
  fp = fopen(location, "w");
  printf("Created file!\n");

  for (int j = 0; j < num_p; j++)
    pthread_create(&pro[j], NULL, producer, fifo);
  for (int k = 0; k < num_c; k++)
    pthread_create(&con[k], NULL, consumer, fifo);

  for (int j = 0; j < num_p; j++)
    pthread_join(pro[j], NULL);

  //To check that producers are done, and thus the while(1) loop can end
  while (1) {
    pthread_cond_broadcast(fifo->notEmpty);
    if (cons_counter == num_c){
      printf("Broadcast is done, all consumers are unblocked!");
      break;
    }
  }

  for (int k = 0; k < num_c; k++)
    pthread_join(con[k], NULL);

  queueDelete(fifo);

  /*----------------Exporting info-----------------------*/
  double avg = (double) waitTimeSum / (PROD_WRK *num_c);
  fprintf(fp,"Total runtime: %f\n", waitTimeSum);
  fprintf(fp,"Average runtime: %f\n", avg);
  fprintf(fp,"Queue size: %d\n", QUEUESIZE);
  fprintf(fp,"Work per producer: %d\n", PROD_WRK);
  fprintf(fp,"Number of producer threads: %d\n", num_p);
  fprintf(fp,"Number of consumer threads: %d\n", num_c);

  fclose(fp);
  /*-----------------------------------------------------*/

  printf("\n\nAverage runtime per thread: %f\nTotal runtimes: %d", avg, cntr);

  return 0;
}

void *producer(void *q) {
  queue *fifo;
  fifo = (queue *)q;

  for (int i = 0; i < PROD_WRK; i++){

    struct workFunction *_wrkF = (struct workFunction *)malloc(sizeof(struct workFunction)); //creating a pointer object named "_wrkF". Typecasted to my created struct
    _wrkF->work = (void *)w_func;
    _wrkF->arg = malloc(sizeof(int));
    *((int*)_wrkF->arg) = ++cntr;

    pthread_mutex_lock(fifo->mut);
    while (fifo->full){
      // printf ("producer: queue FULL.\n");
      pthread_cond_wait(fifo->notFull, fifo->mut);
    }
    queueAdd(fifo, _wrkF);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notEmpty);
  }
  prod_counter++;
  
  return (NULL);
}

void *consumer(void *q) {
  queue *fifo;
  fifo = (queue *)q;

  while (1) {
    pthread_mutex_lock(fifo->mut);
    while (fifo->empty) {
      // printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait(fifo->notEmpty, fifo->mut);
      if (fifo->empty && prod_counter == num_p){ //Meaning: if the FIFO queue is empty AND all producers are done, then continue running the code.
        cons_counter++;
        pthread_mutex_unlock(fifo->mut);
        return (NULL);
      }
    }
    queueDel(fifo);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notFull);
  }
  cons_counter++;

  return (NULL);
}

queue *queueInit(void) {
  queue *q;

  q = (queue *)malloc(sizeof(queue));
  if (q == NULL)
    return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(q->mut, NULL);
  q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notEmpty, NULL);

  return (q);
}

void queueDelete(queue *q) {
  pthread_mutex_destroy(q->mut);
  free(q->mut);
  pthread_cond_destroy(q->notFull);
  free(q->notFull);
  pthread_cond_destroy(q->notEmpty);
  free(q->notEmpty);
  free(q);
}

void queueAdd(queue *q, struct workFunction *in) {
  
  q->buf[q->tail] = in;

  /*-------------------------time--------------------------*/
  gettimeofday (&tmr, NULL);
  in->t0 = tmr.tv_sec * 1e6;
  in->t0 = (in->t0 + tmr.tv_usec) * 1e-6;
  /*-------------------------------------------------------*/

  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel(queue *q) {
  struct workFunction *out = q->buf[q->head];

  /*-------------------------time--------------------------*/
  gettimeofday (&tmr, NULL);
  double delTime = tmr.tv_sec * 1e6;
  delTime = (delTime + tmr.tv_usec) * 1e-6;
  double waitTime = delTime - out->t0;
  fprintf(fp, "%f\n", waitTime); //Add every execution time
  waitTimeSum += waitTime; 
  /*-------------------------------------------------------*/

  ((void (*)())out->work)(*(int *)out->arg);

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;
  
  free(out); //Freeing memory
  
  return;
}
