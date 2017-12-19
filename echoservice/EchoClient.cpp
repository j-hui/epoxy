#include "IEchoService.h"

#include <binder/IServiceManager.h>
#include <utils/String8.h>

#include <cstdio>
#include <iostream>

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
        service += "@" += argv[2] += ":" argv[3];

    cerr << "EchoClient to connect with " << String8(service).string() << endl;

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(ECHO_SERVICE_NAME16);

    sp<IEchoService> es = interface_cast<IEchoService>(binder);

    cerr << "Echoclient sending message: " << String8(msg).string() << endl;
    String16 reply = es->echo(msg);

    cout << "Echoclient received reply: " << String8(reply).c_str() << endl;

    return 0;
}
