#ifndef VCD_INPUT_H
#define VCD_INPUT_H

#include <string>
#include <vector>
#include <fstream>
#include <map>

class VCD_fs
{
public:
    class DirEntry
    {
        friend class VCD_fs;

    public:
        struct FileData {
            uint32_t fileRealSize;
            uint32_t fileFsSize;
            uint32_t fileBytePos;
            uint32_t resv;
        };
        struct DirData {
            uint32_t dirEntries;
            uint32_t entriesDwordPos;
            uint32_t dirSubdirs;
            uint32_t subsDwordPos;
        };

    public:
        bool isDir() const {
            return is_dir;
        }
        bool isFile() const {
            return !is_dir;
        }

        const std::string getName() const {
            return name;
        }

        bool getAllSubs(std::vector<const DirEntry*>& dst) const;
        const DirEntry* getSubEntry(const std::string& name) const;

        const FileData& getFileInfo() const {
            return data.file;
        }

        const DirData& getDirInfo() const {
            return data.dir;
        }

    private:
        DirEntry(const std::string name, bool is_dir, uint32_t data[4]);
        ~DirEntry() {
            for (auto l : leafs) {
                delete l.second;
            }
        }

        void addSubEntry(DirEntry* newLeaf);
        void addSubEntry(const std::string& name, bool is_dir, uint32_t data[4]);
        void remSubEntry(const std::string& name);

        DirEntry* getSubEntry(const std::string& name);

    private:
        std::string name;
        bool is_dir;

        union {
            FileData file;
            DirData dir;
        } data;

        std::map<std::string, DirEntry*> leafs;
    };

    class File
    {
        friend class VCD_fs;

    public:
        enum class SeekMode {
            beg, cur, ate
        };

    public:
        size_t read(void* src, size_t srcSize);

        void seek(long relOffset, SeekMode mode);
        size_t tell() const {
            return fileOffset;
        }

        bool isOpen() const {
            return opened;
        }

    private:
        File(std::ifstream& fs)
        : parent(fs), opened(false), fileSize(0),
          fileImageOffset(0), fileOffset(0)
        {}

        void open(const DirEntry& entry, size_t dirTableEnd);

    private:
        std::ifstream& parent;
        bool opened;

        size_t fileSize;
        size_t fileImageOffset;

        size_t fileOffset;
    };

public:
    VCD_fs() {
    }

    VCD_fs(const std::string& imagePath) {
        this->mount(imagePath);
    }

    ~VCD_fs() {
        this->unmount();
    }

    void mount(const std::string& imagePath);
    void unmount();

    bool isMounted() const {
        return mounted;
    }

    File openFile(DirEntry& entry);
    File openFile(const std::string& internalPath);
    DirEntry* getDirEntry(const std::string& internalPath);

    bool unpackFile(DirEntry& entry);
    bool unpackFile(const std::string& internalPath);

private:
    bool parse(DirEntry& curDir);

private:
    bool mounted = false;
    std::ifstream imageFile;

    VCD_fs::DirEntry* root;
    size_t dirTableEnd;
};

#endif // VCD_INPUT_H
