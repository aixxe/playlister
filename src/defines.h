#pragma once

#include <windows.h>
#include <offsets.h>

// category id to replace
#define OVERRIDE_CATEGORY_ID    191

// bar texture to use for custom playlists
#define DEFAULT_BAR_TEXTURE     "so_b_playlist"

#define FOLDER_OPEN_SOUND_ID    0x30E0

#define CATEGORY_COUNT          218
#define BAR_COUNT               7801

#define PLAYER_1                0
#define PLAYER_2                1
#define PLAYER_COUNT            2

#define STYLE_SP                0
#define STYLE_DP                1
#define STYLE_COUNT             2

#define MAX_ENTRIES             32000

#define DIFFICULTY_BEGINNER     0
#define DIFFICULTY_NORMAL       1
#define DIFFICULTY_HYPER        2
#define DIFFICULTY_ANOTHER      3
#define DIFFICULTY_LEGGENDARIA  4

#define CLEAR_NO_PLAY           0
#define CLEAR_FAILED            1
#define CLEAR_ASSIST_EASY       2
#define CLEAR_EASY              3
#define CLEAR_NORMAL            4
#define CLEAR_HARD              5
#define CLEAR_EX_HARD           6
#define CLEAR_FULL_COMBO        7

#define BAR_STYLE_NORMAL        0
#define BAR_STYLE_RAINBOW       1
#define BAR_STYLE_LIGHTNING     2

#define GAME_MODE_STEP_UP       7
#define GAME_MODE_ARENA         9
#define GAME_MODE_BPL_BATTLE    10

// categories that are okay to override on the main song select list
// we don't want playlists in step up mode, so all of these are from there
auto static constexpr good_ids = {
    155, // so_setpiece
    156, // so_setpiece_ura
    157, // ss_stup_dif1
    158, // ss_stup_dif2
    159, // ss_stup_dif3
    160, // ss_stup_dif4
    161, // ss_stup_dif5
    162, // ss_stup_dif6
    163, // ss_stup_dif7
    164, // ss_stup_dif8
    165, // ss_stup_dif9
    166, // ss_stup_dif10
    167, // ss_stup_dif11
    168, // ss_stup_dif12
    169, // so_class_mogi_7kyu
    170, // so_class_mogi_6kyu
    171, // so_class_mogi_5kyu
    172, // so_class_mogi_4kyu
    173, // so_class_mogi_3kyu
    174, // so_class_mogi_2kyu
    175, // so_class_mogi_1kyu
    176, // so_class_mogi_syodan
    177, // so_class_mogi_2dan
    178, // so_class_mogi_3dan
    179, // so_class_mogi_4dan
    180, // so_class_mogi_5dan
    181, // so_class_mogi_6dan
    182, // so_class_mogi_7dan
    183, // so_class_mogi_8dan
    184, // so_class_mogi_9dan
    185, // so_class_mogi_10dan
    186, // so_class_mogi_11dan
    187, // so_class_mogi_12dan
};

struct bar_state_t
{
    std::int32_t bar_count; //0x0000
    std::uint8_t pad_0004[4]; //0x0004
    std::int32_t active_bar; //0x0008
    std::int16_t is_folder_open; //0x000C
    std::uint8_t pad_000E[2]; //0x000E
}; static_assert(sizeof(bar_state_t) == 0x10);

struct category_definition_t
{
    std::int8_t allow_difficulty_changes; //0x0000 completely locks ability to change difficulty
    std::int8_t lock_difficulties; //0x0001 will always change back to original_difficulty when moving to next bar
    std::uint8_t pad_0002[6]; //0x0002
    const char* hover_header_texture_name; //0x0008
    const char* hover_subheader_texture_name; //0x0010
    const char* bar_text_texture_name; //0x0018
    const char* bar_bg_texture_name; //0x0020
    const char* ticker_select_text; //0x0028
    std::int32_t flags[4]; //0x0030
}; static_assert(sizeof(category_definition_t) == 0x40);

