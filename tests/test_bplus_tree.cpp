#include <catch2/catch_test_macros.hpp>

#include "../src/BPlusTree.h"
#include "../src/FileNode.h"

#include <stdexcept>

TEST_CASE("B+ tree insertion and indexing") {
    BPlusTree tree("root", 3);

    std::string dir1 = "Documents";
    std::string dir2 = "School";

    REQUIRE(tree.insertDirectory("", dir1));
    REQUIRE(tree.insertFile(dir1, "todo.txt"));
    REQUIRE(tree.insertDirectory(dir1, dir2));
    REQUIRE(tree.insertFile(dir1 + "/" + dir2, "report.pdf"));
    REQUIRE_FALSE(tree.insertFile(dir1, "todo.txt"));

    bool isDirectory = false;
    CHECK(tree.contains(dir1, &isDirectory));
    CHECK(isDirectory);
    CHECK(tree.contains(dir1 + "/todo.txt", &isDirectory));
    CHECK_FALSE(isDirectory);
    CHECK(tree.size() == 4);

    // easier to check with FileNode representation
    FileNode *root = tree.toFileNodes();
    REQUIRE(root != nullptr);
    REQUIRE(root->getChildren().size() == 1);
    FileNode *documents = root->getChildren()[0];
    CHECK(documents->getName() == dir1);
    CHECK(documents->isFolder());
    CHECK(documents->getParent() == root);
    REQUIRE(documents->getChildren().size() == 2);

    FileNode *school = documents->getChildren()[0];
    FileNode *todo = documents->getChildren()[1];
    CHECK(school->getName() == dir2);
    CHECK(school->getParent() == documents);
    CHECK(todo->getName() == "todo.txt");
    REQUIRE(school->getChildren().size() == 1);
    CHECK(school->getChildren()[0]->getName() == "report.pdf");
    CHECK(school->getChildren()[0]->getParent() == school);

    delete root;
}

TEST_CASE("B+ tree splits and sorts") {
    BPlusTree tree("root", 2);

    std::vector<std::string> files;
    files.reserve(10);

    for (int i = 0; i < 10; ++i) {
        std::string name = std::to_string(i) + ".txt";
        REQUIRE(tree.insertFile("", name));
        files.push_back(name);
    }

    for (const std::string &name : files) {
        REQUIRE_FALSE(tree.insertFile("", name));
    }

    FileNode *root = tree.toFileNodes();
    REQUIRE(root->getChildren().size() == 10);
    for (std::size_t i = 1; i < root->getChildren().size(); ++i) {
        // check if sorted
        CHECK(root->getChildren()[i - 1]->getName() < root->getChildren()[i]->getName());
    }
    delete root;

    // to find a file that's not the root, accesses should be > 1
    std::size_t accesses = 0;
    CHECK(tree.contains(files[4], nullptr, &accesses));
    CHECK(accesses >= 1);
    CHECK_FALSE(tree.contains("missing.txt", nullptr, &accesses));
    CHECK(accesses >= 1);
}

TEST_CASE("B+ tree rejects invalid insertions") {
    BPlusTree tree;
    REQUIRE(tree.insertFile("", "todo.txt"));

    // check_false doesn't work
    CHECK_THROWS_AS(tree.insertFile("Missing", "notes.txt"), std::invalid_argument);
    CHECK_THROWS_AS(tree.insertDirectory("todo.txt", "child"), std::invalid_argument);
}

TEST_CASE("B+ tree file removal") {
    BPlusTree tree;

    std::string dir = "Documents";
    std::string file1 = "todo.txt";
    std::string file2 = "notes.txt";

    REQUIRE(tree.insertDirectory("", dir));
    REQUIRE(tree.insertFile(dir, file2));
    REQUIRE(tree.insertFile("", file1));

    // remove nonexistent files
    CHECK_FALSE(tree.removeFile("nonexistent_file.txt"));
    CHECK_FALSE(tree.removeFile(dir));
    CHECK_FALSE(tree.removeDirectory(file1));

    // remove existing files
    REQUIRE(tree.removeFile(dir + "/" + file2));
    CHECK_FALSE(tree.contains(dir + "/" + file2));
    CHECK(tree.contains(dir));
    CHECK(tree.contains(file1));
    CHECK(tree.size() == 2);
    CHECK_FALSE(tree.removeFile(dir + "/" + file2));
}

