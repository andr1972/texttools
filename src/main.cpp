#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <vector>

using namespace std;

vector<char> unixsample {'g','k', 10, 'd', 10, 10, 'j', 10};
vector<char> macsample {13, 'g','k', 13, 'd', 13, 13, 'j', 13};
vector<char> winsample {'g','k', 13,10, 'd', 13,10, 13,10, 'j', 13,10};
vector<char> badwinsample {'g','k', 10,13, 'd', 10,13, 10,13, 'j', 10,13};
vector<char> mixsample {'g',10,10,'a','b',13,'v',10,13,10,'h', 'k', 13,10, 'd', 13,10, 13,10, 'j', 13,10,13};
vector<char> win5 {13,10,13,10,13,10,13,10,13,10};
vector<char> win5bad {13,10,10,13,13,10,10,13,13,10};

enum EolType {etUnix, etWindows, etMac};

struct ConvInfo {
    size_t ChunkSize;
    size_t NeedSize;
    size_t EolsCount = 0;
    size_t BadEolsCount = 0;
    char LastUnpaired = 0;
    EolType ToEolType;
};

void putch(char ch, char *output, size_t &outpos)
{
    output[outpos] = ch;
    outpos++;
}

void putEol(EolType eolType, char *output, size_t &outpos)
{
    if (!output) return;
    switch (eolType)
    {
        case etUnix: putch(10,output, outpos); break;
        case etWindows:
            putch(13,output, outpos);
            putch(10,output, outpos); break;
        case etMac:  putch(13,output, outpos); break;
    }
}

void procChunk(char *data, char *output, ConvInfo &convInfo) {
    char unpaired = convInfo.LastUnpaired;
    size_t sizeOthers = 0;
    convInfo.EolsCount = convInfo.BadEolsCount = 0;
    size_t outpos;
    for (size_t i=0; i<convInfo.ChunkSize; i++) {
        char c = data[i];
        if (c==13)
        {
            if (unpaired!=10) {
                if (convInfo.ToEolType==etUnix)
                    convInfo.BadEolsCount++;
                putEol(convInfo.ToEolType, output, outpos);
                convInfo.EolsCount++;
                unpaired=13;
            }
            else unpaired=0;
        }
        else if (c==10)
        {
            if (unpaired!=13) {
                if (convInfo.ToEolType!=etUnix)
                    convInfo.BadEolsCount++;
                putEol(convInfo.ToEolType, output, outpos);
                convInfo.EolsCount++;
                unpaired=10;
            }
            else unpaired=0;
        }
        else {
            unpaired = 0;
            if (output)
            {
                output[outpos] = c;
                outpos++;
            }
            sizeOthers++;
        }
    }
    convInfo.LastUnpaired = unpaired;
    if (convInfo.ToEolType==etWindows)
        convInfo.NeedSize = sizeOthers+2*convInfo.EolsCount;
    else
        convInfo.NeedSize = sizeOthers+convInfo.EolsCount;
}

/*
 * replace :
 *  . by \.
 *  * by .*
 *  ? by .
 * */
string maskToRegular(string mask) {
    string result="";
    for (int i=0; i<mask.size(); i++)
    {
        char c = mask[i];
        if (c=='.')
            result+="\\.";
        else if (c=='*')
            result+=".*";
        else if (c=='?')
            result+=".";
        else result+=c;
    }
    return result;
}

vector<string> maskToRegular(vector<string> &masks) {
    vector<string>  result;
    for (string mask: masks)
        result.push_back(maskToRegular(mask));
    return result;
}


bool ORfilters(string s, vector<boost::regex> &filters)
{
    for (auto &filter: filters) {
        boost::smatch what;
        if (boost::regex_match(s, what, filter)) return true;
    }
    return false;
}


vector<string> collectFiles(const string path, const vector<string> &masks)
{
    vector<boost::regex> my_filters;
    for (string mask: masks) {
        boost::regex my_filter(mask);
        my_filters.push_back(my_filter);
    }
    vector<string> all_matching_files;
    boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end
    for (boost::filesystem::directory_iterator i(path); i != end_itr; ++i)
    {
        if (!boost::filesystem::is_regular_file(i->status())) continue;
        if (!ORfilters(i->path().filename().string(), my_filters)) continue;
        all_matching_files.push_back(i->path().string());
    }
    return all_matching_files;
}

