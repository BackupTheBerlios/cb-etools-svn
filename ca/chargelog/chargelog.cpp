#include "wx/ctb/getopt.h"
#include "wx/ctb/iobase.h"
#include "wx/ctb/serport.h"
#include "wx/ctb/timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *chargelog_swversion = "V1.09";

// ----------------- options -------------------------------
const char *options="B:b:c:C:dDhHiIs:S:t:T:";
const char *helpOptions =
    {
        "\n"
        "usage: chargelog [commandlineoptions]                                         \n"
        "   -b9600 || 38400       : select baudrate, default is 38400                  \n"
        "   -c1    .. 99          : comport number,  default is 1                      \n"
        "   -d                    : print debug messages                               \n"
        "   -h                    : print this help text                               \n"
        "   -i                    : print info about data values in chargelog          \n"
        "   -s1    .. 3248        : start reading at this chargelog entry number       \n"
        "   -t250  .. 2500        : communication timeout in ms (default is 250ms)     \n"
        "\n"
    };

//  " log ; DATE     ; TIME     ;Type;AERR;  ACAP ; AVOLT ; ATEMP ;  EXT1 ;  EXT2 "
const char *helpInfo =
    {
        "\n"
        "Info about CA6470/72 chargelog data values\n\n"
        "Type: STATE   description          extra information values     EXT1 ;  EXT2  \n"
        "  0 : CLICAL  calibration of accu current measurem. offsets    ACIZ0 ; ACIZ1  \n"
        "  1 : CLSTART start of fast-charge (FC) cycle                 aciamp ; wNCAP  \n"
        "  2 : CLBFUL  normal end of FC by dTemperature/dt             aciamp ; dT/dt  \n"
        "  3 : CLSTOP  end of FC by limit error (T,V,I,VEXT)           aciamp ; wNCAP  \n"
        "  4 : CLLOWV  lowbat or low-voltage NMI if acc_cap > 25%%      aciamp ; wNCAP  \n"
        "  5 : CLCORR  correction of actual nominal acc_cap             wOCAP ; wCCAP  \n"
        "aciamp accu current [ 0.1mA  ] ( - load, + charge )                           \n"
        "dT/dt  delta Temperature / dt  [ 0.1*gradC / min ]                            \n"
        "wNCAP  nominal accu capacity [ 0.1mAh ]      wOCAP = old, wCCAP = corrected   \n"
        "\n"
        "AERR   hex coded accu error codes, bit descriptions                           \n"
        "       ETMAX  0x80 : max. fast-charge (FC) time (3.5h) exceeded               \n"
        "       EACCT  0x40 : accu temperature (FC start <0|>40C, stop >50C accu HOT)  \n"
        "       EACCI  0x20 : accu charging current too high (>1.6A)                   \n"
        "       EACCV  0x10 : accu voltage too high/low (FC start <8.8V, stop >14.4V)  \n"
        "       EBFUL  0x08 : battery already full (accu capacity is >90%% wNCAP)       \n"
        "       ECHT   0x04 : external charge voltage not present                      \n"
        "ACAP   accu capacity    [ 0.1mAh ]                                            \n"
        "AVOLT  accu voltage     [ mV ]                                                \n"
        "ATEMP  accu temperature [ 0.1*gradC ]                                         \n"
        "\n"
    };


// ----------------- globals -------------------------------
//wxBaud baudrate = wxBAUD_9600;
wxBaud baudrate = wxBAUD_38400;
int  devcomn = 1;
char devname[25] = "com1";
char CRLF[] = "\r\n";
long timeout = 250;
bool debug = 0;
int  startclogn = 1 ;


typedef unsigned char byte;
typedef unsigned short word;

/* structure for charge-log
   size: 20bytes         */
struct CHARGELOG
{
    byte DATE[3];       /* date YMD */
    byte TIME[3];       /* time HMS */

    byte bCLTYPE;       /* type of clog entry  */
    byte bAERR;         /* actual accu error value */

