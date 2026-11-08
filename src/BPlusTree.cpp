#include "BPlusTree.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "FileNode.h"

// do they not have a better way of making private helpers
// it's ok bro c++29 fixes this bro
namespace {

bool isNoPath(const std::string &component) {
    return !component.empty() && component.find('/') == std::string::npos;
}

bool validPath(const std::string &path) {
    if (path.empty()) {
        return true;
    }

    std::size_t start = 0;

    while (start < path.size()) {
        const std::size_t separator = path.find('/', start);
        const std::size_t end = separator == std::string::npos ? path.size() : separator;

        if (end == start) {
            return false;
        }

        start = end + 1;
    }

    return path[path.size() - 1] != '/';
}

} // namespace

BPlusTree::BPlusTree(const std::string &rootName, std::size_t maxKeysPerNode) {
    this->root = nullptr;
    this->rootName = rootName;
    this->maxKeysPerNode = maxKeysPerNode;
    this->entryCount = 0;

    if (this->maxKeysPerNode < 2) {
        throw std::invalid_argument("A B+ tree node must hold at least two keys");
    }
    if (!isNoPath(this->rootName)) {
        throw std::invalid_argument("The root name must be a single path component");
    }

    root = new BPlusNode(true);
}

BPlusTree::~BPlusTree() {
    delete root;
}

std::size_t BPlusTree::size() const {
    return entryCount;
}

bool BPlusTree::insertFile(const std::string &parentPath, const std::string &name) {
    if (!isNoPath(name)) {
        throw std::invalid_argument("File name must not have '/'");
    }

    if (!validPath(parentPath)) {
        throw std::invalid_argument("Invalid parent path");
    }

    if (!parentPath.empty()) {
        bool isDirectory = false;

        if (!this->contains(parentPath, &isDirectory) || !isDirectory) {
            throw std::invalid_argument("No parent found");
        }
    }

    // if no parent path, insert at root
    const std::string path = parentPath.empty() ? name : parentPath + "/" + name;

    return this->insert(path, false);
}

bool BPlusTree::insertDirectory(const std::string &parentPath, const std::string &name) {
    if (!isNoPath(name)) {
        throw std::invalid_argument("Invalid path");
    }

    if (!validPath(parentPath)) {
        throw std::invalid_argument("Invalid parent path");
    }

    if (!parentPath.empty()) {
        bool isDirectory = false;

        if (!this->contains(parentPath, &isDirectory) || !isDirectory) {
            throw std::invalid_argument("The parent directory does not exist");
        }
    }

    const std::string path = parentPath.empty() ? name : parentPath + "/" + name;

    return this->insert(path, true);
}

bool BPlusTree::insert(const std::string &path, bool isDirectory) {
    BPlusNode *leaf = this->findLeaf(path);
    auto position = this->findKey(leaf, path);

    if (position != leaf->keys.end() && *position == path) {
        return false;
    }

    const std::size_t index = position - leaf->keys.begin();

    leaf->keys.insert(position, path);
    leaf->directories.insert(leaf->directories.begin() + index, isDirectory);
    ++entryCount;

    if (leaf->keys.size() > maxKeysPerNode) {
        this->splitLeaf(leaf);
    }

    return true;
}

BPlusNode *BPlusTree::findLeaf(const std::string &path) const {
    BPlusNode *node = root;

    while (!node->leaf) {
        std::size_t idx = 0;

        while (idx < node->keys.size() && path >= node->keys[idx]) {
            ++idx;
        }

        node = node->children[idx];
    }

    return node;
}

std::vector<std::string>::iterator BPlusTree::findKey(BPlusNode *node, const std::string &key) const {
    for (auto it = node->keys.begin(); it != node->keys.end(); ++it) {
        if (*it >= key) {
            return it;
        }
    }
    return node->keys.end();
}

void BPlusTree::splitLeaf(BPlusNode *leaf) {
    // split leaf node in half
    const std::size_t middle = (leaf->keys.size() + 1) / 2;
    BPlusNode *right = new BPlusNode(true);

    right->parent = leaf->parent;

    // Move half to the new node
    for (std::size_t i = middle; i < leaf->keys.size(); ++i) {
        right->keys.push_back(leaf->keys[i]);
        right->directories.push_back(leaf->directories[i]);
    }

    leaf->keys.resize(middle);
    leaf->directories.resize(middle);

    right->nextLeaf = leaf->nextLeaf;
    leaf->nextLeaf = right;
    // insert into parent node; will create one if needed
    this->insertIntoParent(leaf, right->keys[0], right);
}

