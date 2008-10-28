/*
*  get_cb_rev  -  tool to get the revision number from a
*                 unicode build of codeblocks.exe
*
*  originally created: tiwag, 14. Feb. 2006
*
*  CHANGELOG:
*  16. Nov. 2006 - revision string was changed to "svn build  rev"

*  24. Oct. 2008 - adapted to new build system and revisioning mechanism
    -   changed autorevision build tool in order to save
        "tiwagsvnrevtag" prior to the revision number
    -   revision info is now saved in codeblocks.dll
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <string>
#include <fstream>
using namespace std;

void Usage()
{
    puts( "Usage: get_cb_rev [Options] [Directory]" );
    puts( "         Get CodeBlocks Revision number" );
    puts( "Directory:" );
    puts( "         of codeblocks.dll" );
    puts( "Options:" );
    puts( "    -h   help, show usage instructions" );
    puts( "    -w   write a file named as revnr" );
    puts( "    -v   verbose, print debug messages " );
    exit( 1 );
}

bool findrevnr( char *memblock, int size, char *revstr, int rs, char *revnr, int rn )
{
    int i, j, r;
    r = 0;                    //revision string idx
    bool found = false;

    for ( i = 0; i < size; i++ )
    {
        if ( memblock[ i ] == revstr[ r ] )
        {
            r++;
            if ( r == rs )
                found = true;
        }
        else
        {
            r = 0;
        }

        if ( found )
            break;
    }

    if ( found )
    {
        i++;
        for ( j = 0;j < rn;j++ )
        {
            revnr[ j ] = memblock[ i ];
            i += 2;
        }
    }

    return found;
}


int main( int argc, char** argv )
{

    string cbFile;
    bool do_help = false;
    bool do_write = false;
    bool do_verbose = false;

    for ( int i = 1; i < argc; ++i )
    {
        if ( strcmp( "-h", argv[ i ] ) == 0 )
            do_help = true;
        else if ( strcmp( "-w", argv[ i ] ) == 0 )
            do_write = true;
        else if ( strcmp( "-v", argv[ i ] ) == 0 )
            do_verbose = true;
        else if ( cbFile.empty() )
            cbFile.assign( argv[ i ] );
        else
            break;
    }

    if ( do_help )
    {
        Usage();
    }

    // cout << "*(cbFile.end()-1) = \"" << *(cbFile.end()-1) << endl;
    if ( cbFile.empty() != true
            && *( cbFile.end() - 1 ) != '\\'
            && *( cbFile.end() - 1 ) != '/' )
    {
        cbFile.append( "\\" );
    }

    cbFile.append( "codeblocks.dll" );

    // reading a complete binary file
    ifstream::pos_type size;
    char * memblock;

    ifstream file ( cbFile.c_str(), ios::in | ios::binary | ios::ate );
    if ( file.is_open() )
    {
        size = file.tellg();
        memblock = new char [ size ];
        file.seekg ( 0, ios::beg );
        file.read ( memblock, size );
        file.close();

        char revstr[] =
        {
            't', 0,
            'i', 0,
            'w', 0,
            'a', 0,
            'g', 0,
            's', 0,
            'v', 0,
            'n', 0,
            'r', 0,
            'e', 0,
            'v', 0,
            't', 0,
            'a', 0,
            'g', 0,
              0, 0
        };

        char revnr[] = "...." ;

        if ( findrevnr( memblock, size,
                        revstr, sizeof( revstr ),
                        revnr, sizeof( revnr ) - 1 )
           )
        {
            if ( do_verbose )
            {
                cout << "file \"" << cbFile.c_str() << "\" : ";
                cout << "revision ";
            }
            cout << revnr << endl;

            if ( do_write )
            {
                string ofname = ".my.cb_rev_";
                ofname.append(revnr);
                ofstream fout( ofname.c_str(), ios::trunc );
                fout << revnr << endl;
                fout.close();
            }
        }
        else
        {
            cout << "revision info not found\n";
        }

        delete[] memblock;
    }
    else
    {
        cout << "file \"" << cbFile.c_str() << "\" not found\n\n";
        Usage();
    }
    return 0;
}

