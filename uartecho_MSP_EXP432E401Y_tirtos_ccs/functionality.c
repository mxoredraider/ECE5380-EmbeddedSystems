/*
 * functionality.c
 *
 *  Created on: Jun 7, 2024
 *      Author: Mason Obregon
 */


#include "header.h"


void MSGParser_About(char *buffer)
{
    int assignment = 9;                             // Define current assignment #
    char message[MessageLength];
    sprintf(message, "Author: Mason Obregon\r\n");      //Author
    write(message);
    sprintf(message, "Assignment: %d\r\n", assignment);  //Assignment #
    write(message);
    sprintf(message, "Version: %s\r\n", __VERSION__);      //Version
    write(message);
    sprintf(message, "Date/Time: %s\r\n", __TIMESTAMP__);//Timestamp
    write(message);
}

void MSGParser_Audio(char *buffer)
{
   bool transfer;
   transfer = SPI_transfer(Glo.Devices.spi, &Glo.Devices.spiTransaction);
   if(!transfer)
   {    while(1);   }
   if(!Glo.AudioTx.state)
   {
       if(Glo.AudioTx.completedTx)
       {
           Glo.Devices.spiTransaction.txBuf = Glo.AudioTx.PingBuffTx;
           Glo.AudioTx.state = 1;
       }
       return;
   }
   Glo.AudioTx.readindex++;
   if(Glo.AudioTx.readindex >= AUDIO_BLOCK_SIZE)
   {
       if(Glo.AudioTx.state == 1 && Glo.AudioTx.completedTx == Glo.AudioTx.PongBuffTx)
       {
           Glo.Devices.spiTransaction.txBuf = Glo.AudioTx.PongBuffTx;
           Glo.AudioTx.readindex = 0;
           Glo.AudioTx.state = 2;
       }
       else if(Glo.AudioTx.state == 2 && Glo.AudioTx.completedTx == Glo.AudioTx.PingBuffTx)
       {
           Glo.Devices.spiTransaction.txBuf = Glo.AudioTx.PingBuffTx;
           Glo.AudioTx.readindex = 0;
           Glo.AudioTx.state = 1;
       }
       else
       {    Glo.AudioTx.readindex--; }
   }
   else
   {
       if(Glo.AudioTx.state == 1)
       {    Glo.Devices.spiTransaction.txBuf = &Glo.AudioTx.PingBuffTx[Glo.AudioTx.readindex]; }
       else
       {    Glo.Devices.spiTransaction.txBuf = &Glo.AudioTx.PongBuffTx[Glo.AudioTx.readindex]; }
   }
}

void MSGParser_Callback(char *buffer)
{
    char *loc, message[MessageLength];
    int i, count;
    uint32_t gateKey;
    loc = nextword(buffer);
    if(!*loc)
    {
       for(i = 0; i < CALLBACK_COUNT; i++)
       {
           sprintf(message, "Callback %d\tC:%d\tP: %s\r\n", i, Glo.Callback[i].count, Glo.Callback[i].payload);
           write(message);
       }
       return;
    }
    if(!isdigit(*loc))
    {
        sprintf(message, "You must enter a callback index.\r\n");
        write(message);
        return;
    }
    i = atoi(loc);
    if(i < 0 || i >= CALLBACK_COUNT )
    {
        sprintf(message, "Invalid callback index!(0-%d).\r\n", CALLBACK_COUNT - 1);
        write(message);
        return;
    }
    loc = nextword(loc);
    if(!*loc)
    {
        sprintf(message,"Callback %d\tC:%d\tP: %s\r\n", i, Glo.Callback[i].count, Glo.Callback[i].payload);
        write(message);
        return;
    }
    else if(!isdigit(*loc) && *loc != '-')
    {
        write("You must enter a count for callback to repeat.\r\n");
        return;
    }
    else if(*loc == '-')
    {
        loc++;
        if(!isdigit(*loc))
        {
            write("You must enter a count for callback to repeat.\r\n");
            return;
        }
        count = -atoi(loc);
    }
    else
    {   count = atoi(loc);  }
    if(!count)
    {
        Glo.Callback[i].count = 0;
        Glo.Callback[i].payload[0] = 0;
        sprintf(message, "Callback %d\tC:%d\tP: %s\r\n", i, Glo.Callback[i].count, Glo.Callback[i].payload);
        write(message);
        return;
    }
    loc = nextword(loc);
    if(!*loc)
    {
        sprintf(message, "You must enter a payload for the callback.\r\n");
        write(message);
        return;
    }
    gateKey = GateSwi_enter(Glo.Bios.gateCallback);
    strcpy(Glo.Callback[i].payload, loc);
    Glo.Callback[i].count = count;
    GateSwi_leave(Glo.Bios.gateCallback,gateKey);
    sprintf(message, "Callback %d\tC:%d\tP: %s\r\n", i, Glo.Callback[i].count, Glo.Callback[i].payload);
    write(message);
}

