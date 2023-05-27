#include "TcpExample.h"
#include "UdpExample.h"
#include "SslExample.h"
#include "HttpExample.h"
#include "HttpsExample.h"

int main(int argc, char* argv[])
{
    DoTcpExample();
    DoUdpExample();
    DoSslExample();
    DoHttpExample();
    DoHttpsExample();
}