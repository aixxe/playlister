#include "defines.h"
#include "playlists.h"
#include "util/config.h"
#include "util/memory.h"
#include "util/library.h"

using namespace std::chrono_literals;
using namespace std::string_literals;

class category_definition_patch
{
    public:
        explicit category_definition_patch() = default;
        explicit category_definition_patch(category_definition_t* target):
            _original(*target), _target(target) {};

        ~category_definition_patch()
            { if (_target != nullptr) *_target = _original; }

        auto operator->() const -> category_definition_t*
            { return _target; }
    private:
        category_definition_t _original = {};
        category_definition_t* _target = nullptr;
};

auto target_category_patch = category_definition_patch {};
auto category_voices = std::unordered_map<std::string_view, decltype(category_t::id)> {};
auto category_definition_patches = std::unordered_map<decltype(category_t::id), category_definition_patch> {};

namespace playlister
{
    // game context
    util::library base;
    state_t* state = nullptr;
    std::uint64_t* buttons = nullptr;
    music_data_t* music_data = nullptr;
    player_scores_t* player_scores_p1 = nullptr;
    player_scores_t* player_scores_p2 = nullptr;
    CCategoryGameData* category_game_data = nullptr;
    CApplicationConfig* application_config = nullptr;
    CMusicSelectGameData* music_select_game_data = nullptr;
    category_definitions_t* category_definitions = nullptr;

    // game functions
    void* (*reload_lamps) (void*) = nullptr;
    void (*sort_bars) (category_t*, int, bool) = nullptr;
    bool (*play_system_sound) (int) = nullptr;

    // playlister context
    util::config config;
    auto in_playlist_mode = false;
    auto target_category_id = -1;
    auto backup_created = false;
    auto playlist_mode_patches = std::vector<util::memory::code_patch> {};
    category_t categories_backup[2][CATEGORY_COUNT];
    category_t* populated_categories_backup[2][CATEGORY_COUNT];
    category_bar_meta_t bar_meta_backup[2][CATEGORY_COUNT];

    // various temporary state
    CMusicSelectScene* mselect = nullptr;
    auto text_category_id = -1;
    auto last_category_id = -1;
    auto last_music_difficulty_p1 = -1;
    auto last_music_difficulty_p2 = -1;
    music_entry_t* last_music_entry_ptr = nullptr;

    // hooks
    auto music_select_init_hook = safetyhook::InlineHook {};
    auto open_category_hook = safetyhook::MidHook {};
    auto close_category_hook = safetyhook::MidHook {};
    auto draw_bar_text_outer_hook = safetyhook::InlineHook {};
    auto draw_bar_text_inner_hook = safetyhook::InlineHook {};
    auto folder_open_voice_hook = safetyhook::MidHook {};
    auto folder_set_ticker_hook = safetyhook::MidHook {};
    auto setup_categories_hook = safetyhook::InlineHook {};
    auto save_category_hook = safetyhook::InlineHook {};
    auto cursor_lock_hook = safetyhook::InlineHook {};

    // utility
    auto is_valid_game_type()
    {
        // seems to crash in ARENA & BPL BATTLE, so don't run there
        if (state->game_type == GAME_MODE_ARENA || state->game_type == GAME_MODE_BPL_BATTLE)
            return false;

        // doesn't make sense to run in STEP UP as we recycle categories from there
        if (state->game_type == GAME_MODE_STEP_UP)
            return false;

        // probably fine in all other modes
        return true;
    }

    auto find_unused_category(CCategoryGameData* data)
    {
        auto occupied_ids = std::vector<decltype(category_t::id)> {};

        for (auto const& category: data->populated_categories[STYLE_SP])
        {
            if (!category)
                break;
            if (std::ranges::find(occupied_ids, category->id) == occupied_ids.end())
                occupied_ids.push_back(category->id);
        }

        for (auto const& category: data->populated_categories[STYLE_DP])
        {
            if (!category)
                break;
            if (std::ranges::find(occupied_ids, category->id) == occupied_ids.end())
                occupied_ids.push_back(category->id);
        }

        // find a category that isn't in use in both SP & DP
        for (auto const& id: good_ids)
            if (std::ranges::find(occupied_ids, id) == occupied_ids.end())
                return id;

        return -1;
    }

    auto backup_categories()
    {
        spdlog::debug("Backing up original categories...");

        std::memcpy(categories_backup, category_game_data->categories, sizeof(categories_backup));
        std::memcpy(populated_categories_backup, category_game_data->populated_categories, sizeof(populated_categories_backup));
        std::memcpy(bar_meta_backup, category_game_data->bar_meta, sizeof(bar_meta_backup));

        auto sp_count = 0;
        auto dp_count = 0;

        for (auto const& category: category_game_data->populated_categories[STYLE_SP])
        {
            if (!category)
                break;
            sp_count++;
        }

        for (auto const& category: category_game_data->populated_categories[STYLE_DP])
        {
            if (!category)
                break;
            dp_count++;
        }

        spdlog::debug("Backup created successfully. (SP: {}, DP: {})", sp_count, dp_count);

        backup_created = true;
    }