vector<string> collectFilesRecur(const string path, const vector<string> &masks)
{
    vector<boost::regex> my_filters;
    for (string mask: masks) {
        boost::regex my_filter(mask);
        my_filters.push_back(my_filter);
    }
    vector<string> all_matching_files;
    boost::filesystem::recursive_directory_iterator end_itr; // Default ctor yields past-the-end
    for (boost::filesystem::recursive_directory_iterator i(path); i != end_itr; ++i)
    {
        if (!boost::filesystem::is_regular_file(i->status())) continue;
        if (!ORfilters(i->path().filename().string(), my_filters)) continue;
        all_matching_files.push_back(i->path().string());
    }
    return all_matching_files;
}

size_t procFile(string relativePath, EolType eolType, bool bConvert)
{
    ifstream ifs;
    ifs.open(relativePath, ios::binary | ios::in);
    ifs.seekg(0, std::ios::end );
    int64_t fsize = (int64_t)ifs.tellg();
    ifs.seekg(0, std::ios::beg );
    char *buf;
    const int64_t MaxChunkSize = 64*1024;
    size_t maxNeeded = 0;
    int64_t ChunkSize = min(MaxChunkSize, fsize);
    buf = new char[ChunkSize];
    streamsize bytesReaded = ChunkSize;
    ConvInfo convInfo;
    convInfo.LastUnpaired=0;
    convInfo.ToEolType = eolType;
    int64_t sumReaded = 0;
    size_t EolsCount=0, BadEolsCount=0;
    do {
        ifs.read(buf, min(ChunkSize, fsize-sumReaded));
        bytesReaded = ifs.gcount();
        sumReaded += bytesReaded;
        if (ifs.eof()) break;
        convInfo.ChunkSize = bytesReaded;
        procChunk(buf, nullptr, convInfo);
        EolsCount+=convInfo.EolsCount;
        BadEolsCount+=convInfo.BadEolsCount;
        maxNeeded = max(maxNeeded, convInfo.NeedSize);
    } while (bytesReaded==ChunkSize && sumReaded<fsize);
    delete buf;
    ifs.close();
    if (BadEolsCount>0)
        printf("%s %ld/%ld\n",relativePath.c_str(), BadEolsCount, EolsCount);
    return maxNeeded;
}

int procCommandLine(int argc, char * argv[])
{
    EolType eolType;
    if (argc<2) return 1;
    string eolTypeStr = argv[1];
    if (eolTypeStr=="unix")
        eolType = etUnix;
    else if (eolTypeStr=="win")
        eolType = etWindows;
    else if (eolTypeStr=="mac")
        eolType = etMac;
    else return 2;
    if (argc<3) return 1;
    string masklist = argv[2];
    vector<string> masks;
    boost::split(masks,masklist,boost::is_any_of(":"));
    masks = maskToRegular(masks);
    auto v = collectFiles(".", masks);
    for (auto fname: v)
        cout << procFile(fname, eolType, false) << endl;
    return 0;
}

int main(int argc, char * argv[]) {
    if (argc<2)
    {
        cout << "help: eolconv --help   " << endl;
    } else if (string(argv[1])=="--help")
    {
        cout << "usage:" <<endl;
        cout << "eolconv eolType maskList [r] [conv]" << endl;
        cout << "eolType is one of {unix,win,mac}" << endl;
        cout << "maskList is one mask or list divided by colon" << endl;
        cout << "r = recursive subdirectories" << endl;
        cout << "conv = convert, else only print files need convert" << endl << endl;
        cout << "note: on Linux Bash is needed backslash \\ before asterisk *" << endl;
        cout << "example:" <<endl;
        cout << "eolconv unix \\*.cpp:\\*.h conv" << endl;
    } else return procCommandLine(argc,argv);
}
