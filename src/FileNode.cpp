#include "FileNode.h"

#include <algorithm>
#include <iostream>
#include <ostream>

// Constructors/Destructors

FileNode::FileNode() {
    this->name = "";
    this->parent = nullptr;
    this->height = 1;
    this->folder = false;
}

FileNode::FileNode(const std::string &name, FileNode *parent, bool isFolder) {
    this->name = name;
    this->parent = parent;
    this->height = 1;
    this->folder = isFolder;
}

FileNode::~FileNode() {
    for (auto* child : this->children) {
        delete child;
    }
    this->children.clear();
}

// Setters

bool FileNode::addChild(FileNode *child) {
    if (!this->folder) {
        throw std::runtime_error("Cannot add child to a non-folder node");
    }

    for (const auto& n : this->children) {
        if (n->getName() == child->getName()) {
            return false;
        }
    }

    child->setParent(this);
    this->children.push_back(child);
    this->updateHeight();

    return true;
}

void FileNode::setName(const std::string &name) {
    this->name = name;
}

void FileNode::setParent(FileNode *parent) {
    if (parent == nullptr || !parent->isFolder()) {
        throw std::runtime_error("Cannot set parent of a non-folder node");
    }

    this->parent = parent;
}

void FileNode::updateHeight() {
    int maxHeight = 0;
    for (const auto& n : this->children) {
        maxHeight = std::max(maxHeight, n->height);
    }
    this->height = 1 + maxHeight;

    if (this->parent != nullptr) {
        this->parent->updateHeight();
    }
}

// Getters

std::string FileNode::getName() const {
    return this->name;
}

FileNode *FileNode::getParent() const {
    return this->parent;
}

int FileNode::getHeight() const {
    return this->height;
}

bool FileNode::isFolder() const {
    return this->folder;
}

void FileNode::print() const {
    std::cout << this->name << " (height:" << this->height << ")" << std::endl;

    if (this->folder) {
        std::cout << "Children:" << std::endl;
        for (const auto& n : this->children) {
            n->print();
        }
    }

    std::cout << "Parent: " << (this->parent == nullptr ? "null" : this->parent->getName()) << std::endl;
}
