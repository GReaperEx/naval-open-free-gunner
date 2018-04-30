#include "VCD_input.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

#include <cstring>

using namespace std;

bool parseDir(ifstream& infile, VCD_FileTree& curDir, size_t& i, uint32_t dirTableSize);

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

    fs->imageFile.seekg(0x40, ios::beg);
    VCD_FileTree* root = new VCD_FileTree;
    memset(&(root->info), 0, sizeof(root->info));

    for (size_t i = 0x40; i < dirTableSize; i += 48) {
        VCD_FileTree* newLeaf = new VCD_FileTree;
        if (!fs->imageFile.read((char*)&(newLeaf->info), sizeof(newLeaf->info))) {
            cerr << "File \"" << filePath << "\" appears to be corrupted. No directory data." << endl;
            break;
        }

        VCD_FileInfo temp;
        memset(&temp, 0, sizeof(temp));
        if (memcmp(&(newLeaf->info), &temp, sizeof(temp)) == 0) {
            break;
        }

        if (strchr(newLeaf->info.fileName, '.') == nullptr) {
            vector<VCD_FileTree*> dirLeafs;
            dirLeafs.push_back(newLeaf);

            VCD_FileTree tempLeaf;
            bool stop = false;
            for (; i < dirTableSize; i += 48) {
                if (!fs->imageFile.read((char*)&tempLeaf.info, sizeof(tempLeaf.info))) {
                    cerr << "File \"" << filePath << "\" appears to be corrupted. No directory data." << endl;
                    stop = true;
                    break;
                }

                if (strchr(tempLeaf.info.fileName, '.') == nullptr) {
                    newLeaf = new VCD_FileTree;
                    *newLeaf = tempLeaf;
                    dirLeafs.push_back(newLeaf);
                } else {
                    fs->imageFile.seekg(-48, ios::cur);
                    break;
                }
            }
            if (stop) {
                break;
            }

            for (size_t j = 0; j < dirLeafs.size(); ++j) {
                if (!parseDir(fs->imageFile, *dirLeafs[j], i, dirTableSize)) {
                    stop = true;
                    break;
                }
            }

            root->leafs.insert(root->leafs.end(), dirLeafs.begin(), dirLeafs.end());
            if (stop) {
                break;
            }
        } else {
            root->leafs.push_back(newLeaf);
        }
    }

    fs->root = root;
    return fs;
}

bool parseDir(ifstream& infile, VCD_FileTree& curDir, size_t& counter, uint32_t dirTableSize)
{
    for (size_t i = 0; i < curDir.info.dirEntries; ++i) {
        VCD_FileTree* newLeaf = new VCD_FileTree;
        if (!infile.read((char*)&(newLeaf->info), sizeof(newLeaf->info))) {
            break;
        }

        VCD_FileInfo temp;
        memset(&temp, 0, sizeof(temp));
        if (memcmp(&(newLeaf->info), &temp, sizeof(temp)) == 0) {
            break;
        }

        if (strchr(newLeaf->info.fileName, '.') == nullptr) {
            vector<VCD_FileTree*> dirLeafs;
            dirLeafs.push_back(newLeaf);

            VCD_FileTree tempLeaf;
            bool stop = false;
            for (; counter < dirTableSize; counter += 48) {
                if (!infile.read((char*)&tempLeaf.info, sizeof(tempLeaf.info))) {
                    stop = true;
                    break;
                }

                if (strchr(tempLeaf.info.fileName, '.') == nullptr) {
                    newLeaf = new VCD_FileTree;
                    *newLeaf = tempLeaf;
                    dirLeafs.push_back(newLeaf);
                } else {
                    infile.seekg(-48, ios::cur);
                    break;
                }
            }
            if (stop) {
                break;
            }

            for (size_t j = 0; j < dirLeafs.size(); ++j) {
                if (!parseDir(infile, *dirLeafs[j], counter, dirTableSize)) {
                    stop = true;
                    break;
                }
            }

            curDir.leafs.insert(curDir.leafs.end(), dirLeafs.begin(), dirLeafs.end());
            if (stop) {
                break;
            }
        } else {
            curDir.leafs.push_back(newLeaf);
        }
    }

    return true;
}

void _deleteDir(VCD_FileTree* leaf);

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
        fs->imageFile.seekg(leaf->info.fileBytePos, ios::cur);

        buffer.resize(leaf->info.fileRealSize);
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
