
#ifndef QUEUE_H
#define QUEUE_H

#include "list-api.h"

typedef void queue;

 int  enqueue(void **q, void *dp);
void *dequeue(void **q);

#endif
