#include "screens.h"
#include "sprites.h"
#include "mistolito.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "SCREENS";

#define UI_ARENA_HEIGHT 160
#define UI_ARENA_Y 0
#define UI_STATS_HEIGHT 80
#define UI_STATS_Y 160
#define UI_PET_X 40
#define UI_PET_Y 50
#define UI_ENEMY_X 220
#define UI_ENEMY_Y 60

static lv_obj_t *screens[2] = {NULL, NULL};
static screen_id_e current_screen = SCREEN_INIT;

static lv_obj_t *lbl_init_title = NULL;
static lv_obj_t *lbl_init_hint = NULL;
static lv_obj_t *lbl_init_status = NULL;

static lv_obj_t *arena_bg = NULL;
static lv_obj_t *pet_sprite = NULL;
static lv_obj_t *enemy_sprite = NULL;
static lv_obj_t *enemy_shadow = NULL;
static lv_obj_t *pet_shadow = NULL;
static lv_obj_t *enemy_damage_label = NULL;
static lv_obj_t *pet_exp_label = NULL;
static lv_obj_t *enemy_name_label = NULL;
static lv_obj_t *enemy_hp_bar = NULL;
static lv_obj_t *combat_log_label = NULL;

static lv_obj_t *stats_bg = NULL;
static lv_obj_t *pet_name_label = NULL;
static lv_obj_t *level_label = NULL;
static lv_obj_t *exp_label = NULL;
static lv_obj_t *energy_label = NULL;
static lv_obj_t *dp_label = NULL;
static lv_obj_t *profession_label = NULL;
static lv_obj_t *stats_row = NULL;

static lv_style_t style_box;

static const char* state_str[] = {"INIT", "SEARCHING", "COMBAT", "VICTORY", "LEVELUP", "RESTING", "DEAD"};
static const char* prof_str[] = {"Novice", "Warrior", "Mage", "Rogue"};

static void init_styles(void)
{
    lv_style_init(&style_box);
    lv_style_set_bg_color(&style_box, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&style_box, LV_OPA_COVER);
    lv_style_set_border_width(&style_box, 1);
    lv_style_set_border_color(&style_box, lv_color_hex(0xFFFFFF));
    lv_style_set_radius(&style_box, 0);
    lv_style_set_pad_all(&style_box, 2);
}

static void create_init_screen(void)
{
    screens[SCREEN_INIT] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_INIT], lv_color_black(), 0);

    lbl_init_title = lv_label_create(screens[SCREEN_INIT]);
    lv_obj_set_style_text_color(lbl_init_title, lv_color_make(0x00, 0xFF, 0x00), 0);
    lv_obj_set_style_text_font(lbl_init_title, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_init_title, 5, 5);
    lv_label_set_text(lbl_init_title, "MistolitoRPG v2.0");

    lbl_init_hint = lv_label_create(screens[SCREEN_INIT]);
    lv_obj_set_style_text_color(lbl_init_hint, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_init_hint, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_init_hint, 5, 30);
    lv_label_set_text(lbl_init_hint, "Press BOOT to start");

    lbl_init_status = lv_label_create(screens[SCREEN_INIT]);
    lv_obj_set_style_text_color(lbl_init_status, lv_color_make(0xAA, 0xAA, 0xAA), 0);
    lv_obj_set_style_text_font(lbl_init_status, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_init_status, 5, 55);
    lv_label_set_text(lbl_init_status, "Waiting...");

    ESP_LOGI(TAG, "INIT screen created");
}

