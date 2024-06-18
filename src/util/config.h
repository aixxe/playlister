#pragma once

#include <filesystem>

namespace util
{
    template <typename T> concept config_value =
        std::is_same_v<T, std::string>
     || std::is_same_v<T, bool>
     || std::is_same_v<T, std::vector<std::string>>;

    class config
    {
        public:
            config() = default;

            explicit config(const std::filesystem::path& file)
            {
                try
                    { _config = YAML::LoadFile(file.string()); }
                catch (YAML::BadFile& e)
                    { /* all config options have defaults, so this is non-fatal */ }
            }

            template <config_value T> T get(std::string const& key, T const& fallback) const
            {
                if (!_config[key])
                    return fallback;

                if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, bool>)
                    return _config[key].as<T>();

                if constexpr (std::is_same_v<T, std::vector<std::string>>)
                {
                    auto result = std::vector<std::string>(_config[key].size());

                    for (auto const& node: _config[key])
                        result.push_back(node.as<std::string>());

                    return result;
                }

                return _config[key].as<T>();
            }
        private:
            YAML::Node _config;
    };
}