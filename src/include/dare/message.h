#ifndef MESSAGE_H
#define MESSAGE_H
#include <sys/queue.h>

struct tailq_entry_t {
	uint8_t type;
	uint16_t connection_id;
	uint64_t req_id;
	ssize_t data_size;
	char data[1024]; //TODO
	TAILQ_ENTRY(tailq_entry_t) entries;     /* Tail queue. */
};
typedef struct tailq_entry_t tailq_entry_t;

TAILQ_HEAD(, tailq_entry_t) tailhead;

pthread_spinlock_t tailq_lock;

#endif