/*
B+ tree implementation
Due to how B+ trees are structured, a BPlusNode class is used.
*/
#pragma once

#include <string>
#include <vector>
class FileNode; // too impratcical to use as node data, but is needed for toFileNodes() (web app rendering)

class BPlusNode {
  public:
    BPlusNode(bool isLeaf) {
        this->leaf = isLeaf;
        this->parent = nullptr;
        this->nextLeaf = nullptr;
    }

    ~BPlusNode() {
        for (BPlusNode *child : children) {
            delete child;
        }
    }

    bool leaf;                         // is a leaf node
    std::vector<std::string> keys;     // data
    std::vector<bool> directories;     // true if the child key is a directory, false if it's a file
    std::vector<BPlusNode *> children; // key data
    BPlusNode *parent;                 // parent pointer
    BPlusNode *nextLeaf;               // next pointer
};

class BPlusTree {
  private:
    BPlusNode *root;            // root node
    std::string rootName;       // name of the root directory (default "root")
    std::size_t maxKeysPerNode; // maximum number of keys per node (level)
    std::size_t entryCount;     // total file/directory count

    /*
    Finds the leaf node based on the path
    Returns nullptr if not found
    */
    BPlusNode *findLeaf(const std::string &path) const;
    /*
    Returns the position where a key is found or should be inserted
    Equivalent of a binary search
    */
    std::vector<std::string>::iterator findKey(BPlusNode *node, const std::string &key) const;
    /*
    B+ tree insertion. Splits node if needed
    */
    bool insert(const std::string &path, bool isDirectory);
    /*
    Splits apart leaf node, pushes the middle key up to the parent
    */
    void splitLeaf(BPlusNode *leaf);
    /*
    Inserts into the given node. Splits node if needed
    */
    void insertIntoParent(BPlusNode *left, const std::string &separator, BPlusNode *right);
    /*
    Splitting for non-leaf nodes, pushes the middle key up to the parent
    */
    void splitInternal(BPlusNode *node);
    /*
    Removes the specified path. Returns true if successful, false otherwise. Rebalances
    */
    bool remove(const std::string &path);
    /*
    Rebalances tree after a removal
    Tries borrowing before merging
    */
    void rebalance(BPlusNode *node);
    /*
    Takes keys/children from left/right leaf/internal nodes to rebalance
    */
    void borrowFromLeft(BPlusNode *node, BPlusNode *left);
    void borrowFromRight(BPlusNode *node, BPlusNode *right);

    /*
    Merges two nodes into one and updates the parent accordingly
    */
    void mergeNodes(BPlusNode *left, BPlusNode *right);
    /*
    Updates the separator keys in the parent nodes up to the root
    */
    void updateSeparators(BPlusNode *node);

  public:
    BPlusTree(const std::string &rootName = "root", std::size_t maxKeysPerNode = 3);
    ~BPlusTree();

    /*
    If a test B+ tree is copied, that will be megabytes of data duplicated because C++ loves making copies of everything
    So this is being disabled.
    */
    BPlusTree(const BPlusTree &) = delete;
    BPlusTree &operator=(const BPlusTree &) = delete;

    std::size_t size() const;

    /*
    Inserts a file into the B+ tree under the specified parent path.
    The parent directory must exist.
    */
    bool insertFile(const std::string &parentPath, const std::string &name);

    /*
    Inserts a directory into the B+ tree under the specified parent path.
    The parent directory must exist.
    */
    bool insertDirectory(const std::string &parentPath, const std::string &name);

    /*
    Returns false for a missing path or the wrong entry type.
    */
    bool removeFile(const std::string &path);

    /*
    Removes the directory and all descendants. The root cannot be removed.
    */
    bool removeDirectory(const std::string &path);

    /*
    Checks a relative path. Optional outputs report its type and tree-node visits.
    isDirectory and nodesAccessed are for the web app to track index count and to compare against the splay tree performance
    */
    bool contains(const std::string &path, bool *isDirectory = nullptr, std::size_t *nodesAccessed = nullptr) const;

    /*
    Creates a FileNode-based representation of the B+ tree (for the web app rendering)
    */
    FileNode *toFileNodes() const;
};
