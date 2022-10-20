#include "csapp.h"
#include "sbuf.h"

/* Create an empty, bounded, shared FIFO buffer with n slots */

void sbuf_init(sbuf_t *sp, int n) {
  sp->buf = Calloc(n, sizeof(int));
  sp->n = n;                       /* Buffer holds max of n items */
  sp->slot_num = n;
  sp->item_num = 0;
  sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
  // Sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */
  // Sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
  // Sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
  pthread_mutex_init(&sp->mutex, NULL);
  pthread_cond_init(&sp->slots, NULL);
  pthread_cond_init(&sp->items, NULL);
}

/* Clean up buffer sp */

void sbuf_deinit(sbuf_t *sp) {
  Free(sp->buf);
  pthread_mutex_destroy(&sp->mutex);
  pthread_cond_destroy(&sp->slots);
  pthread_cond_destroy(&sp->items);
}

/* Insert item onto the rear of shared buffer sp */

void sbuf_insert(sbuf_t *sp, int item) { //slot - 1; item + 1
  // P(&sp->slots);                   /* Wait for available slot */
  // P(&sp->mutex);                   /* Lock the buffer */
  //sp->buf[(++sp->rear)%(sp->n)] = item; /* Insert the item */
  // V(&sp->mutex);                   /* Unlock the buffer */
  // V(&sp->items);                   /* Announce available item */
  pthread_mutex_lock(&sp->mutex);
  while (sp->slot_num == 0) { // wait until sp->slot_num - 1 >= 0;
    pthread_cond_wait(&sp->slots, &sp->mutex);
  }
  sp->buf[(++sp->rear)%(sp->n)] = item;  /* Insert the item */
  sp->slot_num--;
  sp->item_num++;
  pthread_cond_signal(&sp->items); // wake up
  pthread_mutex_unlock(&sp->mutex);  
}

/* Remove and return the first item from buffer sp */

int sbuf_remove(sbuf_t *sp) { //slot + 1; item - 1
  int item;
  // P(&sp->items);                   /* Wait for available item */
  // P(&sp->mutex);                   /* Lock the buffer */
  //item = sp->buf[(++sp->front)%(sp->n)]; /* Remove the item */
  // V(&sp->mutex);                   /* Unlock the buffer */
  // V(&sp->slots);                   /* Announce available slot */
  pthread_mutex_lock(&sp->mutex);
  while (sp->item_num == 0) { // wait until sp->item_num - 1 >= 0;
    pthread_cond_wait(&sp->items, &sp->mutex);
  }
  item = sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
  sp->slot_num++;
  sp->item_num--;
  pthread_cond_signal(&sp->slots);  // wake up
  pthread_mutex_unlock(&sp->mutex);
  return item;
}