void BPlusTree::insertIntoParent(BPlusNode *left, const std::string &separator, BPlusNode *right) {
    if (left->parent == nullptr) {
        BPlusNode *newRoot = new BPlusNode(false);

        newRoot->keys.push_back(separator);
        newRoot->children.push_back(left);
        newRoot->children.push_back(right);
        left->parent = newRoot;
        right->parent = newRoot;
        root = newRoot;

        return;
    }

    BPlusNode *parent = left->parent;
    std::size_t index = 0;

    while (index < parent->children.size() && parent->children[index] != left) {
        ++index;
    }

    // insert separator key and right child after left
    parent->keys.insert(parent->keys.begin() + index, separator);
    parent->children.insert(parent->children.begin() + index + 1, right);
    right->parent = parent;

    // exceeds; split internal node
    if (parent->keys.size() > maxKeysPerNode) {
        this->splitInternal(parent);
    }
}

void BPlusTree::splitInternal(BPlusNode *node) {
    const std::size_t middle = node->keys.size() / 2;
    const std::string separator = node->keys[middle]; // the key that gets pushed up
    BPlusNode *right = new BPlusNode(false);

    right->parent = node->parent;

    // move half of keys/children to the new node
    for (std::size_t i = middle + 1; i < node->keys.size(); ++i) {
        right->keys.push_back(node->keys[i]);
    }

    for (std::size_t i = middle + 1; i < node->children.size(); ++i) {
        right->children.push_back(node->children[i]);
        node->children[i]->parent = right;
    }

    node->keys.resize(middle);
    node->children.resize(middle + 1);
    // push the middle into the parent
    this->insertIntoParent(node, separator, right);
}

bool BPlusTree::removeFile(const std::string &path) {
    if (path.empty() || !validPath(path)) {
        throw std::invalid_argument("Removal requires a non-empty relative path");
    }

    bool isDirectory = false;

    if (!this->contains(path, &isDirectory) || isDirectory) {
        return false;
    }

    return this->remove(path);
}

bool BPlusTree::removeDirectory(const std::string &path) {
    if (path.empty() || !validPath(path)) {
        throw std::invalid_argument("Invalid path");
    }

    bool isDirectory = false;

    if (!this->contains(path, &isDirectory) || !isDirectory) {
        return false;
    }

    const std::string prefix = path + "/"; // to check for descendants
    std::vector<std::string> descendants;
    BPlusNode *leaf = this->findLeaf(prefix);
    auto pos = this->findKey(leaf, prefix);
    std::size_t index = pos - leaf->keys.begin();

    while (leaf != nullptr) {
        for (int _ = 0; index < leaf->keys.size(); ++index) {
            const std::string &key = leaf->keys[index];

            // no longer descendant; break
            if (key.substr(0, prefix.size()) != prefix) {
                leaf = nullptr;
                break;
            }
            descendants.push_back(key);
        }
        if (leaf != nullptr) {
            leaf = leaf->nextLeaf;
            index = 0;
        }
    }

    // Remove in reverse order
    for (auto entry = descendants.rbegin(); entry != descendants.rend(); ++entry) {
        this->remove(*entry);
    }

    return this->remove(path);
}

bool BPlusTree::remove(const std::string &path) {
    BPlusNode *leaf = this->findLeaf(path);
    auto position = this->findKey(leaf, path);

    if (position == leaf->keys.end() || *position != path) {
        return false;
    }

    const std::size_t index = position - leaf->keys.begin();

    leaf->keys.erase(position);
    // need to convert index to size_t and then add to the iterator
    // Why can't things be easier
    leaf->directories.erase(leaf->directories.begin() + index);
    --this->entryCount;
    this->rebalance(leaf);

    return true;
}

void BPlusTree::updateSeparators(BPlusNode *node) {
    while (node != nullptr) {
        if (!node->leaf) {
            node->keys.clear();
            // get leftmost key of each child and add it to the node keys
            for (std::size_t i = 1; i < node->children.size(); ++i) {
                BPlusNode *leaf = node->children[i];

                while (!leaf->leaf) {
                    leaf = leaf->children[0];
                }

                node->keys.push_back(leaf->keys[0]);
            }
        }
        // recursive
        node = node->parent;
    }
}

void BPlusTree::rebalance(BPlusNode *node) {
    if (node == this->root) {
        // one node; make its single child the new root if it exists
        if (!node->leaf && node->children.size() == 1) {
            this->root = node->children[0];
            this->root->parent = nullptr;
            node->children.clear();
            delete node;
        }

        this->updateSeparators(this->root);
        return;
    }

    const std::size_t minimum = node->leaf ? (this->maxKeysPerNode + 1) / 2 : this->maxKeysPerNode / 2;
    BPlusNode *parent = node->parent;

    // node has enough keys; done
    if (node->keys.size() >= minimum) {
        this->updateSeparators(parent);
        return;
    }

    // get node index to find left/right nodes
    std::size_t index = 0;

    while (parent->children[index] != node) {
        ++index;
    }
    BPlusNode *left = index == 0 ? nullptr : parent->children[index - 1];
    BPlusNode *right = index + 1 == parent->children.size() ? nullptr : parent->children[index + 1];

    // borrow from left/right nodes before merging

    if (left != nullptr && left->keys.size() > minimum) {
        this->borrowFromLeft(node, left);
        return;
    }

    if (right != nullptr && right->keys.size() > minimum) {
        this->borrowFromRight(node, right);
        return;
    }

    // merge with left/right
    if (left != nullptr) {
        this->mergeNodes(left, node);
    } else {
        this->mergeNodes(node, right);
    }

    // recursive
    this->rebalance(parent);
}

