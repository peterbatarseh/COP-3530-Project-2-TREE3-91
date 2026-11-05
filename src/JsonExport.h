#pragma once

#include <string>

#include "FileNode.h"

/*
Writes a FileNode tree to JSON so the web frontend (web/app.js) can display it.

Format the website expects:
{
  "implementation": "B+ Tree",
  "root": {
    "name": "root",
    "folder": true,
    "accesses": 1,          // nodes the tree touched to reach this node
    "children": [ ... ]     // same shape, only present on folders
  }
}

"accesses" is the number the website adds to TOTAL NODES ACCESSED when you
open a folder. Right now it's a placeholder (a plain linear search from the
root). Once the B+ tree and splay tree are written, each one should report
its own real count here.
*/
std::string toJson(const FileNode *root, const std::string &implementation);

// Writes toJson(...) to a file. Returns false if the file couldn't be opened.
bool writeJson(const FileNode *root, const std::string &implementation, const std::string &path);
