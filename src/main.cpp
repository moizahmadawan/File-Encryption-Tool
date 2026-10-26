#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/*
 * File Encryption Tool (educational project)
 * Cipher: repeating-key XOR with a randomly generated nonce.
 *
 * IMPORTANT SECURITY NOTE:
 * This is a learning/demo cipher, NOT modern secure encryption.
 * Do not use it to protect sensitive or valuable data. For real security,
 * use a vetted authenticated-encryption library such as libsodium or OpenSSL.
 */

static constexpr char MAGIC[8] = {'F','E','T','O','O','L','0','1'};
static constexpr std::uint32_t FORMAT_VERSION = 1;
static constexpr std::size_t NONCE_SIZE = 16;
static constexpr std::size_t SALT_SIZE = 16;
static constexpr std::size_t KEY_MIN = 8;
static constexpr std::size_t KEY_MAX = 1024;

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

bool isSafeKey(const std::string& key) {
    return key.size() >= KEY_MIN && key.size() <= KEY_MAX;
}

bool readFile(const fs::path& path, std::vector<std::uint8_t>& data, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "Could not open input file.";
        return false;
    }

    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size < 0) {
        error = "Could not determine input file size.";
        return false;
    }
    in.seekg(0, std::ios::beg);

    data.resize(static_cast<std::size_t>(size));
    if (!data.empty()) {
        in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!in) {
            error = "Failed while reading input file.";
            return false;
        }
    }
    return true;
}

bool writeFile(const fs::path& path, const std::vector<std::uint8_t>& data, std::string& error) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "Could not create output file. Check the path and permissions.";
        return false;
    }
    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    if (!out) {
        error = "Failed while writing output file.";
        return false;
    }
    return true;
}

std::vector<std::uint8_t> randomBytes(std::size_t count) {
    // std::random_device is used for demo nonce/salt generation. Its guarantees
    // vary by implementation; this is another reason the project is educational.
    std::random_device rd;
    std::vector<std::uint8_t> bytes(count);
    for (auto& byte : bytes) {
        byte = static_cast<std::uint8_t>(rd() & 0xFF);
    }
    return bytes;
}

std::uint32_t fnv1a32(const std::vector<std::uint8_t>& data) {
    std::uint32_t hash = 2166136261u;
    for (std::uint8_t b : data) {
        hash ^= b;
        hash *= 16777619u;
    }
    return hash;
}

void appendU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
    }
}

bool readU32(const std::vector<std::uint8_t>& data, std::size_t& offset, std::uint32_t& value) {
    if (offset + 4 > data.size()) return false;
    value = 0;
    for (int i = 0; i < 4; ++i) {
        value = (value << 8) | data[offset++];
    }
    return true;
}

std::vector<std::uint8_t> makeKeystream(const std::string& key,
                                        const std::vector<std::uint8_t>& nonce,
                                        std::size_t length) {
    // Educational deterministic stream expansion; not a cryptographic PRNG.
    std::vector<std::uint8_t> stream(length);
    std::uint32_t state = 2166136261u;
    for (unsigned char c : key) {
        state ^= c;
        state *= 16777619u;
    }
    for (std::uint8_t b : nonce) {
        state ^= b;
        state *= 16777619u;
    }

    for (std::size_t i = 0; i < length; ++i) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        stream[i] = static_cast<std::uint8_t>((state >> ((i % 4) * 8)) & 0xFF);
        stream[i] ^= static_cast<std::uint8_t>(key[i % key.size()]);
        stream[i] ^= nonce[i % nonce.size()];
    }
    return stream;
}

std::vector<std::uint8_t> transform(const std::vector<std::uint8_t>& input,
                                    const std::string& key,
                                    const std::vector<std::uint8_t>& nonce) {
    const auto stream = makeKeystream(key, nonce, input.size());
    std::vector<std::uint8_t> output(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        output[i] = input[i] ^ stream[i];
    }
    return output;
}

bool pathsReferToSameFile(const fs::path& input, const fs::path& output) {
    std::error_code ec1, ec2;
    const fs::path a = fs::weakly_canonical(input, ec1);
    const fs::path b = fs::weakly_canonical(output, ec2);
    if (!ec1 && !ec2 && a == b) return true;

    // If output exists, filesystem equivalence can detect aliases/hard links.
    std::error_code ec3;
    if (fs::exists(input, ec3) && fs::exists(output, ec3)) {
        std::error_code ec4;
        if (fs::equivalent(input, output, ec4) && !ec4) return true;
    }
    return false;
}

bool encryptFile(const fs::path& inputPath, const fs::path& outputPath,
                 const std::string& key, std::string& error) {
    if (!isSafeKey(key)) {
        error = "Passphrase must contain between 8 and 1024 characters.";
        return false;
    }
    if (pathsReferToSameFile(inputPath, outputPath)) {
        error = "Input and output paths must be different. Original file was not changed.";
        return false;
    }

    std::vector<std::uint8_t> plain;
    if (!readFile(inputPath, plain, error)) return false;

    const auto nonce = randomBytes(NONCE_SIZE);
    const auto salt = randomBytes(SALT_SIZE);
    const auto cipher = transform(plain, key, nonce);

    std::vector<std::uint8_t> container;
    container.insert(container.end(), std::begin(MAGIC), std::end(MAGIC));
    appendU32(container, FORMAT_VERSION);
    container.insert(container.end(), nonce.begin(), nonce.end());
    container.insert(container.end(), salt.begin(), salt.end());
    appendU32(container, static_cast<std::uint32_t>(std::min<std::size_t>(
        plain.size(), std::numeric_limits<std::uint32_t>::max())));
    container.insert(container.end(), cipher.begin(), cipher.end());

    // Non-cryptographic corruption check only; it is NOT an authentication tag.
    const std::uint32_t checksum = fnv1a32(plain);
    appendU32(container, checksum);

    if (!writeFile(outputPath, container, error)) return false;
    std::cout << "Encryption completed successfully.\n"
              << "Input bytes : " << plain.size() << "\n"
              << "Output bytes: " << container.size() << "\n"
              << "Output file : " << outputPath.string() << "\n";
    return true;
}

