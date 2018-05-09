#include "IEchoService.h"

#include <binder/IServiceManager.h>
#include <utils/String8.h>

#include <cstdio>
#include <iostream>
#include <chrono>

using namespace std;
using namespace android;

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 4) {
        cerr << "usage: " << *argv << " <msg> [<host> <port>]" << endl;
        return 1;
    }

    String16 msg = String16(argv[1]);
    String16 service = ECHO_SERVICE_NAME16;

    if (argc == 4)
        service += String16("@")
                + String16(argv[2])
                + String16(":")
                + String16(argv[3]);

    cerr << "EchoClient to connect with " << String8(service).string() << endl;

    sp<IServiceManager> sm = defaultServiceManager();

    auto get_b = chrono::high_resolution_clock::now();
    sp<IBinder> binder = sm->getService(service);
    auto get_e = chrono::high_resolution_clock::now();

    if (!binder) {
        cerr << "could not retrieve binder. exiting." << endl;
        return 2;
    }

    sp<IEchoService> es = interface_cast<IEchoService>(binder);

    cerr << "Echoclient sending message: " << String8(msg).string() << endl;

    auto call_b = chrono::high_resolution_clock::now();
    String16 reply = es->echo(msg);
    auto call_e = chrono::high_resolution_clock::now();

    cerr << "Echoclient received reply: " << String8(reply).c_str() << endl;

    auto get_t = chrono::duration<double, micro>(get_e - get_b).count();
    auto call_t = chrono::duration<double, micro>(call_e - call_b).count();

    cout << "retrieving service: " << get_t << endl;
    cout << "performing RPC call: " << call_t << endl;

    return 0;
}