    word wACAP;         /* actual accu capacity  [0.1*mAh]   */
    word wAVOLT;        /* actual accu voltage   [0.1*V]   */
    word wATEMP;        /* actual accu temperature  [0.1*gradC]   */
    word wEXT1;         /* extended info, depends on CLTYPE */
    word wEXT2;         /* extended info, depends on CLTYPE */
    word wclcs;         /* checksum */
};


// ----------------- functions -------------------------------

// readline: read a line (without CRLF) into answer from comport dev
// returns   0 - OK, line with CRLF could be read
//           1 - read timeout error, nothing or no CRLF received
int readline( wxSerialPort *dev, char *answer )
{
    char *receivedBytes = NULL;
    int rd = dev->ReadUntilEOS(&receivedBytes,CRLF,timeout);
    if(rd > 0)
    {
        receivedBytes[rd] = 0;
        memcpy(answer,receivedBytes,rd+1);
        if(debug) printf("<%s\n",answer);
        return 0;
    }
    return 1;
}

// command : sends cmd+CRLF to instrument, receives answer (without CRLF)
//           communication error handling (retry cmd when *EC occurred)
// returns   0 - OK, answer received
//           1 - read timeout error, nothing or no CRLF received
//           2 - write timeout error, could not send
//           3 - permanent *EC communication error (2x retry before give up)
//           4 - *EX execution error
//           5 - *ES syntax error
int command( wxSerialPort *dev, char *cmd, char *answer )
{
    int cmdsize = strlen(cmd);
    int restarts = 0;

restart:
    ++restarts;
    *answer = 0;
    if(debug) printf(">%s\n",cmd);

    // we are using a timer for timeout test. After the given timeout
    // is reached, the to value will be set to 1
    int to = 0;
    timer t(timeout,&to,NULL);
    t.start();
    dev->Writev(cmd,cmdsize,&to);
    dev->Writev(CRLF,2,&to);
    if(to)
    {
        if(debug) printf("\nERROR: command %s write timeout (%d ms)\n",cmd,(int)timeout);
        t.stop();
        return 2;
    }
    t.stop();

    if ( !readline( dev, answer ))
    {
        // CA6472 communication error handling
        // communication error, flush input buffer and retry max. 2x
        if(0==strcmp(answer,"*EC"))
        {
            if (debug) printf("ERROR: *EC  communication error detected \n");
            while( !readline( dev, answer ))
            {
                if (debug) printf("ERROR: *EC  flushing receive buffer \n");
                *answer=0;
            }
            if( restarts < 3 )
            {
                if (debug) printf("\nERROR: *EC  retry #%d to send command \n", restarts);
                goto restart;
            }
            if (debug) printf("\nERROR: *EC  no more retries, aborting \n");
            return 3;
        }

        if(0==strcmp(answer,"*EX"))
        {
            if(debug) printf("ERROR: *EX  execution error detected \n");
            return 4;
        }

        if(0==strcmp(answer,"*ES"))
        {
            if(debug) printf("ERROR: *ES  syntax error detected \n");
            return 5;
        }

        //all went well !!
        return 0;
    }

    if(debug) printf("ERROR: command %s read timeout (%d ms)\n",cmd,(int)timeout);
    return 1;
}


void fprintfusedoptions (FILE *to)
{
    fprintf(to,"used options ");
    fprintf(to,"-c%d ",devcomn);
    fprintf(to,"-b%d ",baudrate);
    fprintf(to,"-s%d ",startclogn);
    fprintf(to,"-t%d ",(int)timeout);
    if(debug) fprintf(to,"-d ");
    fprintf(to,"\n");
}


void invalidarg ()
{
    fprintf(stderr,"\ninvalid argument detected\n");
    fprintf(stderr,helpOptions);
    exit(0);
}


void errorexit ( int exitcode )
{
    //TODO: print error description based on exitcode
    fprintf(stderr,"\nchargelog ERROR %d\n\n",exitcode);
    fprintfusedoptions(stderr);
    fprintf(stderr,"\n");
    exit(exitcode);
}

