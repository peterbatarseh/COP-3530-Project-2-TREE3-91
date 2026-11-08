#pragma once

#include <cstddef>
#include <string>
#include <vector>

class FileNode;
class BPlusTree;
class NaryTree;

// copy the generated tree into each implementation 
void loadIntoBPlus(const FileNode *root, BPlusTree &tree);
void loadIntoNary(const FileNode *root, NaryTree &tree);


std::vector<std::string> allFilePaths(const FileNode *root);

// nodes accessed to find something
std::size_t bplusLookup(BPlusTree &tree, const std::string &path);
int splayLookup(NaryTree &tree, const std::string &path);  // file path
int splayFolderLookup(NaryTree &tree, const std::string &path);