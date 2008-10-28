    #include <iostream>
#include <string>
#include <fstream>
#include <windows.h>
using namespace std;

int main(int argc, char *argv[])
{
    char* lc;
    char ld[64] = "";

    //get date and time string
    lc = &ld[0];
    int i = GetDateFormat( 0,0,NULL,"yy'.'MM'.'dd ",lc,20);
    lc = &ld[i-1];
    GetTimeFormat( 0,0,NULL,"HH'-'mm'-'ss",lc,20);

    string msg = ld;

    #define BS 40
    DWORD buffersize = BS;
    char buffer[BS] = "";

    GetUserName(buffer, &buffersize);
    msg = msg + " : " + buffer;

    //append spaces so that one entry is 100 bytes long
    int j = 99 - msg.length();
    j = ( j < 0 )? 0 : j ;
    for (i=1; i < j; i++ ) msg += " ";
    msg += "\n";

    //append entry to logfile
    string ofname = (argc > 1)? argv[1] : "dt";
    ofstream fout( ofname.c_str(), ios::out | ios::app );
    if(fout.good()) {
        fout << msg;
    }
    fout.close();

    cerr << msg;
    return 0;
}
