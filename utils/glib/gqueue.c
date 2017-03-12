/*
 * pkg-config - Return metainformation about installed libraries
 * 
 * $ pkg-config --cflags glib-2.0
 * $ pkg-config --libs glib-2.0
 *
 * $ cc `pkg-config --cflags --libs glib-2.0` gqueue.c -o gqueue
 */

#include <glib.h>

GQueue pkt_queue;

typedef struct Packet {
    void *data;
    int size;
    struct timeval created_time;
} Packet;

void *g_memdup(const void *ptr, unsigned size)
{
    unsigned char *dup;
    unsigned i;

    if (!ptr) {
        return NULL;
    }

    dup = g_malloc(size);
    for (i = 0; i < size; i++)
        dup[i] = ((unsigned char *)ptr)[i];
    return dup;
}

Packet *packet_new(const void *data, int size)
{
    Packet *pkt = g_slice_new(Packet);

    pkt->data = g_memdup(data, size);
    pkt->size = size;
    gettimeofday(&pkt->created_time, NULL);
    return pkt;
}

static int packet_enqueue(uint8_t *buf, uint32_t packet_len)
{
	Packet *pkt = NULL;

	pkt = packet_new(buf, packet_len);

	// mutex_lock
	g_queue_push_tail(&pkt_queue, pkt);
	// mutex_unlock
}

void packet_destroy(Packet *pkt)
{
    g_free(pkt->data);
    g_slice_free(Packet, pkt);
}

static void packet_dequeue()
{
    Packet *pkt = NULL;

    // mutex_lock
    while (!g_queue_is_empty(&pkt_queue)) {
        pkt = g_queue_pop_head(&pkt_queue);

        packet_destroy(pkt);
    }
    // mutex_unlock
}

int main(int argc, char const *argv[])
{
    g_queue_init(&pkt_queue);
    // mutex_init
	return 0;
}