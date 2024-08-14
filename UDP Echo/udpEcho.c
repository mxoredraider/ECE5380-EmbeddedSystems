/*
 * Copyright (c) 2015-2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *    ======== udpEcho.c ========
 *    Contains BSD sockets code.
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <pthread.h>
/* BSD support */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>

#include <ti/net/slnetutils.h>

#include "header.h"

#define UDPPACKETSIZE 1472
#define MAXPORTLEN    6
#define DEFAULT_PORT  1000

extern void fdOpenSession();
extern void fdCloseSession();
extern void *TaskSelf();

char *UDPParse(char *buffer, struct sockaddr_in *clientAddr);

void *RxFxn(void *arg0)
{
    int                bytesRcvd;
    int                status;
    int                server = -1;
    fd_set             readSet;
    struct addrinfo    hints;
    struct addrinfo    *res, *p;
    struct sockaddr_in clientAddr;
    struct ip_mreq{
                        struct in_addr imr_multiaddr;
                        struct in_addr imr_interface;
    };
    struct ip_mreq     mreq;
    socklen_t          addrlen;
    char               buffer[UDPPACKETSIZE];
    char               portNumber[MAXPORTLEN];
    char               message[MessageLength];
    char               *loc;
    uint32_t    addr;

    fdOpenSession(TaskSelf());

    sprintf(message, "-print UDP Receive started\r\n");
    addPayload(message);

    sprintf(portNumber, "%d", *(uint16_t *)arg0);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    /* Obtain addresses suitable for binding to */
    status = getaddrinfo(NULL, portNumber, &hints, &res);
    if (status != 0) {
        sprintf(message, "-print Error: getaddrinfo() failed: %s\r\n",
            gai_strerror(status));
        addPayload(message);
        goto shutdown;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        server = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server == -1) {
            continue;
        }

        mreq.imr_multiaddr.s_addr = htonl(Glo.IP.multicast);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        status = setsockopt(server, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

        status = bind(server, p->ai_addr, p->ai_addrlen);
        if (status != -1) {
            break;
        }

        close(server);
    }

    if (server == -1) {
        sprintf(message, "-print Error: socket not created.\r\n");
        addPayload(message);
        goto shutdown;
    } else if (p == NULL) {
        sprintf(message, "-print Error: bind failed.\r\n");
        addPayload(message);
        goto shutdown;
    } else {
        freeaddrinfo(res);
        res = NULL;
    }

    do {
        /*
         *  readSet and addrlen are value-result arguments, which must be reset
         *  in between each select() and recvfrom() call
         */
        FD_ZERO(&readSet);
        FD_SET(server, &readSet);
        addrlen = sizeof(clientAddr);

        /* Wait forever for the reply */
        status = select(server + 1, &readSet, NULL, NULL, NULL);
        if (status > 0) {
            if (FD_ISSET(server, &readSet)) {
                bytesRcvd = recvfrom(server, buffer, UDPPACKETSIZE, 0,
                        (struct sockaddr *)&clientAddr, &addrlen);
                if(bytesRcvd > 0)
                {
                    addr = (clientAddr.sin_addr.s_addr & 0xFF000000) >> 24;
                    addr |= (clientAddr.sin_addr.s_addr & 0xFF0000) >> 8;
                    addr |= (clientAddr.sin_addr.s_addr & 0xFF00) << 8;
                    addr |= (clientAddr.sin_addr.s_addr & 0xFF) << 24;

                    loc = eatspaces(buffer);
                    if(strstr(loc, "-voice") == loc)
                    {
                        if(Glo.Call.callcount > 0)
                            MSGParser_Voice(loc, addr);
                        continue;
                    }

                    sprintf(message, "-print UDP %d.%d.%d.%d-> %s",
                            (uint8_t) (addr >> 24), (uint8_t) (addr >> 16),
                            (uint8_t) (addr >> 8), (uint8_t) addr, buffer);
                    addPayload(message);
                    addPayload(buffer);
                }
            }
        }
    } while (status > 0);

shutdown:
    if (res) {
        freeaddrinfo(res);
    }

    if (server != -1) {
        close(server);
    }

    fdCloseSession(TaskSelf());

    return (NULL);
}

