#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include "common.h"

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

