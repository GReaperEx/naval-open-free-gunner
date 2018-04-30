#include "modes.h"

#include "decode_sound.h"
#include "pack_mpeg2.h"

#include <iostream>

#include <SDL2/SDL.h>

#include <AL/al.h>
#include <AL/alc.h>

#include <GL/gl.h>

extern "C" {
#include "mpeg2dec/mpeg2.h"
#include "mpeg2dec/mpeg2convert.h"
}

using namespace std;

bool playBGM(const std::string& relPath)
{
    string filePath = relPath;
    if (!filePath.empty() && filePath.back() != '/') {
        filePath += "/";
    }
    filePath += "M";

    int answer;
    cout << "Which music file would you like to play?\n( 0 - 27 ): ";
    if (!(cin >> answer) || answer < 0 || answer > 27) {
        return false;
    }
    if (answer < 10) {
        filePath += "0";
    }
    filePath += to_string(answer) + ".AST";

    cout << "Loading: " << filePath << endl << endl;

    ALCdevice *dev;
    ALCcontext *ctx;

    dev = alcOpenDevice(NULL);
    if (!dev) {
        return false;
    }
    ctx = alcCreateContext(dev, NULL);
    alcMakeContextCurrent(ctx);
    if (!ctx) {
        return false;
    }

    ALuint soundBuffer = loadAST(filePath);
    if (soundBuffer == 0) {
        return false;
    }

    ALuint source;
    alGenSources(1, &source);

    alSourcef(source, AL_GAIN, 0.1f);
    alSourceQueueBuffers(source, 1, &soundBuffer);
    alSourcePlay(source);

    for (;;) {
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if (state > 0 && state != AL_PLAYING) {
            break;
        }
        SDL_Delay(250);
    }

    alDeleteSources(1, &source);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(dev);

    return true;
}

bool _playBR(const std::string& relPath, int answer);
bool _playINTER(const std::string& relPath, int answer);
bool _playREP(const std::string& relPath, int answer);
bool _playTV(const std::string& relPath, int answer);
bool _playLocal(const std::string& relPath, int answer);

void _playSE(const std::string& relPath)
{
    string filePath = relPath + ".VB2";

    cout << "Loading: " << filePath << endl << endl;

    ALCdevice *dev;
    ALCcontext *ctx;

    dev = alcOpenDevice(NULL);
    if (!dev) {
        return;
    }
    ctx = alcCreateContext(dev, NULL);
    alcMakeContextCurrent(ctx);
    if (!ctx) {
        return;
    }

    vector<ALuint> soundBuffer = loadVB2(filePath);
    if (soundBuffer.empty()) {
        return;
    }

    ALuint source;
    alGenSources(1, &source);

    alSourcef(source, AL_GAIN, 0.1f);

    size_t soundIndex = 0;

    alSourcei(source, AL_BUFFER, soundBuffer[soundIndex++]);
    alSourcePlay(source);

    for (;;) {
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if (state > 0 && state != AL_PLAYING) {
            if (soundBuffer.size() <= soundIndex) {
                break;
            }

            alSourcei(source, AL_BUFFER, soundBuffer[soundIndex++]);
            alSourcePlay(source);
        }
        SDL_Delay(250);
    }

    alDeleteBuffers(soundBuffer.size(), &soundBuffer[0]);
    alDeleteSources(1, &source);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(dev);
}

bool playSE(const std::string& relPath)
{
    string filePath = relPath;
    if (!filePath.empty() && filePath.back() != '/') {
        filePath += "/";
    }

    int answer1;
    cout << "Which folder?\n"
            "1) BR00\n"
            "2) BR01\n"
            "3) BR02\n"
            "4) BR03\n"
            "5) INTER\n"
            "6) REP00\n"
            "7) REP01\n"
            "8) REP02\n"
            "9) REP03\n"
            "10) TV00\n"
            "11) TV01\n"
            "12) TV02\n"
            "13) TV03\n"
            "14) ./\n"
            ": ";
    if (!(cin >> answer1) || answer1 < 1 || answer1 > 14) {
        return false;
    }

    switch (answer1)
    {
    case 1:
    case 2:
    case 3:
    case 4:
        while (_playBR(filePath, answer1)) {
            continue;
        }
    break;
    case 5:
        while (_playINTER(filePath, answer1)) {
            continue;
        }
    break;
    case 6:
    case 7:
    case 8:
    case 9:
        while (_playREP(filePath, answer1)) {
            continue;
        }
    break;
    case 10:
    case 11:
    case 12:
    case 13:
        while (_playTV(filePath, answer1)) {
            continue;
        }
    break;
    case 14:
        while (_playLocal(filePath, answer1)) {
            continue;
        }
    break;
    }

    return true;
}

