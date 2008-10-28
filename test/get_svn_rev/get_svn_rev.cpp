/*
* get_svn_rev - a tool to get Subversion revision as filename
* 2006-09-06 tiwag
*
* $Revision: 18 $
* $Id: get_svn_rev.cpp 18 2006-10-24 21:54:48Z tiwag.cb $
* $HeadURL: https://tiwag.googlecode.com/svn/test/_projects/get_svn_rev/get_svn_rev.cpp $
*/

#include <stdio.h>
#include <windows.h>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
    string outputFile("_svn_update_revision_");
    string line("");
    string revision("");
    string svninfo("");
    SetEnvironmentVariable("LANG","C");
    string svncmd("svn info ");
    FILE *svn = popen(svncmd.c_str(), "r");

    if (svn)
    {
        const size_t BUFLEN = 4*1024;
        char buf[BUFLEN];
        while (fgets(buf, BUFLEN-1 , svn))
        {
            line.assign(buf);
            svninfo.append(line);
            if (line.find("Last Changed Rev: ") != string::npos)
            {
                revision = line.substr(strlen("Last Changed Rev: "));
                string lbreak("\r\n");
                size_t i;
                while ((i = revision.find_first_of(lbreak)) != string::npos)
                    revision.erase(revision.length()-1);
            }
        }
    }
    cout << svninfo;
    if ( ! revision.empty() )
    {
        string delcmd("del " + outputFile + "* /F /Q > nul");
        system(delcmd.c_str());
        outputFile.append(revision+"_");
        ofstream fout( outputFile.c_str(), ios::trunc );
        fout << svninfo;
        fout.close();
    }
    return 0;
}
