#pragma once

#include <string>
#include <vector>

#include "FileNode.h"
#include "SplayTree.h"

class DirNode : public FileNode {
public:
    std::vector<DirNode *> subdirs;
    SplayTree files;

    DirNode(std::string name, DirNode *parent);
    ~DirNode();

    DirNode *getParentDir();
};

class NaryTree {
private:
    DirNode *root;
    DirNode *current;
    int dirAccesses;

    DirNode *findSubdir(DirNode *dir, std::string name);
    int addUpAccesses(DirNode *dir);
    void resetAccesses(DirNode *dir);

public:
    NaryTree(std::string rootName = "root");
    ~NaryTree();

    bool makeDir(std::string name);
    bool addFile(std::string name);

    bool findFile(std::string name);
    FileNode *getFile(std::string name);
    bool removeFile(std::string name);

    bool changeDir(std::string name);
    bool goUp();
    void goToRoot();

    DirNode *getCurrentDir();
    std::string getCurrentName();
    std::string getCurrentPath();
    int getSubdirCount();
    int getFileCount();
    std::vector<std::string> listSubdirs();
    std::vector<std::string> listFiles();

    int getNodeAccesses();
    void resetNodeAccesses();
};