bool playMOVIE(const std::string& relPath)
{
    string filePath = relPath;
    if (!filePath.empty() && filePath.back() != '/') {
        filePath += "/";
    }

    int answer;

    cout << "Which movie would you like to play?\n";
    cout << "1) AUTODEMO\n";
    cout << "2) KOEI\n";
    cout << "3) OP\n";
    cout << "4) STAFF\n: ";
    if (!(cin >> answer) || answer < 1 || answer > 4) {
        return false;
    }

    string windowName;

    switch (answer)
    {
    case 1:
        filePath += "AUTODEMO";
        windowName = "AUTODEMO";
    break;
    case 2:
        filePath += "KOEI";
        windowName = "KOEI";
    break;
    case 3:
        filePath += "OP";
        windowName = "OP";
    break;
    case 4:
        filePath += "STAFF";
        windowName = "STAFF";
    break;
    }
    filePath += ".PSS";

    cout << "Loading: " << filePath << endl << endl;

    ALCdevice *dev;
    ALCcontext *ctx;

    dev = alcOpenDevice(NULL);
    if (!dev) {
        return false;
    }
    ctx = alcCreateContext(dev, NULL);
    alcMakeContextCurrent(ctx);
    if (!ctx) {
        return false;
    }

    ALuint source, alBuffer;
    alGenSources(1, &source);
    alGenBuffers(1, &alBuffer);

    alSourcef(source, AL_GAIN, 0.05f);

    mpeg2dec_t *decoder = mpeg2_init();
    if (decoder == nullptr) {
        cerr << "Couldn't allocate a decoder object!" << endl;
        return false;
    }

    const mpeg2_info_t *info = mpeg2_info(decoder);
    size_t size = (size_t)-1;

    ifstream infile(filePath, ios::binary);
    if (!infile.is_open()) {
        string temp = filePath;
        for (size_t i = 0; i < filePath.size(); ++i) {
        	temp[i] = tolower(filePath[i]);
        }

        infile.open(temp, ios::binary);
        if (!infile.is_open()) {
            cout << "Can't open file!" << endl;
            return false;
        }
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window* window = SDL_CreateWindow(windowName.c_str(),
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    SDL_GL_SetSwapInterval(0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, GL_TRUE);

    glClear(GL_COLOR);
    SDL_GL_SwapWindow(window);

    vector<uint8_t> videoBuffer, audioBuffer;

    for (;;) {
        try {
            if (!pack_mpeg2(infile, videoBuffer, audioBuffer)) {
                break;
            }
        } catch (string exc) {
            cerr << "Error: " << exc << endl;
            exit(-1);
        }
    }

    decode_ads(audioBuffer);

    alBufferData(alBuffer, AL_FORMAT_STEREO16, &audioBuffer[0], audioBuffer.size(), 48000);
    alSourcei(source, AL_BUFFER, alBuffer);

    while (size != 0) {
        mpeg2_state_t state = mpeg2_parse(decoder);

        switch (state)
        {
        case STATE_BUFFER:
            if (videoBuffer.empty()) {
                size = 0;
                break;
            }
            mpeg2_buffer(decoder, (uint8_t*)&videoBuffer[0], (uint8_t*)&videoBuffer[0] + videoBuffer.size());
            videoBuffer.clear();
        break;
        case STATE_SEQUENCE:
            mpeg2_convert(decoder, mpeg2convert_rgb24, nullptr);
            alSourcePlay(source);
        break;
        case STATE_SLICE:
            if (info->display_fbuf) {
                glClear(GL_COLOR);

                glRasterPos2f(-1.0f, 1.0f);
                glPixelZoom(1.0f, -1.0f);
                glDrawPixels(info->sequence->width, info->sequence->height, GL_RGB, GL_UNSIGNED_BYTE, info->display_fbuf->buf[0]);

                SDL_GL_SwapWindow(window);
                SDL_Delay(28);
            }
        break;
        case STATE_END:
        case STATE_INVALID_END:
            size = 0;
	    break;
	    case STATE_SEQUENCE_REPEATED:
        case STATE_GOP:
        case STATE_PICTURE:
        case STATE_SLICE_1ST:
        case STATE_PICTURE_2ND:
        case STATE_INVALID:
        case STATE_SEQUENCE_MODIFIED:
        break;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type)
            {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                size = 0;
            break;
            }
        }

    }

    SDL_Quit();

    mpeg2_close(decoder);

    alDeleteSources(1, &source);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(dev);

    return true;
}

bool _playBR(const std::string& relPath, int answer)
{
    string filePath = relPath + "BR0" + to_string(answer - 1) + "/BR";
    int answer2;

    cout << "Which sound file would you like to play?\n( 1 - 10 ): ";
    if (!(cin >> answer2) || answer2 < 1 || answer2 > 10) {
        return false;
    }

    answer2 += (answer - 1)*10;
    if (answer2 < 10) {
        filePath += "0";
    }
    filePath += to_string(answer2);

    _playSE(filePath);

    return true;
}

bool _playINTER(const std::string& relPath, int)
{
    string filePath  = relPath + "INTER/INTER0";
    int answer2;

    cout << "Which sound file would you like to play?\n( 0 - 4 ): ";
    if (!(cin >> answer2) || answer2 < 1 || answer2 > 10) {
        return false;
    }
    filePath += to_string(answer2);

    _playSE(filePath);

    return true;
}

bool _playREP(const std::string& relPath, int answer)
{
    string filePath = relPath;
    int answer2;

    switch (answer)
    {
        case 6:
            filePath += "REP0" + to_string(answer - 6) + "/REP";
            cout << "Which sound file would you like to play?\n( 0 - 10 ): ";
            if (!(cin >> answer2) || answer2 < 0 || answer2 > 10) {
                return false;
            }

            if (answer2 < 10) {
                filePath += "0";
            }
            filePath += to_string(answer2);
        break;
        case 7:
        case 8:
            filePath += "REP0" + to_string(answer - 6) + "/REP";
            cout << "Which sound file would you like to play?\n( 1 - 10 ): ";
            if (!(cin >> answer2) || answer2 < 1 || answer2 > 10) {
                return false;
            }

            answer2 += (answer - 6)*10;
            if (answer2 < 10) {
                filePath += "0";
            }
            filePath += to_string(answer2);
        break;
        case 9:
            filePath += "REP0" + to_string(answer - 6) + "/REP";
            cout << "Which sound file would you like to play?\n( 1 - 9 ): ";
            if (!(cin >> answer2) || answer2 < 1 || answer2 > 9) {
                return false;
            }

            answer2 += (answer - 6)*10;
            if (answer2 < 10) {
                filePath += "0";
            }
            filePath += to_string(answer2);
        break;
    }

    _playSE(filePath);

    return true;
}

bool _playTV(const std::string& relPath, int answer)
{
    string filePath = relPath;
    int answer2;

    switch (answer)
    {
        case 10:
            filePath += "TV0" + to_string(answer - 10) + "/TV";
            cout << "Which sound file would you like to play?\n( 0 - 10 ): ";
            if (!(cin >> answer2) || answer2 < 0 || answer2 > 10) {
                return false;
            }

            if (answer2 < 10) {
                filePath += "0";
            }
            filePath += to_string(answer2);
        break;
        case 11:
        case 12:
            filePath += "TV0" + to_string(answer - 10) + "/TV";
            cout << "Which sound file would you like to play?\n( 1 - 10 ): ";
            if (!(cin >> answer2) || answer2 < 1 || answer2 > 10) {
                return false;
            }

            answer2 += (answer - 9)*10;
            if (answer2 < 10) {
                filePath += "0";
            }
            filePath += to_string(answer2);
        break;
        case 13:
            filePath += "TV0" + to_string(answer - 10) + "/TV";
            cout << "Which sound file would you like to play?\n( 1 - 9 ): ";
            if (!(cin >> answer2) || answer2 < 1 || answer2 > 9) {
                return false;
            }

            answer2 += (answer - 9)*10;
            if (answer2 < 10) {
                filePath += "0";
            }
            filePath += to_string(answer2);
        break;
    }

    _playSE(filePath);

    return true;
}

bool _playLocal(const std::string& relPath, int)
{
    string filePath = relPath + "SE_";
    int answer2;

    cout << "Which sound file would you like to play?\n"
            "1) COM\n2) STR\n3) TAC\n: ";
    if (!(cin >> answer2) || answer2 < 1 || answer2 > 4) {
        return false;
    }

    switch (answer2)
    {
    case 1:
        filePath += "COM";
    break;
    case 2:
        filePath += "STR";
    break;
    case 3:
        filePath += "TAC";
    break;
    }

    _playSE(filePath);

    return true;
}