void MSGParser_Error()
{
    char message[MessageLength];

    sprintf(message, "\r\nErrors:\r\n\n");
    write(message);
    sprintf(message, "Buffer Overflow:\t%d\r\n", Glo.Err.buffoverflow);
    write(message);
    sprintf(message, "Unknown Command:\t%d\r\n", Glo.Err.commandunknown);
    write(message);
    sprintf(message, "Queue Overflow:\t\t%d\r\n", Glo.Err.qoverflow);
    write(message);
}

void MSGParser_Fs(char *buffer)
{
    char *loc, message[MessageLength];
    int fs;
    loc = nextword(buffer);
    if(!*loc)
    {
        sprintf(message, "Sample Frequency: %d\r\n", Glo.Fs);
        write(message);
    }
    else if(!isdigit(*loc))
    {   write("You must enter a sample frequency.\r\n");    }
    else
    {
        fs = atoi(loc);
        if(1000000/fs < MIN_PERIOD_US)
        {
            sprintf(message, "The sample frequency cannot be greater than %d Hz\r\n", 1000000 / MIN_PERIOD_US);
            write(message);
        }
        else
        {
                Glo.Fs = fs;
                sprintf(message, "Sample Frequency: %d\r\n", Glo.Fs);
                write(message);
        }
    }
}

void MSGParser_GPIO(char *buffer)
{
    int i, out;
    char *loc, message[MessageLength];
    loc = nextword(buffer);
    if(!*loc)
    {
        for(i = 0;i < GPIO_OUT_COUNT;i++)
        {
            out = GPIO_read(i);
            sprintf(message, "GPIO %d\t%d\r\n", i, out);
            write(message);
        }
        return;
    }
    else if(*loc == '\x7f')
    {   return; }
    if(!isdigit(*loc) && *loc != '-')
    {
        sprintf(message, "You must enter a GPIO number.");
        return;
    }
    i = atoi(loc);
    if(i < 0 || i > GPIO_OUT_COUNT - 1)
    {
        sprintf(message, "You must enter a GPIO number from 0 to %d.\r\n", GPIO_OUT_COUNT -1);
        write(message);
        return;
    }
    loc = nextword(loc);
    if(!*loc || *loc == 'r')
    {
        out = GPIO_read(i);
        sprintf(message, "GPIO %d\t%d\r\n", i, out);
        write(message);
    }
    else if(*loc == 'w')
    {
        loc = nextword(loc);
        if(!*loc)
        {
            sprintf(message, "You must enter an output state to write.\r\n");
            write(message);
            return;
        }
        out = atoi(loc);
        if(out == 0 || out == 1)
        {
            GPIO_write(i, out);
            sprintf(message, "GPIO %d\t%d\r\n", i, out);
            write(message);
        }
        else
        {
            sprintf(message, "Output state must either be 0 or 1.\r\n");
            write(message);
        }
    }
    else if(*loc == 't')
    {   GPIO_toggle(i); }
    else
    {   error_unknown(buffer);  }
}

