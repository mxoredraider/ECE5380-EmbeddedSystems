/*
 * infrastructure.c
 *
 *  Created on: Jun 8, 2024
 *      Author: Mason Obregon
 */

#include "header.h"

extern GateSwi_Handle gateSwi0, gateSwi1, gateSwi2;
extern Semaphore_Handle semaphore0, semaphore1;
extern Swi_Handle swi0, swi1, swi2, swi3;

void addchar(char input)
{
    int i;
    int l = strlen(Glo.messagebuffer);
    char message[3 * MessageLength]; message[0] = 0;
    if(l >= MessageLength - 2)
    {
        addError("buffoverflow");
        addPayload("-print \r\nThe message is too long...clearing message buffer.");
        Glo.messagei = 0;
        Glo.messagebuffer[Glo.messagei] = 0;
        return;
    }
    for(i = l; i >= Glo.messagei; i--)
    {
        Glo.messagebuffer[i + 1] = Glo.messagebuffer[i];
        if(Glo.messagebuffer[i])
        {   strcat(message, "\x1b[D");  }
    }
    Glo.messagebuffer[Glo.messagei] = input;
    write(Glo.messagebuffer + Glo.messagei);
    write(message);
    Glo.messagei++;
}

void addCommand(char *buffer)
{
    int i;
    if(Glo.cmd.cmdsaved < PREV_CMD_COUNT - 1)
    {   Glo.cmd.cmdsaved++; }
    for(i = Glo.cmd.cmdsaved; i > 1; i--)
    {
        strcpy(Glo.cmd.message[i].buffer, Glo.cmd.message[i-1].buffer);
    }
    strcpy(Glo.cmd.message[1].buffer, buffer);
}

void addError(char *error)
{
    if(strcmp(error, "buffoverflow") == 0)
    {   Glo.Err.buffoverflow++; }
    else if(strcmp(error, "commandunknown") == 0)
    {   Glo.Err.commandunknown++;   }
    else if(strcmp(error, "qoverflow") == 0)
    {   Glo.Err.qoverflow++;    }
}

void addPayload(char *buffer)
{
    uint32_t gateKey;
    int Qnext;
    gateKey = GateSwi_enter(Glo.Bios.gatePayload);

    if(Glo.Q.write == QLENGTH - 1)
    {   Qnext = 0; }
    else
    {   Qnext = Glo.Q.write + 1; }
    if(Qnext == Glo.Q.read)
    {
        addError("qoverflow");
        GateSwi_leave(Glo.Bios.gatePayload, gateKey);
        return;
    }
    strcpy(Glo.Q.payload[Glo.Q.write].buffer, buffer);
    Glo.Q.write = Qnext;
    Semaphore_post(Glo.Bios.semPayload);
    GateSwi_leave(Glo.Bios.gatePayload, gateKey);
}

