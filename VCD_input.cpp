#include "VCD_input.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

#include <cstring>

using namespace std;

bool VCD_fs::DirEntry::getAllSubs(std::vector<const DirEntry*>& dst) const
{
    if (!isDir()) {
        return false;
    }

    dst.clear();
    for (auto it = leafs.begin(); it != leafs.end(); ++it) {
        dst.push_back(it->second);
    }
    return true;
}

const VCD_fs::DirEntry* VCD_fs::DirEntry::getSubEntry(const std::string& name) const
{
    if (!isDir()) {
        return nullptr;
    }

    for (auto it = leafs.begin(); it != leafs.end(); ++it) {
        if (it->first == name) {
            return it->second;
        }
    }
    return nullptr;
}

VCD_fs::DirEntry::DirEntry(const std::string name, bool is_dir, uint32_t data[4])
{
    this->name = name;
    this->is_dir = is_dir;
    if (is_dir) {
        this->data.dir.dirEntries = data[0];
        this->data.dir.entriesDwordPos = data[1];
        this->data.dir.dirSubdirs = data[2];
        this->data.dir.subsDwordPos = data[3];
    } else {
        this->data.file.fileRealSize = data[0];
        this->data.file.fileFsSize = data[1];
        this->data.file.fileBytePos = data[2];
        this->data.file.resv = data[3];
    }
}

void VCD_fs::DirEntry::addSubEntry(DirEntry* newLeaf)
{
    if (newLeaf) {
        leafs[newLeaf->getName()] = newLeaf;
    }
}

void VCD_fs::DirEntry::addSubEntry(const std::string& name, bool is_dir, uint32_t data[4])
{
    DirEntry* newLeaf = new DirEntry(name, is_dir, data);
    leafs[name] = newLeaf;
}

void VCD_fs::DirEntry::remSubEntry(const std::string& name)
{
    auto it = leafs.find(name);
    if (it != leafs.end()) {
        leafs.erase(it);
    }
}

VCD_fs::DirEntry* VCD_fs::DirEntry::getSubEntry(const std::string& name)
{
    auto it = leafs.find(name);
    if (it != leafs.end()) {
        return it->second;
    }
    return nullptr;
}


size_t VCD_fs::File::read(void* src, size_t srcSize)
{
    if (isOpen() && src) {
        if (srcSize > fileSize - fileOffset) {
            srcSize = fileSize - fileOffset;
        }
        parent.seekg(fileImageOffset + fileOffset, std::ios::beg);
        if (!parent.read((char*)src, srcSize)) {
            cerr << "Unable to read file from VCD image." << endl;
            return 0;
        }
        fileOffset += srcSize;

        return srcSize;
    }
    return 0;
}

void VCD_fs::File::seek(long relOffset, SeekMode mode)
{
    if (isOpen()) {
        switch (mode)
        {
        case SeekMode::beg:
            fileOffset = relOffset;
        break;
        case SeekMode::cur:
            fileOffset += relOffset;
        break;
        case SeekMode::ate:
            fileOffset = fileSize - relOffset;
        break;
        }
    }
}

void VCD_fs::File::open(const DirEntry& entry, size_t dirTableEnd)
{
    if (!entry.isDir()) {
        DirEntry::FileData info = entry.getFileInfo();

        fileSize = info.fileRealSize;
        fileImageOffset = info.fileBytePos + dirTableEnd;
        fileOffset = 0;
        opened = true;
    }
}

void VCD_fs::mount(const std::string& imagePath)
{
    if (mounted) {
        unmount();
    }

    imageFile.open(imagePath, ios::binary);
    if (!imageFile.is_open()) {
        cerr << "Can't open \"" << imagePath << "\" for reading." << endl;
        return;
    }

    imageFile.seekg(0xC, ios::beg);
    if (!imageFile.read((char*)&dirTableEnd, sizeof(dirTableEnd))) {
        cerr << "File \"" << imagePath << "\" appears to be corrupted. No header data." << endl;
        imageFile.close();
        return;
    }

    imageFile.seekg(0x10, ios::beg);

    char name[32];
    uint32_t data[4];

    if (!imageFile.read(name, sizeof(name)) || !imageFile.read((char*)data, sizeof(data))) {
        cerr << "File \"" << imagePath << "\" appears to be corrupted. No directory data." << endl;
        imageFile.close();
        return;
    }
    root = new DirEntry(name, true, data);

    if (!parse(*root)) {
        delete root;
        root = nullptr;
        return;
    }
    mounted = true;
}

void VCD_fs::unmount()
{
    if (mounted) {
        mounted = false;
        imageFile.close();
    }
    if (root) {
        delete root;
        root = nullptr;
    }
}

