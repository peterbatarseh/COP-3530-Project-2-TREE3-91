# TREE(3) FILE SYSTEM

## Authors

- Peter Batarseh
- Muhammad Abdullah Abid
- Nadeem Taysun

## Build/Run

Run build.py (requires CMake/Ninja/C++) and pick a target:

1. Main - generates the 110k file dataset, loads it into both trees and compares node accesses
2. Tests - B+ tree and FileNode tests
3. ExportWeb - starts the website, then open http://127.0.0.1:8000
4. SplayTests - splay tree tests

The web server closes when the terminal running it closes.