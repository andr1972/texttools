#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <vector>
#include "common.h"

using namespace std;

enum ActionType {atAdd, atRemove};

const uint8_t UTF_16LE[2] = {0xFF, 0xFE};
const uint8_t UTF_16BE[2] = {0xFE, 0xFF};
const uint8_t UTF_32LE[4] = {0xFF, 0xFE, 0x00, 0x00};
const uint8_t UTF_32BE[4] = {0x00, 0x00, 0xFE, 0xFF};
const uint8_t UTF_8[3] = {0xEF, 0xBB, 0xBF};

enum BomType {btNone, btUTF8, btUTF16LE, btUTF16BE, btUTF32LE, btUTF32BE};

struct ConvInfo {
    BomType bomType;
    bool hasZeroByte;
};

ConvInfo detect(string relativePath) {
    ifstream ifs;
    ifs.open(relativePath, ios::binary | ios::in);
    ifs.seekg(0, std::ios::end );
    int64_t fsize = (int64_t)ifs.tellg();
    ifs.seekg(0, std::ios::beg );
    char *buf;
    const int64_t MaxChunkSize = 64*1024;
    int64_t ChunkSize = min(MaxChunkSize, fsize);
    buf = new char[ChunkSize];
    ConvInfo convInfo;
    convInfo.hasZeroByte = false;
    int64_t sumReaded = 0;
    char BOM[4];
    ifs.read(BOM, 4);
    streamsize bytesReaded = ifs.gcount();
    convInfo.bomType = btNone;
    if (bytesReaded>=4)
    {
        if (memcmp(BOM, UTF_32LE, 4)==0) convInfo.bomType = btUTF32LE;
        else if (memcmp(BOM, UTF_32BE, 4)==0) convInfo.bomType = btUTF32BE;
    }
    if (bytesReaded>=3)
    {
        if (memcmp(BOM, UTF_8, 3)==0) convInfo.bomType = btUTF8;
    }
    if (bytesReaded>=2)
    {
        if (memcmp(BOM, UTF_16LE, 4)==0) convInfo.bomType = btUTF16LE;
        else if (memcmp(BOM, UTF_16BE, 4)==0) convInfo.bomType = btUTF16BE;
    }
    do {
        ifs.read(buf, min(ChunkSize, fsize - sumReaded));
        bytesReaded = ifs.gcount();
        sumReaded += bytesReaded;
        for (size_t i=0; i<bytesReaded; i++) {
            char c = buf[i];
            if (c==0)
                convInfo.hasZeroByte = true;
        }
        if (ifs.eof()) break;
    } while (bytesReaded==ChunkSize && sumReaded<fsize);
    delete buf;
    ifs.close();
    return convInfo;
}

ConvInfo addBOM8(string relativePath) {
    ifstream ifs;
    ifs.open(relativePath, ios::binary | ios::in);
    ifs.seekg(0, std::ios::end);
    int64_t fsize = (int64_t) ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    char *buf;
    const int64_t MaxChunkSize = 64 * 1024;
    int64_t ChunkSize = min(MaxChunkSize, fsize);
    buf = new char[ChunkSize];
    string tempName = getTempName(relativePath);
    ofstream ofs;
    ofs.open(tempName, ios::binary | ios::out);
    ofs.write((char*)UTF_8, 3);
    int64_t sumReaded = 0;
    streamsize bytesReaded = 0;
    do {
        ifs.read(buf, min(ChunkSize, fsize - sumReaded));
        bytesReaded = ifs.gcount();
        sumReaded += bytesReaded;
        ofs.write(buf, bytesReaded);
        if (ifs.eof()) break;
    } while (bytesReaded==ChunkSize && sumReaded<fsize);
    ofs.close();
    delete buf;
    ifs.close();
    if (tempName!="") {
        boost::filesystem::remove(relativePath);
        boost::filesystem::rename(tempName, relativePath);
    }
}

ConvInfo removeBOM8(string relativePath) {
    ifstream ifs;
    ifs.open(relativePath, ios::binary | ios::in);
    ifs.seekg(0, std::ios::end);
    int64_t fsize = (int64_t) ifs.tellg();
    ifs.seekg(3, std::ios::beg);
    char *buf;
    const int64_t MaxChunkSize = 64 * 1024;
    int64_t ChunkSize = min(MaxChunkSize, fsize);
    buf = new char[ChunkSize];
    string tempName = getTempName(relativePath);
    ofstream ofs;
    ofs.open(tempName, ios::binary | ios::out);
    int64_t sumReaded = 0;
    streamsize bytesReaded = 0;
    do {
        ifs.read(buf, min(ChunkSize, fsize - sumReaded));
        bytesReaded = ifs.gcount();
        sumReaded += bytesReaded;
        ofs.write(buf, bytesReaded);
        if (ifs.eof()) break;
    } while (bytesReaded==ChunkSize && sumReaded<fsize);
    delete buf;
    ofs.close();
    ifs.close();
    if (tempName!="") {
        boost::filesystem::remove(relativePath);
        boost::filesystem::rename(tempName, relativePath);
    }
}

void procFile(string relativePath, ActionType action, bool bConvert, bool bShowBinary)
{
    ConvInfo convInfo = detect(relativePath);
    if (action==atAdd) {
        if (convInfo.bomType!=btNone) return;
    }
    else {
        if (convInfo.bomType!=btUTF8) return;
    }
    if (convInfo.hasZeroByte && bShowBinary)
        printf("%s binary\n", relativePath.c_str());
    if (!convInfo.hasZeroByte) {
        printf("%s", relativePath.c_str());
        if (bConvert) {
            if (action==atAdd) {
                addBOM8(relativePath);
            }
            else {
                removeBOM8(relativePath);
            }
            printf(" --- converted\n");
        }
        else printf("\n");
    }
}

int procCommandLine(int argc, char * argv[])
{
    ActionType action;
    if (argc<2) return 1;
    string eolTypeStr = argv[1];
    if (eolTypeStr=="add")
        action = atAdd;
    else if (eolTypeStr=="remove")
        action = atRemove;
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
        procFile(fname, action, bConvert, bShowBinary);
    return 0;
}

int main(int argc, char * argv[]) {
    if (argc<2)
    {
        cout << "help: utf8bom --help   " << endl;
    } else if (string(argv[1])=="--help")
    {
        cout << "usage:" <<endl;
        cout << "utf8bom [add/remove] maskList [r] [conv] [bin]" << endl;
        cout << "maskList is one mask or list divided by colon" << endl;
        cout << "r = recursive subdirectories" << endl;
        cout << "conv = convert, else only print files need convert" << endl << endl;
        cout << "bin = show binary files, containing byte 0" << endl << endl;
        cout << "note: on Linux Bash is needed backslash \\ before asterisk *" << endl;
        cout << "example:" <<endl;
        cout << "utf8bom remove \\*.tex conv" << endl;
    } else return procCommandLine(argc,argv);
}
