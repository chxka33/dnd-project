#include "core/SettingsSerializer.hpp"

#include "utils/ExecutablePath.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <exception>
#include <fstream>
#include <system_error>
#include <utility>

namespace dnd::core
{

    namespace
    {

        const char *screenModeToString(AppConfig::ScreenMode mode)
        {
            switch (mode)
            {
            case AppConfig::ScreenMode::Windowed:
                return "windowed";
            case AppConfig::ScreenMode::Fullscreen:
                return "fullscreen";
            case AppConfig::ScreenMode::Borderless:
                return "borderless";
            }
            return "borderless";
        }

        bool screenModeFromString(const std::string &value, AppConfig::ScreenMode &mode)
        {
            if (value == "windowed")
                mode = AppConfig::ScreenMode::Windowed;
            else if (value == "fullscreen")
                mode = AppConfig::ScreenMode::Fullscreen;
            else if (value == "borderless")
                mode = AppConfig::ScreenMode::Borderless;
            else
                return false;
            return true;
        }

        unsigned clampDimension(unsigned value, unsigned fallback)
        {
            if (value < 320)
                return fallback;
            return std::min(value, 16384u);
        }

        bool readNonNegativeInteger(const nlohmann::json &value, long long &out)
        {
            if (value.is_number_unsigned())
            {
                out = static_cast<long long>(value.get<unsigned long long>());
                return true;
            }
            if (value.is_number_integer())
            {
                out = value.get<long long>();
                return true;
            }
            return false;
        }

        void setError(std::string *outError, std::string message)
        {
            if (outError)
                *outError = std::move(message);
        }

    } // namespace

    std::filesystem::path SettingsSerializer::defaultPath()
    {
        return utils::executableDir() / "settings.json";
    }

    bool SettingsSerializer::load(AppConfig &config,
                                  const std::filesystem::path &filePath,
                                  std::string *outError)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            std::error_code ec;
            if (std::filesystem::exists(filePath, ec))
                setError(outError, "Cannot open settings file: " + filePath.string());
            else if (outError)
                outError->clear();
            return false;
        }

        nlohmann::json j = nlohmann::json::parse(file, nullptr, false);
        if (j.is_discarded() || !j.is_object())
        {
            setError(outError, "Settings file is not valid JSON: " + filePath.string());
            return false;
        }

        try
        {
            if (j.contains("display") && j["display"].is_object())
            {
                const nlohmann::json &display = j["display"];
                long long number = 0;
                if (display.contains("width") && readNonNegativeInteger(display["width"], number) && number > 0)
                    config.width = clampDimension(static_cast<unsigned>(number), config.width);
                if (display.contains("height") && readNonNegativeInteger(display["height"], number) && number > 0)
                    config.height = clampDimension(static_cast<unsigned>(number), config.height);
                if (display.contains("fps") && readNonNegativeInteger(display["fps"], number) && number >= 0)
                    config.fps = static_cast<unsigned>(std::min<long long>(number, 1000));
                if (display.contains("mode") && display["mode"].is_string())
                {
                    AppConfig::ScreenMode mode = config.screenMode;
                    if (screenModeFromString(display["mode"].get<std::string>(), mode))
                        config.screenMode = mode;
                }
            }

            if (j.contains("interface") && j["interface"].is_object())
            {
                const nlohmann::json &interface = j["interface"];
                if (interface.contains("uiScale") && interface["uiScale"].is_number())
                    config.uiScale = std::clamp(interface["uiScale"].get<float>(), 0.f, 4.f);
            }

            if (j.contains("paths") && j["paths"].is_object())
            {
                const nlohmann::json &paths = j["paths"];
                if (paths.contains("campaigns") && paths["campaigns"].is_string())
                    config.campaignDir = paths["campaigns"].get<std::string>();
                if (paths.contains("saves") && paths["saves"].is_string())
                    config.saveDir = paths["saves"].get<std::string>();
            }
        }
        catch (const std::exception &e)
        {
            setError(outError, std::string("Failed to read settings: ") + e.what());
            return false;
        }

        return true;
    }

    bool SettingsSerializer::save(const AppConfig &config,
                                  const std::filesystem::path &filePath,
                                  std::string *outError)
    {
        nlohmann::json j;
        j["version"] = kCurrentFormatVersion;
        j["display"] = {
            {"width", config.width},
            {"height", config.height},
            {"fps", config.fps},
            {"mode", screenModeToString(config.screenMode)}};
        j["interface"] = {
            {"uiScale", config.uiScale}};
        j["paths"] = {
            {"campaigns", config.campaignDir},
            {"saves", config.saveDir}};

        std::error_code ec;
        if (filePath.has_parent_path())
            std::filesystem::create_directories(filePath.parent_path(), ec);
        if (ec)
        {
            setError(outError, "Cannot create settings directory: " + ec.message());
            return false;
        }

        std::ofstream file(filePath);
        if (!file.is_open())
        {
            setError(outError, "Cannot write settings file: " + filePath.string());
            return false;
        }

        file << j.dump(4) << '\n';
        if (!file.good())
        {
            setError(outError, "Failed to write settings file: " + filePath.string());
            return false;
        }
        return true;
    }

} // namespace dnd::core
