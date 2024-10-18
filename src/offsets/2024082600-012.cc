versions.push_back({
    .GAME_VERSION            = "2024082600-012",

    .DLL_CODE_SIZE           = 0xB13600,
    .DLL_ENTRYPOINT          = 0x9B323C,
    .DLL_IMAGE_SIZE          = 0x78B4000,

    .PLAY_SOUND_FN           = base + 0x04B3440,
    .RELOAD_LAMPS_FN         = base + 0x04F22C0,
    .SORT_BARS_FN            = base + 0x0725A00,
    .GET_MUSIC_DATA_FN       = base + 0x081E170,
    .GET_APP_CFG_FN          = base + 0x0803F70,
    .CATEGORY_DEFS           = base + 0x0E8B860,

    .DRAW_BAR_TEXT_HOOK_B    = base + 0x04DEAE0,
    .DRAW_BAR_TEXT_HOOK_A    = base + 0x04E5EA0,
    .FOLDER_VOICE_OPEN_HOOK  = base + 0x04EBE22,
    .OPEN_CATEGORY_HOOK      = base + 0x04F2496,
    .CLOSE_CATEGORY_HOOK     = base + 0x04F2829,
    .FOLDER_TICKER_TEXT_HOOK = base + 0x04F308B,
    .CURSOR_LOCK_HOOK        = base + 0x04F3440,
    .SETUP_CATEGORIES_HOOK   = base + 0x0723BB0,
    .SAVE_CATEGORY_HOOK      = base + 0x0750670,
    .MSELECT_EV2_INIT_HOOK   = base + 0x0839830,
    .MSELECT_INIT_HOOK       = base + 0x083A260,
    .RESET_STATE_HOOK        = base + 0x08163F0,

    .BAR_CUSTOM_TEXT_PATCH   = base + 0x04E5F53,
    .BAR_CONTEST_TEXT_PATCH  = base + 0x04E6221,
    .BAR_TOURISM_BADGE_PATCH = base + 0x04E6391,
    .BAR_CONTEST_LAMP_PATCH  = base + 0x04E6517,
    .BAR_BATTLE_LAMP_PATCH   = base + 0x04E660B,
    .BAR_SPLIT_LAMP_PATCH    = base + 0x04E7BAD,
    .RANDOM_SPLIT_LAMP_PATCH = base + 0x04E994E,

    .SCORES_P1               = base + 0x2B3C084,
    .SCORES_P2               = base + 0x486ADDC,
    .CATEGORY_DATA           = base + 0x68F9C10,
    .MSELECT_DATA            = base + 0x68F9C28,
    .GAME_STATE              = base + 0x6B1DD40,
    .BUTTON_STATE            = base + 0x6FDCEFC,
});