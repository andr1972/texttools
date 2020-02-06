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
    bool hasZeroByte;
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
    size_t outpos = 0;
    convInfo.hasZeroByte = false;
    for (size_t i=0; i<convInfo.ChunkSize; i++) {
        char c = data[i];
        if (c==0)
            convInfo.hasZeroByte = true;
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


vector<string> collectFilenames(const string path, const vector<string> &masks)
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
        all_matching_files.push_back(i->path().filename().string());
    }
    return all_matching_files;
}

vector<string> collectDirs(const string path)
{
    vector<string> all_matching_files;
    boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end
    for (boost::filesystem::directory_iterator i(path); i != end_itr; ++i)
    {
        if (!boost::filesystem::is_directory(i->status())) continue;
        string dirname = i->path().filename().string();
        if (dirname!=".git")
            all_matching_files.push_back(dirname);
    }
    return all_matching_files;
}

vector<string> collectFilesRecur(const string path, const vector<string> &masks)
{
    vector<string> list;
    vector<string> listF = collectFilenames(path, masks);
    for (string fname: listF)
        list.push_back(path+"/"+fname);
    listF.clear();
    vector<string> listD = collectDirs(path);
    for (string dirname: listD)
    {
        vector<string> tmp = collectFilesRecur(path+"/"+dirname, masks);
        list.insert(list.end(), tmp.begin(), tmp.end());
    }
    return list;
}

/*I cant sinmply use below function, because I not go to .git folders*/
vector<string> collectFilesRecurBoost(const string path, const vector<string> &masks)
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


string getTempName(string filename) {
    const int maxTrials = 100;
    for (int i=0; i<maxTrials; i++)
    {
        string result = filename+".tmp"+to_string(i);
        if (!boost::filesystem::exists(result)) return result;
    }
    cout << "Cant create temp name for " << filename << endl;
    exit(-1);
}

size_t procFileStage(string relativePath, EolType eolType, bool bConvert, ConvInfo &genconvInfo, size_t outneeded)
{
    ifstream ifs;
    ifs.open(relativePath, ios::binary | ios::in);
    ifs.seekg(0, std::ios::end );
    int64_t fsize = (int64_t)ifs.tellg();
    ifs.seekg(0, std::ios::beg );
    char *buf;
    const int64_t MaxChunkSize = 64*1024;
    char *outbuf = nullptr;
    ofstream ofs;
    string tempName = "";
    if (bConvert) {
        tempName = getTempName(relativePath);
        ofs.open(tempName, ios::binary | ios::out);
        outbuf = new char[outneeded];
    }
    size_t maxNeeded = 0;
    int64_t ChunkSize = min(MaxChunkSize, fsize);
    buf = new char[ChunkSize];
    streamsize bytesReaded = ChunkSize;
    ConvInfo convInfo;
    convInfo.LastUnpaired=0;
    convInfo.ToEolType = eolType;
    int64_t sumReaded = 0;
    genconvInfo.EolsCount=0;
    genconvInfo.BadEolsCount=0;
    genconvInfo.hasZeroByte = false;
    do {
        ifs.read(buf, min(ChunkSize, fsize-sumReaded));
        bytesReaded = ifs.gcount();
        sumReaded += bytesReaded;
        if (ifs.eof()) break;
        convInfo.ChunkSize = bytesReaded;
        procChunk(buf, outbuf, convInfo);
        genconvInfo.EolsCount+=convInfo.EolsCount;
        genconvInfo.BadEolsCount+=convInfo.BadEolsCount;
        if (convInfo.hasZeroByte)
            genconvInfo.hasZeroByte = true;
        if (outbuf)
            ofs.write(outbuf, convInfo.NeedSize);
        maxNeeded = max(maxNeeded, convInfo.NeedSize);
    } while (bytesReaded==ChunkSize && sumReaded<fsize);
    delete buf;
    delete outbuf;
    if (ofs.is_open()) ofs.close();
    ifs.close();
    if (bConvert && tempName!="") {
        boost::filesystem::remove(relativePath);
        boost::filesystem::rename(tempName, relativePath);
    }
    return maxNeeded;
}

void procFile(string relativePath, EolType eolType, bool bConvert, bool bShowBinary)
{
    ConvInfo genconvInfo;
    size_t needed = procFileStage(relativePath, eolType, false, genconvInfo, 0);
    if (genconvInfo.hasZeroByte && bShowBinary)
        printf("%s binary\n", relativePath.c_str());
    if (genconvInfo.BadEolsCount>0 && !genconvInfo.hasZeroByte) {
        printf("%s %ld/%ld\n", relativePath.c_str(), genconvInfo.BadEolsCount, genconvInfo.EolsCount);
        if (bConvert)
            procFileStage(relativePath, eolType, true, genconvInfo, needed);
    }
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
    bool bRecursive = false;
    bool bConvert = false;
    bool bShowBinary = false;
    for (int i=3; i<argc; i++)
    {
        string arg = argv[i];
        if(arg=="r")
            bRecursive = true;
        else if(arg=="conv")
            bConvert = true;
        else if(arg=="bin")
            bShowBinary = true;
    }
    masks = maskToRegular(masks);
    vector<string> v;
    if (bRecursive)
        v = collectFilesRecur(".", masks);
    else
        v = collectFiles(".", masks);
    for (auto fname: v)
        procFile(fname, eolType, bConvert, bShowBinary);
    return 0;
}

int main(int argc, char * argv[]) {
    if (argc<2)
    {
        cout << "help: eolconv --help   " << endl;
    } else if (string(argv[1])=="--help")
    {
        cout << "usage:" <<endl;
        cout << "eolconv eolType maskList [r] [conv] [bin]" << endl;
        cout << "eolType is one of {unix,win,mac}" << endl;
        cout << "maskList is one mask or list divided by colon" << endl;
        cout << "r = recursive subdirectories" << endl;
        cout << "conv = convert, else only print files need convert" << endl << endl;
        cout << "bin = show binary files, containing byte 0" << endl << endl;
        cout << "note: on Linux Bash is needed backslash \\ before asterisk *" << endl;
        cout << "example:" <<endl;
        cout << "eolconv unix \\*.cpp:\\*.h conv" << endl;
    } else return procCommandLine(argc,argv);
}