void *TxFxn(void *arg0)
{
    int                bytesSent;
    int                bytesRequested;
    int                status;
    int                server = -1;
    fd_set             writeSet;
    struct addrinfo    hints;
    struct addrinfo    *res, *p;
    struct sockaddr_in clientAddr;
    socklen_t          addrlen;
    char               portNumber[MAXPORTLEN];
    char               message[MessageLength];
    bool               broadcast = TRUE;
    uint32_t           gateKey;
    char *loc;

    fdOpenSession(TaskSelf());

    sprintf(portNumber, "%d", *(uint16_t *)arg0);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    /* Obtain addresses suitable for binding to */
    status = getaddrinfo(NULL, portNumber, &hints, &res);
    if (status != 0) {
        sprintf(message, "-print Error: getaddrinfo() failed: %s\r\n",
            gai_strerror(status));
        addPayload(message);
        goto shutdown;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        server = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server == -1) {
            continue;
        }
    }

    sprintf(message, "-print UDP Transmit started\r\n");
    addPayload(message);

    sprintf(portNumber, "%d", *(uint16_t *)arg0);

    if (server == -1) {
        sprintf(message, "-print Error: socket not created.\r\n");
        addPayload(message);
        goto shutdown;
    } else {
        freeaddrinfo(res);
        res = NULL;
    }

    status = setsockopt(server, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    do {
        /*
         *  writeSet and addrlen are value-result arguments, which must be reset
         *  in between each select() and recvfrom() call
         */
        FD_ZERO(&writeSet);
        FD_SET(server, &writeSet);
        addrlen = sizeof(clientAddr);

        Semaphore_pend(Glo.Bios.semUDPOut, BIOS_WAIT_FOREVER);

        loc = UDPParse(Glo.QUDPOut.buffer[Glo.QUDPOut.read], &clientAddr);
        if(!*loc)
            continue;
        //strcpy(buffer, loc);
        bytesRequested = strlen(loc) + 1;
        bytesRequested += Glo.QUDPOut.bytecount[Glo.QUDPOut.read];

        status = select(server + 1, NULL, &writeSet, NULL, NULL);
        if (status > 0) {
            bytesSent = sendto(server, loc, bytesRequested, 0,
                    (struct sockaddr *)&clientAddr, addrlen);
            if (bytesSent < 0) {
                sprintf(message, "-print Error: sendto failed.\r\n");
                addPayload(message);
                goto shutdown;
            }
        }
        gateKey = GateSwi_enter(Glo.Bios.gateUDPOut);
        Glo.QUDPOut.buffer[Glo.QUDPOut.read][0] = 0;
        if(Glo.QUDPOut.read >= QLENGTH - 1)
            Glo.QUDPOut.read = 0;
        else
            Glo.QUDPOut.read++;
        GateSwi_leave(Glo.Bios.gateUDPOut, gateKey);
    } while (status > 0);

shutdown:

    if (res) {
        freeaddrinfo(res);
    }

    if (server != -1) {
        close(server);
    }


    fdCloseSession(TaskSelf());

    return (NULL);
}

char *UDPParse(char *buffer, struct sockaddr_in *clientAddr)
{
    char *loc, *portloc;
    uint32_t addr;
    uint16_t port;
    addr = getAddr(buffer);
    clientAddr->sin_addr.s_addr = (addr & 0xFF000000) >> 24;
    clientAddr->sin_addr.s_addr |= (addr & 0xFF0000) >> 8;
    clientAddr->sin_addr.s_addr |= (addr & 0xFF00) << 8;
    clientAddr->sin_addr.s_addr |= (addr & 0xFF) << 24;
    loc = strchr(buffer, '-');
    portloc = strchr(buffer, ':');
    if(loc > portloc && portloc)
    {
        if(isdigit(*(portloc + 1)))
            portloc++;
        else
            portloc = nextword(portloc);
        if(!isdigit(*portloc))
        {
            port = DEFAULT_PORT;
        }
        else
            port = atoi(portloc);
    }
    else
        port = DEFAULT_PORT;
    clientAddr->sin_port = (port & 0xFF00) >> 8;
    clientAddr->sin_port |= (port & 0xFF) << 8;
    clientAddr->sin_family = AF_INET;
    return loc;
}
