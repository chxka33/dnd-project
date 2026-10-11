#include "core/ResourceManager.hpp"

#include <filesystem>

namespace dnd::core {

namespace {

bool loadFrom(sf::Texture& r, const std::string& p)     { return r.loadFromFile(p); }
bool loadFrom(sf::Font& r, const std::string& p)        { return r.loadFromFile(p); }
bool loadFrom(sf::SoundBuffer& r, const std::string& p) { return r.loadFromFile(p); }

} // namespace

template <class Resource>
const Resource& ResourceManager::acquire(Cache<Resource>& cache, const std::string& path) {
    const std::string full = (std::filesystem::path(m_root) / path).string();

    if (const auto it = cache.find(full); it != cache.end()) {
        return *it->second;
    }

    auto resource = std::make_unique<Resource>();
    if (!loadFrom(*resource, full)) {
        throw ResourceError("Failed to load resource: " + full);
    }
    return *cache.emplace(full, std::move(resource)).first->second;
}

const sf::Texture& ResourceManager::texture(const std::string& path) {
    return acquire(m_textures, path);
}
const sf::Font& ResourceManager::font(const std::string& path) {
    return acquire(m_fonts, path);
}
const sf::SoundBuffer& ResourceManager::soundBuffer(const std::string& path) {
    return acquire(m_sounds, path);
}

void ResourceManager::clear() {
    m_textures.clear();
    m_fonts.clear();
    m_sounds.clear();
}

} // namespace dnd::core
