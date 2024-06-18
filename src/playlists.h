#pragma once

#include "defines.h"

struct playlist_chart_t
{
    std::uint32_t id;
    std::uint8_t difficulty;
    std::uint8_t bar_style;
    music_entry_t* entry;
};

struct playlist_t
{
    std::string name;
    std::string voice;
    std::string bar_texture;
    std::string ticker_text;
    std::uint8_t play_style;
    std::vector<playlist_chart_t> charts;
};

extern std::vector<playlist_t> playlist_data;

void load_playlists(const std::filesystem::path& dir, music_data_t* music_data);