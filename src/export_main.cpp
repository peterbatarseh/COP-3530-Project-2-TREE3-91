//Generates the dataset, loads it into both trees and starts the web server

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

    //load the same data into both trees
    std::cout << "Loading into B+ tree and splay tree..." << std::endl;
    BPlusTree bplus("root");
    NaryTree nary("root");
    loadIntoBPlus(root, bplus);
    loadIntoNary(root, nary);

    int result = runWebServer(root, bplus, nary);  //pass the trees to the server


    delete root;
    return result;
}