markdown
# PSARc v1.0

**PSARc** is a versatile command-line archive utility designed and developed by Juan José Ponteprino (SplinterGU) in 2023. This tool allows you to create, extract, list, and obtain information about archives effortlessly.

## Usage

```sh
./psar [options] [file]...
```

## Examples

```sh
./psar -cf archive.pak foo bar  # Create (store) archive.pak from foo and bar.
./psar -czf archive.pak foo bar # Create (zlib) archive.pak from foo and bar.
./psar -lf archive.pak          # List files in archive.pak.
./psar -xf archive.pak          # Extract all files from archive.pak.
```

### Operation Modes:

- `-c, --create` : Create an archive.
- `-x, --extract` : Extract files.
- `-l, --list` : List contents.
- `-i, --info` : Show archive information.

### Operation Modifiers:

- `-f, --file=FILE` : Specify the file (mandatory).
- `-b, --block-size=BYTES` : Set the block size in bytes (default: 65536).

### Compression (Default: No Compression - Store):

- `-z, --zlib` : Use zlib compression.
- `-j, --lzma` : Use lzma compression.
- `-0` : Compress faster (only for lzma).
- `-1, --fast` : Compress faster.
- `-9, --best` : Compress better.
- `-e, --extreme` : Extreme compression (only for lzma).

### Archive Flags (Default: Relative Paths and Case-Sensitive):

- `-I, --ignore-case` : Ignore case when matching file selection patterns during creation (ignored during extraction, uses creation setting).
- `-A, --absolute-paths` : Use absolute paths for file names.

### File Name Selection:

- `-s, --source-dir=DIR` : Set the base directory for source files.
- `-t, --target-dir=DIR` : Specify the directory where extracted files will be placed.
- `-r, --recursive` : Recurse into directories.
- `-T, --trim-path` : Remove all file paths from/to the archive.

### Overwrite Control:

- `-y, --overwrite` : Force overwrite of the output file.
- `-S, --skip-existing-files` : Don't replace existing files when extracting, silently skip over them.

### Other Options:

- `-n, --num-threads=NUM` : Specify the number of threads (default: auto, based on CPU cores).
- `-o, --output-format=FORMAT` : Specify the output format for information display (available formats: json, csv, xml).
- `-v, --verbose` : List processed files in detail.
- `-h, --help` : Show this help.
- `-V, --version` : Show program version.

## How to Build

To build PSARc, you'll need the following dependencies:

- [xz](https://github.com/tukaani-project/xz) (For Windows)

### Build Instructions

1. Clone this repository:

   ```sh
   git clone https://github.com/SplinterGU/PSARc.git
   cd PSARc
   ```

2. Build the project:

   ```sh
   mkdir build
   cd build
   cmake ..
   make
   ```

3. Run PSARc:

   ```sh
   ./psar [options] [file]...
   ```

## License

This software is provided under the terms of the MIT License. You may freely use, modify, and distribute this software, subject to the conditions and limitations of the MIT License. For more details, please see the LICENSE file included with this software.

## Contact and Support

- Report bugs to: splintergu@gmail.com
- Home page: [PSARc on GitHub](https://github.com/SplinterGU/PSARc)
