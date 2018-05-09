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

void* handleConn(void* arg)
{
    int sock = (long) arg;

    ALOGI("Handler started on fd %d", sock);

    for (;;) {
        uint64_t n_len;
        status_t err;

        if (recv(sock, &n_len, sizeof(n_len), MSG_WAITALL) != sizeof(n_len)) {
            goto quit;
        }

        uint64_t len = ntohl(n_len);

        char *service_buf[len];
        if (recv(sock, service_buf, len, MSG_WAITALL) < 0) {
            goto quit;
        }

        String16 service = String16((const char16_t*)service_buf,
                len / sizeof(char16_t));
        
        uint32_t code, flags;
        if (recv(sock, &code, sizeof(code), MSG_WAITALL) != sizeof(code)) {
            goto quit;
        }
        if (recv(sock, &flags, sizeof(flags), MSG_WAITALL) != sizeof(flags)) {
            goto quit;
        }
        if (recv(sock, &n_len, sizeof(n_len), MSG_WAITALL) != sizeof(n_len)) {
            goto quit;
        }
        len = ntohl(n_len);

        //ALOGD("Handler on fd %d receiving data of length %llu (net: %llu)",
                //sock, len, n_len);

        Parcel data;
        data.setDataSize(len);
        if (recv(sock, (void*) data.data(), len, MSG_WAITALL) < 0) {
            goto quit;
        }

        Parcel reply;

        sp < IServiceManager > sm = defaultServiceManager();
        sp < IBinder > binder = sm->getService(service);

        if (!binder) {
            ALOGW("fd %d: service %s not found",
                    sock, String8(service).string());
            err = NAME_NOT_FOUND;
            goto send_err;
        }

        err = binder->transact(code, data, &reply);
        if (err != NO_ERROR) {
            ALOGW("fd %d: transact failed: %s", sock, strerror(status));
            goto send_err;
        }

        len = reply.dataSize() * sizeof(*reply.data());
        n_len = htonl(len);

        if (send(sock, &n_len, sizeof(n_len), 0) < 0) {
            goto quit;
        }
        if (send(sock, reply.data(), len, 0) < 0) {
            goto quit;
        }
    }

send_err:
    len = 0xffffffffffffffff;
    n_len = htonl(len);
    if (send(sock, &n_len, sizeof(n_len), 0) < 0) {
        goto quit;
    }
    if (send(sock, &err, sizeof(err), 0) < 0) {
        goto quit;
    }
    continue;
quit:
    ALOGE("invalid send/recv: %s", strerror(errno));
    ALOGW("quitting handler fd: %d", sock);
    close(sock);
    pthread_exit(NULL);
}

int runServer(unsigned short port) {
    int servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (servSock < 0) {
        ALOGE("Could not create listening socket: %s", strerror(errno));
        return 1;
    }

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    if (::bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        ALOGE("Could not bind socket on port %d: %s", port, strerror(errno));
        return 1;
    }

    if (listen(servSock, MAXPENDING) < 0) {
        ALOGE("Could not listen: %s", strerror(errno));
        return 1;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (;;) {
        struct sockaddr_in clntAddr;
        socklen_t clntLen = sizeof(clntAddr);
        int clntSock = accept(servSock, (struct sockaddr*)&clntAddr, &clntLen);
        if (clntSock < 0) {
            ALOGE("Accept failed: %s", strerror(errno));
            continue;
        }

        char *clntIP = inet_ntoa(clntAddr.sin_addr);

        ALOGI("Accepted connection from %s on fd %d", clntIP, clntSock);
        
        pthread_t t;
        int err = pthread_create(&t, &attr, handleConn,
                (void*) (long) clntSock);
        if (err) {
            ALOGE("pthread_create error: %s", strerror(errno));
        }
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