void Device_init()
{
    ADCBuf_Handle adcbuf;
    ADCBuf_Params adcbufParams;
    SPI_Params spiParams;
    SPI_Handle spi;
    Timer_Params timerParams;
    Timer_Handle timer0, timer1;
    UART_Params uartParams;
    UART_Handle uart, uart7;
    int status, i;

    /* Call driver init functions */
    ADCBuf_init();
    GPIO_init();
    SPI_init();
    Timer_init();
    UART_init();

    /* Create ADCBuf */
    ADCBuf_Params_init(&adcbufParams);
    adcbufParams.returnMode = ADCBuf_RETURN_MODE_CALLBACK;
    adcbufParams.recurrenceMode = ADCBuf_RECURRENCE_MODE_CONTINUOUS;
    adcbufParams.callbackFxn = Callback_ADCBuf;
    adcbufParams.samplingFrequency = 8000;

    adcbuf = ADCBuf_open(CONFIG_ADCBUF_0, &adcbufParams);
    if(adcbuf == NULL) {
        while(1);
    }

    ADCBuf_Conversion conversion = {0};
    conversion.adcChannel = ADCBUF_CHANNEL_0;
    conversion.sampleBuffer = Glo.ADCBuf.PingBuffRx;
    conversion.sampleBufferTwo = Glo.ADCBuf.PongBuffRx;
    conversion.samplesRequestedCount = AUDIO_BLOCK_SIZE;


    /* Create SPI */
    SPI_Params_init(&spiParams);
    spiParams.dataSize = 16;
    spiParams.frameFormat = SPI_POL0_PHA1;

    spi = SPI_open(CONFIG_SPI_0, &spiParams);
    if(spi == NULL) {
        while(1);
    }

    /* Create Timer 0 */
    Timer_Params_init(&timerParams);
    timerParams.periodUnits = Timer_PERIOD_US;
    timerParams.period = 1000000;
    timerParams.timerMode = Timer_CONTINUOUS_CALLBACK;
    timerParams.timerCallback = Callback0_Timer0;

    timer0 = Timer_open(CONFIG_TIMER_0, &timerParams);
    if (timer0 == NULL) {
        /* Timer_open() failed */
        while (1);
    }

    /* Create Timer 1 */
    Timer_Params_init(&timerParams);
    timerParams.periodUnits = Timer_PERIOD_HZ;
    timerParams.period = 100;
    timerParams.timerMode = Timer_CONTINUOUS_CALLBACK;
    timerParams.timerCallback = Ticker_Timer1;

    timer1 = Timer_open(CONFIG_TIMER_1, &timerParams);
    if (timer1 == NULL) {
        /* Timer_open() failed */
        while (1);
    }

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.baudRate = 115200;

    uart = UART_open(CONFIG_UART_0, &uartParams);
    if (uart == NULL) {
        /* UART_open() failed */
        while (1);
    }

    /* Create UART7 with data processing on. */
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readReturnMode = UART_RETURN_NEWLINE;
    uartParams.baudRate = 115200;
    uartParams.readEcho = UART_ECHO_OFF;

    uart7 = UART_open(CONFIG_UART_1, &uartParams);
    if (uart7 == NULL) {
        /* UART_open() failed */
        while (1);
    }

    /* Bios Handles */
    Glo.Bios.gatePayload    = gateSwi0;
    Glo.Bios.gateCallback   = gateSwi1;
    Glo.Bios.gateTicker     = gateSwi2;
    Glo.Bios.semPayload     = semaphore0;
    Glo.Bios.semADCBuf      = semaphore1;
    Glo.Bios.switimer       = swi0;
    Glo.Bios.swisw1         = swi1;
    Glo.Bios.swisw2         = swi2;
    Glo.Bios.switicker      = swi3;

    /* Device Handles */
    Glo.Devices.adcbuf      = adcbuf;
    Glo.Devices.spi         = spi;
    Glo.Devices.timer       = timer0;
    Glo.Devices.ticker      = timer1;
    Glo.Devices.uart        = uart;
    Glo.Devices.uart7       = uart7;

    /* Initialize ADCBuf */
    Glo.ADCBuf.conversion = conversion;
    Glo.ADCBuf.completedRx = 0;
    Glo.ADCBuf.converting = 0;

    /* Initialize Audio Buffers */
    ClearAudioBuffers();

    /* Initialize Message Buffer */
    Glo.messagebuffer[0] = 0;
    Glo.messagei = 0;

    /* Initialize Previous Commands structure */
    Glo.cmd.index = 0;
    Glo.cmd.cmdsaved = 0;

    /* Initialize Callback structure */
    for(i = 0; i < CALLBACK_COUNT; i++)
    {
        Glo.Callback[i].index = i;
        Glo.Callback[i].count = 0;
        Glo.Callback[i].payload[0] = 0;
    }

    /* Initialize Registers */
    for(i = 0; i < REG_COUNT; i++)
    {
        Glo.Reg[i].index = i;
        Glo.Reg[i].value = 10 * i;
    }

    /* Initialize Scipt */
    for(i = 0; i < SCRIPT_LENGTH; i++)
    {
        Glo.Script[i].index = i;
        Glo.Script[i].payload[0] = 0;
    }

    /* Initialize Sample Frequency */
    Glo.Fs = 8000;

    /* Initialize Sine */
    SinLUT();
    Glo.Sine.LUTposition = 0;
    Glo.Sine.LUTstep = 0;
    Glo.Sine.freq = 0;
    Glo.Sine.val = 0;

    /* Initialize SPI count to 1 */
    Glo.Devices.spiTransaction.count = 1;

    /* Initialize Timer */
    Glo.Timer.period = 1000000;

    /* Initialize Error Counts */
    Glo.Err.buffoverflow = 0;
    Glo.Err.commandunknown = 0;
    Glo.Err.qoverflow = 0;

    /* Initialize Tickers */
    for(i = 0; i > TICKER_COUNT; i++)
    {
        Glo.Ticker[i].index = i;
        Glo.Ticker[i].count = 0;
        Glo.Ticker[i].delay = 0;
        Glo.Ticker[i].period = 0;
        Glo.Ticker[i].payload[0] = 0;
    }

    /* Enable GPIO Interrupts */
    GPIO_enableInt(GPIO_SW1);
    GPIO_enableInt(GPIO_SW2);

    /* Start timers */
    status = Timer_start(timer0);
    if(status == Timer_STATUS_ERROR) {
        /* Timer_start failed */
        while(1);
    }

    status = Timer_start(timer1);
    if(status == Timer_STATUS_ERROR) {
        /* Timer_start failed */
        while(1);
    }
}

