#include "decode_sound.h"

#include <climits>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <AL/alc.h>

#include <SDL2/SDL.h>

#define AST_SIG 5526337

using namespace std;

struct ASTfileHeader
{
    uint32_t sig;
    uint32_t freq;
    uint32_t blockSize;
    uint32_t sampleDataSize;
};

int16_t clamp16(int value)
{
    if (value < SHRT_MIN) {
        value = SHRT_MIN;
    } else if (value > SHRT_MAX) {
        value = SHRT_MAX;
    }
    return value;
}

static const double VAG_f[16][2] = {
        {   0.0        ,   0.0        },
        {  60.0 / 64.0 ,   0.0        },
        { 115.0 / 64.0 , -52.0 / 64.0 },
        {  98.0 / 64.0 , -55.0 / 64.0 },
        { 122.0 / 64.0 , -60.0 / 64.0 },
        /* extended table from PPSSPP (PSP emu), found by tests
         * (only seen in inFamous PS3, very rare, possibly "SVAG" or "VAG-HE") */
        {   0.0        ,   0.0        },
        {   0.0        ,   0.0        },
        {  52.0 / 64.0 ,   0.0        },
        {  55.0 / 64.0 ,  -2.0 / 64.0 },
        {  60.0 / 64.0 ,-125.0 / 64.0 },
        {   0.0        ,   0.0        },
        {   0.0        , -91.0 / 64.0 },
        {   0.0        ,   0.0        },
        {   2.0 / 64.0 ,-216.0 / 64.0 },
        { 125.0 / 64.0 ,  -6.0 / 64.0 },
        {   0.0        ,-151.0 / 64.0 },
};

// Borrowed from vgmstream
void decode_psx(uint8_t* encodedData, int16_t * outbuf, int32_t samples_to_do)
{

	int predict_nr, shift_factor, sample;
	int32_t hist1[2]= { 0 };
	int32_t hist2[2]= { 0 };

	short scale;
	int i;
	uint8_t flag;

	int framesin = 0;

	int index[2] = { 0, 1 };
	int channel = 0;
    int upperLimit = (samples_to_do/28)*16;
    int realLimit = upperLimit;

    if (realLimit % 4096 != 0) {
        realLimit -= realLimit % 4096;
    }

	for (framesin = 0; framesin < realLimit; framesin+=16) {
        predict_nr = encodedData[framesin] >> 4;
        shift_factor = encodedData[framesin] & 0xf;
        flag = encodedData[framesin + 1]; /* only lower nibble needed */

        channel = (framesin % 4096 >= 2048);

        for (i=0; i<28; ++i) {
            sample=0;

            if(flag<0x07) {
                short sample_byte = (short)(unsigned char)encodedData[framesin+2+i/2];

                scale = ((i&1 ? /* odd/even byte */
                         sample_byte >> 4 :
                         sample_byte & 0x0f)<<12);

                sample=(int)((scale >> shift_factor)+hist1[channel]*VAG_f[predict_nr][0]+hist2[channel]*VAG_f[predict_nr][1]);
            }

            outbuf[index[channel]] = clamp16(sample);
            index[channel] += 2;

            hist2[channel]=hist1[channel];
            hist1[channel]=sample;
        }
	}
	for (int i = max(index[0], index[1]); i < upperLimit; ++i) {
        outbuf[i] = 0;
    }
}

void decode_psx_mono(uint8_t* encodedData, int16_t * outbuf, int32_t samples_to_do)
{

	int predict_nr, shift_factor, sample;
	int32_t hist1= 0;
	int32_t hist2= 0;

	short scale;
	int i;
	uint8_t flag;

	int framesin = 0;

	int index = 0;
    int upperLimit = (samples_to_do/28)*16;

	for (framesin = 0; framesin < upperLimit; framesin+=16) {
        predict_nr = encodedData[framesin] >> 4;
        shift_factor = encodedData[framesin] & 0xf;
        flag = encodedData[framesin + 1]; /* only lower nibble needed */

        for (i=0; i<28; ++i) {
            sample=0;

            if(flag<0x07) {
                short sample_byte = (short)(unsigned char)encodedData[framesin+2+i/2];

                scale = ((i&1 ? /* odd/even byte */
                         sample_byte >> 4 :
                         sample_byte & 0x0f)<<12);

                sample=(int)((scale >> shift_factor)+hist1*VAG_f[predict_nr][0]+hist2*VAG_f[predict_nr][1]);
            }

            outbuf[index++] = clamp16(sample);

            hist2=hist1;
            hist1=sample;
        }
	}
}

