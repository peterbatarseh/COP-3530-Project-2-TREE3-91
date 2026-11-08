#include "TreeLoader.h"

#include <sstream>

#include "BPlusTree.h"
#include "FileNode.h"
#include "NaryTree.h"

namespace {

std::string join(const std::string &path, const std::string &name) {
    return path.empty() ? name : path + "/" + name;
}

void addToBPlus(const FileNode *dir, const std::string &path, BPlusTree &tree) {
    for (const FileNode *child : dir->getChildren()) {
        if (child->isFolder()) {
            tree.insertDirectory(path, child->getName());
            addToBPlus(child, join(path, child->getName()), tree);
        } else {
            tree.insertFile(path, child->getName());
        }
    }
}

void addToNary(const FileNode *dir, NaryTree &tree) {
    for (const FileNode *child : dir->getChildren()) {
        if (child->isFolder()) {
            tree.makeDir(child->getName());
            tree.changeDir(child->getName());
            addToNary(child, tree);
            tree.goUp();
        } else {
            tree.addFile(child->getName());
        }
    }
}

void collectFiles(const FileNode *dir, const std::string &path, std::vector<std::string> &out) {
    for (const FileNode *child : dir->getChildren()) {
        if (child->isFolder())
            collectFiles(child, join(path, child->getName()), out);
        else
            out.push_back(join(path, child->getName()));
    }
}

std::vector<std::string> split(const std::string &path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '/')) {
        if (!part.empty())
            parts.push_back(part);
    }
    return parts;
}

} // namespace

void loadIntoBPlus(const FileNode *root, BPlusTree &tree) {
    addToBPlus(root, "", tree);
}

void loadIntoNary(const FileNode *root, NaryTree &tree) {
    // Insertion requires starting from the root to find the correct path
    tree.goToRoot();
    addToNary(root, tree);
    // Go back to root for future insertions + reset node accesses
    tree.goToRoot();
    tree.resetNodeAccesses();
}

std::vector<std::string> allFilePaths(const FileNode *root) {
    std::vector<std::string> paths;
    collectFiles(root, "", paths);
    return paths;
}

std::size_t bplusLookup(BPlusTree &tree, const std::string &path) {
    std::size_t visits = 0;
    tree.contains(path, nullptr, &visits);
    return visits;
}

int splayLookup(NaryTree &tree, const std::string &path) {
    std::vector<std::string> parts = split(path);
    if (parts.empty())
        return 0;

    tree.goToRoot();
    tree.resetNodeAccesses();
    for (std::size_t i = 0; i + 1 < parts.size(); i++) {
        if (!tree.changeDir(parts[i]))
            break;
    }
    tree.findFile(parts.back());
    int accesses = tree.getNodeAccesses();
    tree.goToRoot();
    return accesses;
}

int splayFolderLookup(NaryTree &tree, const std::string &path) {
    tree.goToRoot();
    tree.resetNodeAccesses();
    for (const std::string &dir : split(path)) {
        if (!tree.changeDir(dir))
            break;
    }
    int accesses = tree.getNodeAccesses();
    tree.goToRoot();
    return accesses;
}