    auto restore_categories()
    {
        // revert category definition patches
        category_definition_patches.clear();

        // sanity check
        if (!backup_created)
            return spdlog::warn("Attempted to restore categories with no backup!");

        // do restore
        spdlog::debug("Restoring original categories...");
        std::memcpy(category_game_data->categories, categories_backup, sizeof(categories_backup));
        std::memcpy(category_game_data->populated_categories, populated_categories_backup, sizeof(populated_categories_backup));
        std::memcpy(category_game_data->bar_meta, bar_meta_backup, sizeof(bar_meta_backup));

        // bar count is for the current play style
        auto populated_bar_count = 0;

        for (auto const& category: category_game_data->populated_categories[state->play_style])
            if (category)
                populated_bar_count++;

        spdlog::debug("Setting populated bar count to {}...", populated_bar_count);
        category_game_data->bar_count = populated_bar_count;
    }

    auto add_music(category_t* category, music_entry_t* music, std::uint8_t difficulty, std::uint8_t bar_style = BAR_STYLE_NORMAL)
    {
        if (category->bar_count == 1)
        {
            category->entries[0].music_entry_ptr = music;
			category->entries[0].p1_difficulty = difficulty;
			category->entries[0].p1_difficulty_original = difficulty;
			category->entries[0].p2_difficulty = difficulty;
			category->entries[0].p2_difficulty_original = difficulty;
            for (auto& style: category->entries[0].bar_style)
                style = bar_style;

			category->bars[0].type = 1;
			category->bars[0].bar = &category->entries[0];

			category->bar_count = 2;
			category->song_count = 1;
            category->song_count_flag = 1;
        }
        else if (category->bar_count == 2)
        {
			category->entries[1].music_entry_ptr = music;
			category->entries[1].p1_difficulty = difficulty;
			category->entries[1].p1_difficulty_original = difficulty;
			category->entries[1].p2_difficulty = difficulty;
			category->entries[1].p2_difficulty_original = difficulty;
            for (auto& style: category->entries[1].bar_style)
                style = bar_style;

			// add the RANDOM SELECT bar
            category->bars[0].type = 2;
            category->bars[0].bar = reinterpret_cast<category_bar_t*>(&category->id2);

			// we just overwrote the bar for the first song, so add it again
			category->bars[1].type = 1;
			category->bars[1].bar = &category->entries[0];

			// then add the bar for the new song
			category->bars[2].type = 1;
			category->bars[2].bar = &category->entries[1];

			category->bar_count = 4;
			category->song_count = 2;
            category->song_count_flag = 2;
        }
        else if (category->bar_count >= 3)
        {
            auto bar_index = category->bar_count - 1;
			auto music_index = category->song_count;

			category->entries[music_index].music_entry_ptr = music;
			category->entries[music_index].p1_difficulty = difficulty;
			category->entries[music_index].p1_difficulty_original = difficulty;
			category->entries[music_index].p2_difficulty = difficulty;
			category->entries[music_index].p2_difficulty_original = difficulty;
            for (auto& style: category->entries[music_index].bar_style)
                style = bar_style;

			category->bars[bar_index].type = 1;
			category->bars[bar_index].bar = &category->entries[music_index];

			category->bar_count++;
			category->song_count++;
        }
    }

    auto add_music(category_t* category, const playlist_chart_t& chart)
        { add_music(category, chart.entry, chart.difficulty, chart.bar_style); }

    auto empty_category(category_t* category)
    {
        category->bar_count = 1;
        category->song_count = 0;
        category->song_count_flag = 0;

        for (auto& i: category->entries)
            i.music_entry_ptr = nullptr;

        for (auto i = 1; i < BAR_COUNT; ++i)
        {
            auto& bar = category->bars[i];

            if (bar.type == 0)
                break;

            bar.type = 0;
            bar.bar = nullptr;
        }
    }

