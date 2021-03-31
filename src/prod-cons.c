#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define QUEUESIZE 10
#define LOOP 20
#define num_p 8 //num_p(2,8)
#define num_c 8 //num_c(2,8)

void *producer (void *args);
void *consumer (void *args);

typedef struct {
  struct workFunction * buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

//New fifo items
struct workFunction {
  void * (*work)(void *);
  void * arg;
};

//void func to be used from struct.work
void * print(int arg) //TODO ALLAXE TO TI KANEI
{
  // printf("Ειμαι ο Γιωτο no.%d!\n", arg);
}
//TODO ALLAXE REPO ONOMA SE EPETIT
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, struct workFunction* in);
void queueDel (queue *q);

int main ()
{
  queue *fifo;
  pthread_t* pro = (pthread_t *)malloc(sizeof(pthread_t) * num_p);
  pthread_t* con = (pthread_t *)malloc(sizeof(pthread_t) * num_c);

  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }
  
  for(int j = 0; j < num_p; j++)
    pthread_create (&pro[j], NULL, producer, fifo);
  for(int k = 0; k < num_c; k++)
    pthread_create (&con[k], NULL, consumer, fifo);
  
  for(int j = 0; j < num_p; j++)
    pthread_join (pro[j], NULL);
  for(int k = 0; k < num_c; k++)
    pthread_join (con[k], NULL);
  queueDelete (fifo);

  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  fifo = (queue *)q;

  for (int i = 0; i < LOOP; i++) {

    struct workFunction * _wrkF = (struct workFunction *)malloc(sizeof(struct workFunction)); //creating a pointer object named "_func". Typecasted to my created struct
    _wrkF->work = (void *)print;
    _wrkF->arg = malloc(sizeof(int));

    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    queueAdd (fifo, _wrkF);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
    printf ("producer: sent %d.\n", i); //TODO ΕΛΛΙΠΕΣ
  }
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  fifo = (queue *)q;

  // struct workFunction *d; 
 //TODO while1
  for (int i = 0; i < LOOP; i++) {
    
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    queueDel (fifo);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
    printf ("consumer: recieved %d.\n", i); //TODO ΕΛΛΙΠΕΣ
  }
  return (NULL);
}

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, struct workFunction* in ) //alazw
{
  q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;  
  q->empty = 0;

  return;
}

void queueDel (queue *q)
{
  struct workFunction* out = q->buf[q->head];
  ((void(*)())out->work)(*(int *)out->arg); //this is how it runs, thanks stack

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}