void ClearAudioBuffers()
{
    int i;
    for(i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        Glo.AudioTx.PingBuffTx[i] = AUDIO_INIT;
        Glo.AudioTx.PongBuffTx[i] = AUDIO_INIT;
    }
    Glo.AudioTx.completedTx = 0;
    Glo.AudioTx.readindex = 0;
    Glo.AudioTx.state = 0;
}

bool checkRegIn(char *input, char in)
{
    int reg;
    char message[MessageLength];
    if(*input != '#')
    {
        if(!isdigit(*input))
        {
            sprintf(message, "You must input the argument for input %c.\r\n", in);
            write(message);
            return 0;
        }
        reg = atoi(input);
        if(reg < 0 || reg >= REG_COUNT)
        {
            sprintf(message, "Invalid register index for input %c! Must be 0-%d\r\n", in, REG_COUNT - 1);
            write(message);
            return 0;
        }
    }
    if(*input == '#')
    {   input++;    }
    if(*input != '-' && !isdigit(*input))
    {
        sprintf(message, "You must input the argument for input %c.\r\n", in);
        write(message);
        return 0;
    }
    if(*input == '-')
    {   input++;    }
    if(!isdigit(*input))
        {
            sprintf(message, "You must input the argument for input %c.\r\n", in);
            write(message);
            return 0;
        }
    return 1;
}

void deletechar()
{
    int i;
    int l = strlen(Glo.messagebuffer);
    char message[2]; message[1] = 0;
    Glo.messagei--;
    write("\x1b[D");
    for(i = Glo.messagei; i < l; i++)
    {
        Glo.messagebuffer[i] = Glo.messagebuffer[i + 1];
        message[0] = Glo.messagebuffer[i];
        if(message[0] == 0)
        {   write(" "); }
        else
        {   write(message); }
    }
   // write(" ");
    for(i = l; i > Glo.messagei; i--)
    {   write("\x1b[D");    }
}

