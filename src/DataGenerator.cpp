#include "DataGenerator.h"

#include <algorithm>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace {

const char *EXTENSIONS[] = {".txt", ".pdf", ".png", ".jpg", ".cpp", ".h", ".docx", ".mp3", ".csv", ".zip"};
const int NUM_EXTENSIONS = sizeof(EXTENSIONS) / sizeof(EXTENSIONS[0]);

// e.g. folder_000042 or file_000103.pdf
std::string makeName(const char *prefix, int id, const char *extension = "") {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s_%06d%s", prefix, id, extension);
    return buf;
}

struct FolderEntry {
    FileNode *node;
    int depth;
};

template <typename T>
const T &pickRandom(const std::vector<T> &items, std::mt19937 &rng) {
    std::uniform_int_distribution<size_t> dist(0, items.size() - 1);
    return items[dist(rng)];
}

void collectStats(const FileNode *node, int depth, TreeStats &stats) {
    stats.maxDepth = std::max(stats.maxDepth, depth);
    if (!node->isFolder()) {
        stats.files++;
        return;
    }
    if (depth == 1) stats.topFolders++;
    if (depth >= 2) stats.subdirectories++;

    const auto &children = node->getChildren();
    stats.largestFolder = std::max(stats.largestFolder, (int)children.size());
    for (const FileNode *child : children) {
        collectStats(child, depth + 1, stats);
    }
}

}  // namespace

FileNode *generateTree(const GeneratorConfig &config) {
    std::mt19937 rng(config.seed);
    std::uniform_int_distribution<int> pickExtension(0, NUM_EXTENSIONS - 1);
    int nextId = 1;

    FileNode *root = new FileNode("root", nullptr, true);
    std::vector<FolderEntry> allFolders = {{root, 0}};
    std::vector<FolderEntry> canHoldFolders;  // folders that aren't too deep for a subfolder

    // top level folders
    for (int i = 0; i < config.minTopFolders; i++) {
        FileNode *folder = new FileNode(makeName("folder", nextId++), nullptr, true);
        root->addChild(folder);
        allFolders.push_back({folder, 1});
        if (1 < config.maxDepth - 1) canHoldFolders.push_back({folder, 1});
    }

    // subfolders go in a random existing folder
    for (int i = 0; i < config.minSubdirectories && !canHoldFolders.empty(); i++) {
        FolderEntry parent = pickRandom(canHoldFolders, rng);
        FileNode *folder = new FileNode(makeName("folder", nextId++), nullptr, true);
        parent.node->addChild(folder);

        FolderEntry entry = {folder, parent.depth + 1};
        allFolders.push_back(entry);
        if (entry.depth < config.maxDepth - 1) canHoldFolders.push_back(entry);
    }

    // files go in any random folder
    for (int i = 0; i < config.minFiles; i++) {
        FolderEntry parent = pickRandom(allFolders, rng);
        std::string name = makeName("file", nextId++, EXTENSIONS[pickExtension(rng)]);
        parent.node->addChild(new FileNode(name, nullptr, false));
    }

    return root;
}

TreeStats getStats(const FileNode *root) {
    TreeStats stats;
    collectStats(root, 0, stats);
    return stats;
}