VCD_fs::File VCD_fs::openFile(DirEntry& entry)
{
    File newFile(imageFile);
    newFile.open(entry, dirTableEnd);

    return newFile;
}

VCD_fs::File VCD_fs::openFile(const std::string& internalPath)
{
    const DirEntry* entry = getDirEntry(internalPath);
    File newFile(imageFile);

    if (entry) {
        newFile.open(*entry, dirTableEnd);
    }

    return newFile;
}

VCD_fs::DirEntry* VCD_fs::getDirEntry(const std::string& internalPath)
{
    DirEntry* leaf = root;
    string entryName;

    auto it = internalPath.begin();
    if (*it == '/') {
        ++it;
    }
    for (; it < internalPath.end(); ++it) {
        if (*it == '/') {
            leaf = leaf->getSubEntry(entryName);
            if (leaf == nullptr) {
                return nullptr;
            }
            entryName.clear();
        } else {
            entryName.push_back(*it);
        }
    }
    if (!entryName.empty()) {
        leaf = leaf->getSubEntry(entryName);
        if (leaf == nullptr) {
            return nullptr;
        }
    }

    return leaf;
}

bool VCD_fs::parse(DirEntry& curDir)
{
    imageFile.seekg(curDir.data.dir.entriesDwordPos*4, ios::beg);
    for (uint32_t i = 0; i < curDir.data.dir.dirEntries; ++i) {
        char name[32];
        uint32_t data[4];

        if (!imageFile.read(name, sizeof(name)) || !imageFile.read((char*)data, sizeof(data))) {
            return false;
        }
        curDir.addSubEntry(name, false, data);
    }

    vector<VCD_fs::DirEntry*> dirEntries;

    imageFile.seekg(curDir.data.dir.subsDwordPos*4, ios::beg);
    for (uint32_t i = 0; i < curDir.data.dir.dirSubdirs; ++i) {
        char name[32];
        uint32_t data[4];

        if (!imageFile.read(name, sizeof(name)) || !imageFile.read((char*)data, sizeof(data))) {
            return false;
        }
        DirEntry* newLeaf = new DirEntry(name, true, data);
        curDir.addSubEntry(newLeaf);
        dirEntries.push_back(newLeaf);
    }

    for (auto l : dirEntries) {
        if (!parse(*l)) {
            return false;
        }
    }

    return true;
}

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

bool VCD_fs::unpackFile(DirEntry& leaf)
{
    if (leaf.isDir()) {
        return false;
    }

    VCD_fs::File infile = openFile(leaf);
    if (!infile.isOpen()) {
        return false;
    }

    infile.seek(0, File::SeekMode::ate);
    size_t bufSize = infile.tell();
    infile.seek(0, File::SeekMode::beg);

    vector<uint8_t> buffer;
    buffer.resize(bufSize);
    if (infile.read(&buffer[0], bufSize) != bufSize) {
        return false;
    }

    pakFileHeader header = *((pakFileHeader*)&buffer[0]);
    if (strncmp(header.sig, "PACK", 4) != 0) {
        return false;
    }

    for (unsigned i = 0; i < header.entriesCount; ++i) {
        pakEntry entry = ((pakEntry*)&buffer[16])[i];
        VCD_fs::DirEntry* newLeaf;
        uint32_t data[4];

        if ((entry.mode & 0x1) == 0) {
            pakEntryRev revEntry = ((pakEntryRev*)&buffer[48])[i];

            data[0] = revEntry.fileRealSize;
            data[1] = revEntry.filePackSize;
            data[2] = revEntry.fileByteOffset + leaf.data.file.fileBytePos;
            data[3] = 0;

            newLeaf = new VCD_fs::DirEntry(revEntry.entryName, false, data);
        } else {
            data[0] = entry.fileRealSize;
            data[1] = entry.filePackSize;
            data[2] = entry.fileByteOffset + leaf.data.file.fileBytePos;
            data[3] = 0;

            newLeaf = new VCD_fs::DirEntry(entry.entryName, false, data);
        }

        if (newLeaf->getName() != "none") {
            leaf.addSubEntry(newLeaf);
        } else {
            delete newLeaf;
        }
    }

    leaf.data.dir.dirEntries = leaf.leafs.size();
    leaf.data.dir.entriesDwordPos = 0xFFFFFFFF;
    leaf.data.dir.dirSubdirs = 0;
    leaf.data.dir.subsDwordPos = 0xFFFFFFFF;
    leaf.is_dir = true;

    return true;
}

bool VCD_fs::unpackFile(const std::string& internalPath)
{
    auto entry = getDirEntry(internalPath);
    if (!entry) {
        return false;
    }
    return unpackFile(*entry);
}
