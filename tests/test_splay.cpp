#include <catch2/catch_test_macros.hpp>
#include "../src/SplayTree.h"
#include "../src/NaryTree.h"

FileNode* file(std::string name) {
    return new FileNode(name, nullptr, false);
}

TEST_CASE("Splay tree insert and search", "[splay]") {
    SplayTree t;
    CHECK(t.empty());

    CHECK(t.insert(file("b.txt")));
    CHECK(t.insert(file("a.txt")));
    CHECK(t.insert(file("c.txt")));
    CHECK(t.size() == 3);

    FileNode* dup = file("a.txt");
    CHECK_FALSE(t.insert(dup));
    delete dup;
    CHECK(t.size() == 3);

    CHECK(t.contains("a.txt"));
    CHECK_FALSE(t.contains("z.txt"));

    FileNode* found = t.find("b.txt");
    REQUIRE(found != nullptr);
    CHECK(found->getName() == "b.txt");
    CHECK_FALSE(found->isFolder());
    CHECK(t.find("z.txt") == nullptr);
}

TEST_CASE("Splay tree moves accessed node to root", "[splay]") {
    SplayTree t;
    t.insert(file("a"));
    t.insert(file("b"));
    t.insert(file("c"));
    t.insert(file("d"));
    CHECK(t.rootName() == "d");

    t.contains("a");
    CHECK(t.rootName() == "a");

    t.resetNodeAccesses();
    t.contains("a");
    CHECK(t.getNodeAccesses() == 1);
}

TEST_CASE("Splay tree stays sorted and handles removal", "[splay]") {
    SplayTree t;
    std::vector<std::string> names = {"m", "c", "x", "a", "e", "q", "z"};
    for (int i = 0; i < names.size(); i++) {
        t.insert(file(names[i]));
    }
    CHECK(t.inorderNames() == std::vector<std::string>{"a", "c", "e", "m", "q", "x", "z"});

    CHECK(t.remove("m"));
    CHECK_FALSE(t.remove("m"));
    CHECK(t.size() == 6);
    CHECK_FALSE(t.contains("m"));
    CHECK(t.inorderNames() == std::vector<std::string>{"a", "c", "e", "q", "x", "z"});

    CHECK(t.remove("a"));
    CHECK(t.remove("z"));
    CHECK(t.inorderNames() == std::vector<std::string>{"c", "e", "q", "x"});
}

TEST_CASE("N-ary tree directories and navigation", "[nary]") {
    NaryTree tree("root");
    CHECK(tree.getCurrentPath() == "/root");
    CHECK_FALSE(tree.goUp());

    CHECK(tree.makeDir("docs"));
    CHECK(tree.makeDir("pics"));
    CHECK_FALSE(tree.makeDir("docs"));
    CHECK(tree.getSubdirCount() == 2);

    CHECK(tree.changeDir("docs"));
    CHECK(tree.getCurrentName() == "docs");
    CHECK(tree.getCurrentPath() == "/root/docs");
    CHECK_FALSE(tree.changeDir("nope"));

    CHECK(tree.goUp());
    CHECK(tree.getCurrentPath() == "/root");
}

TEST_CASE("N-ary tree files live in each directory's splay tree", "[nary]") {
    NaryTree tree;
    tree.makeDir("docs");
    tree.addFile("readme.md");
    CHECK(tree.getFileCount() == 1);

    tree.changeDir("docs");
    CHECK(tree.getFileCount() == 0);
    CHECK(tree.addFile("essay.docx"));
    CHECK(tree.addFile("notes.txt"));
    CHECK_FALSE(tree.addFile("notes.txt"));
    CHECK(tree.getFileCount() == 2);
    CHECK(tree.findFile("essay.docx"));
    CHECK_FALSE(tree.findFile("readme.md"));

    CHECK(tree.removeFile("notes.txt"));
    CHECK(tree.listFiles() == std::vector<std::string>{"essay.docx"});

    tree.goToRoot();
    CHECK(tree.findFile("readme.md"));
}

TEST_CASE("N-ary tree counts node accesses", "[nary]") {
    NaryTree tree;
    tree.makeDir("a");
    tree.makeDir("b");
    tree.resetNodeAccesses();
    CHECK(tree.getNodeAccesses() == 0);

    tree.changeDir("b");
    CHECK(tree.getNodeAccesses() == 2);

    tree.addFile("f1");
    tree.resetNodeAccesses();
    tree.findFile("f1");
    CHECK(tree.getNodeAccesses() == 1);
}

TEST_CASE("N-ary tree is built from FileNodes", "[nary]") {
    NaryTree tree("root");
    tree.makeDir("docs");
    tree.changeDir("docs");
    tree.addFile("essay.docx");

    DirNode *docs = tree.getCurrentDir();
    CHECK(docs->isFolder());
    REQUIRE(docs->getParent() != nullptr);
    CHECK(docs->getParent()->getName() == "root");

    FileNode *essay = tree.getFile("essay.docx");
    REQUIRE(essay != nullptr);
    CHECK_FALSE(essay->isFolder());
    CHECK(essay->getParent() == docs);
}
