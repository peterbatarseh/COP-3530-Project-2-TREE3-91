#pragma once

class FileNode;

// Initializes server
// might remove this this is very unecessary
int runWebServer(const FileNode* bplusRoot, const FileNode* splayRoot);
