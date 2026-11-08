#pragma once

class FileNode;
class BPlusTree; 
class NaryTree;  

// Initializes server
// root is the generated file system, the trees already have it loaded
int runWebServer(const FileNode *root, BPlusTree &bplus, NaryTree &nary);