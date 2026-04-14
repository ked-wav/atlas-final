#include "storage/CredentialStore.h"

#include <cstdlib>

namespace atlas::storage {

namespace {
bool insecureStorageAllowed() {
    const char* value = std::getenv("ATLAS_ALLOW_INSECURE_STORAGE");
    return value != nullptr && std::string(value) == "1";
}
} // namespace

bool CredentialStore::setSecret(const std::string& key, const std::string& value) {
    if (!insecureStorageAllowed()) {
        return false;
    }
    secrets_[key] = value;
    return true;
}

std::string CredentialStore::getSecret(const std::string& key) const {
    if (!insecureStorageAllowed()) {
        return {};
    }
    auto it = secrets_.find(key);
    return it == secrets_.end() ? std::string{} : it->second;
}

} // namespace atlas::storage