void escapeseq(char seq)
{
    if(seq == 'A')
    {
        if(Glo.cmd.index < Glo.cmd.cmdsaved)
        {
            Glo.cmd.index++;
            loadCommand();
        }
    }
    else if(seq == 'B')
    {
        if(Glo.cmd.index > 0)
        {
            Glo.cmd.index--;
            loadCommand();
        }
    }
    else if(seq == 'C')
    {
        if(Glo.messagei < strlen(Glo.messagebuffer))
        {
            Glo.messagei++;
            write("\x1b[C");
        }
    }
    else if(seq == 'D')
    {
        if(Glo.messagei > 0)
        {
            Glo.messagei--;
            write("\x1b[D");
        }
    }
    else if(seq == '3')
    {
        UART_read(Glo.Devices.uart, &seq, 1);
        if(seq == '~')
        {
            Glo.messagei++;
            write("\x1b[C");
            deletechar();
        }
    }
}

char error_nospace(char *buffer, int l)
{
    int i;
    char input;
    char error[MessageLength], fix[MessageLength];  //error message
    char yn[] = "You must enter y/n.\r\n";
    memset(error, 0, MessageLength);
    memset(fix, 0, MessageLength);
    for (i = 0; i < l; i++)
    {
        fix[i] = *buffer;
        buffer++;
    }
    strcat(fix, " ");
    strcat(fix, buffer);
    sprintf(error, "Did you mean '%s' [y/n]\r\n", fix);
    UART_write(Glo.Devices.uart, error, strlen(error));

    while(1)
    {
        UART_read(Glo.Devices.uart, &input, 1);
        UART_write(Glo.Devices.uart, &input, 1);
        UART_write(Glo.Devices.uart, Glo.var.enter, strlen(Glo.var.enter));
        input = tolower(input);
        if(input == 'y' || input == 'n')
        {   return input; }
        else
        {
            UART_write(Glo.Devices.uart,yn,strlen(yn));
        }
    }
}

void error_unknown(char *buffer)
{
    char message[MessageLength];
    addError("commandunknown");
    sprintf(message, "'%s' is not recognized.\r\n", buffer);
    write(message);
}

int getRegIn(char *input)
{
    int val;
    if(*input == '#')
    {
        input++;
        if(*input == '-')
        {
            input++;
            val = -atoi(input);
        }
        else
        {
            val = atoi(input);
        }
    }
    else
    {   val = Glo.Reg[atoi(input)].value;   }
    return val;
}

void help()
{
    write("-----------------------------\r\n");
    write("Brief description of command.\r\n\n");
    write("-help [Command]\r\n\n");
    write("Supported Commands:\r\n");
    write("-about\r\n");
    write("-callback\r\n");
    write("-cls (or cls)\r\n");
    write("-error\r\n");
    write("-gpio\r\n");
    write("-if\r\n");
    write("-memr\r\n");
    write("-print\r\n");
    write("-reg\r\n");
    write("-script\r\n");
    write("-ticker\r\n");
    write("-timer\r\n");
    write("-uart\r\n");
}

void help_about()
{
    write("-----------------------------\r\n");
    write("Displays the properties of the Command Line Interface.\r\n\n-about\r\n\n");
}

void help_callback()
{
    write("-----------------------------\r\n");
    write("Sets the payload and count for the callback function.\r\n\n");
    write("-callback [Index] [Count] [Payload]\r\n\n");
    write("Callback 0: Timer\r\nCallback 1: Left Switch 1\r\nCallback 2: Right Switch 2\r\n\n");
    write("Ex:\r\n");
    write("-callback\t\t\t| Show all callbacks.\r\n");
    write("-callback 1\t\t\t| Display callback 1.\r\n");
    write("-callback 2 10 -print hello\t| Print 'hello' when SW2 is pressed(10 times).\r\n");
}

void help_cls()
{
    write("-----------------------------\r\n");
    write("Clears the screen.\r\n\n-cls (or cls)\r\n\n");
}

