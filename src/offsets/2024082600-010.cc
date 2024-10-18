versions.push_back({
    .GAME_VERSION            = "2024082600-010",

    .DLL_CODE_SIZE           = 0xBE2400,
    .DLL_ENTRYPOINT          = 0xA800BC,
    .DLL_IMAGE_SIZE          = 0x79C9000,

    .PLAY_SOUND_FN           = base + 0x0580E30,
    .RELOAD_LAMPS_FN         = base + 0x05BFAB0,
    .SORT_BARS_FN            = base + 0x07F2C90,
    .GET_MUSIC_DATA_FN       = base + 0x08EB270,
    .GET_APP_CFG_FN          = base + 0x08D1090,
    .CATEGORY_DEFS           = base + 0x0F8B860,

    .DRAW_BAR_TEXT_HOOK_B    = base + 0x05AC2D0,
    .DRAW_BAR_TEXT_HOOK_A    = base + 0x05B3690,
    .FOLDER_VOICE_OPEN_HOOK  = base + 0x05B9612,
    .OPEN_CATEGORY_HOOK      = base + 0x05BFC86,
    .CLOSE_CATEGORY_HOOK     = base + 0x05C0019,
    .FOLDER_TICKER_TEXT_HOOK = base + 0x05C087B,
    .CURSOR_LOCK_HOOK        = base + 0x05C0C30,
    .SETUP_CATEGORIES_HOOK   = base + 0x07F0E40,
    .SAVE_CATEGORY_HOOK      = base + 0x081D900,
    .MSELECT_EV2_INIT_HOOK   = base + 0x0906930,
    .MSELECT_INIT_HOOK       = base + 0x0907360,
    .RESET_STATE_HOOK        = base + 0x08E34F0,

    .BAR_CUSTOM_TEXT_PATCH   = base + 0x05B3743,
    .BAR_CONTEST_TEXT_PATCH  = base + 0x05B3A11,
    .BAR_TOURISM_BADGE_PATCH = base + 0x05B3B81,
    .BAR_CONTEST_LAMP_PATCH  = base + 0x05B3D07,
    .BAR_BATTLE_LAMP_PATCH   = base + 0x05B3DFB,
    .BAR_SPLIT_LAMP_PATCH    = base + 0x05B539D,
    .RANDOM_SPLIT_LAMP_PATCH = base + 0x05B713E,

    .SCORES_P1               = base + 0x2C3D084,
    .SCORES_P2               = base + 0x496BDDC,
    .CATEGORY_DATA           = base + 0x69FAC50,
    .MSELECT_DATA            = base + 0x69FAC68,
    .GAME_STATE              = base + 0x6C1ED80,
    .BUTTON_STATE            = base + 0x70DDF3C,
});