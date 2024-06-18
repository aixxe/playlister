#include "playlists.h"

std::vector<playlist_t> playlist_data {};

inline auto difficulty_exists(music_entry_t* entry, std::uint8_t play_style, std::uint8_t difficulty)
    { return entry->rating[play_style][difficulty] != 0; }

void load_playlists(const std::filesystem::path& dir, music_data_t* music_data)
{
    // Construct lookup table from entry IDs to music entries.
    spdlog::debug("Processing music data... (version: {}, occupied entries: {}, max entries: {}, first entry: 0x{:X})",
                  music_data->game_version, music_data->occupied_entries,
                  music_data->maximum_entries, reinterpret_cast<std::uintptr_t>(&music_data->first));

    auto index = std::unordered_map<decltype(music_entry_t::entry_id), music_entry_t*> {};

    for (auto entry = &music_data->first; entry->entry_id != 0; entry++)
        index[entry->entry_id] = entry;

    spdlog::debug("Searching for playlists in directory '{}'...", dir.string());

    // Start parsing playlists from disk.
    for (auto const& it: std::filesystem::directory_iterator(dir))
    {
        if (!it.is_regular_file() || !it.path().extension().string().ends_with(".yml"))
            continue;

        spdlog::debug("Parsing playlist file '{}'...", it.path().string());

        // Attempt to parse the playlist.
        try
        {
            auto root = YAML::LoadFile(it.path().string());

            if (!root.IsSequence())
                throw std::runtime_error("root node is not a sequence");

            for (auto const& node: root)
            {
                if (!node.IsMap())
                    throw std::runtime_error("playlist node is not a map");

                auto const name = node["name"].as<std::string>();
                auto const play_style = node["play style"].as<std::string>();
                auto const charts = node["charts"];

                if (!charts.IsSequence())
                    throw std::runtime_error("charts node is not a sequence");

                auto playlist = playlist_t {};

                playlist.name = name;
                playlist.play_style = (play_style == "DP") ? STYLE_DP: STYLE_SP;

                // optional fields
                if (node["voice"].IsDefined())
                    playlist.voice = node["voice"].as<std::string>();
                if (node["bar texture"].IsDefined())
                    playlist.bar_texture = node["bar texture"].as<std::string>();
                if (node["ticker text"].IsDefined())
                    playlist.ticker_text = node["ticker text"].as<std::string>();

                spdlog::debug("Resolving charts for playlist '{}'...", name);

                for (auto const& chart: charts)
                {
                    if (!chart.IsMap())
                        throw std::runtime_error("chart node is not a map");

                    auto const id = chart["entry id"].as<std::uint32_t>();

                    auto const difficulty = [] (auto value) -> std::uint8_t
                    {
                        if (value == "beginner")
                            return DIFFICULTY_BEGINNER;
                        if (value == "normal")
                            return DIFFICULTY_NORMAL;
                        if (value == "hyper")
                            return DIFFICULTY_HYPER;
                        if (value == "another")
                            return DIFFICULTY_ANOTHER;
                        if (value == "leggendaria")
                            return DIFFICULTY_LEGGENDARIA;
                        throw std::runtime_error("invalid difficulty");
                    } (chart["difficulty"].as<std::string>());

                    auto const bar_style = [&] () -> std::uint8_t
                    {
                        if (!chart["bar style"].IsDefined())
                            return BAR_STYLE_NORMAL;

                        auto const& value = chart["bar style"].as<std::string>();

                        if (value == "rainbow")
                            return BAR_STYLE_RAINBOW;
                        if (value == "lightning")
                            return BAR_STYLE_LIGHTNING;

                        return BAR_STYLE_NORMAL;
                    } ();

                    // Ensure the ID exists in the music LUT.
                    if (!index.contains(id))
                    {
                        spdlog::warn("Playlist '{}' references unknown music entry ID {}!", name, id);
                        continue;
                    }

                    // Ensure this is a valid chart by checking the chart rating.
                    if (!difficulty_exists(index[id], playlist.play_style, difficulty))
                    {
                        spdlog::warn("Playlist '{}' references unknown chart ID {}!", name, id);
                        continue;
                    }

                    // Add the chart to the playlist.
                    playlist.charts.emplace_back(playlist_chart_t {
                        .id = id,
                        .difficulty = difficulty,
                        .bar_style = bar_style,
                        .entry = index[id],
                    });
                }

                // If the playlist is empty, don't add it.
                if (playlist.charts.empty())
                {
                    spdlog::warn("Skipping playlist '{}', no valid charts found!", name);
                    continue;
                }

                // Add the playlist to the playlist data.
                spdlog::info("Loaded playlist '{}' ({}) with {} chart(s)!",
                    name, (play_style == "DP") ? "DP": "SP", playlist.charts.size());
                playlist_data.emplace_back(playlist);
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("Failed to parse playlist '{}'!", it.path().string());
            spdlog::error("Parse error: {}", e.what());
        }
    }
}