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
    size_t EolsCount;
    size_t BadEolsCount;
    char LastUnpaired;
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
    convInfo.EolsCount = 0;
    convInfo.BadEolsCount = 0;
    size_t sizeOthers = 0;
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

int main() {
    ConvInfo convInfo;
    convInfo.LastUnpaired=0;
    convInfo.ChunkSize = mixsample.size();
    convInfo.ToEolType = etWindows;
    procChunk(mixsample.data(), NULL, convInfo);

    /*cout << countEols(, 0) << endl;
    cout << countEols(macsample, 10) << endl;
    cout << countEols(macsample, 0) << endl;
    cout << countEols(winsample, 0) << endl;
    cout << countEols(badwinsample, 0) << endl;
    cout << countEols(mixsample, 0) << endl;*/
    return 0;
}
