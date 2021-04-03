#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define QUEUESIZE 10
#define PROD_WRK 30
#define num_p 8 //num_p(2,8)
#define num_c 8 //num_c(2,8)

int prod_counter = 0;
int cons_counter = 0;
int cntr = 0;

struct timeval tmr;

long int waitTimeSum = 0; //TODO
//addition
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
  //εδω παιρνει τιμη double t0 kronos empala
  double t0;
  void *(*work)(void *);
  void *arg;
};

//void func to be used from struct.work
void *print(int arg) { 
  // printf("Ειμαι ο Γιωτο no.%d!\n", arg); 
}

//TODO ALLAXE REPO ONOMA SE EPETIT

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

  for (int j = 0; j < num_p; j++)
    pthread_create(&pro[j], NULL, producer, fifo);
  for (int k = 0; k < num_c; k++)
    pthread_create(&con[k], NULL, consumer, fifo);

  for (int j = 0; j < num_p; j++)
    pthread_join(pro[j], NULL);

  //To check that producers are done, and thus the while(1) loop can end
  while (1) {
    pthread_cond_broadcast(fifo->notEmpty);
    if (cons_counter == num_c)
      printf("Broadcast is done all consumers are unblocked!");
    break;
  }
  for (int k = 0; k < num_c; k++)
    pthread_join(con[k], NULL);

  queueDelete(fifo);

  //printing sum
  double avg = (double) waitTimeSum / (double)cntr;
  printf("\n\nAverage runtime per thread: %ld", waitTimeSum);
  return 0;
}

void *producer(void *q) {
  queue *fifo;
  fifo = (queue *)q;

  for (int i = 0; i < PROD_WRK; i++){

    struct workFunction *_wrkF = (struct workFunction *)malloc(sizeof(struct workFunction)); //creating a pointer object named "_func". Typecasted to my created struct
    _wrkF->work = (void *)print;
    _wrkF->arg = malloc(sizeof(int));

    pthread_mutex_lock(fifo->mut);
    while (fifo->full){
      printf ("producer: queue FULL.\n");
      pthread_cond_wait(fifo->notFull, fifo->mut);
    }
    queueAdd(fifo, _wrkF);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notEmpty);
    printf ("producer: sent %d.\n", i); //TODO ΕΛΛΙΠΕΣ
  }
  prod_counter++;
  printf("Bye bye from producer  no.%d\n", prod_counter);
  
  return (NULL);
}

void *consumer(void *q) {
  queue *fifo;
  fifo = (queue *)q;

  while (1) {
    pthread_mutex_lock(fifo->mut);
    while (fifo->empty) {
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait(fifo->notEmpty, fifo->mut);
      if (fifo->empty && prod_counter == num_p){ //Meaning: if the FIFO queue is empty AND all producers are done, then continue running the code.
        cons_counter++;
        pthread_mutex_unlock(fifo->mut);
        printf("Bye bye from consumer  no.%d\n", cons_counter);

        return (NULL);
      }
    }
    queueDel(fifo);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notFull);
  }
  cons_counter++;
  printf("Bye bye from consumer  no.%d\n", cons_counter);

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

void queueAdd(queue *q, struct workFunction *in) {//alazw 
  
  q->buf[q->tail] = in;

  in->t0 = tmr.tv_sec * 1e6;
  in->t0 = (in->t0 + tmr.tv_usec) * 1e-6;

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

  //------------------------------------------time
  // gettimeofday (&t2, NULL);
  double delTime = tmr.tv_sec * 1e6;
  delTime = (delTime + tmr.tv_usec) * 1e-6; //pernw krno

  double waitTime = delTime - out->t0;
  waitTimeSum += waitTime; 
  cntr++;
  //time ------------------------------------------

  ((void (*)())out->work)(*(int *)out->arg); //this is how it runs, thanks stack


  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;
  
  free(out); //Freeing some memory
  
  return;
}
