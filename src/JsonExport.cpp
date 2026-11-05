#include "JsonExport.h"

#include <fstream>
#include <sstream>

namespace {

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

// Placeholder access count: visiting each node on the path from the root,
// scanning siblings left to right until the right one is found.
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

void writeNode(std::ostringstream &out, const FileNode *node, int indent) {
    std::string pad(indent, ' ');
    out << pad << "{\n";
    out << pad << "  \"name\": \"" << escape(node->getName()) << "\",\n";
    out << pad << "  \"folder\": " << (node->isFolder() ? "true" : "false") << ",\n";
    out << pad << "  \"accesses\": " << placeholderAccesses(node);

    if (node->isFolder()) {
        out << ",\n" << pad << "  \"children\": [";
        const auto &children = node->getChildren();
        for (size_t i = 0; i < children.size(); i++) {
            out << (i == 0 ? "\n" : ",\n");
            writeNode(out, children[i], indent + 4);
        }
        out << (children.empty() ? "]" : "\n" + pad + "  ]");
    }
    out << "\n" << pad << "}";
}

}  // namespace

std::string toJson(const FileNode *root, const std::string &implementation) {
    std::ostringstream out;
    out << "{\n  \"implementation\": \"" << escape(implementation) << "\",\n  \"root\":\n";
    writeNode(out, root, 2);
    out << "\n}\n";
    return out.str();
}

bool writeJson(const FileNode *root, const std::string &implementation, const std::string &path) {
    std::ofstream file(path);
    if (!file) return false;
    file << toJson(root, implementation);
    return true;
}