void MSGParser_Help(char *buffer)
{
    char *loc;
    loc = nextword(buffer);
    if(!*loc)
    {
        help();
        return;
    }
    else if(*loc == '\x7f')
    {   return; }
    if(strstr(loc, "about") == loc) //Parses for the function needed help with
    {   help_about();   }
    else if(strstr(loc, "callback") == loc)
    {   help_callback();    }
    else if(strstr(loc, "cls") == loc)
    {   help_cls();     }
    else if(strstr(loc, "error") == loc)
    {   help_error();   }
    else if(strstr(loc, "gpio") == loc)
    {   help_gpio();    }
    else if(strstr(loc, "if") == loc)
    {   help_if();  }
    else if(strstr(loc, "memr") == loc)
    {   help_memr();    }
    else if(strstr(loc, "print") == loc)
    {   help_print();   }
    else if(strstr(loc, "reg") == loc)
    {   help_reg(); }
    else if(strstr(loc, "script") == loc)
    {   help_script();  }
    else if(strstr(loc, "ticker") == loc)
    {   help_ticker();  }
    else if(strstr(loc, "timer") == loc)
    {   help_timer();   }
    else if(strstr(loc, "uart") == loc)
    {   help_uart();    }
    else
    {   error_unknown(buffer);  }
}

void MSGParser_If(char *buffer)
{
    int a, b;
    bool checka, checkb;
    char *loc, *condt, *condf, cond;
    loc = nextword(buffer);
    checka = checkRegIn(loc, 'a');
    a = getRegIn(loc);
    loc = nextword(loc);
    cond = *loc;
    loc = nextword(loc);
    checkb = checkRegIn(loc, 'b');
    b = getRegIn(loc);
    if(!checka || !checkb)
    {   return; }
    loc = strstr(loc, "?");
    if(!loc)
    {
        write("'?' must be entered before true and false conditions.\r\n");
        return;
    }
    loc = eatspaces(loc + 1);
    if(*loc == ':')
    {
        condt = 0;
        condf = eatspaces(loc + 1);
        if(!*condf)
        {  condf = 0;    }
    }
    else
    {
        if(!*loc)
        {   condt = 0;  }
        else
        {   condt = loc;    }
        loc = strstr(loc, ":");
        if(!loc)
        {   condf = 0;  }
        else
        {
            *loc = 0;
            condf = eatspaces(loc + 1);
            if(!*condf)
            {   condf = 0;  }
        }
    }
    if(cond == '>')
    {
        if(a > b)
        {
            if(condt)
            {   addPayload(condt);  }
        }
        else
        {
            if(condf)
            {   addPayload(condf);  }
        }
    }
    else if(cond == '=')
    {
        if(a == b)
        {
            if(condt)
            {   addPayload(condt);  }
        }
        else
        {
            if(condf)
            {   addPayload(condf);  }
        }
    }
    else if(cond == '<')
    {
        if(a < b)
        {
            if(condt)
            {   addPayload(condt);  }
        }
        else
        {
            if(condf)
            {   addPayload(condf);  }
        }
    }
    else
    {
        write("Invalid conditional operator! Must be '>', '=', or '<'.\r\n");
    }
}

void MSGParser_Memr(char *buffer)
{
    int mem;
    char *loc, message[MessageLength];
    loc = nextword(buffer);
    if(*loc == '\x7f')
    {   return; }
    else if(!*loc || !isdigit(*loc))
    {
        sprintf(message, "You need to enter an address to read.\r\n");
        write(message);
        return;
    }
    else if(*loc == '0' && *(loc + 1) == 'x')
    {   loc = loc + 2;  }
    mem = strtol(loc, 0, 16);
    if (mem >= 0x00000000 & mem < 0x00100000 || mem >= 0x20000000 & mem < 0x20040000)
    {
        mem = mem & 0xFFFFFFF0;
        int *data0 = (int *)mem;
        int *data4 = (int *)(mem + 0x4);
        int *data8 = (int *)(mem + 0x8);
        int *dataC = (int *)(mem + 0xC);
        sprintf(message, "0x%08X 0x%08X 0x%08X 0x%08X\r\n", mem, mem + 0x4, mem + 0x8, mem + 0xC);
        write(message);
        sprintf(message, "0x%08X 0x%08X 0x%08X 0x%08X\r\n", *data0, *data4, *data8, *dataC);
        write(message);

    }
    else
    {
        sprintf(message, "0x%08X is an invalid address!\r\n", mem);
        write(message);
    }


}

void MSGParser_Print(char *buffer)
{
    int l = strlen("-print");
    char *ptr;
    ptr = buffer + l;
    if(*ptr != ' ')
    {
        error_unknown(buffer);
        return;
        /*input = error_nospace(buffer, l);
        if(input == 'n')
        {
            error_unknown(buffer);
            return;
        }*/
    }
    else
    {   ptr++;   }
    write(ptr);
    write("\r\n");
}

