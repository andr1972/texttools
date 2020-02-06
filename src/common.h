#pragma once
#include <vector>
#include <string>
using namespace std;
string getTempName(string filename);
vector<string> maskToRegular(vector<string> &masks);
vector<string> collectFilesRecur(const string path, const vector<string> &masks);
vector<string> collectFiles(const string path, const vector<string> &masks);