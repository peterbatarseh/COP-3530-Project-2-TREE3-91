#include "SplayTree.h"

SplayNode::SplayNode(FileNode *file) {
    this->file = file;
    this->left = nullptr;
    this->right = nullptr;
    this->parent = nullptr;
}

SplayTree::SplayTree() {
    this->root = nullptr;
    this->count = 0;
    this->nodeAccesses = 0;
}

SplayTree::~SplayTree() {
    destroy(this->root);
}

void SplayTree::destroy(SplayNode *node) {
    if (node == nullptr) {
        return;
    }
    destroy(node->left);
    destroy(node->right);
    delete node->file;
    delete node;
}

void SplayTree::rotateLeft(SplayNode *x) {
    SplayNode *y = x->right;

    x->right = y->left;
    if (y->left != nullptr) {
        y->left->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == nullptr) {
        this->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}

void SplayTree::rotateRight(SplayNode *x) {
    SplayNode *y = x->left;

    x->left = y->right;
    if (y->right != nullptr) {
        y->right->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == nullptr) {
        this->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }

    y->right = x;
    x->parent = y;
}

void SplayTree::splay(SplayNode *x) {
    while (x->parent != nullptr) {
        SplayNode *p = x->parent;
        SplayNode *g = p->parent;

        if (g == nullptr) {
            if (x == p->left) {
                rotateRight(p);
            } else {
                rotateLeft(p);
            }
        } else if (x == p->left && p == g->left) {
            rotateRight(g);
            rotateRight(p);
        } else if (x == p->right && p == g->right) {
            rotateLeft(g);
            rotateLeft(p);
        } else if (x == p->right && p == g->left) {
            rotateLeft(p);
            rotateRight(g);
        } else {
            rotateRight(p);
            rotateLeft(g);
        }
    }
}

SplayNode *SplayTree::findNode(std::string name) {
    SplayNode *current = this->root;
    SplayNode *last = nullptr;

    while (current != nullptr) {
        this->nodeAccesses++;
        last = current;

        if (name == current->file->getName()) {
            return current;
        }

        if (name < current->file->getName()) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    return last;
}

FileNode *SplayTree::find(std::string name) {
    SplayNode *node = findNode(name);
    if (node == nullptr) {
        return nullptr;
    }

    splay(node);

    if (node->file->getName() == name) {
        return node->file;
    }
    return nullptr;
}

bool SplayTree::contains(std::string name) {
    return find(name) != nullptr;
}

bool SplayTree::insert(FileNode *file) {
    std::string name = file->getName();
    SplayNode *last = findNode(name);

    if (last != nullptr && last->file->getName() == name) {
        splay(last);
        return false;
    }

    SplayNode *node = new SplayNode(file);
    node->parent = last;
    if (last == nullptr) {
        this->root = node;
    } else if (name < last->file->getName()) {
        last->left = node;
    } else {
        last->right = node;
    }

    splay(node);
    this->count++;
    return true;
}

bool SplayTree::remove(std::string name) {
    // contains() splays the target to the root
    if (!contains(name)) {
        return false;
    }

    SplayNode *target = this->root;
    SplayNode *leftSubtree = target->left;
    SplayNode *rightSubtree = target->right;
    delete target->file;
    delete target;
    this->count--;

    if (leftSubtree == nullptr) {
        this->root = rightSubtree;
        if (rightSubtree != nullptr) {
            rightSubtree->parent = nullptr;
        }
        return true;
    }

    leftSubtree->parent = nullptr;
    this->root = leftSubtree;

    SplayNode *biggest = leftSubtree;
    while (biggest->right != nullptr) {
        this->nodeAccesses++;
        biggest = biggest->right;
    }
    splay(biggest);

    this->root->right = rightSubtree;
    if (rightSubtree != nullptr) {
        rightSubtree->parent = this->root;
    }
    return true;
}

int SplayTree::size() {
    return this->count;
}

bool SplayTree::empty() {
    return this->count == 0;
}

std::string SplayTree::rootName() {
    if (this->root == nullptr) {
        return "";
    }
    return this->root->file->getName();
}

void SplayTree::inorder(SplayNode *node, std::vector<std::string> &names) {
    if (node == nullptr) {
        return;
    }
    inorder(node->left, names);
    names.push_back(node->file->getName());
    inorder(node->right, names);
}

std::vector<std::string> SplayTree::inorderNames() {
    std::vector<std::string> names;
    inorder(this->root, names);
    return names;
}

int SplayTree::getNodeAccesses() {
    return this->nodeAccesses;
}

void SplayTree::resetNodeAccesses() {
    this->nodeAccesses = 0;
}
