/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

namespace BinaryData
{

//================== 4thFloorSpeakers.dat ==================
static const unsigned char temp_483209f6[] =
"// speaker layout, MAT 4th floor, 1 May, 2009\n"
"\n"
"// relative to sweet spot, centre of room, in inches / degrees\n"
"\n"
"// x increases to the front\n"
"// y increases to the right\n"
"// z increases above\n"
"\n"
"//azimuth\t\televation\n"
"\n"
"\n"
"//ring of 8 at ear level  and 4 in the sky\n"
"\n"
"// AZ\t\tEL\t\tDIST\n"
"\n"
"315\t\t0\t\t1\n"
"0\t\t0\t\t1\n"
"45\t\t0\t\t1\n"
"90\t\t0\t\t1\n"
"135\t\t0\t\t1\n"
"180\t\t0\t\t1\n"
"225\t\t0\t\t1\n"
"270\t\t0\t\t1\n"
"\n"
"315\t\t45\t\t1\n"
"45\t\t45\t\t1\n"
"135\t\t45\t\t1\n"
"225\t\t45\t\t1\n";

const char* _4thFloorSpeakers_dat = (const char*) temp_483209f6;

//================== Speaker_layout.dat ==================
static const unsigned char temp_cfbabe08[] =
"% This is a data file used to describe the layout of LSpeakers of the VBAP classes.\n"
"% It is meant to be used by the VBAP::read_layout_file(\"filename\" ) method of the VBAP classes developed by\n"
"% Doug McCoy for his Master's thesis at UCSB. \n"
"% all lines with a '%' character are treated as comments from the '%' to the end of the line\n"
"% Syntax:\n"
"\n"
"mode: 3\t\t\t\t\t% (either 2 or 3) if 2, second column is ignored\n"
"speaker:\t30\t\t0\t\t% azimuth  is measured from the x axis within the x/y plane (-180 < azi <= 180)\n"
"\t\t\t\t\t\t% elevation is measured perpendicularly from the x/y plane  (-90 < ele <= 90)\n"
"\t\t\t\t\t\t% all points are assumed to be unit length from the origin\n"
"speaker:\t-30\t\t0\n"
"speaker:\t-90\t\t0\n"
"speaker:\t-150\t\t0\n"
"speaker:\t150\t\t0\n"
"speaker:\t90\t\t0\n"
"speaker:\t0\t\t90\n"
"speaker:\t0\t\t-90\n";

const char* Speaker_layout_dat = (const char*) temp_cfbabe08;


const char* getNamedResource (const char*, int&) throw();
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes) throw()
{
    unsigned int hash = 0;
    if (resourceNameUTF8 != 0)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0x5e48dd6f:  numBytes = 393; return _4thFloorSpeakers_dat;
        case 0x540e1562:  numBytes = 760; return Speaker_layout_dat;
        default: break;
    }

    numBytes = 0;
    return 0;
}

}