static void create_game_screen(void)
{
    screens[SCREEN_GAME] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_GAME], lv_color_black(), 0);

    lv_obj_t *scr = screens[SCREEN_GAME];

    arena_bg = lv_obj_create(scr);
    lv_obj_set_size(arena_bg, LCD_WIDTH, UI_ARENA_HEIGHT);
    lv_obj_set_pos(arena_bg, 0, UI_ARENA_Y);
    lv_obj_set_style_bg_color(arena_bg, lv_color_hex(0xEBCE87), 0);
    lv_obj_set_style_border_width(arena_bg, 0, 0);
    lv_obj_set_style_radius(arena_bg, 0, 0);
    lv_obj_clear_flag(arena_bg, LV_OBJ_FLAG_SCROLLABLE);

    sprites_create_arena_background(arena_bg);

    pet_shadow = lv_obj_create(arena_bg);
    lv_obj_set_size(pet_shadow, 60, 10);
    lv_obj_set_pos(pet_shadow, UI_PET_X + 6, UI_PET_Y + 95);
    lv_obj_set_style_bg_color(pet_shadow, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(pet_shadow, LV_OPA_30, 0);
    lv_obj_set_style_border_width(pet_shadow, 0, 0);
    lv_obj_set_style_radius(pet_shadow, 5, 0);

    pet_sprite = lv_animimg_create(arena_bg);
    lv_obj_set_size(pet_sprite, 72, 97);
    lv_obj_set_pos(pet_sprite, UI_PET_X, UI_PET_Y);
    sprites_set_idle_animation(pet_sprite);

    enemy_shadow = lv_obj_create(arena_bg);
    lv_obj_set_size(enemy_shadow, 45, 8);
    lv_obj_set_pos(enemy_shadow, UI_ENEMY_X + 2, UI_ENEMY_Y + 26);
    lv_obj_set_style_bg_color(enemy_shadow, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(enemy_shadow, LV_OPA_30, 0);
    lv_obj_set_style_border_width(enemy_shadow, 0, 0);
    lv_obj_set_style_radius(enemy_shadow, 5, 0);
    lv_obj_add_flag(enemy_shadow, LV_OBJ_FLAG_HIDDEN);

    enemy_sprite = lv_animimg_create(arena_bg);
    lv_obj_set_size(enemy_sprite, 50, 28);
    lv_obj_set_pos(enemy_sprite, UI_ENEMY_X, UI_ENEMY_Y);
    lv_obj_add_flag(enemy_sprite, LV_OBJ_FLAG_HIDDEN);

    enemy_damage_label = lv_label_create(arena_bg);
    lv_label_set_text(enemy_damage_label, "");
    lv_obj_set_style_text_color(enemy_damage_label, lv_color_hex(0xF800), 0);
    lv_obj_set_style_text_font(enemy_damage_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(enemy_damage_label, UI_ENEMY_X + 10, UI_ENEMY_Y - 25);
    lv_obj_add_flag(enemy_damage_label, LV_OBJ_FLAG_HIDDEN);

    pet_exp_label = lv_label_create(arena_bg);
    lv_label_set_text(pet_exp_label, "");
    lv_obj_set_style_text_color(pet_exp_label, lv_color_hex(0xFFE0), 0);
    lv_obj_set_style_text_font(pet_exp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(pet_exp_label, UI_PET_X, UI_PET_Y - 25);
    lv_obj_add_flag(pet_exp_label, LV_OBJ_FLAG_HIDDEN);

    enemy_name_label = lv_label_create(arena_bg);
    lv_label_set_text(enemy_name_label, "---");
    lv_obj_set_style_text_color(enemy_name_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(enemy_name_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(enemy_name_label, UI_ENEMY_X - 10, UI_ENEMY_Y + 35);

    enemy_hp_bar = lv_bar_create(arena_bg);
    lv_obj_set_size(enemy_hp_bar, 70, 8);
    lv_obj_set_pos(enemy_hp_bar, UI_ENEMY_X - 10, UI_ENEMY_Y + 53);
    lv_bar_set_range(enemy_hp_bar, 0, 100);
    lv_bar_set_value(enemy_hp_bar, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(enemy_hp_bar, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(enemy_hp_bar, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);

    combat_log_label = lv_label_create(arena_bg);
    lv_label_set_text(combat_log_label, "");
    lv_obj_set_style_text_color(combat_log_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(combat_log_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(combat_log_label, 5, 5);

    stats_bg = lv_obj_create(scr);
    lv_obj_set_size(stats_bg, LCD_WIDTH, UI_STATS_HEIGHT);
    lv_obj_set_pos(stats_bg, 0, UI_STATS_Y);
    lv_obj_add_style(stats_bg, &style_box, 0);
    lv_obj_clear_flag(stats_bg, LV_OBJ_FLAG_SCROLLABLE);

    pet_name_label = lv_label_create(stats_bg);
    lv_label_set_text(pet_name_label, "Mistolito");
    lv_obj_set_style_text_color(pet_name_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(pet_name_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(pet_name_label, 5, 2);

    level_label = lv_label_create(stats_bg);
    lv_label_set_text(level_label, "Lv.1");
    lv_obj_set_style_text_color(level_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(level_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(level_label, 75, 2);

    exp_label = lv_label_create(stats_bg);
    lv_label_set_text(exp_label, "XP:0%");
    lv_obj_set_style_text_color(exp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(exp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(exp_label, 115, 2);

    energy_label = lv_label_create(stats_bg);
    lv_label_set_text(energy_label, "EN:10");
    lv_obj_set_style_text_color(energy_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(energy_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(energy_label, 170, 2);

    dp_label = lv_label_create(stats_bg);
    lv_label_set_text(dp_label, "DP:0");
    lv_obj_set_style_text_color(dp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(dp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(dp_label, 225, 2);

    profession_label = lv_label_create(stats_bg);
    lv_label_set_text(profession_label, "NOVICE");
    lv_obj_set_style_text_color(profession_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(profession_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(profession_label, 5, 22);

    stats_row = lv_label_create(stats_bg);
    lv_label_set_text(stats_row, "Str 10 Dex 10 Con 10 Int 10 Wis 10 Cha 10");
    lv_obj_set_style_text_color(stats_row, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(stats_row, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(stats_row, 5, 42);

    ESP_LOGI(TAG, "GAME screen created");
}

void screens_init(void)
{
    init_styles();
    sprites_init();
    create_init_screen();
    create_game_screen();
    ESP_LOGI(TAG, "All screens initialized");
}

void screens_load(screen_id_e id)
{
    if (id >= sizeof(screens) / sizeof(screens[0]) || screens[id] == NULL) {
        ESP_LOGE(TAG, "Invalid screen id: %d", id);
        return;
    }

    lv_scr_load(screens[id]);
    current_screen = id;
    ESP_LOGI(TAG, "Loaded screen: %d", id);
}

void screens_update(game_snapshot_t *snap)
{
    static char buf[64];

    if (current_screen == SCREEN_INIT || snap == NULL) {
        return;
    }

    lv_label_set_text(pet_name_label, snap->pet.name);

    lv_snprintf(buf, sizeof(buf), "Lv.%d", snap->pet.level);
    lv_label_set_text(level_label, buf);

    uint8_t xp_pct = snap->pet.exp_next > 0 ? (snap->pet.exp * 100) / snap->pet.exp_next : 0;
    lv_snprintf(buf, sizeof(buf), "XP:%d%%", xp_pct);
    lv_label_set_text(exp_label, buf);

    lv_snprintf(buf, sizeof(buf), "EN:%d", snap->pet.energy);
    lv_label_set_text(energy_label, buf);

    lv_snprintf(buf, sizeof(buf), "DP:%lu", (unsigned long)snap->pet.dp);
    lv_label_set_text(dp_label, buf);

    const char *prof_name = (snap->pet.profession < 4) ? prof_str[snap->pet.profession] : "???";
    lv_label_set_text(profession_label, prof_name);

    lv_snprintf(buf, sizeof(buf), "Str %d Dex %d Con %d Int %d Wis %d Cha %d",
        snap->pet.str, snap->pet.dex, snap->pet.con,
        snap->pet.intel, snap->pet.wis, snap->pet.cha);
    lv_label_set_text(stats_row, buf);

    if (snap->encounter.count > 0 && snap->encounter.enemies[0].alive) {
        enemy_t *enemy = &snap->encounter.enemies[0];
        lv_obj_clear_flag(enemy_sprite, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(enemy_shadow, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(enemy_name_label, enemy->name);

        int16_t hp_pct = (enemy->hp * 100) / enemy->hp_max;
        lv_bar_set_value(enemy_hp_bar, hp_pct, LV_ANIM_ON);

        sprites_set_enemy_slime_animation(enemy_sprite);
    } else {
        lv_obj_add_flag(enemy_sprite, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(enemy_shadow, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(enemy_name_label, "---");
        lv_bar_set_value(enemy_hp_bar, 100, LV_ANIM_OFF);
    }

    if (snap->state < 7) {
        lv_label_set_text(combat_log_label, state_str[snap->state]);
    }
}

screen_id_e screens_get_current(void)
{
    return current_screen;
}

void screens_show_damage_popup(int16_t damage, bool is_critical)
{
    static char buf[16];
    if (is_critical) {
        lv_snprintf(buf, sizeof(buf), "%d!", damage);
    } else {
        lv_snprintf(buf, sizeof(buf), "%d", damage);
    }
    lv_label_set_text(enemy_damage_label, buf);
    lv_obj_clear_flag(enemy_damage_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(enemy_damage_label, lv_color_hex(0xF800), 0);
}

void screens_show_miss(void)
{
    lv_label_set_text(enemy_damage_label, "MISS");
    lv_obj_clear_flag(enemy_damage_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(enemy_damage_label, lv_color_hex(0xFFFFFF), 0);
}

void screens_clear_damage_popup(void)
{
    lv_obj_add_flag(enemy_damage_label, LV_OBJ_FLAG_HIDDEN);
}

void screens_show_exp_popup(uint32_t exp)
{
    static char buf[16];
    lv_snprintf(buf, sizeof(buf), "+%lu XP", (unsigned long)exp);
    lv_label_set_text(pet_exp_label, buf);
    lv_obj_set_style_text_color(pet_exp_label, lv_color_hex(0xFFE0), 0);
    lv_obj_clear_flag(pet_exp_label, LV_OBJ_FLAG_HIDDEN);
}

void screens_clear_exp_popup(void)
{
    lv_obj_add_flag(pet_exp_label, LV_OBJ_FLAG_HIDDEN);
}

void screens_set_pet_animation(int anim_type)
{
    switch (anim_type) {
        case 0:
            sprites_set_idle_animation(pet_sprite);
            break;
        case 1:
            sprites_set_attack_animation(pet_sprite);
            break;
        case 2:
            sprites_set_hit_animation(pet_sprite);
            break;
        case 3:
            sprites_set_death_animation(pet_sprite);
            break;
        case 4:
            sprites_set_levelup_animation(pet_sprite);
            break;
        default:
            sprites_set_idle_animation(pet_sprite);
            break;
    }
}

void screens_set_enemy_animation(int anim_type)
{
    switch (anim_type) {
        case 0:
            sprites_set_enemy_slime_animation(enemy_sprite);
            break;
        case 1:
            sprites_set_enemy_fish_animation(enemy_sprite);
            break;
        case 2:
            sprites_set_enemy_fly_animation(enemy_sprite);
            break;
        case 3:
            sprites_set_enemy_blocker_animation(enemy_sprite);
            break;
        case 4:
            sprites_set_enemy_poker_animation(enemy_sprite);
            break;
        default:
            sprites_set_enemy_slime_animation(enemy_sprite);
            break;
    }
}
