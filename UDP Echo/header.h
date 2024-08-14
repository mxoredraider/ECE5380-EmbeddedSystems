/*
 * header.h
 *
 *  Created on: Jun 5, 2024
 *      Author: Mason Obregon
 */

#ifndef HEADER_H_
#define HEADER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* Driver Header files */
#include <ti/drivers/ADCBuf.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/SPI.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* Bios Libraries*/
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/gates/GateSwi.h>

void Device_init();

/* Tasks */
void taskFirst();
void taskADCStream();
void taskUART0();
void taskUART7();
void taskPayload();

/* Callbacks */
void Callback_ADCBuf();
void Callback0_Timer0();
void Callback1_LeftSW1();
void Callback2_RightSw2();
void Ticker_Timer1();
void Swi_Timer0();
void Swi_Ticker();
void Swi_LeftSW1();
void Swi_RightSW2();

void addchar(char input);
void addCall(uint32_t addr);
void removeCall(uint32_t addr);
void addCommand(char *buffer);
void addDiscover(uint32_t addr);
void removeDiscover(uint32_t addr);
void addError(char *error);
void addPayload(char *buffer);
void ClearAudioBuffers();
bool checkRegIn(char *input, char in);
void deletechar();
char *eatspaces(char *buffer);
void escapeseq(char seq);
char error_nospace(char *buffer, int l);
void error_unknown(char *buffer);
uint32_t getAddr(char *addrstr);
int getRegIn(char *input);
void help();
void help_about();
void help_callback();
void help_cls();
void help_error();
void help_gpio();
void help_if();
void help_memr();
void help_print();
void help_reg();
void help_script();
void help_ticker();
void help_timer();
void help_uart();
void loadCommand();
void loadScript(int line);
void SinLUT();


void MSGParser(char *buffer);
void MSGParser_About(char *buffer);
void MSGParser_Audio(char *buffer);
void MSGParser_Callback(char *buffer);
void MSGParser_Dial(char *buffer);
void MSGParser_Discover(char *buffer);
void MSGParser_Error();
void MSGParser_Fs(char *buffer);
void MSGParser_GPIO(char *buffer);
void MSGParser_Help(char *buffer);
void MSGParser_If(char *buffer);
void MSGParser_Memr(char *buffer);
void MSGParser_NETUDP(char *buffer, int bytecount);
void MSGParser_Print(char *buffer);
void MSGParser_Reg(char *buffer);
void MSGParser_Script(char *buffer);
void MSGParser_Sine(char *buffer);
void MSGParser_Stream(char *buffer);
void MSGParser_Ticker(char *buffer);
void MSGParser_Timer(char *buffer);
void MSGParser_Uart(char *buffer);
void MSGParser_Voice(char *buffer, uint32_t addr);

char *nextword(char *ptr);

void write(char *buffer);

#define ADC_DELAY           8
#define AUDIO_BLOCK_SIZE    128
#define AUDIO_INIT          8192
#define CALLBACK_COUNT      3
#define DISCOVER_COUNT      8
#define GPIO_OUT_COUNT      6
#define GPIO_SW1            6
#define GPIO_SW2            7
#define LUTLENGTH           256
#define MessageLength       80
#define MIN_PERIOD_US       100
#define UDPPACKET_SIZE      300
#define PREV_CMD_COUNT      16
#define QLENGTH             32
#define REG_COUNT           32
#define SCRIPT_LENGTH       64
#define SCRIPT_MAX          8
#define TICKER_COUNT        16
#define UDPPACKETSIZE       1472



extern const char cls[];

typedef struct _adcbuf
{
    uint16_t PingBuffRx[AUDIO_BLOCK_SIZE];
    uint16_t PongBuffRx[AUDIO_BLOCK_SIZE];
    uint32_t microvoltBuffer[AUDIO_BLOCK_SIZE];
    uint16_t *completedRx;
    int converting; //0-idle, 1-converting ping, 2-converting pong
    ADCBuf_Conversion conversion;
} ADCBuf;

typedef struct _audiotx
{
    uint16_t PingBuffTx[AUDIO_BLOCK_SIZE];
    uint16_t PongBuffTx[AUDIO_BLOCK_SIZE];
    uint16_t *completedTx;
    int readindex; //Index reading for ping or pong
    int state; //0-idle, 1-read ping, 2-read pong
} AudioTx;

