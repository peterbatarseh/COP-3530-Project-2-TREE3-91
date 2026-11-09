// Main: builds the test data, loads it into both trees, and compares them
//
// AI use: Claude (Anthropic), model Claude Opus 5.5
// I looked up how to benchmark in C++ and used AI to learn how std::chrono
// (steady_clock, duration_cast) works for timing, and how to use a random
// number generator with a seed to pick the same files every run.
// I studied the examples, then wrote this file myself.
// Prompts:
//  - "how do i benchmark code in c++ using chrono and steady_clock, and how do i pick random items with a seed so the test is the same every run"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "DataGenerator.h"
#include "BPlusTree.h"
#include "NaryTree.h"
#include "TreeLoader.h"

// steady_clock is used for timing because it never jumps
using Clock = std::chrono::steady_clock;

// how many milliseconds since start
long long  msSince(Clock::time_point start){
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
}


// usage: Main [files] [top folders] [subdirectories] [seed]
int main(int argc, char *argv[]) {
    // default settings, can be changed from the command line
    GeneratorConfig config;
    if (argc > 1) config.minFiles = std::atoi(argv[1]);
    if (argc > 2) config.minTopFolders = std::atoi(argv[2]);
    if (argc > 3) config.minSubdirectories = std::atoi(argv[3]);
    if (argc > 4) config.seed = (unsigned int)std::strtoul(argv[4], nullptr, 10);

    // build the random file tree and time it
    auto start = Clock::now();
    FileNode *root = generateTree(config);
    auto end = Clock::now();

    // print info about the tree
    TreeStats stats = getStats(root);
    std::cout << "Files:          " << stats.files << "\n"
              << "Top folders:    " << stats.topFolders << "\n"
              << "Subdirectories: " << stats.subdirectories << "\n"
              << "Deepest level:  " << stats.maxDepth << "\n"
              << "Largest folder: " << stats.largestFolder << " items\n"
              << "Built in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms (seed " << config.seed << ")\n\n";

    // load the same data into both trees and time each one
    BPlusTree bplus("root");
    NaryTree nary("root");

    start = Clock::now();
    loadIntoBPlus(root, bplus);
    std::cout << "Loaded B+ tree in    " << msSince(start) << " ms\n";

    start = Clock::now();
    loadIntoNary(root, nary);
    std::cout << "Loaded splay tree in " << msSince(start) << " ms\n\n";

    // pick 1000 random files (same seed = same files every run, so it's a fair test)
    std::vector<std::string> files = allFilePaths(root);
    std::mt19937 rng(config.seed);
    std::uniform_int_distribution<std::size_t> pick(0, files.size() - 1);
    const int searches = 1000;

    //search both trees for each file and add up the node accesses
    long long bplusTotal = 0, splayTotal = 0;
    for (int i = 0; i < searches; i++) {
        const std::string &path = files[pick(rng)];
        bplusTotal += bplusLookup(bplus, path);
        splayTotal += splayLookup(nary, path);
    }

    //print the average for each tree
    std::cout << "Average node accesses over " << searches << " searches:\n"
              << "  B+ tree:    " << (double)bplusTotal / searches << "\n"
              << "  Splay tree: " << (double)splayTotal / searches << "\n";

    delete root;
    return 0;
}