void MSGParser_Reg(char *buffer)
{
    int i, val1, val2;
    bool check1 = 0, check2 = 0;
    char *loc, message[MessageLength];
    char *input1, *input2;
    loc = nextword(buffer);
    if(!*loc)
    {
        for(i = 0; i < REG_COUNT; i++)
        {
            sprintf(message, "Reg %d:\t%d (0x%08X)\r\n", i, Glo.Reg[i].value, Glo.Reg[i].value);
            write(message);
        }
        return;
    }
    else if(!isdigit(*loc))
    {
        write("You must enter a register index.\r\n");
        return;
    }
    i = atoi(loc);
    if(i < 0 || i >= REG_COUNT)
    {
        sprintf(message, "Invalid output register index! Must be 0-%d\r\n", REG_COUNT);
        write(message);
        return;
    }
    loc = nextword(loc);
    if(!*loc)
    {   goto print; }
    input1 = nextword(loc);
    input2 = nextword(input1);

    if(strstr(loc, "r") == loc)
    {
        val1 = atoi(input1);
        if(val1 < 0 || val1 >= REG_COUNT)
        {
            sprintf(message, "Invalid register index for input 1! Must be 0-%d\r\n", REG_COUNT);
            write(message);
            return;
        }
        goto print;
    }
    else if(strstr(loc, "w") == loc)
    {
        if(!isdigit(*input1) && *input1 != '-')
        {
            write("You must enter a value to write.\r\n");
            return;
        }
        else if(*input1 == '-')
        {
            input1++;
            if(!isdigit(*input1))
            {
                write("You must enter a value to write.\r\n");
                return;
            }
            val1 = -atoi(input1);
        }
        else
        {   val1 = atoi(input1);    }
        Glo.Reg[i].value = val1;
        goto print;
    }
    else if(strstr(loc, "++") == loc)
    {
        Glo.Reg[i].value++;
        goto print;
    }
    else if(strstr(loc, "--") == loc)
    {
        Glo.Reg[i].value--;
        goto print;
    }
    else if(strstr(loc, "+") == loc)
    {
        check1 = checkRegIn(input1, '1');
        check2 = checkRegIn(input2, '2');
        if(!check1 || !check2)
        {   return; }
        val1 = getRegIn(input1);
        val2 = getRegIn(input2);
        Glo.Reg[i].value = val1 + val2;
    }
    else if(strstr(loc, "-") == loc)
    {
        check1 = checkRegIn(input1, '1');
        if(!check1)
        {   return; }
        val1 = getRegIn(input1);
        if(!*input2)
        {
            Glo.Reg[i].value = -val1;
            goto print;
        }
        check2 = checkRegIn(input2, '2');
        if(!check2)
        {   return; }
        val2 = getRegIn(input2);
        Glo.Reg[i].value = val1 - val2;
        goto print;
    }
    else if(strstr(loc, "~") == loc)
    {
        check1 = checkRegIn(input1, '1');
        if(!check1)
        {   return; }
        val1 = getRegIn(input1);
        Glo.Reg[i].value = ~val1;
        goto print;
    }
    else if(strstr(loc, "&") == loc)
    {
        check1 = checkRegIn(input1, '1');
        check2 = checkRegIn(input2, '2');
        if(!check1 || !check2)
        {   return; }
        val1 = getRegIn(input1);
        val2 = getRegIn(input2);
        Glo.Reg[i].value = val1 & val2;
        goto print;
    }
    else if(strstr(loc, "|") == loc)
    {
        check1 = checkRegIn(input1, '1');
        check2 = checkRegIn(input2, '2');
        if(!check1 || !check2)
        {   return; }
        val1 = getRegIn(input1);
        val2 = getRegIn(input2);
        Glo.Reg[i].value = val1 | val2;
        goto print;
    }
    else if(strstr(loc, "^") == loc)
    {
        check1 = checkRegIn(input1, '1');
        check2 = checkRegIn(input2, '2');
        if(!check1 || !check2)
        {   return; }
        val1 = getRegIn(input1);
        val2 = getRegIn(input2);
        Glo.Reg[i].value = val1 ^ val2;
        goto print;
    }
    else if(strstr(loc, "*") == loc)
    {
        check1 = checkRegIn(input1, '1');
        check2 = checkRegIn(input2, '2');
        if(!check1 || !check2)
        {   return; }
        val1 = getRegIn(input1);
        val2 = getRegIn(input2);
        Glo.Reg[i].value = val1 * val2;
        goto print;
    }
    else if(strstr(loc, "/") == loc)
    {
        check1 = checkRegIn(input1, '1');
        check2 = checkRegIn(input2, '2');
        if(!check1 || !check2)
        {   return; }
        val1 = getRegIn(input1);
        val2 = getRegIn(input2);
        Glo.Reg[i].value = val1 / val2;
        goto print;
    }
    else if(strstr(loc, "%") == loc)
    {
        check1 = checkRegIn(input1, '1');
        check2 = checkRegIn(input2, '2');
        if(!check1 || !check2)
        {   return; }
        val1 = getRegIn(input1);
        val2 = getRegIn(input2);
        Glo.Reg[i].value = val1 % val2;
        goto print;
    }
    else if(strstr(loc, "max") == loc)
    {
        check1 = checkRegIn(input1, '1');
        check2 = checkRegIn(input2, '2');
        if(!check1 || !check2)
        {   return; }
        val1 = getRegIn(input1);
        val2 = getRegIn(input2);
        if(val1 >= val2)
        {   Glo.Reg[i].value = val1;    }
        else
        {   Glo.Reg[i].value = val2;    }
        goto print;
    }
    else if(strstr(loc, "min") == loc)
    {
        check1 = checkRegIn(input1, '1');
        check2 = checkRegIn(input2, '2');
        if(!check1 || !check2)
        {   return; }
        val1 = getRegIn(input1);
        val2 = getRegIn(input2);
        if(val1 <= val2)
        {   Glo.Reg[i].value = val1;    }
        else
        {   Glo.Reg[i].value = val2;    }
        goto print;
    }
    else if (strstr(loc, "=") == loc)
    {
        check1 = checkRegIn(input1, '1');
        if(!check1)
        {   return; }
        val1 = getRegIn(input1);
        Glo.Reg[i].value = val1;
        goto print;
    }
    else if(strstr(loc, "x") == loc)
    {
        int reg;
        if(!isdigit(*input1))
        {
            write("You must input the argument for input 1.\r\n");
            return;
        }
        reg = atoi(input1);
        if(reg < 0 || reg >= REG_COUNT)
        {
            sprintf(message, "Invalid register index for input 1! Must be 0-%d\r\n", REG_COUNT);
            write(message);
            return;
        }
        val1 = Glo.Reg[reg].value;
        Glo.Reg[reg].value = Glo.Reg[i].value;
        Glo.Reg[i].value = val1;
        sprintf(message, "Reg %d:\t%d (0x%08X)\r\n", i, Glo.Reg[i].value, Glo.Reg[i].value);
        write(message);
        sprintf(message, "Reg %d:\t%d (0x%08X)\r\n", reg, Glo.Reg[reg].value, Glo.Reg[reg].value);
        write(message);
        return;
    }
    else
    {
        write("Invalid operator!\r\n");
        return;
    }
    print:
        sprintf(message, "Reg %d:\t%d (0x%08X)\r\n", i, Glo.Reg[i].value, Glo.Reg[i].value);
        write(message);
        return;
}

