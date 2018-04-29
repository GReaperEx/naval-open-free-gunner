#include <fstream>
#include <vector>
#include <string>

#include <iomanip>
#include <iostream>

#include <cstdint>
#include <cstring>

using namespace std;

void pack(ifstream& mpeg2_file, vector<uint8_t>& videoStream, vector<uint8_t>& audioStream);

bool pack_mpeg2(ifstream& mpeg2_file, vector<uint8_t>& videoStream, vector<uint8_t>& audioStream)
{
    uint8_t code[4];

    for (;;) {
        if (!mpeg2_file.read((char*)code, 4)) {
            throw string("Failed while trying to read pack start code");
        }
        if (memcmp(code, "\x00\x00\x01\xBA", 4) == 0) {
            pack(mpeg2_file, videoStream, audioStream);
            return true;
        } else if (memcmp(code, "\x00\x00\x01\xB9", 4) == 0) {
            return false;
        }
    }
    return false;
}

// Just skips those bytes
void pack_header(ifstream& mpeg2_file);

void PES_packet(ifstream& mpeg2_file, vector<uint8_t>& videoStream, vector<uint8_t>& audioStream);

void pack(ifstream& mpeg2_file, vector<uint8_t>& videoStream, vector<uint8_t>& audioStream)
{
    pack_header(mpeg2_file);

    uint8_t code[4];
    for (;;) {
        if (!mpeg2_file.read((char*)code, 4)) {
            throw string("Failed while trying to read packet start code");
        }

        if (memcmp(code, "\x00\x00\x01", 3) != 0 ||
            memcmp(code, "\x00\x00\x01\xBA", 4) == 0 ||
            memcmp(code, "\x00\x00\x01\xB9", 4) == 0) {
            mpeg2_file.seekg(-4, ios::cur);
            break;
        }
        mpeg2_file.seekg(-1, ios::cur);
        PES_packet(mpeg2_file, videoStream, audioStream);
    }
}

// Skips those bytes too
void system_header(ifstream& mpeg2_file);

void pack_header(ifstream& mpeg2_file)
{
    mpeg2_file.seekg(9, ios::cur);

    uint8_t stuffing;
    if (!mpeg2_file.read((char*)&stuffing, 1)) {
        throw string("Failed while trying to read pack stuffing length");
    }
    mpeg2_file.seekg(stuffing & 0x7, ios::cur);

    uint8_t code[4];
    if (!mpeg2_file.read((char*)code, 4)) {
        throw string("Failed while trying to read system header start code");
    }
    if (memcmp(code, "\x00\x00\x01\xBB", 4) == 0) {
        system_header(mpeg2_file);
    } else {
        mpeg2_file.seekg(-4, ios::cur);
    }
}

void swap16(void* byteStream)
{
    uint8_t temp = ((uint8_t*)byteStream)[0];
    ((uint8_t*)byteStream)[0] = ((uint8_t*)byteStream)[1];
    ((uint8_t*)byteStream)[1] = temp;
}

void swap32(void* byteStream)
{
    uint16_t temp = ((uint16_t*)byteStream)[0];
    ((uint16_t*)byteStream)[0] = ((uint16_t*)byteStream)[1];
    ((uint16_t*)byteStream)[1] = temp;

    swap16(((uint16_t*)byteStream));
    swap16(((uint16_t*)byteStream) + 1);
}

void system_header(ifstream& mpeg2_file)
{
    uint16_t headerLen;
    if (!mpeg2_file.read((char*)&headerLen, sizeof(headerLen))) {
        throw string("Failed while trying to read system header length");
    }

    swap16(&headerLen);
    mpeg2_file.seekg(headerLen, ios::cur);
}

void PES_packet(ifstream& mpeg2_file, vector<uint8_t>& videoStream, vector<uint8_t>& audioStream)
{
    uint8_t stream_id;
    uint16_t packetLen;

    if (!mpeg2_file.read((char*)&stream_id, sizeof(stream_id))) {
        throw string("Failed while trying to read packet stream id");
    }
    if (!mpeg2_file.read((char*)&packetLen, sizeof(packetLen))) {
        throw string("Failed while trying to read packet length");
    }

    swap16(&packetLen);

    if (stream_id != 0xBE) {
        mpeg2_file.seekg(2, ios::cur);

        uint8_t headerDataLen;
        if (!mpeg2_file.read((char*)&headerDataLen, sizeof(headerDataLen))) {
            throw string("Failed while trying to read packet stream id");
        }
        mpeg2_file.seekg(headerDataLen, ios::cur);
        packetLen -= 3 + headerDataLen;
    }
    if (stream_id == 0xBD) {
        vector<uint8_t> tempBuffer(packetLen, 0);

        mpeg2_file.read((char*)&tempBuffer[0], tempBuffer.size());
        audioStream.insert(audioStream.end(), tempBuffer.begin() + 4, tempBuffer.end());
    } else if ((stream_id & 0xF0) == 0xE0) {
        vector<uint8_t> tempBuffer(packetLen, 0);

        mpeg2_file.read((char*)&tempBuffer[0], tempBuffer.size());
        videoStream.insert(videoStream.end(), tempBuffer.begin(), tempBuffer.end());
    } else {
        mpeg2_file.seekg(packetLen, ios::cur);
    }
}