    auto calculate_clear_lamps(CCategoryGameData* data, int style, int id)
    {
        auto category = &data->categories[style][id];
        auto bar_meta = &data->bar_meta[style][id];

        // set initial clear lamp for the category
        std::uint8_t lowest_clear[PLAYER_COUNT] = { CLEAR_FULL_COMBO, CLEAR_FULL_COMBO };

        spdlog::debug("Calculating clear lamps for category {}...",
            category_definitions->categories[category->bar_type].bar_bg_texture_name);

        for (auto entry: category->entries)
        {
            if (!entry.music_entry_ptr)
                break;

            auto const entry_id = entry.music_entry_ptr->entry_id;
            auto const difficulty = entry.p1_difficulty_original; // same for both players..?

            // calculate clear lamp for both players
            for (auto player = PLAYER_1; player < PLAYER_COUNT; player++)
            {
                // continue early if one player already has no lamp for this folder
                // as we might still have some charts to check for the other player
                if (lowest_clear[player] == CLEAR_NO_PLAY)
                    continue;

                // does a chart with this difficulty exist?
                if (entry.music_entry_ptr->rating[style][difficulty] == 0)
                    continue;

                // check against the players score data
                auto const scores = (player == PLAYER_1 ? player_scores_p1: player_scores_p2);
                auto const player_lamp = scores->scores[style][entry_id].clear_type[difficulty];

                if (player_lamp <= 0)
                    lowest_clear[player] = CLEAR_NO_PLAY; // no score on this, so no folder lamp for you
                else if (player_lamp < lowest_clear[player])
                    lowest_clear[player] = player_lamp; // lower the lamp if necessary
            }
        }

        bar_meta->p1_beginner_clear_lamp = lowest_clear[0];
        bar_meta->p2_beginner_clear_lamp = lowest_clear[1];
        bar_meta->p1_normal_clear_lamp = lowest_clear[0];
        bar_meta->p2_normal_clear_lamp = lowest_clear[1];
        bar_meta->p1_hyper_clear_lamp = lowest_clear[0];
        bar_meta->p2_hyper_clear_lamp = lowest_clear[1];
        bar_meta->p1_another_clear_lamp = lowest_clear[0];
        bar_meta->p2_another_clear_lamp = lowest_clear[1];
        bar_meta->p1_leggendaria_clear_lamp = lowest_clear[0];
        bar_meta->p2_leggendaria_clear_lamp = lowest_clear[1];

        spdlog::debug("Clear lamp for category {} = P1:{}, P2:{}",
            category_definitions->categories[category->bar_type].bar_bg_texture_name,
            lowest_clear[0], lowest_clear[1]);
    }

    auto create_populated_category(int style, int id, const playlist_t& playlist)
    {
        if (category_definition_patches.contains(id))
            { spdlog::error("Attempted to patch an already patched category definition!"); }
        else
        {
            category_definition_patches[id] = category_definition_patch(&category_definitions->categories[id]);
            category_definition_patches[id]->allow_difficulty_changes = true;
            category_definition_patches[id]->lock_difficulties = true;
            if (!config.get("custom textures", false) || playlist.bar_texture.empty())
                category_definition_patches[id]->bar_bg_texture_name = DEFAULT_BAR_TEXTURE;
            else
                category_definition_patches[id]->bar_bg_texture_name = playlist.bar_texture.data();
            category_definition_patches[id]->bar_text_texture_name = nullptr;
            category_definition_patches[id]->flags[0] = 2;
            category_definition_patches[id]->flags[1] = 0;
            category_definition_patches[id]->flags[2] = 0;
            category_definition_patches[id]->flags[3] = 1;
            spdlog::debug("Patching category definition {}... ({})", id, playlist.bar_texture);
        }

        // everything should use the same id internally
        category_game_data->categories[style][id].id = OVERRIDE_CATEGORY_ID;

        // set a reference back to the playlist in a (hopefully) unused field
        category_game_data->categories[style][id].userdata = (void*) &playlist;

        // make it appear as something else by setting the bar type
        category_game_data->categories[style][id].bar_type = id;

        // properly clean things out first
        empty_category(&category_game_data->categories[style][id]);

        for (auto const& chart: playlist.charts)
            add_music(&category_game_data->categories[style][id], chart);

        // clear badges set by the game
        category_game_data->bar_meta[style][id].weekly = false;
        category_game_data->bar_meta[style][id].todays_featured = false;
        category_game_data->bar_meta[style][id].bpl = false;
        category_game_data->bar_meta[style][id].venue_tournament = false;
        category_game_data->bar_meta[style][id].kac = false;

        // use player scores to build clear lamps for folders
        calculate_clear_lamps(category_game_data, style, id);

        // make the category appear
        category_game_data->populated_categories[style][id] = &category_game_data->categories[style][id];
    }