ALuint loadAST(const string& filepath)
{
    ifstream infile(filepath, ios::binary);
    if (!infile.is_open()) {
        string temp = filepath;
        for (int i = 0; i < filepath.size(); ++i) {
        	temp[i] = tolower(filepath[i]);
        }

        infile.open(temp, ios::binary);
        if (!infile.is_open()) {
            cout << "Can't open file!" << endl;
            return 0;
        }
    }

    ASTfileHeader header;
    infile.read((char*)&header, sizeof(header));
    if (header.sig != AST_SIG) {
        return 0;
    }

    vector<uint8_t> encodedData;
    uint8_t* pcmData;
    size_t dataSize;

    dataSize = (header.sampleDataSize/16)*28*2;
    pcmData = new uint8_t[dataSize];
    encodedData.resize(header.sampleDataSize);

    infile.seekg(2048, ios::beg);
    infile.read((char*)&encodedData[0], encodedData.size());
    decode_psx(&encodedData[0], (int16_t*)&pcmData[0], dataSize/2);

    ALuint audioBuffer = 0;
    alGenBuffers(1, &audioBuffer);
    alBufferData(audioBuffer, AL_FORMAT_STEREO16, pcmData, dataSize, header.freq);

    delete [] pcmData;

    return audioBuffer;
}

struct VH2map
{
    int frequency;
    int dataStart;
    int dataSize;
};

vector<VH2map> loadVH2(const string& filepath)
{
    vector<VH2map> blockMap;

    ifstream infile(filepath, ios::binary | ios::ate);
    if (!infile.is_open()) {
        string temp = filepath;
        for (int i = 0; i < filepath.size(); ++i) {
        	temp[i] = tolower(filepath[i]);
        }

        infile.open(temp, ios::binary);
        if (!infile.is_open()) {
            cout << "Can't open file!" << endl;
            return blockMap;
        }
    }

    size_t fileSize = infile.tellg();
    infile.seekg(8, ios::beg);
    for (size_t i = 0; i < fileSize - 8; i += 8) {
        VH2map temp;
        infile.read((char*)&temp.frequency, 4);
        infile.read((char*)&temp.dataSize, 4);

        if (!blockMap.empty()) {
            if (blockMap.back().frequency != temp.frequency) {
                temp.dataStart = blockMap.back().dataStart + blockMap.back().dataSize;
                blockMap.push_back(temp);
            } else {
                blockMap.back().dataSize += temp.dataSize;
            }
        } else {
            temp.dataStart = 0;
            blockMap.push_back(temp);
        }
    }

    return blockMap;
}

vector<ALuint> loadVB2(const string& filepath)
{
    vector<ALuint> alBuffers;

    ifstream infile(filepath, ios::binary | ios::ate);
    if (!infile.is_open()) {
        string temp = filepath;
        for (int i = 0; i < filepath.size(); ++i) {
        	temp[i] = tolower(filepath[i]);
        }

        infile.open(temp, ios::binary);
        if (!infile.is_open()) {
            cout << "Can't open file!" << endl;
            return alBuffers;
        }
    }

    size_t fileSize = infile.tellg();
    infile.seekg(0, ios::beg);

    vector<uint8_t> encodedData;
    vector<uint8_t> pcmData;

    pcmData.resize(fileSize/16*28*2);
    encodedData.resize(fileSize);

    infile.read((char*)&encodedData[0], encodedData.size());
    decode_psx_mono(&encodedData[0], (int16_t*)&pcmData[0], pcmData.size()/2);

    string headerPath = filepath;
    headerPath[headerPath.size() - 2] = 'H';
    vector<VH2map> blockMap = loadVH2(headerPath);

    alBuffers.resize(blockMap.size());
    alGenBuffers(alBuffers.size(), &alBuffers[0]);

    for (size_t i = 0; i < alBuffers.size(); ++i) {
        alBufferData(alBuffers[i], AL_FORMAT_MONO16, &pcmData[blockMap[i].dataStart/16*28*2], blockMap[i].dataSize/16*28*2, blockMap[i].frequency);
    }

    return alBuffers;
}

void decode_ads(vector<uint8_t>& buffer)
{
    if (memcmp(&buffer[0], "SShd", 4) != 0 ||
        memcmp(&buffer[32], "SSbd", 4) != 0) {
        cout << "Couldn't decode ADS!" << endl;
    }

    vector<uint8_t> result(buffer.size() - 40, 0);

    int channel;
    int index[2] = { 0, 1 };
    int size = buffer.size() - 40;
    uint8_t *buf = &buffer[40];

    if (size % 1024 != 0) {
        size -= size % 1024;
    }

    for (int i = 0; i < size/2; ++i) {
        if (i % 512 < 256) {
            channel = 0;
        } else {
            channel = 1;
        }
        ((int16_t*)&result[0])[index[channel]] = ((int16_t*)&buf[0])[i];
        index[channel] += 2;
    }

    buffer = result;
}
