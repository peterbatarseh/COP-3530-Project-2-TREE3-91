// Builds a sample file system and writes it to web/data/ for the website.
//
// Usage:  ExportWeb [output folder]
// Default output folder is ../web/data (works when run from a build folder
// like cmake-build-debug/ or build/).

#include <iostream>
#include <string>

#include "FileNode.h"
#include "JsonExport.h"

static FileNode *folder(FileNode *parent, const std::string &name) {
    auto *n = new FileNode(name, nullptr, true);
    if (parent) parent->addChild(n);
    return n;
}

static void file(FileNode *parent, const std::string &name) {
    parent->addChild(new FileNode(name, nullptr, false));
}

static FileNode *buildSample() {
    FileNode *root = folder(nullptr, "root");

    FileNode *docs = folder(root, "Documents");
    file(docs, "resume.pdf");
    file(docs, "notes.txt");
    FileNode *school = folder(docs, "School");
    file(school, "COP3530_syllabus.pdf");
    file(school, "project2_report.docx");

    FileNode *pics = folder(root, "Pictures");
    file(pics, "beach.png");
    file(pics, "family.jpg");
    folder(pics, "Screenshots");

    FileNode *code = folder(root, "Code");
    file(code, "main.cpp");
    file(code, "FileNode.h");
    file(code, "FileNode.cpp");

    file(root, "todo.txt");
    return root;
}

int main(int argc, char *argv[]) {
    std::string outDir = argc > 1 ? argv[1] : "../web/data";

    // TODO: once the real trees exist, load the same files into the B+ tree
    // and the splay tree and export each one with its own access counts.
    FileNode *root = buildSample();
    bool ok = writeJson(root, "B+ Tree", outDir + "/bplus.json") &&
              writeJson(root, "Splay Tree", outDir + "/splay.json");
    delete root;

    if (!ok) {
        std::cerr << "Could not write to " << outDir
                  << " (make sure the folder exists, or pass the path as an argument)\n";
        return 1;
    }
    std::cout << "Wrote bplus.json and splay.json to " << outDir << "\n";
    return 0;
}
