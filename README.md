# File Encryption Tool (C++)

A console-based C++17 project demonstrating binary file handling, passphrase validation, a versioned file format, basic corruption detection, and encryption/decryption workflow.

> **Security warning:** This is an educational demonstration, not secure cryptography. It uses a custom XOR-based stream transform and a non-cryptographic checksum. Do not use it to protect real confidential data. For production use, use a vetted authenticated-encryption library such as libsodium (e.g. XChaCha20-Poly1305) or OpenSSL (e.g. AES-256-GCM), with proper key derivation and nonce handling.

## Features
- Encrypt and decrypt arbitrary binary files (not just text).
- File input/output uses binary mode.
- Passphrase length validation (8–1024 characters).
- File signature and format version validation.
- Rejects identical input/output paths to avoid overwriting the source.
- Basic corruption / wrong-passphrase detection using a checksum.
- No third-party libraries required for the demo implementation.

## Folder structure
```text
FileEncryptionTool/
├── src/
│   └── main.cpp
├── docs/
│   ├── PROJECT_REPORT.md
│   ├── TECHNICAL_DOCUMENTATION.md
│   └── FLOWCHART.md
├── .gitignore
├── README.md
└── LICENSE
```

## Requirements
- C++17 compiler: GCC 8+, Clang 7+, or a recent MSVC toolchain.
- Windows users can use Visual Studio Build Tools or MinGW-w64.
- VS Code with the C/C++ extension is recommended.

## Compile on Windows (PowerShell)

Open PowerShell in the project root:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic .\src\main.cpp -o .\FileEncryptionTool.exe
.\FileEncryptionTool.exe
```

If `g++` is not recognized, install a MinGW-w64 distribution and add its `bin` directory to PATH, or use MSVC's Developer PowerShell:

```powershell
cl /std:c++17 /EHsc /W4 .\src\main.cpp /Fe:FileEncryptionTool.exe
.\FileEncryptionTool.exe
```

## Quick test

1. Create `sample.txt` in the project folder with any text.
2. Run the executable and choose `1`.
3. Input path: `sample.txt`
4. Output path: `sample.txt.fet`
5. Enter a passphrase of at least 8 characters, e.g. `demo-passphrase-123`.
6. Choose `2` and decrypt `sample.txt.fet` to `restored.txt` using the exact same passphrase.
7. Compare the files:

```powershell
Get-FileHash .\sample.txt -Algorithm SHA256
Get-FileHash .\restored.txt -Algorithm SHA256
```

The SHA256 hashes should match.

## File format (version 1)

| Field | Size | Purpose |
|---|---:|---|
| Magic signature `FETOOL01` | 8 bytes | Identifies this tool's file |
| Version | 4 bytes | Big-endian format version |
| Nonce | 16 bytes | Per-file input to the demo stream generator |
| Reserved salt | 16 bytes | Reserved for future key derivation |
| Original length | 4 bytes | Original content length (v1 supports up to 4 GiB in header) |
| Ciphertext | Variable | Transformed file bytes |
| FNV-1a checksum | 4 bytes | Detects some errors; not a secure MAC |

## Limitations
- The custom transform is not a recognized modern cryptographic algorithm.
- The checksum is not authentication and can be forged.
- `std::random_device` quality is implementation-dependent.
- Very large files are loaded into memory as a whole.
- The format header length field is 32-bit; use this only for modest educational test files.

## Suggested future improvements
- Replace the demo transform with libsodium/OpenSSL authenticated encryption.
- Derive encryption keys from passphrases using Argon2id or scrypt/PBKDF2 as appropriate.
- Stream large files in chunks.
- Add automated unit tests and a CMake build.
- Add a secure password prompt and stronger error handling.