void get_date_time (char *res, int SIZE)
{
    time_t curtime;
    struct tm *loctime;

    /* Get the current time. */
    curtime = time (NULL);

    /* Convert it to local time representation. */
    loctime = localtime (&curtime);

    /* Print out the date and time in the standard format. */
    // sprintf(res,asctime (loctime));

    /* Print it out in a nice format. */
    strftime (res, SIZE, "%Y.%m.%d %H:%M:%S", loctime);
}


//convert from string to int
int convert ( char *ans, int start, int len, char *format )
{
    char x[] = "FFFF";
    int result=-1;
    int i=0;
    for( ; i<len ; i++ )
        x[i]=ans[start+i+6]; // first 6 chars are the rom-address
    x[i]=0;
    i = sscanf(x, format, &result);
    if( i != 1 ) errorexit(41);
    return result;
}


int main(int argc,char *argv[])
{
    int val;
    while ((val=getopt(argc,argv,(char *)options))!=EOF)
    {
        switch (val)
        {
        case 'B' :
        case 'b' :
            if ( optarg == 0 ) invalidarg();
            baudrate = (wxBaud)(strtol(optarg,NULL,10) == 9600 ? 9600 : 38400);
            break;
        case 'C' :
        case 'c' :
        {
            if ( optarg == 0 ) invalidarg();
            devcomn = strtol(optarg,NULL,10);
            if( devcomn > 0 && devcomn < 100 )
            {
                sprintf(devname,"\\\\.\\com%d",devcomn);
            }
            else invalidarg();
            break;
        }
        case 'H' :
        case 'h' :
            printf(helpOptions);
            exit(0);
        case 'I' :
        case 'i' :
            printf(helpInfo);
            exit(0);
        case 'S' :
        case 's' :
            if ( optarg == 0 ) invalidarg();
            startclogn = strtol(optarg,NULL,10);
            startclogn = startclogn > 3248 ? 3248 :
                         startclogn < 1 ? 1 : startclogn;
            break;
        case 'T' :
        case 't' :
            if ( optarg == 0 ) invalidarg();
            timeout = strtol(optarg,NULL,10);
            timeout = timeout > 2500 ? 2500 :
                      timeout < 250 ? 250 : timeout;
            break;
        case 'D' :
        case 'd' :
            debug = 1;
            break;
        }
    }

    if(debug)
    {
        printf("\n-----------------------------------------------------------------------------");
        printf("\nDEBUGSTART of %s\n",argv[0]);
        fprintfusedoptions(stdout);
    }

    // device is a serial port
    wxSerialPort *dev;
    dev = new wxSerialPort();

    // try to open the given port
    if(dev->Open(devname) < 0)
    {
        fprintf(stderr,"\nCould not open com%d\n",devcomn);
        delete dev;
        errorexit(11);
    }

    // set the baudrate
    dev->SetBaudRate(baudrate);
    // ok, device is ready for communication

    char cmd[40];
    char ans[1024];
    int ret = 0;

    if(debug) printf("\ntesting com%d with baudrate %d\n\n",devcomn,baudrate);
    ret = command(dev, "###", ans );
    if(ret!=5)      // test *ES
    {
        fprintf(stderr,"\nNo CA6470/72 detected on com%d using baudrate %d\n",devcomn,baudrate);
        baudrate = (wxBaud) ((38400 == baudrate) ? 9600 : 38400);
        fprintf(stderr,"retrying communication with baudrate %d\n",baudrate);
        dev->SetBaudRate(baudrate);

        //retrying with alternative baudrate
        ret = command(dev, "###", ans );
    }
    if(ret!=5) errorexit(12);   // test *ES

    ret = command(dev, "Vparnoux", ans );
    if (ret) errorexit(13);
    if(0!=strcmp(ans,"OK")) errorexit(14);

    ret = command(dev, "Id", ans );
    if (ret) errorexit(15);
    if(ans!=strstr(ans,"CA647")) errorexit(16);

    memcpy(cmd,ans,strlen(ans)+1); // temporary save instrument name in cmd

    ret = command(dev, "Tr", ans );
    if (ret) errorexit(17);
    if (17!=strlen(ans)) errorexit(18);

    if(debug) printf("\ncommunication is ok, used baudrate is %d\n",baudrate);

    const int datestrSIZE = 40;
    char datestr[datestrSIZE] = "get_date_time";
    get_date_time(datestr,datestrSIZE);

    printf("\n-----------------------------------------------------------------------------");
    printf("\n chargelog          system time %s     ",datestr);
    printf("\n %s          instrument time   %s    %s",chargelog_swversion,ans,cmd);
    printf("\n-----------------------------------------------------------------------------");

    const int rcldirstart  = 0x010000;           // flrom: start address of chargelog directory
    const int rclogstart   = rcldirstart + 406;  // flrom: start of first chargelog entry

    int romaddr = rcldirstart;  // romaddress for access to cl-directory
    int clogzerobytes = 0;      // number of bytes already zeroed
    int clogentries = 0;        // number of cl-entries
    int lastbyte = 0;           // last cl-directory byte, check for erased bits

    if(debug) printf("\nscan chargelog directory\n");
    while( romaddr<rclogstart && lastbyte==0 )
    {
        sprintf(cmd, "Rr%06X", romaddr++);
        ret = command(dev, cmd, ans );
        if (ret) errorexit(21);
        if (2!=strlen(ans)) errorexit(22);
        ret = sscanf(ans, "%x", &lastbyte);
        if (ret != 1) errorexit(23);

        if(lastbyte==0) ++clogzerobytes;
        else break;
    }
    clogentries = clogzerobytes*8;

    //test bits of last byte
    if(debug)
    {
        printf("last non-zerobyte: %02x\n",lastbyte);
        printf("clogentries start: %d\n", clogentries);
    }
    for (int bitmask = 0x80; bitmask != 0; bitmask>>=1)
    {
        if(debug) printf("bitmask : %02x ", bitmask);
        if((lastbyte & bitmask) == 0)
        {
            ++clogentries;
            if(debug) printf(" +  : %d\n",clogentries);
        }
        else
        {
            if(debug) printf(" - break\n");
            break;
        }
    }

    if(debug) printf("\n%d chargelogentries found\n",clogentries);

#if 0
    printf("sizeof(byte) %d\n", sizeof(byte));              //  1
    printf("sizeof(word) %d\n", sizeof(word));              //  2
    printf("sizeof(CHARGELOG) %d\n", sizeof(CHARGELOG));    // 20 -> OK
#endif

    CHARGELOG *clp;
    clp = new CHARGELOG[clogentries];   // create chargelog table

    // read the chargelogs, starting from given number
    if (!(startclogn < clogentries+1)) startclogn = clogentries;  //get at least the last chargelog
    int clog =  startclogn-1;
    const int clogsize = 20;

    //--------------"          1         2         3         4         5         6         7         8"
    //--------------"012345678901234567890123456789012345678901234567890123456789012345678901234567890"
    char header[] = " log ; DATE     ; TIME     ;Type;AERR;  ACAP ; AVOLT ; ATEMP ;  EXT1 ;  EXT2 ";

    while( clog < clogentries )
    {
        div_t temp;
        temp = div( clog - (startclogn-1), 25 );     // print the header every 25 lines

        if ( !debug && 0 == temp.rem )
        {
            printf("\n");
            printf("%s", header);
            if (debug) printf("\n");
        }

        if(debug) printf("\n-----------------------------------------------------------------------------\n");
        romaddr = rclogstart + clog * clogsize;
        sprintf(cmd, "Rd%06X%06X", romaddr, clogsize);
        ret = command(dev, cmd, ans );
        if (ret) errorexit(31);
        if (38!=strlen(ans)) errorexit(32);

        //fill CHARGELOG structure with read bytes
        char *y = (char *)(&clp[clog]);  // char pointer to clp structure
        int pos = 0;  //start position in answer string data

        //first 6 bytes, DATE & TIME, BCD coded
        for(int i=0; i<6; i++)
        {
            *y++ = convert( ans, pos, 2, "%d");
            pos += 2;
        }

        //next 2 bytes, bCLTYPE & bAERR, hex coded
        for(int i=0; i<2; i++)
        {
            *y++ = convert( ans, pos, 2, "%x");
            pos += 2;
        }

        //next word wACAP hex coded
        clp[clog].wACAP = convert( ans, pos, 4, "%x");
        pos += 4;

        //next word wAVOLT ,hex coded
        clp[clog].wAVOLT = convert( ans, pos, 4, "%x");
        pos += 4;

        //next word wATEMP ,hex coded
        clp[clog].wATEMP = convert( ans, pos, 4, "%x");
        pos += 4;

        //next word wEXT1 ,hex coded
        clp[clog].wEXT1 = convert( ans, pos, 4, "%x");
        pos += 4;

        //get next data line from receive buffer
        ret = readline( dev, ans );
        if (ret) errorexit(33);
        if (38!=strlen(ans)) errorexit(34);

        //next word wEXT2 ,hex coded
        clp[clog].wEXT2 = convert( ans, 0, 4, "%x");

        //test if we are really at the end of the chargelog
        if ( clog == clogentries - 1 )
            if(0!=strcmp(&ans[8+6],"FFFFFFFFFFFFFFFFFFFFFFFF")) errorexit(35);

        //get terminating "FF" from receive buffer
        ret = readline( dev, ans );
        if (ret) errorexit(36);
        if(0!=strcmp(ans,"FF")) errorexit(37);

        if(debug) printf("%s", header);
        //"          1         2         3         4         5         6         7         8
        //"012345678901234567890123456789012345678901234567890123456789012345678901234567890
        //" log ; DATE     ; TIME     ;Type;AERR;  ACAP ; AVOLT ; ATEMP ;  EXT1 ;  EXT2 "
        //"   1 ; 05 12 31 ; 00 01 02 ;  0 ; 84 ;    84 ; 10595 ;  1600 ;  4259 ;  4242 "
        printf("\n");
        //printf("\nlog        ");
        printf("%4d ; ",clog+1);
        //printf("\nDATE       ");
        printf("%02d-%02d-%02d ",clp[clog].DATE[0],clp[clog].DATE[1],clp[clog].DATE[2]);
        printf("; ");
        //printf("\nTIME       ");
        printf("%02d:%02d:%02d ",clp[clog].TIME[0],clp[clog].TIME[1],clp[clog].TIME[2]);
        printf("; ");

        //printf("\nbCLTYPE    ");
        printf("%2d ",clp[clog].bCLTYPE);
        printf("; ");

        //printf("\nbAERR      ");
        printf("%2X ",clp[clog].bAERR);
        printf("; ");

        //printf("\nwACAP      ");
        printf("%5d ",clp[clog].wACAP);
        printf("; ");

        //printf("\nwAVOLT     ");
        printf("%5d ",clp[clog].wAVOLT);
        printf("; ");

        //printf("\nwATEMP     ");
        printf("%5d ",clp[clog].wATEMP);
        printf("; ");

        //printf("\nwEXT1      ");
        int ext1 = clp[clog].wEXT1;
        switch ( clp[clog].bCLTYPE )
        {
        case 1:
        case 2:
        case 3:
        case 4:
            if (ext1 > 32767) ext1 = ext1-65536;
        default:
            break;
        }
        printf("%5d ",ext1);
        printf("; ");

        //printf("\nwEXT2      ");
        printf("%5d ",clp[clog].wEXT2);

        //process next chargelog entry
        ++clog;
    } // while( clog < clogentries )
    printf("\n-----------------------------------------------------------------------------\n");

    //cleanup
    delete clp;
    dev->Close();
    delete dev;
    return 0;
}
