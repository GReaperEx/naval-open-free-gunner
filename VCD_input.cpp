#include "VCD_input.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

#include <cstring>

using namespace std;

bool parseDir(ifstream& infile, VCD_FileTree& curDir);

void _deleteDir(VCD_FileTree* leaf);

VCD_fs* VCD_open(const std::string& filePath)
{
    VCD_fs* fs = new VCD_fs;

    fs->imageFile.open(filePath, ios::binary);
    if (!fs->imageFile.is_open()) {
        cerr << "Can't open \"" << filePath << "\" for reading." << endl;
        return nullptr;
    }

    uint32_t dirTableSize;
    fs->imageFile.seekg(0xC, ios::beg);
    if (!fs->imageFile.read((char*)&dirTableSize, sizeof(dirTableSize))) {
        cerr << "File \"" << filePath << "\" appears to be corrupted. No header data." << endl;
        return nullptr;
    }

    fs->dirTableEnd = dirTableSize;

    fs->imageFile.seekg(0x10, ios::beg);
    VCD_FileTree* root = new VCD_FileTree;
    if (!fs->imageFile.read((char*)&(root->info), sizeof(root->info))) {
        cerr << "File \"" << filePath << "\" appears to be corrupted. No directory data." << endl;
        return nullptr;
    }
    if (strchr(root->info.fileName, '.') != nullptr) {
        cerr << "File \"" << filePath << "\" appears to be corrupted. First entry must be a directory." << endl;
        return nullptr;
    }

    if (!parseDir(fs->imageFile, *root)) {
        _deleteDir(root);
        return nullptr;
    }

    fs->root = root;
    return fs;
}

bool parseDir(ifstream& infile, VCD_FileTree& curDir)
{
    infile.seekg(curDir.info.dir.entriesDwordPos*4, ios::beg);
    for (uint32_t i = 0; i < curDir.info.dir.dirEntries; ++i) {
        VCD_FileTree* newLeaf = new VCD_FileTree;
        if (!infile.read((char*)&(newLeaf->info), sizeof(newLeaf->info))) {
            return false;
        }
        curDir.leafs.push_back(newLeaf);
    }

    vector<VCD_FileTree*> dirEntries;

    infile.seekg(curDir.info.dir.subsDwordPos*4, ios::beg);
    for (uint32_t i = 0; i < curDir.info.dir.dirSubdirs; ++i) {
        VCD_FileTree* newLeaf = new VCD_FileTree;
        if (!infile.read((char*)&(newLeaf->info), sizeof(newLeaf->info))) {
            return false;
        }
        curDir.leafs.push_back(newLeaf);
        dirEntries.push_back(newLeaf);
    }

    for (auto l : dirEntries) {
        if (!parseDir(infile, *l)) {
            return false;
        }
    }

    return true;
}

void VCD_close(VCD_fs* fs)
{
    if (fs) {
        _deleteDir(fs->root);
        delete fs;
    }
}

void _deleteDir(VCD_FileTree* leaf)
{
    if (leaf) {
        for (auto l : leaf->leafs) {
            _deleteDir(l);
        }
        delete leaf;
    }
}

bool VCD_isDir(const VCD_FileTree* leaf)
{
    return strchr(leaf->info.fileName, '.') == nullptr;
}

VCD_FileTree* VCD_getDirEntry(const std::string& filePath, VCD_fs* fs)
{
    VCD_FileTree* leaf = fs->root;
    string entryName;

    auto it = filePath.begin();
    if (*it == '/') {
        ++it;
    }
    for (; it < filePath.end(); ++it) {
        if (*it == '/') {
            auto oldLeaf = leaf;
            for (auto l : leaf->leafs) {
                if (l->info.fileName == entryName) {
                    leaf = l;
                    break;
                }
            }
            if (oldLeaf == leaf) {
                return nullptr;
            }
            entryName.clear();
        } else {
            entryName.push_back(*it);
        }
    }
    if (!entryName.empty()) {
        auto oldLeaf = leaf;
        for (auto l : leaf->leafs) {
            if (l->info.fileName == entryName) {
                leaf = l;
                break;
            }
        }
        if (oldLeaf == leaf) {
            return nullptr;
        }
    }

    return leaf;
}

bool VCD_inputFile(std::vector<uint8_t>& buffer, VCD_FileTree* leaf, VCD_fs* fs)
{
    if (leaf && !VCD_isDir(leaf)) {
        fs->imageFile.seekg(fs->dirTableEnd, ios::beg);
        fs->imageFile.seekg(leaf->info.file.fileBytePos, ios::cur);

        buffer.resize(leaf->info.file.fileRealSize);
        if (!fs->imageFile.read((char*)&buffer[0], buffer.size())) {
            cerr << "Unable to read VCD image for file." << endl;
            return false;
        }
        return true;
    }
    return false;
}

bool VCD_inputFile(std::vector<uint8_t>& buffer, const std::string& filePath, VCD_fs* fs)
{
    return VCD_inputFile(buffer, VCD_getDirEntry(filePath, fs), fs);
}
