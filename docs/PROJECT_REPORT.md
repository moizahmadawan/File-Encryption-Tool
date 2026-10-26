# Project Report: File Encryption Tool

## Title
**File Encryption Tool Using C++**

## Abstract
This project implements a menu-driven console application in C++17 to demonstrate file protection workflows. It reads files in binary mode, transforms the bytes with a passphrase-based educational XOR stream, stores the result in a versioned container, and supports restoration with the same passphrase. The program includes path validation, format checks, basic corruption detection, and clear console messages.

This project is intended for academic learning only. Its custom transform and checksum are not suitable for protecting confidential information.

## 1. Introduction
Digital files may contain private or important information. This project introduces the basic programming workflow behind a file encryption utility: obtaining user input, validating paths, reading bytes, processing data, writing output, and reporting errors.

## 2. Problem statement
Beginners often need a practical project that demonstrates file handling and reversible data processing. The goal is to provide a simple console application that can transform a file into an unreadable-looking binary container and restore it using the same passphrase.

## 3. Objectives
- Apply C++ functions and standard library features.
- Practice binary file input/output.
- Implement a menu-driven console interface.
- Validate user input and file format.
- Demonstrate reversible byte transformation.
- Document compilation, testing, limitations, and future work.

## 4. Scope
The tool supports modest-sized files that can fit in memory, with a 32-bit content-length field. It does not provide a graphical interface, cloud storage, account management, or production-grade cryptographic protection.

## 5. Tools and technologies
- Language: C++17
- Standard library: `iostream`, `fstream`, `filesystem`, `vector`, `string`, `random`, `cstdint`
- Editor: Visual Studio Code (recommended)
- Compiler: GCC/MinGW-w64 or MSVC

## 6. Methodology
1. Show a menu and read the selected operation.
2. Collect input path, output path, and passphrase.
3. Validate paths and passphrase length.
4. Read the input file in binary mode.
5. For encryption, generate nonce/salt bytes and transform file bytes.
6. Write a header, transformed data, and checksum to the output.
7. For decryption, validate the header and length.
8. Reverse the transform and compare the checksum.
9. Write restored content only when the checksum matches.
10. Display success or a descriptive error.

## 7. Testing
The planned test cases are documented in `TECHNICAL_DOCUMENTATION.md`. The main acceptance test is to encrypt a sample file, decrypt it with the same passphrase, and compare SHA-256 hashes of the original and restored files.

## 8. Results
Expected successful output includes a confirmation message and input/output byte counts. Decryption with the correct passphrase should recreate the original bytes. A wrong passphrase should normally fail the checksum check, though the checksum is not a secure authentication mechanism.

## 9. Limitations
- Custom XOR stream is not modern cryptography.
- FNV-1a checksum does not provide secure integrity/authentication.
- Randomness depends on the platform's `std::random_device`.
- Whole files are loaded into memory.
- The 32-bit file length field limits the supported format.

## 10. Future enhancements
- Replace the demo transform with a maintained library and authenticated encryption.
- Add Argon2id/scrypt/PBKDF2 key derivation.
- Support chunked file processing.
- Add automated tests and CMake.
- Add password masking and a secure GUI.
- Add progress reporting and a robust recovery strategy.

## Conclusion
The File Encryption Tool is a compact C++ project demonstrating binary file handling, modular functions, input validation, a custom file format, and reversible byte processing. It is suitable for classroom demonstration when its security limitations are clearly stated. It must not be used as a substitute for a trusted encryption library.
