#include <stddef.h>
#include "queue.h"

void
InitQueue (
    Queue *queuePtr,
    QueueSetNextProc setNextProc,
    QueueGetNextProc getNextProc)
{
    queuePtr->headPtr = NULL;
    queuePtr->tailPtr = NULL;
    queuePtr->setNextProc = setNextProc;
    queuePtr->getNextProc = getNextProc;
}

void
QueuePush (
    Queue *queuePtr,
    QueueEntry entry)
{
    if (queuePtr->tailPtr == NULL) {
	queuePtr->headPtr = entry;
    } else {
	queuePtr->setNextProc(queuePtr->tailPtr, entry);
    }
    queuePtr->tailPtr = entry;
    queuePtr->setNextProc(entry, NULL);
}

QueueEntry
QueuePop (
    Queue *queuePtr)
{
    QueueEntry entry;

    entry = queuePtr->headPtr;
    if (entry != NULL) {
	queuePtr->headPtr = queuePtr->getNextProc(entry);
	if (queuePtr->headPtr == NULL) {
	    queuePtr->tailPtr = NULL;
	}
    }

    return entry;
}

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