struct category_definitions_t
{
    std::uint8_t pad_0000[8]; //0x0000
    category_definition_t categories[CATEGORY_COUNT]; //0x0008
}; static_assert(sizeof(category_definitions_t) == 0x3688);

struct notes_radar_t
{
    std::int32_t notes; //0x0000
    std::int32_t peak; //0x0004
    std::int32_t scratch; //0x0008
    std::int32_t soflan; //0x000C
    std::int32_t charge; //0x0010
    std::int32_t chord; //0x0014
}; static_assert(sizeof(notes_radar_t) == 0x18);

struct music_entry_t
{
    const char title[64]; //0x0000
    const char title_ascii[64]; //0x0040
    const char genre[64]; //0x0080
    const char artist[64]; //0x00C0
    std::int32_t texture_title; //0x0100
    std::int32_t texture_artist; //0x0104
    std::int32_t texture_genre; //0x0108
    std::int32_t texture_load; //0x010C
    std::int32_t texture_list; //0x0110
    std::int32_t font_idx; //0x0114
    std::uint16_t game_version; //0x0118
    std::uint16_t other_folder; //0x011A
    std::uint16_t bemani_folder; //0x011C
    std::uint16_t splittable_diff; //0x011E
    std::uint8_t rating[2][5]; //0x0120
    std::uint8_t pad_012A[6]; //0x012A
    std::uint32_t bpm[2][5][2]; //0x0130 max bpm first, then min bpm
    std::uint8_t pad_0180[48]; //0x0180
    std::int32_t notes[2][5]; //0x01B0
    std::uint8_t pad_01D8[88]; //0x01D8
    notes_radar_t notes_radar[2][5]; //0x0230 0:B, 1:N, 2:H, 3:A, 4:L
    std::uint8_t pad_0320[144]; //0x0320
    std::uint32_t entry_id; //0x03B0
    std::int32_t volume; //0x03B4
    std::uint8_t pad_03B8[372]; //0x03B8
}; static_assert(sizeof(music_entry_t) == 0x52C);

struct music_data_t
{
    std::uint8_t header[4]; //0x0000 "IIDX"
    std::uint32_t game_version; //0x0004
    std::uint16_t occupied_entries; //0x0008
    std::uint16_t maximum_entries; //0x000A
    std::uint8_t pad_000C[64004]; //0x000C
    music_entry_t first; //0xFA10
}; static_assert(sizeof(music_data_t) == 0xFF3C);

struct category_bar_t
{
	music_entry_t* music_entry_ptr; //0x0000
	std::int32_t p1_difficulty; //0x0008
	std::int32_t p2_difficulty; //0x000C
	std::int32_t p1_difficulty_original; //0x0010
	std::int32_t p2_difficulty_original; //0x0014
    std::uint8_t pad_0018[4]; //0x0018
	std::int32_t category_id; //0x001C
	std::int32_t bar_style[5]; //0x0020
    std::uint8_t pad_0034[12]; //0x0034
}; static_assert(sizeof(category_bar_t) == 0x40);

struct category_bar_list_t
{
	std::int32_t type; //0x0000 2=random 1=used 0=unused
	std::uint8_t pad_0004[4]; //0x0004
    category_bar_t* bar; //0x0008
}; static_assert(sizeof(category_bar_list_t) == 0x10);

struct category_meta_t
{
    std::int32_t id; //0x0000
    std::uint8_t pad_0004[12]; //0x0004
    void* userdata; //0x0010
    std::uint8_t pad_0018[24]; //0x0018
    std::int32_t id2; //0x0030
    std::uint8_t pad_0034[12]; //0x0034
}; static_assert(sizeof(category_meta_t) == 0x40);

struct category_t
{
	std::int32_t id; //0x0000
	std::int32_t is_open; //0x0004
	std::int32_t bar_count; //0x0008
	std::int32_t song_count; //0x000C
	std::int32_t song_count_flag; //0x0010 1 if only 1 song, 2 if >1
	std::int32_t active_bar; //0x0014 only set if category is open
	std::int32_t selected_difficulty; //0x0018
    std::uint8_t pad_001C[20]; //0x001C
	category_bar_list_t bars[BAR_COUNT + 1]; //0x0030
    category_meta_t meta; //0x1E7D0
    category_bar_t entries[BAR_COUNT]; //0x1E810
}; static_assert(sizeof(category_t) == 0x98650);

