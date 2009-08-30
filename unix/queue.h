#ifndef __POSIX_SIGNAL_QUEUE_H

typedef void * QueueEntry;

typedef void (*QueueSetNextProc) (QueueEntry newEntry,
				  QueueEntry nextEntry);

typedef QueueEntry (*QueueGetNextProc) (QueueEntry entry);

typedef struct {
    QueueEntry headPtr;
    QueueEntry tailPtr;
    QueueSetNextProc setNextProc;
    QueueGetNextProc getNextProc;
} Queue;

MODULE_SCOPE
void
InitQueue (
    Queue *queuePtr,
    QueueSetNextProc setNextProc,
    QueueGetNextProc getNextProc);

MODULE_SCOPE
void
QueuePush (
    Queue *queuePtr,
    QueueEntry entry);

MODULE_SCOPE
QueueEntry
QueuePop (
    Queue *queuePtr);

#define __POSIX_SIGNAL_QUEUE_H
#endif /* __POSIX_SIGNAL_QUEUE_H */

/* vim: set ts=8 sts=4 sw=4 sts=4 noet: */
