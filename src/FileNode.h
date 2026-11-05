#pragma once

#include <string>
#include <vector>

class FileNode {
protected:
    bool folder;
    std::string name;
    FileNode *parent;
    std::vector<FileNode *> children;
    int height;
public:
    FileNode();
    FileNode(const std::string &name, FileNode *parent, bool isFolder);
    ~FileNode();

    // Strange behavior happens with copying, so I'm turning this off
    FileNode(const FileNode&) = delete;
    FileNode& operator=(const FileNode&) = delete;

    std::string getName() const;
    FileNode *getParent() const;

    void setName(const std::string &name);
    void setParent(FileNode *parent);
    /*
    Attempts to insert a child, returning false if unsuccessful (name collision)
    */
    bool addChild(FileNode *child);
    int getHeight() const;
    void updateHeight();

    bool isFolder() const;
    const std::vector<FileNode *> &getChildren() const;
    void print() const;
};
