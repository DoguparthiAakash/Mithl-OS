#include <iostream>
#include <fstream>
#include <string>

using namespace std;


int main() {
    string text;
    string name;
    cout << "Insert name with .txt extension (eg. name.txt)";
    getline(cin, name);
    if (name == "con.txt") {
        cout << "Can't write to file";
        return 1;
    } else if (name == "aux.txt") {
        cout << "Can't write to file";
        return 1;
    } else if (name == "prn.txt") {
        cout << "Can't write to file";
        return 1;
    } else if (name == "con") {
        cout << "Can't write to file";
        return 1;
    } else if (name == "aux") {
        cout << "Can't write to file";
        return 1;
    } else if (name == "prn") {
        cout << "Can't write to file";
        return 1;
    }
    ofstream file(name);
    cout << "Insert text down here, when finished press enter" << endl;
    getline(cin, text);
    file << text;
    return 0;
}