bool decryptFile(const fs::path& inputPath, const fs::path& outputPath,
                 const std::string& key, std::string& error) {
    if (!isSafeKey(key)) {
        error = "Passphrase must contain between 8 and 1024 characters.";
        return false;
    }
    if (pathsReferToSameFile(inputPath, outputPath)) {
        error = "Input and output paths must be different. Encrypted file was not changed.";
        return false;
    }

    std::vector<std::uint8_t> container;
    if (!readFile(inputPath, container, error)) return false;

    const std::size_t headerSize = 8 + 4 + NONCE_SIZE + SALT_SIZE + 4;
    if (container.size() < headerSize + 4) {
        error = "File is too small or is not a supported FETool file.";
        return false;
    }
    if (!std::equal(std::begin(MAGIC), std::end(MAGIC), container.begin())) {
        error = "Invalid file signature. This file was not created by this tool.";
        return false;
    }

    std::size_t offset = 8;
    std::uint32_t version = 0;
    if (!readU32(container, offset, version) || version != FORMAT_VERSION) {
        error = "Unsupported or invalid file format version.";
        return false;
    }

    std::vector<std::uint8_t> nonce(container.begin() + static_cast<std::ptrdiff_t>(offset),
                                    container.begin() + static_cast<std::ptrdiff_t>(offset + NONCE_SIZE));
    offset += NONCE_SIZE;
    offset += SALT_SIZE; // Reserved for a future key-derivation format.

    std::uint32_t storedLength = 0;
    if (!readU32(container, offset, storedLength)) {
        error = "Corrupted file header.";
        return false;
    }

    if (container.size() < offset + 4) {
        error = "Corrupted file: missing checksum.";
        return false;
    }
    const std::size_t cipherLength = container.size() - offset - 4;
    if (cipherLength != storedLength) {
        error = "File length does not match the header. File may be corrupted.";
        return false;
    }

    std::vector<std::uint8_t> cipher(container.begin() + static_cast<std::ptrdiff_t>(offset),
                                     container.end() - 4);
    std::size_t checksumOffset = container.size() - 4;
    std::uint32_t expectedChecksum = 0;
    if (!readU32(container, checksumOffset, expectedChecksum)) {
        error = "Could not read checksum.";
        return false;
    }

    const auto plain = transform(cipher, key, nonce);
    if (fnv1a32(plain) != expectedChecksum) {
        error = "Decryption verification failed. The passphrase may be wrong or the file may be damaged.";
        return false;
    }

    if (!writeFile(outputPath, plain, error)) return false;
    std::cout << "Decryption completed successfully.\n"
              << "Output bytes: " << plain.size() << "\n"
              << "Output file : " << outputPath.string() << "\n";
    return true;
}

void printBanner() {
    std::cout << "\n====================================\n"
              << "       FILE ENCRYPTION TOOL\n"
              << "====================================\n"
              << "Educational C++ console project\n"
              << "Security note: demo cipher only; not for sensitive data.\n\n";
}

void printMenu() {
    std::cout << "1. Encrypt a file\n"
              << "2. Decrypt a file\n"
              << "3. Show project information\n"
              << "0. Exit\n";
}

void showInfo() {
    std::cout << "\nProject: File Encryption Tool\n"
              << "Language: Standard C++17\n"
              << "Features: binary file I/O, passphrase validation, format header,\n"
              << "          input/output path checks, checksum-based error detection.\n"
              << "Cipher: custom educational XOR stream transform.\n"
              << "Warning: this is not cryptographically secure. Use a vetted\n"
              << "authenticated encryption library for real confidentiality.\n\n";
}

int main() {
    printBanner();
    while (true) {
        printMenu();
        const std::string choice = readLine("Choose an option: ");

        if (choice == "0") {
            std::cout << "Goodbye!\n";
            break;
        }
        if (choice == "3") {
            showInfo();
            continue;
        }
        if (choice != "1" && choice != "2") {
            std::cout << "Invalid option. Please enter 0, 1, 2, or 3.\n\n";
            continue;
        }

        const std::string input = readLine("Input file path: ");
        const std::string output = readLine("Output file path: ");
        const std::string key = readLine("Passphrase (8-1024 characters): ");

        if (input.empty() || output.empty()) {
            std::cout << "Input and output paths cannot be empty.\n\n";
            continue;
        }

        std::string error;
        bool success = false;
        try {
            if (choice == "1") {
                success = encryptFile(fs::path(input), fs::path(output), key, error);
            } else {
                success = decryptFile(fs::path(input), fs::path(output), key, error);
            }
        } catch (const fs::filesystem_error& ex) {
            error = std::string("Filesystem error: ") + ex.what();
        } catch (const std::exception& ex) {
            error = std::string("Unexpected error: ") + ex.what();
        }

        if (!success) {
            std::cout << "Operation failed: " << error << "\n";
        }
        std::cout << "\n";
    }
    return 0;
}
