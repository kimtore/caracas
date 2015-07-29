#include <zmq.h>
#include <stdlib.h>
#include <syslog.h>

#define PUBLISHER "tcp://0.0.0.0:9090"
#define SUBSCRIBER "tcp://0.0.0.0:9080"

int main(int argc, char *argv[])
{
    void * context;
    void * sub;
    void * pub;

    /* Syslog initialization */
    openlog("proxy", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "ZeroMQ message proxy initializing.");

    /* Create ZeroMQ context */
    context = zmq_ctx_new();
    if (!context) {
        syslog(LOG_EMERG, "Failed to create ZeroMQ context: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    /* Create ZeroMQ subscriber socket */
    sub = zmq_socket(context, ZMQ_XSUB);
    if (!sub) {
        syslog(LOG_EMERG, "Failed to create ZeroMQ subscriber socket: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    /* Create ZeroMQ publisher socket */
    pub = zmq_socket(context, ZMQ_XPUB);
    if (!pub) {
        syslog(LOG_EMERG, "Failed to create ZeroMQ publisher socket: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    /* Bind ZeroMQ subscriber socket */
    if (zmq_bind(sub, SUBSCRIBER) == -1) {
        syslog(LOG_EMERG, "Failed to bind ZeroMQ subscriber socket: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }
    syslog(LOG_INFO, "Started ZeroMQ XSUB socket on %s", SUBSCRIBER);

    /* Bind ZeroMQ publisher socket */
    if (zmq_bind(pub, PUBLISHER) == -1) {
        syslog(LOG_EMERG, "Failed to bind ZeroMQ publisher socket: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }
    syslog(LOG_INFO, "Started ZeroMQ XPUB socket on %s", PUBLISHER);

    /* Finally, start the ZeroMQ proxy. This function will always return -1. */
    syslog(LOG_INFO, "ZeroMQ message proxy started.");
    zmq_proxy(sub, pub, NULL);
    syslog(LOG_INFO, "ZeroMQ message proxy terminating: %s", zmq_strerror(errno));

    return 0;
}
