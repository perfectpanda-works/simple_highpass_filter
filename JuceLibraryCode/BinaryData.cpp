/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

namespace BinaryData
{

//================== bk.png ==================
static const unsigned char temp_binary_data_0[] =
{ 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,1,244,0,0,1,44,1,3,0,0,0,15,117,211,139,0,0,0,6,80,76,84,69,195,195,195,127,127,127,207,230,32,207,0,0,0,84,73,68,65,84,120,218,237,203,161,1,0,0,8,3,160,253,127,177,209,186,100,177,66,39,185,77,126,124,
223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,223,247,125,191,45,57,187,34,176,204,111,45,51,
0,0,0,0,73,69,78,68,174,66,96,130,0,0 };

const char* bk_png = (const char*) temp_binary_data_0;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0xad4ba133:  numBytes = 159; return bk_png;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "bk_png"
};

const char* originalFilenames[] =
{
    "bk.png"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
    {
        if (namedResourceList[i] == resourceNameUTF8)
            return originalFilenames[i];
    }

    return nullptr;
}

}
