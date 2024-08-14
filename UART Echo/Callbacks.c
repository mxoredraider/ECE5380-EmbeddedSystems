/*
 * Callbacks.c
 *
 *  Created on: Jun 26, 2024
 *      Author: Mason Obregon
 */

#include "header.h"

void Callback_ADCBuf(ADCBuf_Handle handle, ADCBuf_Conversion *conversion, void *buffer, uint32_t channel, int_fast16_t status)
{
    if(buffer == Glo.ADCBuf.PingBuffRx)
    {   Glo.ADCBuf.converting = 2;  }
    else if(buffer == Glo.ADCBuf.PongBuffRx)
    {   Glo.ADCBuf.converting = 1;  }
    else
    {
        //Invalid buffer location!
        return;
    }
    ADCBuf_adjustRawValues(handle, buffer, AUDIO_BLOCK_SIZE, channel);
    ADCBuf_convertAdjustedToMicroVolts(handle, channel, buffer, Glo.ADCBuf.microvoltBuffer, AUDIO_BLOCK_SIZE);
    Glo.ADCBuf.completedRx = buffer;
    Semaphore_post(Glo.Bios.semADCBuf);
}

void Callback0_Timer0()
{
    if(strcmp(Glo.Callback[0].payload, "-audio") == 0)
    {   MSGParser_Audio("-audio");  }
    else if(strcmp(Glo.Callback[0].payload, "-sine") == 0)
    {   MSGParser_Sine("-sine");    }
    else
    {   Swi_post(Glo.Bios.switimer);    }
}

void Ticker_Timer1()
{
    Swi_post(Glo.Bios.switicker);
}

void Callback1_LeftSW1()
{
    Swi_post(Glo.Bios.swisw1);
}

void Callback2_RightSW2()
{
    Swi_post(Glo.Bios.swisw2);
}

void Swi_Timer0()
{
    uint32_t gateKey;
    gateKey = GateSwi_enter(Glo.Bios.gateCallback);
    if(!(Glo.Callback[0].count) || !(Glo.Callback[0].payload[0]))
    {
        GateSwi_leave(Glo.Bios.gateCallback,gateKey);
        return;
    }
    else
    {   addPayload(Glo.Callback[0].payload);    }
    if(Glo.Callback[0].count > 0)
    {   Glo.Callback[0].count--;    }
    GateSwi_leave(Glo.Bios.gateCallback, gateKey);
}

void Swi_Ticker()
{
    uint32_t gateKey;
    int i;
    gateKey = GateSwi_enter(Glo.Bios.gateTicker);
    for(i = 0; i < TICKER_COUNT; i++)
    {
        if(Glo.Ticker[i].delay > 0)
        {
            Glo.Ticker[i].delay--;
        }
        else if(Glo.Ticker[i].count != 0)
        {
            addPayload(Glo.Ticker[i].payload);
            Glo.Ticker[i].delay = Glo.Ticker[i].period;
            if(Glo.Ticker[i].count > 0)
            {   Glo.Ticker[i].count--;  }
        }
    }
    GateSwi_leave(Glo.Bios.gateTicker, gateKey);
}

void Swi_LeftSW1()
{
    uint32_t gateKey;
    gateKey = GateSwi_enter(Glo.Bios.gateCallback);
    if(!(Glo.Callback[1].count) || !(Glo.Callback[1].payload[0]))
    {
        GateSwi_leave(Glo.Bios.gateCallback, gateKey);
        return;
    }
    addPayload(Glo.Callback[1].payload);
    if(Glo.Callback[1].count > 0)
    {   Glo.Callback[1].count--;    }
    GateSwi_leave(Glo.Bios.gateCallback, gateKey);
}

void Swi_RightSW2()
{
    uint32_t gateKey;
    gateKey = GateSwi_enter(Glo.Bios.gateCallback);
    if(!(Glo.Callback[2].count) || !(Glo.Callback[2].payload[0]))
    {
        GateSwi_leave(Glo.Bios.gateCallback, gateKey);
        return;
    }
    addPayload(Glo.Callback[2].payload);
    if(Glo.Callback[2].count > 0)
    {   Glo.Callback[2].count--;    }
    GateSwi_leave(Glo.Bios.gateCallback, gateKey);

}
