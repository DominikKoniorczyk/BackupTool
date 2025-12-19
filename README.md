# BackupTool

# C++ Backup Tool

A simple C++ tool for backing up files and folders to a chosen destination directory.  
All selected files, folders, and the target backup directory are saved for easy reuse.

## Features

- Backup multiple files and folders at once.
- Choose a target directory for backups.
- Saves the selected sources and destination for future backups.
- Provides both source code and optional precompiled `.exe` for convenience.

## Download Precompiled Executable (Windows)

You can download the ready-to-use `.exe` from the [Releases](https://github.com/DominikKoniorczyk/BackupTool/releases/) section.


## Requirements

- C++ compiler (e.g., GCC, Clang, MSVC) if you want to build from source.
- Windows (if using the provided `.exe`) or any system that supports your compiled binary.

## Installation

1. Clone the repository:

```bash
git clone https://github.com/yourusername/your-repo-name.git
```
Navigate to the project folder:
```bash
cd your-repo-name
```
Build from source (optional):

# Using g++ as example
```bash
g++ -o backup_tool backup_tool.cpp
```
This will create an executable backup_tool (or backup_tool.exe on Windows).

## Usage
From Source Code
1. Compile the tool as described above.

2. Run the program:
### Linux/macOS
```bash
./backup_tool
```
### Windows
```bash
backup_tool.exe   
```

3. Follow the prompts to select files, folders, and a target backup directory.

#### Using Precompiled .exe (Windows)
1. Download the .exe from the releases section or from the bin/ folder (if included).

2. Double-click the .exe or run it via command line.

3. Follow the prompts to perform your backup.

Contributing
Contributions are welcome! Please:

Retain the original author: Dominik Koniorczyk

Clearly indicate your changes

Keep the project under the same GPLv3 license

License
This project is licensed under the GNU GPLv3 License. See the LICENSE file for details.

Author
Dominik Koniorczyk