void MSGParser_Script(char *buffer)
{
    int line;
    char *loc, message[MessageLength];
    loc = nextword(buffer);
    if(!*loc)
    {
        int i;
        write("---------------------------------------------------------------SCRIPT-----------\r\n");
        for(i = 0; i < SCRIPT_LENGTH; i++)
        {
            sprintf(message, "Line %d\t%s\r\n", i, Glo.Script[i].payload);
            write(message);
        }
        return;
    }
    else if(!isdigit(*loc))
    {
        sprintf(message, "You must enter a script line to read or write to.(0-%d)\r\n", SCRIPT_LENGTH - 1);
        write(message);
        return;
    }
    line = atoi(loc);
    loc = nextword(loc);
    if(!*loc)
    {   loadScript(line);   }
    else if(strstr(loc, "clear") == loc)
    {
        Glo.Script[line].payload[0] = 0;
        write("Line cleared!\r\n");
    }
    else
    {
        strcpy(Glo.Script[line].payload, loc);
        sprintf(message, "Line %d\t%s\r\n", line, Glo.Script[line].payload);
        write(message);
    }
}

void MSGParser_Sine(char *buffer)
{
    char *loc, message[MessageLength];
    int freq, lowerindex, upperindex;
    double upperweight, lowerweight, ans;
    bool transfer;
    loc = nextword(buffer);
    if(!*loc)
    {
        transfer = SPI_transfer(Glo.Devices.spi, &Glo.Devices.spiTransaction);
        if(!transfer)
        {
            while(1);
        }
        if(Glo.Timer.period != 1000000/Glo.Fs)
        {
            if(Glo.Sine.freq > Glo.Fs/2)
            {
                Glo.Sine.freq = Glo.Fs/2;
                sprintf(message, "Sine frequency set to %d Hz to prevent aliasing.\r\n", Glo.Sine.freq);
                write(message);
            }
            Glo.Sine.LUTstep = (LUTLENGTH) * (double) Glo.Sine.freq / Glo.Fs;
            sprintf(message, "-timer %d", 1000000/Glo.Fs);
            MSGParser_Timer(message);
        }
        Glo.Sine.LUTposition += Glo.Sine.LUTstep;
        if(Glo.Sine.LUTposition >= LUTLENGTH)
        {   Glo.Sine.LUTposition -= LUTLENGTH;  }
        lowerindex = floor(Glo.Sine.LUTposition);
        upperindex = lowerindex + 1;
        upperweight = Glo.Sine.LUTposition - (double) lowerindex;
        lowerweight = 1. - upperweight;
        ans = lowerweight *(double) Glo.Sinlut[lowerindex] + upperweight *(double) Glo.Sinlut[upperindex];
        Glo.Sine.val = round(ans);
    }
    else if(isdigit(*loc))
    {
        freq = atoi(loc);
        if(freq > Glo.Fs/2)
        {
            sprintf(message, "Nyquist Frequency Limit! Must be %d Hz or less.\r\n", Glo.Fs/2);
            write(message);
        }
        else if(!freq)
        {
            MSGParser_GPIO("-gpio 4 w 1");
            MSGParser_Callback("-callback 0 0");
        }
        else
        {
            Glo.Devices.spiTransaction.txBuf = &Glo.Sine.val;
            Glo.Sine.freq = freq;
            if(freq == Glo.Fs/2)
            {   Glo.Sine.LUTposition = LUTLENGTH/4; }
            Glo.Sine.LUTstep = (LUTLENGTH) * (double) freq / Glo.Fs;
            //Turn off stream here
            MSGParser_GPIO("-gpio 4 w 0");
            sprintf(message, "-timer %d", 1000000/Glo.Fs);
            MSGParser_Timer(message);
            MSGParser_Callback("-callback 0 -1 -sine");
            sprintf(message, "Sine Frequency: %d\r\n", Glo.Sine.freq);
            write(message);
        }
    }
}

