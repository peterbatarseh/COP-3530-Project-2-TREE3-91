#include "WebServer.h"

#include <httplib.h>

#include <iostream>
#include <mutex>

#include "BPlusTree.h"
#include "FileNode.h"
#include "JsonExport.h"
#include "NaryTree.h"
#include "TreeLoader.h"

using namespace std;

int runWebServer(const FileNode *root, BPlusTree &bplus, NaryTree &nary) {
    httplib::Server server;

    // serve the web folder
    bool mounted = server.set_mount_point("/", "web");
    if (!mounted) {
        std::cout << "Server not loading" << std::endl;

        return 1;
    }

    // make the tree data once at the start
    std::cout << "Preparing tree data..." << std::endl;
    const std::string bplusJson = toJson(root, "B+ Tree", [&bplus](const std::string &path) {
        return bplusLookup(bplus, path);
    });

    const std::string splayJson = toJson(root, "Splay Tree", [&nary](const std::string &path) {
        return (std::size_t)splayFolderLookup(nary, path);
    });

    // only one search at a time
    std::mutex treeLock; 

    // send the b+ tree
    server.Get("/backend/bplus", [&bplusJson](const httplib::Request &, httplib::Response &response) {
        response.set_content(bplusJson, "application/json");
    });

    // search the b+ tree for a file
    server.Get("/backend/bplus_accesses", [&bplus, &treeLock](const httplib::Request &request, httplib::Response &response) {
        std::string path = request.get_param_value("path");
        std::lock_guard<std::mutex> guard(treeLock);
        response.set_content(std::to_string(bplusLookup(bplus, path)), "application/json");
    });

    // send the splay tree
    server.Get("/backend/splay", [&splayJson](const httplib::Request &, httplib::Response &response) {
        response.set_content(splayJson, "application/json");
    });

    // search the splay tree for a file
    server.Get("/backend/splay_accesses", [&nary, &treeLock](const httplib::Request &request, httplib::Response &response) {
        std::string path = request.get_param_value("path");
        std::lock_guard<std::mutex> guard(treeLock);
        response.set_content(std::to_string(splayLookup(nary, path)), "application/json");
    });

    if (!server.bind_to_port("127.0.0.1", 8000)) {
        std::cout << "Binding failed. Close old terminal" << std::endl;

        return 1;
    }

    std::cout << "Local server at http://127.0.0.1:8000/ - Stops when you close the terminal." << std::endl;

    if (!server.listen_after_bind()) {
        std::cout << "Listening failed." << std::endl;

        return 1;
    }

    return 0;
}