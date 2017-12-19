#define LOG_TAG "epoxyd"
#include <utils/Log.h>

#include <binder/IEpoxy.h>
#include <binder/Parcel.h>
#include <binder/IServiceManager.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>
#include <utils/String16.h>

#include <cstdio>
#include <iostream>

#define MAXPENDING 10

using namespace android;
using namespace std;

void *handleConn(void *arg)
{
    int sock = (long) arg;

    ALOGD("Handling connection on fd %d", sock);

    for (;;) {
        size_t len, n_len;

        ALOGD("Handler on fd %d waiting on recv()...", sock);
        recv(sock, &n_len, sizeof(n_len), MSG_WAITALL);
        len = ntohl(n_len);

        char *service_buf[len];
        recv(sock, service_buf, sizeof(service_buf), MSG_WAITALL);

        String16 service = String16((const char*)service_buf, len);

        
        uint32_t code, flags;
        recv(sock, &code, sizeof(code), MSG_WAITALL);
        recv(sock, &flags, sizeof(flags), MSG_WAITALL);

        ALOGD("Handler on fd %d received request for service %s with code %d",
                sock, String8(service).string(), code);
        
        recv(sock, &n_len, sizeof(n_len), MSG_WAITALL);
        len = ntohl(n_len);

        ALOGD("Handler on fd %d receiving data of length %d", sock, len);

        Parcel data;
        data.setDataSize(len);
        recv(sock, (void *) data.data(), len, MSG_WAITALL);

        Parcel reply;

        sp < IServiceManager > sm = defaultServiceManager();
        sp < IBinder > binder = sm->getService(service);

        status_t status = binder->transact(code, data, &reply);

        if (status == NO_ERROR) {
            ALOGD("Handler on fd %d: transact succeeded", sock);
        } else {
            ALOGD("Handler on fd %d: transact failed: %s", sock, strerror(status));
        }

        len = reply.dataSize() * sizeof(*reply.data());
        n_len = htonl(len);

        send(sock, &n_len, sizeof(n_len), 0);
        send(sock, reply.data(), len, 0);
        ALOGD("Handler on fd %d finished serving epoxy transaction", sock);
    }

    // never reached
    close(sock);
    pthread_exit(NULL);
}

int runServer(unsigned short port) {
    int servSock, clntSock;
    struct sockaddr_in servAddr, clntAddr;

    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        ALOGE("Could not create listening socket: %s", strerror(errno));
        return 1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
    if (bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        ALOGE("Could not bind to socket on port %d: %s", port, strerror(errno));
        return 1;
    }

    if (listen(servSock, MAXPENDING) < 0) {
        ALOGE("Could not listen: %s", strerror(errno));
        return 1;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ALOGD("epoxyd server created on port %d fd %d", port, servSock);

    for (;;) {
        socklen_t clntLen = sizeof(clntAddr);
        if((clntSock = accept(servSock, (struct sockaddr *) &clntAddr,
                        &clntLen)) < 0) {
            ALOGE("Accept failed: %s", strerror(errno));
        }

        char *clntIP = inet_ntoa(clntAddr.sin_addr);

        ALOGI("Accepted connection from %s on fd %d", clntIP, clntSock);
        
        pthread_t t;
        int err;
        err = pthread_create(&t, &attr, handleConn, (void *) (long) clntSock);
        if (err)
            ALOGE("pthread_create error: %s", strerror(errno));
    }
    // never reached
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        cerr << "usage: " << *argv << " <port>" << endl;        
        return 1;
    }

    unsigned short port = atoi(argv[1]);

    if (port <= 0) {
        cerr << "Error: could not parse port number: " << argv[1] << endl;
        return 1;
    }

    //daemon(0, 0);
    return runServer(port);
}
