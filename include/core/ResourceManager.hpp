#pragma once

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace dnd::core {

/// Ошибка загрузки ресурса (файл не найден / повреждён).
class ResourceError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// Кэш текстур, шрифтов и звуковых буферов.
/// Ссылки, возвращаемые методами, остаются валидными до clear() / уничтожения менеджера.
/// Музыка (sf::Music) не кэшируется — она потоковая и будет жить в SoundManager.
class ResourceManager {
public:
    /// Корневая папка ассетов; относительные пути отсчитываются от неё.
    void setAssetRoot(std::string root) { m_root = std::move(root); }
    const std::string& assetRoot() const { return m_root; }

    const sf::Texture&     texture(const std::string& path);      // "textures/dice.png"
    const sf::Font&        font(const std::string& path);         // "fonts/main.ttf"
    const sf::SoundBuffer& soundBuffer(const std::string& path);  // "audio/hit.wav"

    void clear();

private:
    template <class Resource>
    using Cache = std::unordered_map<std::string, std::unique_ptr<Resource>>;

    template <class Resource>
    const Resource& acquire(Cache<Resource>& cache, const std::string& path);

    std::string m_root = "assets";
    Cache<sf::Texture>     m_textures;
    Cache<sf::Font>        m_fonts;
    Cache<sf::SoundBuffer> m_sounds;
};

} // namespace dnd::core
