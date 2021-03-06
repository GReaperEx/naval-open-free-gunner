#include "modes.h"

#include <iostream>
#include <limits>

using namespace std;

int main(int argc, char* argv[])
{
    string fileBase;

    if (argc > 1) {
        fileBase = argv[1];
    } else {
        cout << "Type the path to the \"Naval Ops - Warship Gunner\" root directory.\n"
                "( relative or absolute ): ";
        getline(cin, fileBase);
    }

    if (!fileBase.empty() && fileBase.back() != '/') {
        fileBase += "/";
    }

    for (;;) {
        int answer;

        cout << "What do you want to do?\n";
        cout << "1) Play music files\n";
        cout << "2) Play sound files\n";
        cout << "3) Play movie files\n";
        cout << "4) Browse VCD files\n: ";
        if (!(cin >> answer) || answer < 1 || answer > 4) {
            break;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (answer)
        {
        case 1:
            while (playBGM(fileBase + "BGM/")) {
                continue;
            }
        break;
        case 2:
            while (playSE(fileBase + "SE/")) {
                continue;
            }
        break;
        case 3:
            while (playMOVIE(fileBase + "MOVIE/")) {
                continue;
            }
        break;
        case 4:
            browseK2(fileBase);
        break;
        }

        cout << endl;
    }

    return 0;
}
