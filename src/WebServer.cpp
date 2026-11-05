#include "WebServer.h"

#include <httplib.h>

#include <iostream>

#include "JsonExport.h"

using namespace std;

// TODO replace with B+ tree class
int runWebServer(const FileNode *bplusRoot, const FileNode *splayRoot) {
    httplib::Server server;

    bool mounted = server.set_mount_point("/", "web");
    if (!mounted) {
        std::cout << "Server not loading" << std::endl;

        return 1;
    }

    // May divine wrath fall upon whoever authored the syntax of lambdas
    // simultaneously bless the people who made this library because I'd rather walk over burning coals than do networking
    server.Get("/backend/bplus", [bplusRoot](const httplib::Request &, httplib::Response &response) {
        response.set_content(toJson(bplusRoot, "B+ Tree"), "application/json");
    });

    server.Get("/backend/bplus_accesses", [](const httplib::Request &request, httplib::Response &response) {
        std::string path = request.get_param_value("path");
        // TODO add B+ implementation
        response.set_content("3", "application/json");
    });

    server.Get("/backend/splay", [splayRoot](const httplib::Request &, httplib::Response &response) {
        response.set_content(toJson(splayRoot, "Splay Tree"), "application/json");
    });

    server.Get("/backend/splay_accesses", [splayRoot](const httplib::Request &request, httplib::Response &response) {
        std::string name = request.get_param_value("name");
        // TODO whatever splay tree implementation Taysun will do and return access count
        response.set_content("3", "application/json");
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
