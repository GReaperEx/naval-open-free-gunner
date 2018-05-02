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
    uint32_t mode;          // Bit0: 1
    uint32_t fileRealSize;
    uint32_t filePackSize;
    uint32_t fileByteOffset;
};

struct pakEntryRev
{
    uint32_t mode;          // Bit0: 0
    uint32_t fileRealSize;
    uint32_t filePackSize;
    uint32_t fileByteOffset;
    char entryName[32];
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

        if ((entry.mode & 0x1) == 0) {
            pakEntryRev revEntry = ((pakEntryRev*)&buffer[48])[i];

            strcpy(newLeaf->info.fileName, revEntry.entryName);
            newLeaf->info.file.fileBytePos = revEntry.fileByteOffset + leaf->info.file.fileBytePos;
            newLeaf->info.file.fileRealSize = revEntry.fileRealSize;
            newLeaf->info.file.fileFsSize = revEntry.filePackSize;
        } else {
            strcpy(newLeaf->info.fileName, entry.entryName);
            newLeaf->info.file.fileBytePos = entry.fileByteOffset + leaf->info.file.fileBytePos;
            newLeaf->info.file.fileRealSize = entry.fileRealSize;
            newLeaf->info.file.fileFsSize = entry.filePackSize;
        }

        if (strcmp(newLeaf->info.fileName, "none") != 0) {
            leaf->leafs.push_back(newLeaf);
        } else {
            delete newLeaf;
        }
    }

    leaf->info.dir.dirEntries = leaf->leafs.size();
    leaf->info.dir.entriesDwordPos = 0xFFFFFFFF;
    leaf->info.dir.dirSubdirs = 0;
    leaf->info.dir.subsDwordPos = 0xFFFFFFFF;
    leaf->isDir = true;

    return true;
}
