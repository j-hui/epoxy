#include "IEchoService.h"

#include <binder/IServiceManager.h>
#include <utils/String8.h>

#include <cstdio>
#include <iostream>

#include <chrono>
#define DEF_TIMER(t) auto t = std::chrono::high_resolution_clock::now()
#define GET_TIMER(t) std::chrono::duration<double, std::micro> \
    (std::chrono::high_resolution_clock::now() - t).count()
#define PRINT_TIMER(o, t) o << "TIME " #t ": " << GET_TIMER(t) << endl

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

    DEF_TIMER(get_service_time);
    sp<IBinder> binder = sm->getService(service);
    PRINT_TIMER(cout, get_service_time);

    if (!binder) {
        cerr << "could not retrieve binder. exiting." << endl;
        return 2;
    }

    sp<IEchoService> es = interface_cast<IEchoService>(binder);

    cerr << "Echoclient sending message: " << String8(msg).string() << endl;

    DEF_TIMER(tx1_time);
    String16 reply = es->echo(msg);
    PRINT_TIMER(cout, tx1_time);

    cerr << "Echoclient received 1st reply: " << String8(reply).c_str() << endl;
    cerr << "Echoclient sending 1st reply..." << endl;
    
    DEF_TIMER(tx2_time);
    reply = es->echo(reply);
    PRINT_TIMER(cout, tx2_time);
    cerr << "Echoclient received 2nd reply: " << String8(reply).c_str() << endl;

    return 0;
}
