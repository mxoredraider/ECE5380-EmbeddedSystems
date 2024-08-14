/*
 * parser.c
 *
 *  Created on: Jun 6, 2024
 *      Author: Mason Obregon
 */

#include "header.h"


void MSGParser(char *buffer)
{
    int i, l;
    buffer = eatspaces(buffer);
    if(!*buffer)
    {   return; }
    char *ptr = buffer;
    char bufflow[MessageLength];
    l = strlen(buffer);
    for (i = 0; i <= l; i++)    //convert all characters in string to lowercase
    {
        bufflow[i] = tolower(*ptr);
        ptr++;
    }

    if(strcmp(bufflow, "-about") == 0)     //parse string
    {   MSGParser_About(bufflow);   }
    else if(strstr(bufflow, "-audio") == bufflow)
    {   MSGParser_Audio(bufflow);   }
    else if(strstr(bufflow, "-callback") == bufflow)
    {   MSGParser_Callback(bufflow);    }
    else if(strcmp(bufflow, "cls") == 0 || strcmp(bufflow, "-cls") == 0)
    {   write("\xc"); }
    else if(strcmp(bufflow, "-error") == 0)
    {   MSGParser_Error();  }
    else if(strstr(bufflow, "-fs") == bufflow)
    {   MSGParser_Fs(bufflow);  }
    else if(strstr(bufflow, "-gpio") == bufflow)
    {   MSGParser_GPIO(bufflow);    }
    else if(strstr(bufflow, "-help") == bufflow)
    {   MSGParser_Help(bufflow);    }
    else if(strstr(bufflow,"-if") == bufflow)
    {   MSGParser_If(bufflow);  }
    else if(strstr(bufflow, "-memr") == bufflow)
    {   MSGParser_Memr(bufflow);    }
    else if(strstr(bufflow, "-print") == bufflow)
    {   MSGParser_Print(buffer);    }
    else if(strstr(bufflow, "-reg") == bufflow)
    {   MSGParser_Reg(bufflow); }
    else if(strstr(bufflow, "-rem") == bufflow)
    {   return; }
    else if(strstr(bufflow, "-script") == bufflow)
    {   MSGParser_Script(bufflow);  }
    else if(strstr(bufflow, "-sine") == bufflow)
    {   MSGParser_Sine(bufflow);    }
    else if(strstr(bufflow, "-stream") == bufflow)
    {   MSGParser_Stream(bufflow);  }
    else if(strstr(bufflow, "-ticker") == bufflow)
    {   MSGParser_Ticker(bufflow);  }
    else if(strstr(bufflow, "-timer") == bufflow)
    {   MSGParser_Timer(bufflow);   }
    else if(strstr(bufflow, "-uart") == bufflow)
    {   MSGParser_Uart(bufflow);    }
    else
    {   error_unknown(bufflow); }

}

