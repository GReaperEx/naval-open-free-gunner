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
    VCD_FileTree* pakLeaf = VCD_getDirEntry(pakFile, fs);
    if (!pakLeaf || VCD_isDir(pakLeaf)) {
        return false;
    }

    vector<uint8_t> buffer;
    if (!VCD_inputFile(buffer, pakLeaf, fs)) {
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

        strcpy(newLeaf->info.fileName, entry.entryName);
        newLeaf->info.file.fileBytePos = entry.fileByteOffset + pakLeaf->info.file.fileBytePos;
        newLeaf->info.file.fileRealSize = entry.fileRealSize;
        newLeaf->info.file.fileFsSize = entry.filePackSize;

        pakLeaf->leafs.push_back(newLeaf);
    }

    char *ptr;
    while ((ptr = strchr(pakLeaf->info.fileName, '.')) != nullptr) {
        *ptr = '_';
    }

    pakLeaf->info.dir.dirEntries = pakLeaf->leafs.size();
    pakLeaf->info.dir.entriesDwordPos = 0xFFFFFFFF;
    pakLeaf->info.dir.dirSubdirs = 0;
    pakLeaf->info.dir.subsDwordPos = 0xFFFFFFFF;

    return true;
}
