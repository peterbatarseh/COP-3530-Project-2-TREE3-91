#include <catch2/catch_test_macros.hpp>
#include "../src/FileNode.h"

TEST_CASE("Node class initializes correctly", "[node]") {
    FileNode root("root", nullptr, true);
    CHECK(root.getName() == "root");
    CHECK(root.isFolder() == true);
    CHECK(root.getParent() == nullptr);
}

TEST_CASE("Add child and height update", "[node]") {
    FileNode *root = new FileNode("root", nullptr, true);
    FileNode *child1 = new FileNode("child1", nullptr, false);
    
    CHECK(root->addChild(child1) == true);
    CHECK(child1->getParent() == root);
    CHECK(root->isFolder() == true);
    CHECK(child1->isFolder() == false);
    
    // Check children/height
    CHECK(child1->getName() == "child1");
    CHECK(child1->getHeight() == 1);
    CHECK(root->getHeight() == 2);
    
    FileNode *child2 = new FileNode("child2", nullptr, true);
    FileNode *grandchild = new FileNode("grandchild", nullptr, false);
    child2->addChild(grandchild);
    
    CHECK(child2->getHeight() == 2);
    CHECK(grandchild->getHeight() == 1);
    
    root->addChild(child2);
    // root -> child2 -> grandchild
    CHECK(root->getHeight() == 3);

    // Should delete all nodes
    delete root;
}