void MSGParser_Stream(char *buffer)
{
    char *loc, message[MessageLength];
    loc = nextword(buffer);
    if(!*loc)
    {
        if(Glo.ADCBuf.converting)
        {   return; }
        MSGParser_Sine("-sine 0");
        MSGParser_GPIO("-gpio 5 w 1");
        MSGParser_GPIO("-gpio 4 w 0");
        Glo.Devices.spiTransaction.txBuf = Glo.AudioTx.PongBuffTx;
        if(Glo.Fs != 8000)
        {   MSGParser_Fs("-fs 8000");   }
        MSGParser_Callback("-callback 0 -1 -audio");
        sprintf(message, "-timer %d", 1000000/Glo.Fs);
        MSGParser_Timer(message);
        if(ADCBuf_convert(Glo.Devices.adcbuf, &Glo.ADCBuf.conversion, 1) == ADCBuf_STATUS_ERROR)
        { while(1); }
        Glo.ADCBuf.converting = 1;
    }
    else if(*loc == '0')
    {
        MSGParser_GPIO("-gpio 4 w 1");
        MSGParser_GPIO("-gpio 5 w 0");
        MSGParser_Callback("-callback 0 0 0");
        if(ADCBuf_convertCancel(Glo.Devices.adcbuf) == ADCBuf_STATUS_ERROR)
        {   while(1);   }
        ClearAudioBuffers();
        Glo.ADCBuf.converting = 0;
    }
}

