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

int countEols(vector<char> &v, ConvInfo &convInfo) {
    char unpaired = convInfo.LastUnpaired;
    int count = 0;
    for (int i=0; i<v.size(); i++) {
        char c = v[i];
        if (c==13)
        {
            if (unpaired!=10) {
                count++;
                unpaired=13;
            }
            else unpaired=0;
        }
        else if (c==10)
        {
            if (unpaired!=13) {
                count++;
                unpaired=10;
            }
            else unpaired=0;
        }
        else unpaired=0;
    }
    convInfo.LastUnpaired = unpaired;
    return count;
}

int main() {
    ConvInfo convInfo;
    convInfo.LastUnpaired=0;
    cout << countEols(mixsample, convInfo) << endl;
    /*cout << countEols(, 0) << endl;
    cout << countEols(macsample, 10) << endl;
    cout << countEols(macsample, 0) << endl;
    cout << countEols(winsample, 0) << endl;
    cout << countEols(badwinsample, 0) << endl;
    cout << countEols(mixsample, 0) << endl;*/
    return 0;
}