typedef struct _audiorx
{
    uint16_t PingBuffRx[AUDIO_BLOCK_SIZE];
    uint16_t PongBuffRx[AUDIO_BLOCK_SIZE];
    uint16_t *completedRx;
} AudioRx;

typedef struct _audioqueue
{
    int8_t index[QLENGTH];
    bool choice[QLENGTH]; //0-ping, 1-pong
    int read;
    int write;
} AudioQueue;

typedef struct _bios
{
    GateSwi_Handle gatePayload;
    GateSwi_Handle gateCallback;
    GateSwi_Handle gateTicker;
    GateSwi_Handle gateUDPOut;
    GateSwi_Handle gateAudio;
    Semaphore_Handle semPayload;
    Semaphore_Handle semADCBuf;
    Semaphore_Handle semUDPOut;
    Swi_Handle switicker;
    Swi_Handle switimer;
    Swi_Handle swisw1;
    Swi_Handle swisw2;
    Swi_Handle swiadcbuf;
} Bios;

typedef struct _call
{
    uint32_t list[DISCOVER_COUNT];
    int callcount;
    bool local; //Flag to tell whether call is local or over IP
} Call;

typedef struct _callback
{
    int index;
    int count;
    char payload[MessageLength];
} Callback;

typedef struct _devices
{
    ADCBuf_Handle adcbuf;
    SPI_Handle spi;
    SPI_Transaction spiTransaction;
    Timer_Handle ticker;
    Timer_Handle timer;
    UART_Handle uart;
    UART_Handle uart7;
} Devices;

typedef struct _errors
{
    int buffoverflow;
    int commandunknown;
    int qoverflow;
} Errors;

typedef struct _ip
{
    uint32_t broadcast;
    uint32_t host;
    uint32_t localhost;
    uint32_t multicast;
    uint32_t discover[DISCOVER_COUNT];
    int addrcount;
} IP;

typedef struct _payload
{
    char buffer[MessageLength];
} Payload;

typedef struct _commands
{
    int index;
    int cmdsaved;
    Payload message[PREV_CMD_COUNT];
} Commands;

typedef struct _queue
{
    Payload  payload[QLENGTH];
    int read;
    int write;
} Queue;

typedef struct _queueudp
{
    char buffer[QLENGTH][UDPPACKET_SIZE];
    int read;
    int write;
    int bytecount[QLENGTH];
} QueueUDP;

typedef struct _register
{
    int index;
    int value;
} Register;

typedef struct _script
{
    int index;
    char payload[MessageLength];
} Script;

typedef struct _sine
{
    int freq;
    uint16_t val;
    double LUTposition;
    double LUTstep;
} Sine;

typedef struct _ticker
{
    int index;
    int count;
    int delay;
    int period;
    char payload[MessageLength];
} Ticker;

typedef struct _timer
{
    int period;     // Period in us
} Timer;

typedef struct _variables
{
    //const char  echoPrompt[MessageLength];
    char  *enter;
    char  *cls;  //clear the screen
} Variables;

typedef struct _globals
{
    char        messagebuffer[MessageLength];
    int         messagei;
    char        uart7buffer[MessageLength];
    ADCBuf      ADCBuf;
    AudioRx     AudioRx[DISCOVER_COUNT];
    AudioTx     AudioTx[DISCOVER_COUNT];
    AudioQueue  AudioQ;
    uint16_t    AudioOut;
    Bios        Bios;
    Call        Call;
    Callback    Callback[CALLBACK_COUNT];
    Commands    cmd;
    Devices     Devices;
    Errors      Err;
    int         Fs;
    IP          IP;
    Queue       Q;
    QueueUDP    QUDPOut;
    Register    Reg[REG_COUNT];
    Script      Script[SCRIPT_LENGTH];
    Sine        Sine;
    uint16_t    Sinlut[LUTLENGTH + 1];
    Ticker      Ticker[TICKER_COUNT];
    Timer       Timer;
    Variables   var;
} Globals;

#ifndef MAIN
extern
#endif
Globals Glo;

#endif /* HEADER_H_ */




