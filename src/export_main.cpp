/*
Builds dummy FileNodes and exports them as JSON to render
Note: this might be a problem given we need a huge dataset and will have to send 100K+ nodes in one payload
It's fine!
*/

#include <iostream>
#include <string>

#include "FileNode.h"
#include "JsonExport.h"
#include "WebServer.h"

static FileNode* folder(FileNode* parent, const std::string& name) {
    auto* n = new FileNode(name, nullptr, true);
    if (parent) parent->addChild(n);
    return n;
}

static void file(FileNode* parent, const std::string& name) {
    parent->addChild(new FileNode(name, nullptr, false));
}

static FileNode* buildSample() {
    FileNode* root = folder(nullptr, "root");

    FileNode* docs = folder(root, "Documents");
    file(docs, "resume.pdf");
    file(docs, "notes.txt");
    FileNode* school = folder(docs, "School");
    file(school, "COP3530_syllabus.pdf");
    file(school, "project2_report.docx");

    FileNode* pics = folder(root, "Pictures");
    file(pics, "beach.png");
    file(pics, "family.jpg");
    folder(pics, "Screenshots");

    FileNode* code = folder(root, "Code");
    file(code, "main.cpp");
    file(code, "FileNode.h");
    file(code, "FileNode.cpp");

    file(root, "todo.txt");
    return root;
}

int main(int argc, char* argv[]) {
    std::string outDir = argc > 1 ? argv[1] : "../web/data";

    // we don't need a json file middleman anymore since it sends it through http
    FileNode* root = buildSample();

    return runWebServer(root, root);
}