    // state management
    auto enter_playlist_mode()
    {
        if (!in_playlist_mode)
            spdlog::debug("Entering playlist mode...");
        else
            spdlog::debug("Re-entering playlist mode..."); // returning from result

        // enable all patches
        for (auto& patch: playlist_mode_patches)
            patch.enable();

        spdlog::debug("Enabled {} code patches.", playlist_mode_patches.size());

        in_playlist_mode = true;

        // patch the flags on the override category
        // we use 'song selection notes' which doesn't have the right settings for difficulty switching
        category_definition_patches[OVERRIDE_CATEGORY_ID] = category_definition_patch(&category_definitions->categories[OVERRIDE_CATEGORY_ID]);
        category_definition_patches[OVERRIDE_CATEGORY_ID]->allow_difficulty_changes = true;
        category_definition_patches[OVERRIDE_CATEGORY_ID]->lock_difficulties = true;
        spdlog::debug("Patching override category definition...");

        auto i = 0;

        for (auto const& playlist: playlist_data)
        {
            if (playlist.play_style != state->play_style)
                continue;

            create_populated_category(state->play_style, i, playlist);

            i++;

            if (i >= CATEGORY_COUNT)
                break;
        }

        // update amount of populated bars
        category_game_data->bar_count = i;
    }

    auto exit_playlist_mode()
    {
        in_playlist_mode = false;

        // restore original song wheel
        restore_categories();

        // disable all patches
        for (auto& patch: playlist_mode_patches)
            patch.disable();

        spdlog::debug("Disabled {} code patches.", playlist_mode_patches.size());
    }

