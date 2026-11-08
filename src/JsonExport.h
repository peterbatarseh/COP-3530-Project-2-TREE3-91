#pragma once

#include <cstddef>
#include <functional>
#include <string>

#include "FileNode.h"

// turns a FileNode tree into JSON for the website
// each node has: name, folder, accesses, children

// gets the access count for a folder from its path ("" = root)
using FolderAccessFn = std::function<std::size_t(const std::string &path)>;

std::string toJson(const FileNode *root, const std::string &implementation,
  const FolderAccessFn &folderAccesses = nullptr);

// same as toJson but saves it to a file
bool writeJson(const FileNode *root, const std::string &implementation, const std::string &path);