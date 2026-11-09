#include "DataGenerator.h"

#include <algorithm>
#include <random>
#include <string>
#include <vector>

std::vector<std::string> EXTENSIONS = {".txt", ".pdf", ".png", ".jpg", ".cpp", ".h", ".docx", ".mp3", ".csv", ".zip"};

// makes names like folder_000042 or file_000103.pdf
std::string makeName(std::string prefix, int id, std::string extension) {
    std::string number = std::to_string(id);
    while (number.size() < 6) {
        number = "0" + number;
    }
    return prefix + "_" + number + extension;
}

// a folder and how deep it is in the tree
struct Folder {
    FileNode *node;
    int depth;
};

// random index from 0 to size - 1
size_t randomIndex(size_t size, std::mt19937 &rng) {
    std::uniform_int_distribution<size_t> dist(0, size - 1);
    return dist(rng);
}

FileNode *generateTree(const GeneratorConfig &config) {
    std::mt19937 rng(config.seed);  // same seed = same tree every time
    std::uniform_int_distribution<int> pickExtension(0, EXTENSIONS.size() - 1);
    int nextId = 1;

    FileNode *root = new FileNode("root", nullptr, true);
    std::vector<Folder> allFolders;     // every folder (files can go in any of these)
    std::vector<Folder> openFolders;    // folders that can still get a subfolder
    allFolders.push_back({root, 0});

    // make the top level folders inside root
    for (int i = 0; i < config.minTopFolders; i++) {
        FileNode *folder = new FileNode(makeName("folder", nextId, ""), nullptr, true);
        nextId++;
        root->addChild(folder);
        allFolders.push_back({folder, 1});
        if (1 < config.maxDepth - 1) {
            openFolders.push_back({folder, 1});
        }
    }

    // put each subfolder inside a random open folder
    for (int i = 0; i < config.minSubdirectories && !openFolders.empty(); i++) {
        Folder parent = openFolders[randomIndex(openFolders.size(), rng)];
        FileNode *folder = new FileNode(makeName("folder", nextId, ""), nullptr, true);
        nextId++;
        parent.node->addChild(folder);

        int depth = parent.depth + 1;
        allFolders.push_back({folder, depth});
        if (depth < config.maxDepth - 1) {
            openFolders.push_back({folder, depth});
        }
    }

    // put each file inside any random folder
    for (int i = 0; i < config.minFiles; i++) {
        Folder parent = allFolders[randomIndex(allFolders.size(), rng)];
        std::string name = makeName("file", nextId, EXTENSIONS[pickExtension(rng)]);
        nextId++;
        parent.node->addChild(new FileNode(name, nullptr, false));
    }

    return root;
}

// goes through the tree and counts everything
void collectStats(const FileNode *node, int depth, TreeStats &stats) {
    stats.maxDepth = std::max(stats.maxDepth, depth);

    if (!node->isFolder()) {
        stats.files++;
        return;
    }

    if (depth == 1) stats.topFolders++;
    if (depth >= 2) stats.subdirectories++;

    stats.largestFolder = std::max(stats.largestFolder, (int)node->getChildren().size());
    for (const FileNode *child : node->getChildren()) {
        collectStats(child, depth + 1, stats);
    }
}

TreeStats getStats(const FileNode *root) {
    TreeStats stats;
    collectStats(root, 0, stats);
    return stats;
}