TEST_CASE("B+ tree directory removal deletes all descendants") {
    BPlusTree tree("root", 2);

    std::string dir = "docs";
    std::string subdir = "school";
    std::string file1 = "report.txt";
    std::string file2 = "notes.txt";

    REQUIRE(tree.insertDirectory("", dir));
    REQUIRE(tree.insertDirectory(dir, subdir));
    REQUIRE(tree.insertFile(dir + "/" + subdir, file1));
    REQUIRE(tree.insertFile(dir, file2));

    std::string otherDir = "docs-other";
    std::string keepFile = "keep.txt";
    std::string docsFile = "docs.txt";
    REQUIRE(tree.insertDirectory("", otherDir));
    REQUIRE(tree.insertFile(otherDir, keepFile));
    REQUIRE(tree.insertFile("", docsFile));

    REQUIRE(tree.removeDirectory(dir));
    CHECK(tree.size() == 3);
    CHECK_FALSE(tree.contains(dir));
    CHECK_FALSE(tree.contains(dir + "/school"));
    CHECK_FALSE(tree.contains(dir + "/school/report.txt"));
    CHECK_FALSE(tree.contains(dir + "/notes.txt"));
    CHECK(tree.contains(otherDir + "/" + keepFile));
    CHECK(tree.contains(docsFile));

    FileNode *root = tree.toFileNodes();
    CHECK(root->getChildren().size() == 2);
    delete root;

    // empty directory
    REQUIRE(tree.insertDirectory("", "empty"));
    CHECK(tree.removeDirectory("empty"));
    CHECK_FALSE(tree.removeDirectory("empty"));
    // invalid paths
    CHECK_THROWS_AS(tree.removeDirectory(""), std::invalid_argument);
    CHECK_THROWS_AS(tree.removeFile("/" + docsFile), std::invalid_argument);
    CHECK_THROWS_AS(tree.removeDirectory("docs//school"), std::invalid_argument);
}

TEST_CASE("B+ tree deletion/rebalancing") {
    for (const std::string &order : {"first", "last", "middle"}) {
        for (std::size_t capacity : {2, 3, 4, 5, 6}) {
            BPlusTree tree("root", capacity);
            std::vector<std::string> remaining;

            // generate files
            for (int i = 0; i < 30; ++i) {
                std::string name = "file" + std::to_string(1000 + i);

                REQUIRE(tree.insertFile("", name));
                remaining.push_back(name);
            }

            while (!remaining.empty()) {
                std::size_t index = 0;

                if (order == "last") {
                    index = remaining.size() - 1;
                } else if (order == "middle") {
                    index = remaining.size() / 2;
                }

                std::string removed = remaining[index];

                REQUIRE(tree.removeFile(removed));
                remaining.erase(remaining.begin() + index);
                CHECK_FALSE(tree.contains(removed));
                CHECK(tree.size() == remaining.size());

                FileNode *root = tree.toFileNodes();

                REQUIRE(root->getChildren().size() == remaining.size());
                for (std::size_t i = 0; i < remaining.size(); ++i) {
                    CHECK(tree.contains(remaining[i]));
                    CHECK(root->getChildren()[i]->getName() == remaining[i]);
                }
                delete root;
            }

            std::size_t visits = 0;

            // should check root and find it's invalid (1 visit)
            CHECK_FALSE(tree.contains("missing", nullptr, &visits));
            CHECK(visits == 1);
            REQUIRE(tree.insertFile("", "new.txt"));
            CHECK(tree.contains("new.txt"));
            CHECK(tree.removeFile("new.txt"));
            CHECK(tree.size() == 0);
        }
    }
}
