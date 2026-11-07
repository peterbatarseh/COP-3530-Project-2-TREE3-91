#include <chrono>
#include <cstdlib>
#include <iostream>

#include "DataGenerator.h"

// usage: Main [files] [top folders] [subdirectories] [seed]
int main(int argc, char *argv[]) {
    GeneratorConfig config;
    if (argc > 1) config.minFiles = std::atoi(argv[1]);
    if (argc > 2) config.minTopFolders = std::atoi(argv[2]);
    if (argc > 3) config.minSubdirectories = std::atoi(argv[3]);
    if (argc > 4) config.seed = (unsigned int)std::strtoul(argv[4], nullptr, 10);

    auto start = std::chrono::steady_clock::now();
    FileNode *root = generateTree(config);
    auto end = std::chrono::steady_clock::now();

    TreeStats stats = getStats(root);
    std::cout << "Files:          " << stats.files << "\n"
              << "Top folders:    " << stats.topFolders << "\n"
              << "Subdirectories: " << stats.subdirectories << "\n"
              << "Deepest level:  " << stats.maxDepth << "\n"
              << "Largest folder: " << stats.largestFolder << " items\n"
              << "Built in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms (seed " << config.seed << ")\n";

    delete root;
    return 0;
}