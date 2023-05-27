/**
 * @file main.cpp
 * @author David Brill (6david6brill6@gmail.com)
 * 
 * @copyright Copyright (c) 2023
 * Distributed under the Apache License 2.0 (see accompanying
 * file LICENSE or copy at http://www.apache.org/licenses/)
 */

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