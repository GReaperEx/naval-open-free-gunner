#include "pak_unpack.h"

#include <cstring>

using namespace std;

struct pakFileHeader
{
    char sig[4]; // "PACK"
    uint16_t blockSize; // Maybe?
    uint16_t entriesCount;
    uint32_t fileByteSize;
    uint32_t _resv;
};

struct pakEntry
{
    char entryName[32];
    uint32_t _resv;
    uint32_t fileRealSize;
    uint32_t filePackSize;
    uint32_t fileByteOffset;
};

bool pak_unpack(const string& pakFile, VCD_fs* fs)
{
    return pak_unpack(VCD_getDirEntry(pakFile, fs), fs);
}

bool pak_unpack(VCD_FileTree* leaf, VCD_fs* fs)
{
    if (VCD_isDir(leaf)) {
        return false;
    }

    vector<uint8_t> buffer;
    if (!VCD_inputFile(buffer, leaf, fs)) {
        return false;
    }
    if (buffer.size() < 16) {
        return false;
    }

    pakFileHeader header = *((pakFileHeader*)&buffer[0]);
    if (strncmp(header.sig, "PACK", 4) != 0) {
        return false;
    }

    for (unsigned i = 0; i < header.entriesCount; ++i) {
        pakEntry entry = ((pakEntry*)&buffer[16])[i];
        VCD_FileTree* newLeaf = new VCD_FileTree;
        newLeaf->isDir = false;

        strcpy(newLeaf->info.fileName, entry.entryName);
        newLeaf->info.file.fileBytePos = entry.fileByteOffset + leaf->info.file.fileBytePos;
        newLeaf->info.file.fileRealSize = entry.fileRealSize;
        newLeaf->info.file.fileFsSize = entry.filePackSize;

        leaf->leafs.push_back(newLeaf);
    }

    leaf->info.dir.dirEntries = leaf->leafs.size();
    leaf->info.dir.entriesDwordPos = 0xFFFFFFFF;
    leaf->info.dir.dirSubdirs = 0;
    leaf->info.dir.subsDwordPos = 0xFFFFFFFF;
    leaf->isDir = true;

    return true;
}
