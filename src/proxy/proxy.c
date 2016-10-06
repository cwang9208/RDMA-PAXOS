#include "../include/proxy/proxy.h"
#include "../include/replica-sys/node.h"

void proxy_on_read(proxy_node* proxy, void* buf, ssize_t ret, int fd)
{

}

proxy_node* proxy_init()
{
    system_initialize();
    return NULL;
}