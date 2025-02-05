/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
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
 *  ======== uartecho.c ========
 */

#include "header.h"

 /*  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /*char        input;
    char        buffer[MessageLength];
    const char  echoPrompt[] = "Welcome to the MSP432:\r\n";
    const char  errorbufoverflow[] = "\r\nThe message is too long...clearing message buffer.\r\n";
    const char  errordel[] = "You can't delete any further dumbass\r\n";
    const char  enter[] = "\r\n";
    const char  cls[] = "\xc";  //clear the screen
    int         i = 0;

    //Define Global Variables
    Glo.var.cls = cls;
    Glo.var.enter = enter;




    UART_write(uart, cls, sizeof(cls));         //clear the screen before prompt is printed
    UART_write(uart, echoPrompt, sizeof(echoPrompt));

    memset(buffer, 0, MessageLength * sizeof(buffer[0])); //Ensure the buffer is cleared
*/
    /* Loop forever echoing */
    while (1)
    {

    }
}
