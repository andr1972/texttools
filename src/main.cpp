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

void detectEols(char *data, ConvInfo &convInfo) {
    char unpaired = convInfo.LastUnpaired;
    convInfo.EolsCount = 0;
    convInfo.BadEolsCount = 0;
    size_t sizeOthers = 0;
    for (int i=0; i<convInfo.ChunkSize; i++) {
        char c = data[i];
        if (c==13)
        {
            if (unpaired!=10) {
                switch (convInfo.ToEolType)
                {
                    case etUnix: convInfo.BadEolsCount++; break;
                    case etWindows: break;
                    case etMac: break;
                }
                convInfo.EolsCount++;
                unpaired=13;
            }
            else unpaired=0;
        }
        else if (c==10)
        {
            if (unpaired!=13) {
                switch (convInfo.ToEolType)
                {
                    case etUnix: break;
                    case etWindows: convInfo.BadEolsCount++; break;
                    case etMac: convInfo.BadEolsCount++; break;
                }
                convInfo.EolsCount++;
                unpaired=10;
            }
            else unpaired=0;
        }
        else {
            unpaired = 0;
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
    detectEols(mixsample.data(), convInfo);
    /*cout << countEols(, 0) << endl;
    cout << countEols(macsample, 10) << endl;
    cout << countEols(macsample, 0) << endl;
    cout << countEols(winsample, 0) << endl;
    cout << countEols(badwinsample, 0) << endl;
    cout << countEols(mixsample, 0) << endl;*/
    return 0;
}
