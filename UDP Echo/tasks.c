/*
 * tasks.c
 *
 *  Created on: Jun 13, 2024
 *      Author: Mason Obregon
 */

// File for all the different threads

#include "header.h"

void taskFirst()
{
    addPayload("-print \xcWelcome to the MSP432:");         //clear the screen and print prompt
}

void taskADCStream()
{
    double conversion;
    int i, index;
    void *bufferin;
    bool choice;
    uint32_t gateKey;
    while(1)
    {
        Semaphore_pend(Glo.Bios.semADCBuf, BIOS_WAIT_FOREVER);
        choice = Glo.AudioQ.choice[Glo.AudioQ.read];
        index = Glo.AudioQ.index[Glo.AudioQ.read];
        if(Glo.Call.local)
        {
            if(choice)
                bufferin = Glo.ADCBuf.PongBuffRx;
            else
                bufferin = Glo.ADCBuf.PingBuffRx;
        }
        else
        {
            if(choice)
                bufferin = Glo.AudioRx[index].PongBuffRx;
            else
                bufferin = Glo.AudioRx[index].PingBuffRx;
        }
        ADCBuf_adjustRawValues(Glo.Devices.adcbuf, bufferin,
                               AUDIO_BLOCK_SIZE, ADCBUF_CHANNEL_0);
        ADCBuf_convertAdjustedToMicroVolts(Glo.Devices.adcbuf, ADCBUF_CHANNEL_0,
                                           bufferin, Glo.ADCBuf.microvoltBuffer,
                                           AUDIO_BLOCK_SIZE);
        if(!choice)
        {
            for(i = 0; i < AUDIO_BLOCK_SIZE; i++)
            {
                conversion = (double) Glo.ADCBuf.microvoltBuffer[i] * 16384. / 3300000.;
                Glo.AudioTx[index].PingBuffTx[i] = round(conversion);
            }
            Glo.AudioTx[index].completedTx = Glo.AudioTx[index].PingBuffTx;
        }
        else
        {
            for(i = 0; i < AUDIO_BLOCK_SIZE; i++)
            {
                conversion = (double) Glo.ADCBuf.microvoltBuffer[i] * 16384. / 3300000.;
                Glo.AudioTx[index].PongBuffTx[i] = round(conversion);
            }
            Glo.AudioTx[index].completedTx = Glo.AudioTx[index].PongBuffTx;

        }
        if(Glo.AudioTx[index].readindex < AUDIO_BLOCK_SIZE - ADC_DELAY - 4)
        {   Glo.AudioTx[index].readindex++; }
        else if(Glo.AudioTx[index].readindex > AUDIO_BLOCK_SIZE - ADC_DELAY + 4)
        {   Glo.AudioTx[index].readindex--; }

        gateKey = GateSwi_enter(Glo.Bios.gateAudio);
        Glo.AudioQ.index[Glo.AudioQ.read] = 0;
        if(Glo.AudioQ.read >= QLENGTH - 1)
            Glo.AudioQ.read = 0;
        else
            Glo.AudioQ.read++;
        GateSwi_leave(Glo.Bios.gateAudio, gateKey);
//        if(!Glo.ADCBuf.state)
//        {
//            Glo.ADCBuf.readindex = 0;
//            Glo.Devices.spiTransaction.txBuf = Glo.ADCBuf.PingBuffRx;
//            if(Glo.Fs != 8000)
//            {   MSGParser_Fs("-fs 8000");   }
//            Glo.ADCBuf.state = 1;
//            sprintf(message, "-timer %d", 1000000/Glo.Fs);
//            MSGParser_Timer(message);
//            MSGParser_Callback("-callback 0 -1 -audio");
//        }
    }
}

void taskUART0()
{
    bool flag;
    char input;
    char seq;
    char *ptr;


    while(1)
    {
        UART_read(Glo.Devices.uart, &input, 1);
        if(input == '\x1b')
        {
            flag = 1;
            continue;
        }
        else if(flag)
        {
            if(input == '[')
            {
                UART_read(Glo.Devices.uart, &input, 1);
                seq = input;
                escapeseq(seq);
                continue;
            }
            else
            {   flag = 0;   }
        }
        if (input == '\x7f')
        {
            if (Glo.messagei > 0)
            {
                deletechar();
                continue;
            }
            else
            {
                addPayload("-print You can't delete any further dumbass");
                Glo.messagebuffer[Glo.messagei] = 0;
                continue;
            }
        }
        if (input == '\r' || input == '\n')
        {
            addPayload("-print ");
            addPayload(Glo.messagebuffer);
            ptr = eatspaces(Glo.messagebuffer);
            if(*ptr)
            {   addCommand(ptr);    }
            Glo.cmd.index = 0;
            Glo.messagei = 0;
            Glo.messagebuffer[Glo.messagei] = 0;
            continue;
        }
        else if (Glo.messagei < MessageLength - 2)  //Need to keep the last message of the buffer 0
        {
            addchar(input);
            /*if(Glo.messagebuffer[Glo.messagei] == 0)
            {   Glo.messagebuffer[Glo.messagei + 1] = 0;    }
            Glo.messagebuffer[Glo.messagei] = input;
            Glo.messagei++;*/
        }
        else
        {
            addError("buffoverflow");
            addPayload("-print \r\nThe message is too long...clearing message buffer.");
            Glo.messagei = 0;
            Glo.messagebuffer[Glo.messagei] = 0;
        }
    }
}

void taskUART7()
{
    int len;
    while(1)
    {
        len = UART_read(Glo.Devices.uart7, Glo.uart7buffer, MessageLength);
        Glo.uart7buffer[len - 1] = 0;
        addPayload(Glo.uart7buffer);
    }
}

void taskPayload()
{
    uint32_t gateKey;
    while(1)
    {
        Semaphore_pend(Glo.Bios.semPayload, BIOS_WAIT_FOREVER);
        MSGParser(Glo.Q.payload[Glo.Q.read].buffer);
        gateKey = GateSwi_enter(Glo.Bios.gatePayload);
        Glo.Q.payload[Glo.Q.read].buffer[0] = 0;
        if(Glo.Q.read >= QLENGTH - 1)
        {   Glo.Q.read = 0; }
        else
        {   Glo.Q.read++;    }
        GateSwi_leave(Glo.Bios.gatePayload, gateKey);
    }
}