    auto install_hooks()
    {
        // game context offsets
        state = reinterpret_cast<decltype(state)>(base + OFFSET_GAME_STATE);
        buttons = reinterpret_cast<decltype(buttons)>(base + OFFSET_BUTTON_STATE);
        music_data = reinterpret_cast<decltype(music_data)>(base + OFFSET_MUSIC_DATA);
        player_scores_p1 = reinterpret_cast<decltype(player_scores_p1)>(base + OFFSET_SCORES_P1);
        player_scores_p2 = reinterpret_cast<decltype(player_scores_p2)>(base + OFFSET_SCORES_P2);
        category_definitions = reinterpret_cast<decltype(category_definitions)>(base + OFFSET_CATEGORY_DEFS);
        application_config = reinterpret_cast<decltype(application_config) (*) ()>(base + OFFSET_GET_APP_CFG_FN)();

        // stuff beyond this point relies on pointers that aren't initialized shortly after game boot
        // they become valid around the time monitor check starts, so wait for it to finish before continuing
        while (application_config->monitor_check_fps == 1.f)
            { std::this_thread::sleep_for(1s); }

        // multi-level pointers, but definitely valid after monitor check
        category_game_data = *reinterpret_cast<decltype(category_game_data)*>(base + OFFSET_CATEGORY_DATA);
        music_select_game_data = *reinterpret_cast<decltype(music_select_game_data)*>(base + OFFSET_MSELECT_DATA);

        // backup the category voice stuff
        for (auto i = 0; i < CATEGORY_COUNT; i++)
        {
            // use the subheader texture name as the key as it's unique and always set
            auto const& key = std::string_view(category_definitions->categories[i].hover_subheader_texture_name);

            // none of them should be null, but may as well be on the safe side
            if (!key.data() || !key.starts_with("ss_"))
                continue;

            // strip off 'ss_' prefix and store in category_voices
            category_voices[key.substr(3)] = i;
        }

        // game functions
        reload_lamps = reinterpret_cast<decltype(reload_lamps)>(base + OFFSET_RELOAD_LAMPS_FN);
        sort_bars = reinterpret_cast<decltype(sort_bars)>(base + OFFSET_SORT_BARS_FN);
        play_system_sound = reinterpret_cast<decltype(play_system_sound)>(base + OFFSET_PLAY_SOUND_FN);

        // code patches that are turned on and off situationally
        playlist_mode_patches.emplace_back(base + BAR_EVENT_BADGE_PATCH, 0xE9, 0x84, 0x00, 0x00, 0x00, 0x90);
        playlist_mode_patches.emplace_back(base + BAR_CONTEST_TEXT_PATCH, 0xE9, 0xD3, 0x01, 0x00, 0x00, 0x90);
        playlist_mode_patches.emplace_back(base + BAR_CUSTOM_TEXT_PATCH, 0xEB, 0x18, 0x90, 0x90, 0x90, 0x90);
        playlist_mode_patches.emplace_back(base + BAR_EVENT_LAMP_PATCH, 0xEB);
        playlist_mode_patches.emplace_back(base + BAR_CONTEST_LAMP_PATCH, 0xEB);
        playlist_mode_patches.emplace_back(base + BAR_BATTLE_LAMP_PATCH, 0xE9, 0x39, 0x02, 0x00, 0x00, 0x90);
        playlist_mode_patches.emplace_back(base + RANDOM_SPLIT_LAMP_PATCH, 0xE9, 0x33, 0x02, 0x00, 0x00, 0x90);
        playlist_mode_patches.emplace_back(base + TOURISM_CRASH_FIX_PATCH, 0xE9, 0x97, 0x00, 0x00, 0x00, 0x90);

        //
        // called in various places but never while inside music select
        // useful place to reset things that will be set in future hooks
        //
        music_select_init_hook = safetyhook::create_inline(base + MSELECT_INIT_HOOK, +[]
        {
            // call the original
            mselect = music_select_init_hook.call<CMusicSelectScene*>();

            if (target_category_id != -1)
                spdlog::debug("Resetting music select state...");

            backup_created = false;   // "clean" category backup should be created once per music select init
            target_category_id = -1;  // ensure setup_categories_hook will re-select an appropriate category
            target_category_patch = category_definition_patch {};  // this should revert the old patch, if there was one

            // restore all category definitions settings to default
            category_definition_patches.clear();

            return mselect;
        });

        //
        // called when opening a category
        //
        open_category_hook = safetyhook::create_mid(base + OPEN_CATEGORY_HOOK,
            [] (SafetyHookContext& ctx)
        {
            // if we're already in playlist mode, don't do anything
            // categories in playlist mode use the same id, so going further would cause issues
            if (in_playlist_mode || !is_valid_game_type())
                return;

            // pull category pointer from stack
            auto const category = *reinterpret_cast<category_t**>(ctx.rsp + 0x20);

            // ignore if this isn't our "fake" category
            if (category->id != target_category_id)
                return;

            // prevent the folder from actually opening
            ctx.rflags |= 0x40;

            // play the folder sound
            play_system_sound(FOLDER_OPEN_SOUND_ID);

            // enter playlist mode
            enter_playlist_mode();

            // reset active bar
            mselect->bar_state.active_bar = 0;
            mselect->bar_state.bar_count = category_game_data->bar_count;

            // reload lamps
            spdlog::debug("Reloading folder lamps...");

            // only necessary when entering playlist mode from the fake category
            // without this, folder lamps would not be accurate until entering and exiting a category
            reload_lamps(&mselect->bar_state);
        });

        //
        // opens the sort menu when pressing black keys, used to exit playlist mode
        //
        close_category_hook = safetyhook::create_mid(base + CLOSE_CATEGORY_HOOK,
            [] (SafetyHookContext& ctx)
        {
            // check if we're in playlist mode
            if (!in_playlist_mode || ctx.rax == 0 || !is_valid_game_type())
                return;

            if (config.get("alternate close", false))
            {
                auto const bits = std::bitset<16>(*buttons);
                auto const count_p1 = bits[1] + bits[3] + bits[5];
                auto const count_p2 = bits[8] + bits[10] + bits[12];

                if (state->play_style == STYLE_SP)
                {
                    if (state->is_active[PLAYER_1] && count_p1 == 1) return;
                    if (state->is_active[PLAYER_2] && count_p2 == 1) return;

                    ctx.rax = 0;

                    if (state->is_active[PLAYER_1] && count_p1 < 3) return;
                    if (state->is_active[PLAYER_2] && count_p2 < 3) return;
                }
                else
                {
                    if (count_p1 == 1 || count_p2 == 1) return;

                    ctx.rax = 0;

                    if (count_p1 < 3 && count_p2 < 3) return;
                }
            }

            // prevent sort menu from appearing
            ctx.rax = 0;

            // play the folder sound
            play_system_sound(FOLDER_OPEN_SOUND_ID);

            // exit playlist mode
            spdlog::debug("Exiting playlist mode...");
            exit_playlist_mode();

            for (auto i = 0; i < CATEGORY_COUNT; ++i)
            {
                if (category_game_data->populated_categories[state->play_style][i] == nullptr)
                    continue;
                if (category_game_data->populated_categories[state->play_style][i]->id == target_category_id)
                {
                    spdlog::debug("Reselected inserted category.");
                    mselect->bar_state.active_bar = i;
                    break;
                }
            }

            mselect->bar_state.bar_count = category_game_data->bar_count;
        });

        //
        // gathers context for the next hook
        //
        draw_bar_text_outer_hook = safetyhook::create_inline(base + DRAW_BAR_TEXT_HOOK_A,
           +[] (std::int32_t* category_id, int a2, int a3, float a4, unsigned int a5)
        {
            // just store the category to use in inner hook
            text_category_id = *category_id;

            // call original function
            return draw_bar_text_outer_hook.call<std::int32_t*, void*, int, int, float, unsigned int>(category_id, a2, a3, a4, a5);
        });

        //
        // overrides folder name text rendering
        //
        draw_bar_text_inner_hook = safetyhook::create_inline(base + DRAW_BAR_TEXT_HOOK_B,
           +[] (int font, int x, int y, int flags, void* buffer, const char* text, float width)
        {
            if (in_playlist_mode || !is_valid_game_type())
            {
                // different font so lowercase characters can be used
                font = 15;

                // adjust position for the new font
                x -= 12;
                y += 2;
                width += 20.f;

                // resolve to the underlying playlist
                auto const& category = category_game_data->categories[state->play_style][text_category_id];
                auto const& playlist = static_cast<playlist_t*>(category.userdata);

                auto const has_texture = config.get("custom textures", false) && !playlist->bar_texture.empty();

                if (!has_texture && !playlist->name.empty())
                    text = playlist->name.c_str();
                else
                    const_cast<char*>(text)[0] = '\0';
            }

            // call the original to render it
            return draw_bar_text_inner_hook.call(font, x, y, flags, buffer, text, width);
        });

        //
        // resolves category id -> system sound id
        //
        folder_open_voice_hook = safetyhook::create_mid(base + FOLDER_VOICE_OPEN_HOOK,
            [] (SafetyHookContext& ctx)
        {
            if (!in_playlist_mode || !is_valid_game_type())
                return;

            // if voices are disabled, return something invalid so the game will use the default
            if (config.get<bool>("disable voice", false))
            {
                ctx.rdx = -1;
                return;
            }

            // avoid pulling the category pointer from the stack by using the active bar instead
            auto const& category = category_game_data->populated_categories[state->play_style][mselect->bar_state.active_bar];
            auto const& playlist = static_cast<playlist_t*>(category->userdata);

            if (!playlist->voice.empty() && category_voices.contains(playlist->voice))
                ctx.rdx = category_voices[playlist->voice];
        });

        //
        // updates the ticker text
        //
        folder_set_ticker_hook = safetyhook::create_mid(base + FOLDER_TICKER_TEXT_HOOK,
            [] (SafetyHookContext& ctx)
        {
            if (!in_playlist_mode || !is_valid_game_type())
                return;

            auto result = "SELECT FROM CUSTOM CATEGORY";

            auto const& category = category_game_data->populated_categories[state->play_style][mselect->bar_state.active_bar];
            auto const& playlist = static_cast<playlist_t*>(category->userdata);

            if (playlist && !playlist->ticker_text.empty())
                result = playlist->ticker_text.c_str();

            ctx.rcx = reinterpret_cast<std::uintptr_t>(result);
        });

        //
        // called when populating the category list
        //
        setup_categories_hook = safetyhook::create_inline(base + SETUP_CATEGORIES_HOOK,
            +[] (CCategoryGameData* a1, int a2) -> std::int64_t
        {
            if (!is_valid_game_type()) // only run in "safe" game modes to prevent crashes
                return setup_categories_hook.call<std::int64_t, CCategoryGameData*, int>(a1, a2);

            // run for the opposite style first, so we can take a backup
            if (!backup_created)
            {
                spdlog::debug("Setting up categories for {}...", state->play_style == STYLE_SP ? "DP": "SP");
                state->play_style = (state->play_style == STYLE_SP ? STYLE_DP: STYLE_SP);
                setup_categories_hook.call(a1, a2);
                state->play_style = (state->play_style == STYLE_SP ? STYLE_DP: STYLE_SP);
            }

            // call the original for the current play style to set up the "known good" categories
            auto result = setup_categories_hook.call<std::int64_t, CCategoryGameData*, int>(a1, a2);

            // find an unused category we can hijack. if there isn't one, we can't do anything
            // todo: probably set some disable flag and ensure every hook respects it if this happens
            if (target_category_id == -1)
                target_category_id = find_unused_category(a1);
            if (target_category_id == -1)
                return result;

            spdlog::debug("Setting up categories for {}...", state->play_style == STYLE_SP ? "SP": "DP");

            for (auto i = 0; i < STYLE_COUNT; ++i)
            {
                auto categories = a1->categories[i];
                auto populated_categories = a1->populated_categories[i];

                // if it already exists, just return the real value
                for (auto j = 0; j < CATEGORY_COUNT; j++)
                {
                    // nullptr means we reached the end
                    if (!populated_categories[j])
                        break;

                    if (populated_categories[j]->id == target_category_id)
                    {
                        // we can't stay in playlist mode after switching play styles
                        if (in_playlist_mode)
                        {
                            spdlog::debug("Switching play styles, exiting playlist mode...");
                            exit_playlist_mode();
                        }

                        // result is already accurate as this isn't the first call
                        return result;
                    }
                }

                // need to do our own count since we're doing both play styles here
                auto index = 0;

                for (auto const& category: category_game_data->populated_categories[i])
                {
                    if (!category)
                        break;

                    index++;
                }

                // clear it out (just in case)
                empty_category(&categories[target_category_id]);

                // add a bunch of dummy songs so that it matches the amount of playlists we have
                for (auto const& playlist: playlist_data)
                    if (playlist.play_style == i)
                        add_music(&categories[target_category_id], &music_data->first, DIFFICULTY_NORMAL);

                // insert it
                populated_categories[index] = &categories[target_category_id];

                spdlog::debug("Inserted category ID #{} into {} slot {}/{}.",
                    target_category_id, i == 0 ? "SP": "DP", index + 1, CATEGORY_COUNT);
            }

            // patch the category we just inserted
            {
                target_category_patch = category_definition_patch(&category_definitions->categories[target_category_id]);
                target_category_patch->hover_header_texture_name = "ss_music_memo";
                target_category_patch->hover_subheader_texture_name = "ss_music_selection_memo";
                target_category_patch->bar_text_texture_name = nullptr;
                target_category_patch->bar_bg_texture_name = "so_favorite";
                target_category_patch->ticker_select_text = "SELECT FROM CUSTOM CATEGORIES";

                // clear out folder lamp
                for (auto& style_bars: a1->bar_meta)
                {
                    style_bars[target_category_id].p1_beginner_clear_lamp = 0;
                    style_bars[target_category_id].p2_beginner_clear_lamp = 0;
                    style_bars[target_category_id].p1_normal_clear_lamp = 0;
                    style_bars[target_category_id].p2_normal_clear_lamp = 0;
                    style_bars[target_category_id].p1_hyper_clear_lamp = 0;
                    style_bars[target_category_id].p2_hyper_clear_lamp = 0;
                    style_bars[target_category_id].p1_another_clear_lamp = 0;
                    style_bars[target_category_id].p2_another_clear_lamp = 0;
                    style_bars[target_category_id].p1_leggendaria_clear_lamp = 0;
                    style_bars[target_category_id].p2_leggendaria_clear_lamp = 0;
                }
            }

            // backup category list AFTER we altered it
            backup_categories();

            // re-enter playlist mode if we were already in it
            if (in_playlist_mode)
            {
                enter_playlist_mode();
                return a1->bar_count;
            }

            // increment and return the new bar count
            spdlog::debug("Result bar count:   {}", result);
            spdlog::debug("Previous bar count: {}", a1->bar_count);
            a1->bar_count = 0;

            for (auto k = 0; k < CATEGORY_COUNT; ++k)
            {
                if (!a1->populated_categories[state->play_style][k])
                    break;
                a1->bar_count++;
            }
            spdlog::debug("New bar count:      {}", a1->bar_count);

            return a1->bar_count;
        });

        //
        // called when leaving music select
        //
        save_category_hook = safetyhook::create_inline(base + SAVE_CATEGORY_HOOK,
            +[] (void* a1, int a2)
        {
            last_category_id = -1;
            last_music_difficulty_p1 = -1;
            last_music_difficulty_p2 = -1;
            last_music_entry_ptr = nullptr;

            if (!in_playlist_mode || !is_valid_game_type())
                return save_category_hook.call<void*, void*, int>(a1, a2);

            for (auto i = 0; i < CATEGORY_COUNT; ++i)
            {
                auto category = category_game_data->populated_categories[state->play_style][i];

                if (!category)
                    break;

                if (category->is_open)
                {
                    spdlog::debug("Storing previous position...");

                    auto const index = category->active_bar - 1;

                    last_category_id = category->bar_type;
                    last_music_difficulty_p1 = category->bars[index].bar->p1_difficulty_original;
                    last_music_difficulty_p2 = category->bars[index].bar->p2_difficulty_original;
                    last_music_entry_ptr = category->bars[index].bar->music_entry_ptr;

                    break;
                }
            }

            return a1;
        });

        //
        // workaround for game opening the wrong category (because the ids are all the same)
        // todo: see how this works with/without cursor lock hex edit
        //
        cursor_lock_hook = safetyhook::create_inline(base + CURSOR_LOCK_HOOK,
            +[] (bar_state_t* bar_state, int a2, int a3, int a4, int a5, char a6, int a7)
        {
            // let the game handle it if we're not in playlist mode
            if (!in_playlist_mode || last_category_id == -1 || !is_valid_game_type())
                return cursor_lock_hook.call<void*, bar_state_t*, int, int, int, int, char, int>(bar_state, a2, a3, a4, a5, a6, a7);

            // setup categories has already been called, so every category should already be closed
            spdlog::debug("Searching for category ID #{}...", last_category_id);

            for (auto i = 0; i < CATEGORY_COUNT; ++i)
            {
                auto category = category_game_data->populated_categories[state->play_style][i];

                if (!category)
                    break;
                if (category->bar_type != last_category_id)
                    continue;

                // folder will not be respecting currently set sort mode upon returning from music select
                // we can call this function, and it'll re-arrange the bars for us... HOPEFULLY
                spdlog::debug("Sorting folder...");
                sort_bars(category, music_select_game_data->sort_mode, true);

                spdlog::debug("Searching for '{}'...", last_music_entry_ptr->title_ascii);

                for (auto j = 0; j < BAR_COUNT; ++j)
                {
                    if (!category->bars[j].bar)
                        break;
                    if (category->bars[j].bar->music_entry_ptr != last_music_entry_ptr)
                        continue;
                    if (category->bars[j].bar->p1_difficulty_original != last_music_difficulty_p1)
                        continue;
                    if (category->bars[j].bar->p2_difficulty_original != last_music_difficulty_p2)
                        continue;

                    spdlog::debug("Restored previous position successfully!");

                    category->is_open = true;
                    category->active_bar = 0;

                    // todo: actually test this (with and without cursor lock hex edit enabled)
                    if (config.get("cursor lock", false))
                        category->active_bar = j + 1;

                    // open this category folder
                    bar_state->active_bar = i;
                    bar_state->is_folder_open = true;

                    break;
                }

                break;
            }

            return (void*) category_game_data;
        });
    }
}

