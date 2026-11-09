#include "TreeLoader.h"

#include <sstream>

#include "BPlusTree.h"
#include "FileNode.h"
#include "NaryTree.h"

// helpers only used in this file
namespace {

    // adds a name to a path
std::string join(const std::string &path, const std::string &name) {
    return path.empty() ? name : path + "/" + name;
}

// copies every folder and file into the B+ tree using full paths
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

// copies every folder and file into the splay tree (go into a folder, add stuff, go back up)
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

// makes a list of every file's full path
void collectFiles(const FileNode *dir, const std::string &path, std::vector<std::string> &out) {
    for (const FileNode *child : dir->getChildren()) {
        if (child->isFolder())
            collectFiles(child, join(path, child->getName()), out);
        else
            out.push_back(join(path, child->getName()));
    }
}

// splits "a/b/c" into "a", "b", "c"
std::vector<std::string> split(const std::string &path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) parts.push_back(part);
    }
    return parts;
}

}  // end of helpers


// fills the B+ tree with the generated data
void loadIntoBPlus(const FileNode *root, BPlusTree &tree) {
    addToBPlus(root, "", tree);
}

// fills the splay tree, then resets the counter so loading doesn't count
void loadIntoNary(const FileNode *root, NaryTree &tree) {
    tree.goToRoot();
    addToNary(root, tree);
    tree.goToRoot();
    tree.resetNodeAccesses();  
}

// every file path (used for the random searches in the benchmark)
std::vector<std::string> allFilePaths(const FileNode *root) {
    std::vector<std::string> paths;
    collectFiles(root, "", paths);
    return paths;
}

// searches the B+ tree and returns how many nodes it visited
std::size_t bplusLookup(BPlusTree &tree, const std::string &path) {
    std::size_t visits = 0;
    tree.contains(path, nullptr, &visits);
    return visits;
}

// searches the splay tree for a file and returns how many nodes it visited
int splayLookup(NaryTree &tree, const std::string &path) {
    std::vector<std::string> parts = split(path);
    if (parts.empty()) return 0;

    tree.goToRoot();
    tree.resetNodeAccesses();
    // go into each folder, then find the file in the last one
    for (std::size_t i = 0; i + 1 < parts.size(); i++) {
        if (!tree.changeDir(parts[i])) break;
    }
    tree.findFile(parts.back());
    int accesses = tree.getNodeAccesses();
    tree.goToRoot();
    return accesses;
}

// same as above but for a folder (how many nodes to open it)
int splayFolderLookup(NaryTree &tree, const std::string &path) {
    tree.goToRoot();
    tree.resetNodeAccesses();
    for (const std::string &dir : split(path)) {
        if (!tree.changeDir(dir)) break;
    }
    int accesses = tree.getNodeAccesses();
    tree.goToRoot();
    return accesses;
}