void help_error()
{
    write("-----------------------------\r\n");
    write("List the error types and recorded occurrences of those errors.\r\n\n");
    write("Errors:\r\n");
    write("Buffer Overflow\t| Too many characters were typing into the message.\r\n");
    write("Unknown Command\t| The entered command was not recognized by the parser.\r\n");
    write("Queue Overflow\t| Amount of commands thrown away due to queue overflow.\r\n");

}

void help_gpio()
{
    write("-----------------------------\r\n");
    write("Read/Write to the GPIOs.\r\n\n");
    write("-gpio [Index(0-5)] [r/w/t] [WriteValue]\r\n\n");
    write("GPIO 0: LED 1\r\nGPIO 1: LED 2\r\nGPIO 2: LED 3\r\n");
    write("GPIO 3: LED 4\r\nGPIO 4: Amp On/Off\r\nGPIO 5: Mic On/Off\r\n\n");
    write("Ex:\r\n");
    write("-gpio 2 r\t| Read the value at GPIO 2.\r\n");
    write("-gpio 0 w 1\t| Write a 1 to GPIO 0.\r\n");
    write("-gpio 5 t\t| Toggle the value at GPIO 5.\r\n");
}

void help_if()
{
    write("-----------------------------\r\n");
    write("Conditional statement that performs PayloadT when true and PayloadF when false.\r\n\n");
    write("-if [A] [>/=/<] [B] ? [PayloadT] : [PayloadF]\r\n");
    write("A: Register A (or (#)immediate value)\r\n");
    write("B: Register B (or (#)immediate value)\r\n");
    write("PayloadT: Payload that runs when condition is true\r\n");
    write("PayloadF: Payload that runs when condition is false\r\n\n");
    write("Ex:\r\n");
    write("-if #10 > #1 ? -print true:-print false\t| Prints true.\r\n");
    write("-if 31 < 1 ? -gpio 0 w 1:-gpio 3 w 1\t| Turns on LED 3 if Reg 1>Reg 31,\r\n"
            "\t\t\t\t\t| else turns LED 0 on.\r\n");
}

void help_memr()
{
    write("-----------------------------\r\n");
    write("Lists the contents of the memory address (in hex).\r\n\n");
    write("-memr [Address (in hex)]\r\n\n");
    write("Valid Memory Addresses:\r\n");
    write("0x00000000 to 0x000FFFFF\r\n");
    write("0x20000000 to 0x2003FFFF\r\n");
}

void help_print()
{
    write("-----------------------------\r\n");
    write("Prints the message on the next line in the command window.\r\n\n");
    write("-print [message]\r\n");
}

void help_reg()
{
    write("-----------------------------\r\n");
    write("Set of 32 registers that can perform simple operations and store those values.\r\n\n");
    write("-reg [Output Reg] [Operation] [Input 1] [Input 2]\r\n\n");
    write("Supported Operations:\r\n");
    write("r\t| Display the value of the output register.\r\n");
    write("w\t| Write the value of input 1 into the output register.\r\n");
    write("++\t| Increment the value of the output register.\r\n");
    write("--\t| Decrement the value of the output register.\r\n");
    write("+\t| Sum the contents of input 1 and 2 and store in the output register.\r\n");
    write("-\t| Subtract the contents of input 2 from input 1 and store in the output\r\n"
            "\t| register.\r\n");
    write("\t| Also stores the negative value of input 1 into output in input 2 is\r\n"
            "\t| absent.\r\n");
    write("~\t| Logical NOT of input 1, stores in output register.\r\n");
    write("&\t| Logical AND of input 1 and 2, stores in output register\r\n");
    write("|\t| Logical OR of input 1 and 2, stores in output register.\r\n");
    write("^\t| Logical XOR of input 1 and 2, stores in the output register.\r\n");
    write("*\t| Stores product of input 1 and 2 into the output register.\r\n");
    write("/\t| Stores the quotient of numerator input 1, and denominator input 2\r\n"
            "\t| into the output register.\r\n");
    write("%\t| Stores the modulus of numerator input 1 and denominator input 2\r\n"
            "\t| into the output register.\r\n");
    write("max\t| Stores the max of input 1 and 2 into the ouptut register.\r\n");
    write("min\t| Stores the min of input 1 and 2 into the output register.\r\n");
    write("=\t| Copies the value of input 1 into the output register.\r\n");
    write("x\t| Exchanges the values of the output register and input 1 register.\r\n");
    write("Note: All operations are expecting register indices except 'r' and 'w',\r\n"
            "'#' preceding inputs can be used to indicate immediate constants.\r\n");


}

