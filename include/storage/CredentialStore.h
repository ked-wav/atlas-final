#pragma once

#include <string>
#include <unordered_map>

namespace atlas::storage {

class CredentialStore {
public:
    bool setSecret(const std::string& key, const std::string& value);
    std::string getSecret(const std::string& key) const;

private:
    // Secure credential storage per OS should use Keychain (macOS/iOS), Credential Manager
    // (Windows), and Keystore (Android). This in-memory fallback is for development only.
    std::unordered_map<std::string, std::string> secrets_;
};

} // namespace atlas::storage
