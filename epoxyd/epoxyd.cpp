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

#if 0
#include <chrono>
#define DEF_TIMER(t) auto t = std::chrono::high_resolution_clock::now()
#define ALOGT(t) ALOGD("TIME " #t ": %lf", \
        std::chrono::duration<double, std::micro> \
        (std::chrono::high_resolution_clock::now() - t).count())
#else
#define DEF_TIMER(t)
#define ALOGT(t)
#endif

void* handleConn(void* arg)
{
    int sock = (long) arg;
    status_t err;

    DEF_TIMER(handler_time);
    {
        DEF_TIMER(recv_time);
        uint64_t n_len;
        if (recv(sock, &n_len, sizeof(n_len), MSG_WAITALL) != sizeof(n_len)) {
            ALOGE("recv() service name name: %s", strerror(errno));
            goto out;
        }

        uint64_t len = ntohl(n_len);

        char *service_buf[len];
        if (recv(sock, service_buf, len, MSG_WAITALL) < 0) {
            ALOGE("recv() service name: %s", strerror(errno));
            goto out;
        }

        String16 service = String16((const char16_t*)service_buf,
                len / sizeof(char16_t));
        
        uint32_t code, flags;
        if (recv(sock, &code, sizeof(code), MSG_WAITALL) != sizeof(code)) {
            ALOGE("recv() code: %s", strerror(errno));
            goto out;
        }
        if (recv(sock, &flags, sizeof(flags), MSG_WAITALL) != sizeof(flags)) {
            ALOGE("recv() flags: %s", strerror(errno));
            goto out;
        }
        if (recv(sock, &n_len, sizeof(n_len), MSG_WAITALL) != sizeof(n_len)) {
            ALOGE("recv() parcel len: %s", strerror(errno));
            goto out;
        }

        len = ntohl(n_len);

        Parcel data;
        data.setDataSize(len);
        if (recv(sock, (void*) data.data(), len, MSG_WAITALL) < 0) {
            ALOGE("recv() parcel data: %s", strerror(errno));
            goto out;
        }

        ALOGT(recv_time);

        DEF_TIMER(binder_time);

        sp < IServiceManager > sm = defaultServiceManager();
        sp < IBinder > binder = sm->getService(service);

        if (!binder) {
            ALOGW("fd %d: service %s not found", sock, String8(service).string());
            err = NAME_NOT_FOUND;
            goto send_err;
        }

        Parcel reply;
        err = binder->transact(code, data, &reply);
        ALOGT(binder_time);

        if (err != NO_ERROR) {
            ALOGW("fd %d: transact failed: %s", sock, strerror(err));
            goto send_err;
        }

        len = reply.dataSize() * sizeof(*reply.data());
        n_len = htonl(len);

        DEF_TIMER(send_time);
        if (send(sock, &n_len, sizeof(n_len), 0) < 0) {
            ALOGE("send() len: %s", strerror(errno));
            goto out;
        }
        if (send(sock, reply.data(), len, 0) < 0) {
            ALOGE("send() data: %s", strerror(errno));
            goto out;
        }

        ALOGT(send_time);
        goto out;
    }

send_err: {
        uint64_t len = 0xffffffffffffffff;
        uint64_t n_len = htonl(len);        // this should be a nop, unnecessary

        if (send(sock, &n_len, sizeof(n_len), 0) < 0) {
            ALOGE("send() error len: %s", strerror(errno));
            goto out;
        }
        if (send(sock, &err, sizeof(err), 0) < 0) {
            ALOGE("send() error: %s", strerror(errno));
            goto out;
        }
    }

out:
    close(sock);
    ALOGT(handler_time);
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
