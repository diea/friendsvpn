#include "proxy.h"

Proxy::Proxy(BonjourRecord& rec, QObject *parent) :
    rec(rec), QObject(parent)
{
}

void Proxy::run() {
    // generate random ULA
    // add it with ifconfig
    // create raw socket to listen on this new IP
    // receive packet, forward it on the secure UDP link
}