void help_script()
{
    write("-----------------------------\r\n");
    write("64 Lines of space to organize and run different commands all at once.\r\n"
            "Different scripts are separated by empty lines.\r\n\n");
    write("-script [Line] [Payload]\r\n\n");
    write("Ex:\r\n");
    write("-script 16\t\t| Runs script from line 16 until an empty line.\r\n");
    write("-script 4 -print hello\t| Writes '-print hello' to line 4.\r\n");
    write("-script 50 clear\t| Empties line 50.\r\n");
}

void help_ticker()
{
    write("-----------------------------\r\n");
    write("Forward schedule events at multiples of the tick interval of 10ms.\r\n\n");
    write("-ticker [Index] [Delay] [Period] [Count] [Payload]\r\n");
    write("Index: 0-15\r\n");
    write("Delay: Ticks before the first event\r\n");
    write("Period: Ticks in between the events\r\n");
    write("Count: Number of times for event to occur\r\n");
    write("Payload: Event to occur\r\n\n");
    write("Ex:\r\n");
    write("-ticker 0 10 100 10 -print hello | Print hello every sec after 0.1s delay.\r\n");

}

void help_timer()
{
    write("-----------------------------\r\n");
    write("Set period of timer(in increments of 10 us).\r\n\n");
    write("-timer [Period]\r\n\n");
    write("Ex:\r\n");
    write("-timer\t\t| Displays the period of the timer.\r\n");
    write("-timer 10000\t| Set the period of the timer to 1 s(1,000,000 us).\r\n");
}

void help_uart()
{
    write("-----------------------------\r\n");
    write("Writes payload out Pin PC5(TX) via UART.(RX receives on Pin PC4)\r\n\n");
    write("-uart [Payload]\r\n\n");
    write("Ex:\r\n");
    write("-uart -print hello\t| Writes command out UART to print hello.\r\n");
}

void loadCommand()
{
    char message[MessageLength]; message[0] = 0;
    int i;
    for(i = 0; i <= strlen(Glo.messagebuffer); i++)
    {   strcat(message, "\x7f");    }
    write(message);
    strcpy(Glo.messagebuffer, Glo.cmd.message[Glo.cmd.index].buffer);
    write(Glo.messagebuffer);
    Glo.messagei = strlen(Glo.messagebuffer);
}

void loadScript(int line)
{
    int i;
    char payload[MessageLength];
    for(i = 0; i < SCRIPT_MAX - 1; i++)
    {
        if(!Glo.Script[line].payload[0] || line >= SCRIPT_LENGTH)
        {   return; }
        addPayload(Glo.Script[line].payload);
        line++;
    }
    if(!Glo.Script[line].payload[0] || line >= SCRIPT_LENGTH)
    {   return; }
    else
    {
        sprintf(payload, "-script %d", line);
        addPayload(payload);
    }
}

char *eatspaces(char *buffer)
{
    while (*buffer == ' ')  //Eat all spaces before or between words
    {   buffer++;   }
    return buffer;
}

char *nextword(char *ptr)
{
    while(*ptr != ' ' && *ptr)
    {   ptr++;  }
    ptr = eatspaces(ptr);
    return ptr;
}

void write(char *buffer)
{
    UART_write(Glo.Devices.uart, buffer, strlen(buffer));
}