struct category_bar_meta_t
{
	std::int8_t weekly; //0x0000
	std::int8_t todays_featured; //0x0001
	std::int8_t bpl; //0x0002
	std::int8_t venue_tournament; //0x0003
	std::int8_t kac; //0x0004
    std::int8_t pad_0005[3]; //0x0005
	std::int32_t p1_beginner_clear_lamp; //0x0008
	std::int32_t p2_beginner_clear_lamp; //0x000C
	std::int32_t p1_normal_clear_lamp; //0x0010
	std::int32_t p2_normal_clear_lamp; //0x0014
	std::int32_t p1_hyper_clear_lamp; //0x0018
	std::int32_t p2_hyper_clear_lamp; //0x001C
	std::int32_t p1_another_clear_lamp; //0x0020
	std::int32_t p2_another_clear_lamp; //0x0024
	std::int32_t p1_leggendaria_clear_lamp; //0x0028
	std::int32_t p2_leggendaria_clear_lamp; //0x002C
    std::int8_t pad_0030[120]; //0x0030
}; static_assert(sizeof(category_bar_meta_t) == 0xA8);

struct CCategoryGameData
{
    std::uint8_t pad_0000[16]; //0x0000
    category_t categories[2][CATEGORY_COUNT]; //0x0010
    category_t* populated_categories[2][CATEGORY_COUNT]; //0x1038C050
	std::int32_t bar_count; //0x1038CDF0 for current play style
    category_bar_meta_t bar_meta[2][CATEGORY_COUNT]; //0x1038CDF4
    std::uint8_t pad_1039EC14[4]; //0x1039EC14
}; static_assert(sizeof(CCategoryGameData) == 0x1039EC18);

struct CMusicSelectGameData
{
    std::uint8_t pad_0000[12]; //0x0000
	std::int32_t last_category; //0x000C
    std::uint8_t pad_0010[4]; //0x0010
    std::int32_t sort_mode; //0x0014
}; static_assert(sizeof(CMusicSelectGameData) == 0x18);

struct CMusicSelectScene
{
    std::uint8_t pad_0000[1184]; //0x0000
	bar_state_t bar_state; //0x04A0
}; static_assert(sizeof(CMusicSelectScene) == 0x4B0);

struct CEvent2MusicSelectScene
{
    std::uint8_t pad_0000[1168]; //0x0000
	bar_state_t bar_state; //0x0490
}; static_assert(sizeof(CEvent2MusicSelectScene) == 0x4A0);

struct CApplicationConfig
{
    std::uint8_t pad_0000[28]; //0x0000
    std::float_t monitor_check_fps; //0x001C
}; static_assert(sizeof(CApplicationConfig) == 0x20);

struct state_t
{
    std::int32_t game_type; //0x0000
    std::int32_t play_style; //0x0004
    std::int32_t active_difficulty[2]; //0x0008 0:b, 1:n, 2:h, 3:a, 4:l
    std::int32_t is_active[2]; //0x0010
    std::uint8_t pad_0018[24]; //0x0018
    music_entry_t* active_music; //0x0030
}; static_assert(sizeof(state_t) == 0x38);

struct score_t
{
    std::int32_t ex_score[5]; //0x0000
    std::int32_t miss_count[5]; //0x0014
    std::int8_t clear_type[5]; //0x0028
    std::int8_t is_populated; //0x002D
    std::uint8_t pad_002E[2]; //0x002E
}; static_assert(sizeof(score_t) == 0x30);

struct player_scores_t
{
    score_t scores[PLAYER_COUNT][MAX_ENTRIES]; //0x0000
}; static_assert(sizeof(player_scores_t) == 0x2EE000);