void BPlusTree::borrowFromLeft(BPlusNode *node, BPlusNode *left) {
    // take from left
    if (node->leaf) {
        node->keys.insert(node->keys.begin(), left->keys.back());
        node->directories.insert(node->directories.begin(), left->directories.back());
        left->keys.pop_back();
        left->directories.pop_back();
    }
    // take from left child if internal node
    else {
        BPlusNode *child = left->children.back();

        left->children.pop_back();
        node->children.insert(node->children.begin(), child);
        child->parent = node;
        this->updateSeparators(left);
    }
    this->updateSeparators(node);
}

void BPlusTree::borrowFromRight(BPlusNode *node, BPlusNode *right) {
    // take from right
    if (node->leaf) {
        node->keys.push_back(right->keys[0]);
        node->directories.push_back(right->directories[0]);

        right->keys.erase(right->keys.begin());
        right->directories.erase(right->directories.begin());
    }
    // take from right child if internal node
    else {
        BPlusNode *child = right->children[0];

        right->children.erase(right->children.begin());
        node->children.push_back(child);
        child->parent = node;
        this->updateSeparators(right);
    }
    this->updateSeparators(node);
}

void BPlusTree::mergeNodes(BPlusNode *left, BPlusNode *right) {
    BPlusNode *parent = left->parent;

    // merge keys of leaf
    if (left->leaf) {
        for (std::size_t i = 0; i < right->keys.size(); ++i) {
            left->keys.push_back(right->keys[i]);
            left->directories.push_back(right->directories[i]);
        }

        left->nextLeaf = right->nextLeaf;
    }
    // merge children of internal node
    else {
        for (BPlusNode *child : right->children) {
            left->children.push_back(child);
            child->parent = left;
        }

        right->children.clear();
    }

    // remove child index and udpdate
    auto position = std::find(parent->children.begin(), parent->children.end(), right);

    parent->children.erase(position);
    delete right;
    this->updateSeparators(left);
}

bool BPlusTree::contains(const std::string &path, bool *isDirectory, std::size_t *nodesAccessed) const {
    BPlusNode *node = root;
    std::size_t visits = 1;

    while (!node->leaf) {
        std::size_t idx = 0;

        while (idx < node->keys.size() && path >= node->keys[idx]) {
            ++idx;
        }

        node = node->children[idx];
        ++visits;
    }

    // track node accesses for web app
    if (nodesAccessed != nullptr) {
        *nodesAccessed = visits;
    }

    auto position = this->findKey(node, path);

    if (position == node->keys.end() || *position != path) {
        return false;
    }

    if (isDirectory != nullptr) {
        const std::size_t index = position - node->keys.begin();
        *isDirectory = node->directories[index];
    }

    return true;
}

FileNode *BPlusTree::toFileNodes() const {
    FileNode *fileRoot = new FileNode(this->rootName, nullptr, true);
    std::vector<std::pair<std::string, FileNode *>> created; // <path, node>

    created.push_back(std::make_pair("", fileRoot));

    // Get to leftmost leaf node
    BPlusNode *leaf = root;

    while (!leaf->leaf) {
        leaf = leaf->children[0];
    }

    while (leaf != nullptr) {
        for (std::size_t i = 0; i < leaf->keys.size(); ++i) {
            const std::string &path = leaf->keys[i];
            // funny how they have niche methods like this but still let strings be passable into functions that take bool as a parameter
            const std::size_t separator = path.find_last_of('/');
            const std::string parentPath = separator == std::string::npos ? "" : path.substr(0, separator);
            const std::string name = separator == std::string::npos ? path : path.substr(separator + 1);

            FileNode *parent = nullptr;

            for (const auto &item : created) {
                if (item.first == parentPath) {
                    parent = item.second;
                    break;
                }
            }
            if (parent == nullptr) {
                delete fileRoot;
                throw std::logic_error("No parent");
            }

            // make node from the leaf data
            FileNode *child = new FileNode(name, nullptr, leaf->directories[i]);

            if (!parent->addChild(child)) {
                delete child;
                delete fileRoot;
                throw std::logic_error("Duplicate");
            }
            created.push_back(std::make_pair(path, child));
        }

        leaf = leaf->nextLeaf;
    }

    return fileRoot;
}
