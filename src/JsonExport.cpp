#include "JsonExport.h"

#include <fstream>
#include <sstream>

namespace {

// make text safe to put inside JSON quotes
std::string escape(const std::string &s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            default:   out += c;
        }
    }
    return out;
}

// a backup count if no tree is passed in 
int placeholderAccesses(const FileNode *node) {
    int count = 1;
    while (node->getParent() != nullptr) {
        const auto &siblings = node->getParent()->getChildren();
        for (const FileNode *s : siblings) {
            count++;
            if (s == node) break;
        }
        node = node->getParent();
    }
    return count;
}

// write one node and everything inside it
void writeNode(std::ostringstream &out, const FileNode *node, int indent,
               const std::string &path, const FolderAccessFn &folderAccesses) {
    // only folders get a count here, files are counted when you click them
    std::size_t accesses = 0;
    if (node->isFolder())
        accesses = folderAccesses ? folderAccesses(path) : placeholderAccesses(node);

    std::string pad(indent, ' ');
    out << pad << "{\n";
    out << pad << "  \"name\": \"" << escape(node->getName()) << "\",\n";
    out << pad << "  \"folder\": " << (node->isFolder() ? "true" : "false") << ",\n";
    out << pad << "  \"accesses\": " << accesses;

    if (node->isFolder()) {
        out << ",\n" << pad << "  \"children\": [";
        const auto &children = node->getChildren();
        for (size_t i = 0; i < children.size(); i++) {
            out << (i == 0 ? "\n" : ",\n");
            const std::string &name = children[i]->getName();
            writeNode(out, children[i], indent + 4, path.empty() ? name : path + "/" + name, folderAccesses);
        }
        out << (children.empty() ? "]" : "\n" + pad + "  ]");
    }
    out << "\n" << pad << "}";
}

}  // namespace (ends the helpers)

std::string toJson(const FileNode *root, const std::string &implementation,
                   const FolderAccessFn &folderAccesses) {
    std::ostringstream out;
    out << "{\n  \"implementation\": \"" << escape(implementation) << "\",\n  \"root\":\n";
    writeNode(out, root, 2, "", folderAccesses);
    out << "\n}\n";
    return out.str();
}

bool writeJson(const FileNode *root, const std::string &implementation, const std::string &path) {
    std::ofstream file(path);
    if (!file) return false;
    file << toJson(root, implementation);
    return true;
}