auto get_dll_path(HMODULE library)
{
    char path[MAX_PATH] = {};
    GetModuleFileNameA(library, path, MAX_PATH);
    return std::filesystem::path(path);
}

auto unload()
{
    spdlog::debug("Removing installed hooks...");

    if (playlister::in_playlist_mode)
        playlister::exit_playlist_mode();

    playlister::music_select_init_hook.reset();
    playlister::open_category_hook.reset();
    playlister::close_category_hook.reset();
    playlister::draw_bar_text_outer_hook.reset();
    playlister::draw_bar_text_inner_hook.reset();
    playlister::folder_open_voice_hook.reset();
    playlister::folder_set_ticker_hook.reset();
    playlister::setup_categories_hook.reset();
    playlister::save_category_hook.reset();
    playlister::cursor_lock_hook.reset();
}

auto DllMain(HMODULE dll_instance, DWORD reason, LPVOID) -> BOOL
{
	if (reason != DLL_PROCESS_ATTACH)
        return TRUE;

	DisableThreadLibraryCalls(dll_instance);

    CreateThread(nullptr, 0, [] (LPVOID param) -> DWORD
    {
        auto const dll_instance = static_cast<HMODULE>(param);
        auto const basedir = get_dll_path(dll_instance).parent_path().append("playlister");

        // parse configuration file
        playlister::config = util::config(basedir / "config.yml");

        // increase verbosity if set in config
        if (playlister::config.get("verbose log", false))
        {
            auto const path = (basedir / "playlister3.log").string();

            spdlog::default_logger()->sinks().emplace_back
                (std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true));

            spdlog::set_level(spdlog::level::debug);
            spdlog::flush_on(spdlog::level::debug);
        }

        spdlog::info("Playlister (v{}) loaded at 0x{:X}", RC_FILEVERSION_STRING, std::uintptr_t(dll_instance));
        spdlog::info("Compiled at {} {} from {}@{}", __DATE__, __TIME__, GIT_BRANCH_NAME, GIT_COMMIT_HASH_SHORT);

        // warn if ifs_hook.dll isn't loaded but custom textures are enabled
        if (!GetModuleHandleA("ifs_hook.dll") && playlister::config.get("custom textures", false))
            spdlog::warn("ifs_hook.dll not found! Custom texture loading may not function correctly.");

        // resolve the game library
        auto modules = std::vector<std::string> {};
        modules = playlister::config.get("game module", modules);

        if (modules.empty())
            modules.push_back(playlister::config.get("game module", "bm2dx.dll"s));

        if (modules.empty())
        {
            spdlog::error("At least one game module must be specified in the configuration file.");
            return EXIT_FAILURE;
        }

        for (auto const& module_name: modules)
        {
            playlister::base = util::library::from_name(module_name);

            if (!playlister::base.start())
                continue;

            spdlog::debug("Using game module '{}'...", module_name);
            break;
        }

        if (!playlister::base.start())
        {
            spdlog::error("Failed to find game module.");
            return EXIT_FAILURE;
        }

#ifdef _DEBUG
        // force trace level messages for debug builds.
        spdlog::set_level(spdlog::level::trace);
#endif

        playlister::install_hooks();
        load_playlists(basedir / "playlists", playlister::music_data);
        
        if (playlist_data.empty())
        {
            spdlog::error("No playlists found. Unloading from process...");
            unload();
            FreeLibraryAndExitThread(static_cast<HMODULE>(param), EXIT_FAILURE);
        }
        
        spdlog::trace("Init complete! Waiting for detach signal...");

#ifdef _DEBUG
        while (true)
        {
            if (GetAsyncKeyState(VK_SHIFT) && GetAsyncKeyState(VK_F10))
                break;
            Sleep(100);
        }

        unload();
        spdlog::debug("Unloading from process...");
        FreeLibraryAndExitThread(static_cast<HMODULE>(param), EXIT_SUCCESS);
#endif

        return EXIT_SUCCESS;
    }, dll_instance, 0, nullptr);

	return TRUE;
}