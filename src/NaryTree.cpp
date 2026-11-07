#include "NaryTree.h"

DirNode::DirNode(std::string name, DirNode *parent) : FileNode(name, parent, true) {
}

DirNode::~DirNode() {
    for (int i = 0; i < this->subdirs.size(); i++) {
        delete this->subdirs[i];
    }
}

DirNode *DirNode::getParentDir() {
    return (DirNode *) this->parent;
}

NaryTree::NaryTree(std::string rootName) {
    this->root = new DirNode(rootName, nullptr);
    this->current = this->root;
    this->dirAccesses = 0;
}

NaryTree::~NaryTree() {
    delete this->root;
}

DirNode *NaryTree::findSubdir(DirNode *dir, std::string name) {
    for (int i = 0; i < dir->subdirs.size(); i++) {
        this->dirAccesses++;
        if (dir->subdirs[i]->getName() == name) {
            return dir->subdirs[i];
        }
    }
    return nullptr;
}

bool NaryTree::makeDir(std::string name) {
    if (findSubdir(this->current, name) != nullptr) {
        return false;
    }
    this->current->subdirs.push_back(new DirNode(name, this->current));
    return true;
}

bool NaryTree::addFile(std::string name) {
    FileNode *file = new FileNode(name, this->current, false);
    if (!this->current->files.insert(file)) {
        delete file;
        return false;
    }
    return true;
}

bool NaryTree::findFile(std::string name) {
    return this->current->files.contains(name);
}

FileNode *NaryTree::getFile(std::string name) {
    return this->current->files.find(name);
}

bool NaryTree::removeFile(std::string name) {
    return this->current->files.remove(name);
}

bool NaryTree::changeDir(std::string name) {
    DirNode *dir = findSubdir(this->current, name);
    if (dir == nullptr) {
        return false;
    }
    this->current = dir;
    return true;
}

bool NaryTree::goUp() {
    if (this->current->getParent() == nullptr) {
        return false;
    }
    this->dirAccesses++;
    this->current = this->current->getParentDir();
    return true;
}

void NaryTree::goToRoot() {
    this->current = this->root;
}

DirNode *NaryTree::getCurrentDir() {
    return this->current;
}

std::string NaryTree::getCurrentName() {
    return this->current->getName();
}

std::string NaryTree::getCurrentPath() {
    std::string path = "";
    DirNode *dir = this->current;
    while (dir != nullptr) {
        path = "/" + dir->getName() + path;
        dir = dir->getParentDir();
    }
    return path;
}

int NaryTree::getSubdirCount() {
    return this->current->subdirs.size();
}

int NaryTree::getFileCount() {
    return this->current->files.size();
}

std::vector<std::string> NaryTree::listSubdirs() {
    std::vector<std::string> names;
    for (int i = 0; i < this->current->subdirs.size(); i++) {
        names.push_back(this->current->subdirs[i]->getName());
    }
    return names;
}

std::vector<std::string> NaryTree::listFiles() {
    return this->current->files.inorderNames();
}

int NaryTree::addUpAccesses(DirNode *dir) {
    int total = dir->files.getNodeAccesses();
    for (int i = 0; i < dir->subdirs.size(); i++) {
        total += addUpAccesses(dir->subdirs[i]);
    }
    return total;
}

void NaryTree::resetAccesses(DirNode *dir) {
    dir->files.resetNodeAccesses();
    for (int i = 0; i < dir->subdirs.size(); i++) {
        resetAccesses(dir->subdirs[i]);
    }
}

int NaryTree::getNodeAccesses() {
    return this->dirAccesses + addUpAccesses(this->root);
}

void NaryTree::resetNodeAccesses() {
    this->dirAccesses = 0;
    resetAccesses(this->root);
}
