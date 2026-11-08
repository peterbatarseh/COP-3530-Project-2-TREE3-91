#pragma once

#include <string>
#include <vector>

#include "FileNode.h"

struct SplayNode {
    FileNode *file;
    SplayNode *left;
    SplayNode *right;
    SplayNode *parent;

    SplayNode(FileNode *file);
};

class SplayTree {
private:
    SplayNode *root;
    int count;
    int nodeAccesses;

    void rotateLeft(SplayNode *x);
    void rotateRight(SplayNode *x);
    void splay(SplayNode *x);

    SplayNode *findNode(std::string name);
    void inorder(SplayNode *node, std::vector<std::string> &names);
    void destroy(SplayNode *node);

public:
    SplayTree();
    ~SplayTree();

    // On false (duplicate name) the caller still owns file
    bool insert(FileNode *file);
    FileNode *find(std::string name);
    bool contains(std::string name);
    bool remove(std::string name);

    int size();
    bool empty();
    std::string rootName();
    std::vector<std::string> inorderNames();

    int getNodeAccesses();
    void resetNodeAccesses();
};
