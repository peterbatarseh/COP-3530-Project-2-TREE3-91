#pragma once

#include "FileNode.h"

// builds a fake file system in memory (nothing is written to disk)
struct GeneratorConfig {
    int minFiles = 110000;
    int minTopFolders = 100;       // folders inside root
    int minSubdirectories = 1000;  // folders inside other folders
    int maxDepth = 8;
    unsigned int seed = 91;        // same seed = same tree every time
};

// caller has to delete the root when done
FileNode *generateTree(const GeneratorConfig &config = GeneratorConfig());

struct TreeStats {
    int files = 0;
    int topFolders = 0;
    int subdirectories = 0;
    int maxDepth = 0;
    int largestFolder = 0;
};
TreeStats getStats(const FileNode *root);