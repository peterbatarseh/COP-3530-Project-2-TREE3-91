/*
Generates the dataset, loads it into both trees and starts the web server
*/

#include <chrono>
#include <iostream>

#include "BPlusTree.h"
#include "DataGenerator.h"
#include "FileNode.h"
#include "NaryTree.h"
#include "TreeLoader.h"
#include "WebServer.h"

int main() {
    std::cout << "Generating dataset..." << std::endl;
    FileNode *root = generateTree();

    std::cout << "Loading into B+ tree and splay tree..." << std::endl;
    BPlusTree bplus("root");
    NaryTree nary("root");

    // time how long each tree takes to build
    auto start = std::chrono::steady_clock::now();
    loadIntoBPlus(root, bplus);
    long long bplusMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    start = std::chrono::steady_clock::now();
    loadIntoNary(root, nary);
    long long splayMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    int result = runWebServer(root, bplus, nary, bplusMs, splayMs);

    // clean up
    delete root;
    return result;
}