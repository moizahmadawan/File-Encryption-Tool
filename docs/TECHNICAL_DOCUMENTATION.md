# Technical Documentation

## 1. Overview
The File Encryption Tool is a C++17 console program that reads a file as bytes, transforms those bytes using a passphrase-derived educational stream, and writes a container file. The reverse operation restores the original bytes when the same passphrase is supplied and the container is intact.

**Security disclaimer:** The implementation is for learning file I/O, program structure, and reversible transformations. It is not safe for protecting real data.

## 2. Functional requirements
1. Display a menu for encryption, decryption, information, and exit.
2. Accept input path, output path, and passphrase.
3. Reject empty paths and passphrases outside 8–1024 characters.
4. Read and write binary files, including images and PDFs.
5. Store a signature, version, nonce, reserved salt, content length, ciphertext, and checksum.
6. Validate the signature/version and length before decryption.
7. Detect many wrong-key or corruption cases with a checksum.
8. Prevent input and output from resolving to the same file.

## 3. Non-functional requirements
- Standard C++17; no third-party dependency for this demonstration.
- Clear console prompts and errors.
- Portable filesystem operations through `std::filesystem`.
- Source code kept in `src/main.cpp`.

## 4. Architecture
The application uses a small functional decomposition:
- `readLine`: obtains one console line.
- `readFile` / `writeFile`: binary file I/O.
- `randomBytes`: produces demo nonce and salt bytes.
- `makeKeystream`: generates a repeatable demo byte stream from key and nonce.
- `transform`: XORs input bytes with the stream; the same function encrypts and decrypts.
- `encryptFile`: builds the container and writes it.
- `decryptFile`: parses and validates the container, restores bytes, checks the checksum, then writes the result.
- `pathsReferToSameFile`: guards against overwriting the source.
- `main`: controls menu flow and reports errors.

## 5. Encryption/decryption concept
For each byte, the program computes:

`ciphertext[i] = plaintext[i] XOR keystream[i]`

XOR is reversible:

`plaintext[i] = ciphertext[i] XOR keystream[i]`

The keystream is generated from the passphrase and a per-file nonce. This custom construction is intentionally educational and is **not cryptographically secure**. It should not be represented as AES, RSA, or production-grade encryption.

## 6. Data format
The binary container contains:
1. 8-byte signature: `FETOOL01`
2. 4-byte big-endian format version
3. 16-byte nonce
4. 16-byte reserved salt
5. 4-byte big-endian original length
6. Transformed data
7. 4-byte big-endian FNV-1a checksum of the original bytes

The current version stores length in 32 bits. The implementation is intended for modest test files and loads the entire file into memory.

## 7. Error handling
The program handles:
- inaccessible or missing input files
- invalid passphrase length
- same input and output path
- invalid file signature
- unsupported format version
- missing checksum or mismatched content length
- failed output writes
- filesystem exceptions

## 8. Build and run
From the project root:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic .\src\main.cpp -o .\FileEncryptionTool.exe
.\FileEncryptionTool.exe
```

## 9. Test plan
| ID | Test | Expected result |
|---|---|---|
| T01 | Encrypt a valid text file | `.fet` container is created |
| T02 | Decrypt using same passphrase | Restored file matches original |
| T03 | Decrypt using wrong passphrase | Verification fails; no output is written |
| T04 | Use a missing input path | Clear error is displayed |
| T05 | Enter a passphrase shorter than 8 chars | Operation is rejected |
| T06 | Use same input and output path | Operation is rejected |
| T07 | Decrypt an unrelated file | Signature validation fails |
| T08 | Encrypt a binary file | Operation completes and bytes can be restored |

## 10. Security recommendations
For a real implementation:
- Use an established authenticated cipher such as XChaCha20-Poly1305 or AES-256-GCM.
- Use a maintained cryptographic library; never design your own cipher.
- Derive keys with a password KDF (e.g. Argon2id) and a random salt.
- Store and verify the authentication tag before writing plaintext.
- Consider chunked streaming, secure temporary files, and platform-specific secret handling.