void MSGParser_Ticker(char *buffer)
{
    int count, delay, period, i;
    char *loc, message[MessageLength];
    uint32_t gateKey;
    loc = buffer;
    loc = nextword(loc);
    if(!*loc)
    {
        for(i = 0; i < TICKER_COUNT; i++)
        {
            sprintf(message, "Ticker %d\tD: %d\tT: %d\tC: %d\tP: %s\r\n", i, Glo.Ticker[i].delay,
                    Glo.Ticker[i].period, Glo.Ticker[i].count, Glo.Ticker[i].payload);
            write(message);
        }
        return;
    }
    else if(!isdigit(*loc))
    {
        sprintf(message, "You must enter a ticker index.\r\n");
        write(message);
        return;
    }
    i = atoi(loc);
    if(i < 0 || i >= TICKER_COUNT)
    {
        sprintf(message, "Invalid ticker index!(0-%d)\r\n", TICKER_COUNT - 1);
        write(message);
        return;
    }
    loc = nextword(loc);
    if(!*loc)
    {
        sprintf(message, "Ticker %d\tD: %d\tT: %d\tC: %d\tP: %s\r\n", i, Glo.Ticker[i].delay,
                Glo.Ticker[i].period, Glo.Ticker[i].count, Glo.Ticker[i].payload);
        write(message);
        return;
    }
    else if(!isdigit(*loc))
    {
        sprintf(message, "You must enter a delay for the ticker.\r\n");
        write(message);
        return;
    }
    delay = atoi(loc);
    loc = nextword(loc);
    if(!*loc || !isdigit(*loc))
    {
        sprintf(message, "You must enter a period for the ticker.\r\n");
        write(message);
        return;
    }
    period = atoi(loc);
    loc = nextword(loc);
    if(!*loc || !isdigit(*loc) & *loc != '-')
    {
        write("You must enter a iterative count for the ticker.\r\n");
        return;
    }
    else if(*loc == '-')
    {
        loc++;
        if(!isdigit(*loc))
        {
            write("You must enter a iterative count for the ticker.\r\n");
            return;
        }
        count = -atoi(loc);
    }
    else
    {
        count = atoi(loc);
    }
    loc = nextword(loc);
    if(!*loc)
    {
        sprintf(message, "You must enter a payload for the ticker.\r\n");
        write(message);
        return;
    }
    gateKey = GateSwi_enter(Glo.Bios.gateTicker);
    Glo.Ticker[i].count = count;
    Glo.Ticker[i].delay = delay;
    Glo.Ticker[i].period = period;
    strcpy(Glo.Ticker[i].payload, loc);
    GateSwi_leave(Glo.Bios.gateTicker, gateKey);
    sprintf(message, "Ticker %d\tD: %d\tT: %d\tC: %d\tP: %s\r\n", i, Glo.Ticker[i].delay,
            Glo.Ticker[i].period, Glo.Ticker[i].count, Glo.Ticker[i].payload);
    write(message);
}

void MSGParser_Timer(char *buffer)
{
    int period, status;
    char *loc, message[MessageLength];
    loc = buffer;
    loc = nextword(loc);
    if(!*loc)
    {
        sprintf(message, "Timer Period: %d us\r\n", Glo.Timer.period);
        write(message);
        return;
    }
    period = atoi(loc);
    if(period < MIN_PERIOD_US)
    {
        sprintf(message, "The timer period must be at least %d us.\r\n", MIN_PERIOD_US);
        write(message);
        return;
    }
    Glo.Timer.period = period;
    status = Timer_setPeriod(Glo.Devices.timer, Timer_PERIOD_US, period);
    if(status == Timer_STATUS_ERROR) {
        /* Timer_start failed */
        while(1);
    }
    sprintf(message, "-print Timer Period: %d us", Glo.Timer.period);
    addPayload(message);
}

void MSGParser_Uart(char *buffer)
{
    buffer = nextword(buffer);
    strcat(buffer, "\n");
    UART_write(Glo.Devices.uart7, buffer, strlen(buffer));
}
