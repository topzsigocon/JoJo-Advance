#include "bn_core.h"
#include "bn_keypad.h"

#include <cstdint>
#include <cstring>

// JoJo-inspired GBA fighting-game prototype.
// Butano provides the C++ GBA runtime/game loop; this first milestone uses
// Mode 3 framebuffer primitives so it needs no ripped or third-party art.
namespace
{
    constexpr int SCREEN_W = 240;
    constexpr int SCREEN_H = 160;
    constexpr uint16_t COLOR_BG = 0x1084;
    constexpr uint16_t COLOR_PANEL = 0x18E7;
    constexpr uint16_t COLOR_GOLD = 0x03FF;
    constexpr uint16_t COLOR_WHITE = 0x7FFF;
    constexpr uint16_t COLOR_RED = 0x001F;
    constexpr uint16_t COLOR_GREEN = 0x03E0;
    constexpr uint16_t COLOR_BLUE = 0x7C00;
    constexpr uint16_t COLOR_DARK = 0x0000;
    volatile uint16_t* const VRAM = reinterpret_cast<volatile uint16_t*>(0x06000000);
    volatile uint16_t& REG_DISPCNT = *reinterpret_cast<volatile uint16_t*>(0x04000000);

    enum class Screen { TITLE, SELECT, BATTLE };
    Screen screen = Screen::TITLE;

    struct FighterInfo { const char* name; const char* part; uint16_t color; bool playable; };
    FighterInfo roster[] = {
        {"JONATHAN", "PART 1", COLOR_BLUE, true}, {"DIO BRANDO", "PART 1", COLOR_RED, true},
        {"JOSEPH", "PART 2", COLOR_GREEN, true}, {"CAESAR", "PART 2", COLOR_BLUE, false},
        {"LISA LISA", "PART 2", COLOR_RED, false}, {"KARS", "PART 2", COLOR_GREEN, false},
        {"WAMUU", "PART 2", COLOR_BLUE, false}, {"ESIDISI", "PART 2", COLOR_RED, false},
        {"STROHEIM", "PART 2", COLOR_GREEN, false}, {"JOTARO", "PART 3", COLOR_BLUE, true},
        {"DIO", "PART 3", COLOR_RED, true}, {"OLD JOSEPH", "PART 3", COLOR_GREEN, false},
        {"KAKYOIN", "PART 3", COLOR_BLUE, false}, {"POLNAREFF", "PART 3", COLOR_RED, false},
        {"AVDOL", "PART 3", COLOR_GREEN, false}, {"IGGY", "PART 3", COLOR_BLUE, false},
        {"HOL HORSE", "PART 3", COLOR_RED, false}, {"VANILLA ICE", "PART 3", COLOR_GREEN, false},
        {"JOSUKE", "PART 4", COLOR_BLUE, false}, {"OKUYASU", "PART 4", COLOR_RED, false},
        {"KOICHI", "PART 4", COLOR_GREEN, false}, {"ROHAN", "PART 4", COLOR_BLUE, false},
        {"KIRA", "PART 4", COLOR_RED, false}, {"SHIGEKIYO", "PART 4", COLOR_GREEN, false},
        {"YUKAKO", "PART 4", COLOR_BLUE, false}, {"GIORNO", "PART 5", COLOR_RED, false},
        {"BRUNO", "PART 5", COLOR_GREEN, false}, {"MISTA", "PART 5", COLOR_BLUE, false},
        {"NARANCIA", "PART 5", COLOR_RED, false}, {"FUGO", "PART 5", COLOR_GREEN, false},
        {"ABBACCHIO", "PART 5", COLOR_BLUE, false}, {"DIAVOLO", "PART 5", COLOR_RED, false},
        {"RISOTTO", "PART 5", COLOR_GREEN, false}, {"JOLYNE", "PART 6", COLOR_BLUE, false},
        {"ERMES", "PART 6", COLOR_RED, false}, {"FOO FIGHTERS", "PART 6", COLOR_GREEN, false},
        {"WEATHER", "PART 6", COLOR_BLUE, false}, {"PUCCI", "PART 6", COLOR_RED, false},
        {"ANASUI", "PART 6", COLOR_GREEN, false}, {"EMPORIO", "PART 6", COLOR_BLUE, false},
        {"JOHNNY", "PART 7", COLOR_RED, false}, {"GYRO", "PART 7", COLOR_GREEN, false},
        {"DIEGO", "PART 7", COLOR_BLUE, false}, {"VALENTINE", "PART 7", COLOR_RED, false},
        {"HOT PANTS", "PART 7", COLOR_GREEN, false}, {"MOUNTAIN TIM", "PART 7", COLOR_BLUE, false},
        {"GAPPY", "PART 8", COLOR_RED, false}, {"JOBIN", "PART 8", COLOR_GREEN, false},
        {"YASUHO", "PART 8", COLOR_BLUE, false}, {"RAI", "PART 8", COLOR_RED, false}
    };
    constexpr int ROSTER_COUNT = sizeof(roster) / sizeof(roster[0]);
    int selected = 0;
    int current_part = 1;
    int p1 = 0;
    int p2 = 1;
    int selection_stage = 0;
    int p1_x = 58;
    int p2_x = 180;
    int p1_hp = 100;
    int p2_hp = 100;
    int p1_y = 116;
    int p2_y = 116;
    int p1_jump = 0;
    int attack_timer = 0;
    int enemy_attack_timer = 0;
    int frame = 0;
    bool guarding = false;
    bool p1_facing_right = true;

    uint16_t rgb(int r, int g, int b)
    {
        return static_cast<uint16_t>((r & 31) | ((g & 31) << 5) | ((b & 31) << 10));
    }

    void pixel(int x, int y, uint16_t color)
    {
        if(x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H)
            VRAM[y * SCREEN_W + x] = color;
    }

    void rect(int x, int y, int w, int h, uint16_t color)
    {
        for(int yy = y; yy < y + h; ++yy)
            for(int xx = x; xx < x + w; ++xx)
                pixel(xx, yy, color);
    }

    void clear(uint16_t color = COLOR_BG)
    {
        for(int i = 0; i < SCREEN_W * SCREEN_H; ++i) VRAM[i] = color;
    }

    // Compact 5x7 uppercase/digit font, drawn directly to the GBA framebuffer.
    const uint8_t* glyph(char c)
    {
        static const uint8_t blank[7] = {0,0,0,0,0,0,0};
        static const uint8_t letters[36][7] = {
            {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
            {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
            {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
            {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
            {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
            {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
            {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
            {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
            {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
            {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
            {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
            {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
            {14,17,17,15,1,1,14}
        };
        if(c >= 'A' && c <= 'Z') return letters[c - 'A'];
        if(c >= '0' && c <= '9') return letters[26 + c - '0'];
        return blank;
    }

    void text(int x, int y, const char* value, uint16_t color, int scale = 1)
    {
        for(int i = 0; value[i]; ++i)
        {
            const uint8_t* rows = glyph(value[i]);
            for(int yy = 0; yy < 7; ++yy)
                for(int xx = 0; xx < 5; ++xx)
                    if(rows[yy] & (1 << (4 - xx))) rect(x + xx * scale, y + yy * scale, scale, scale, color);
            x += 6 * scale;
        }
    }

    void centered(int y, const char* value, uint16_t color, int scale = 1)
    {
        int length = 0; while(value[length]) ++length;
        text((SCREEN_W - length * 6 * scale + scale) / 2, y, value, color, scale);
    }

    void draw_fighter(int x, int y, uint16_t main_color, bool facing_right, bool attacking)
    {
        // Original abstract pixel fighter silhouette; placeholder art, not a copied character sprite.
        int dir = facing_right ? 1 : -1;
        rect(x - 7, y - 34, 14, 12, rgb(24, 19, 15));
        rect(x - 9, y - 22, 18, 21, main_color);
        rect(x - 10, y - 2, 8, 15, rgb(8, 8, 12));
        rect(x + 2, y - 2, 8, 15, rgb(8, 8, 12));
        rect(x - 15, y - 20, 6, 17, main_color);
        rect(x + 9, y - 20, 6, 17, main_color);
        rect(x - 8, y - 35, 16, 3, COLOR_GOLD);
        if(attacking) rect(x + dir * 13 - 2, y - 19, 18, 5, COLOR_GOLD);
        rect(x - 5, y - 16, 10, 2, COLOR_WHITE);
    }

    void draw_header(const char* title)
    {
        rect(0, 0, SCREEN_W, 18, COLOR_DARK);
        centered(5, title, COLOR_GOLD);
        rect(0, 18, SCREEN_W, 1, COLOR_GOLD);
    }

    void draw_title()
    {
        clear(rgb(3, 5, 13));
        rect(8, 22, 224, 116, rgb(8, 10, 22));
        rect(8, 22, 224, 2, COLOR_GOLD);
        rect(8, 136, 224, 2, COLOR_GOLD);
        centered(36, "JOJO ALLSTAR BATTLE", COLOR_GOLD, 2);
        centered(62, "GBA HOMEBREW PROTOTYPE", COLOR_WHITE);
        rect(45, 82, 150, 1, rgb(12, 18, 28));
        centered(96, "PRESS START", COLOR_WHITE);
        centered(116, "50 FIGHTERS PLANNED", rgb(18, 24, 31));
    }

    void draw_select()
    {
        clear(rgb(4, 6, 15));
        draw_header("FIGHTER SELECT");
        // Roster grid: 5 columns x 2 rows; L/R cycles through the 50-entry list.
        int visible[10];
        int visible_count = 0;
        for(int i = 0; i < ROSTER_COUNT && visible_count < 10; ++i)
            if(roster[i].part[5] == char('0' + current_part)) visible[visible_count++] = i;
        for(int slot = 0; slot < 10; ++slot)
        {
            int col = slot % 5;
            int row = slot / 5;
            int x = 5 + col * 47;
            int y = 25 + row * 25;
            bool exists = slot < visible_count;
            int idx = exists ? visible[slot] : -1;
            bool active = exists && idx == selected;
            rect(x, y, 43, 20, active ? rgb(12, 20, 31) : rgb(7, 10, 18));
            if(active) { rect(x, y, 43, 1, COLOR_GOLD); rect(x, y + 19, 43, 1, COLOR_GOLD); }
            if(exists)
            {
                char short_name[7];
                int n = 0;
                while(roster[idx].name[n] && n < 6) { short_name[n] = roster[idx].name[n]; ++n; }
                short_name[n] = '\0';
                text(x + 3, y + 7, short_name, active ? COLOR_GOLD : (roster[idx].playable ? COLOR_WHITE : rgb(12, 15, 20)));
            }
        }
        text(8, 72, roster[selected].part, COLOR_GOLD);
        rect(8, 80, 224, 70, rgb(8, 11, 22));
        text(15, 87, selection_stage == 0 ? "PLAYER 1" : "PLAYER 2", COLOR_GOLD);
        text(15, 100, roster[selected].name, COLOR_WHITE, 2);
        text(15, 124, roster[selected].part, rgb(18, 23, 29));
        text(15, 137, roster[selected].playable ? "PROTOTYPE FIGHTER" : "ROSTER SLOT PLANNED", roster[selected].playable ? COLOR_GREEN : COLOR_RED);
        text(151, 89, "ARROWS MOVE", COLOR_WHITE);
        text(151, 101, "A CONFIRM", COLOR_WHITE);
        text(151, 113, "B BACK", COLOR_WHITE);
        text(151, 125, "START BATTLE", COLOR_WHITE);
    }

    void draw_battle()
    {
        clear(rgb(5, 7, 17));
        // Stylized original test arena.
        rect(0, 18, 240, 70, rgb(8, 15, 29));
        for(int x = 0; x < 240; x += 24) rect(x, 80, 1, 8, rgb(12, 20, 31));
        rect(0, 88, 240, 3, COLOR_GOLD);
        rect(0, 91, 240, 69, rgb(10, 11, 18));
        rect(0, 146, 240, 14, rgb(4, 5, 9));
        text(8, 5, roster[p1].name, COLOR_WHITE);
        text(150, 5, roster[p2].name, COLOR_WHITE);
        rect(8, 15, 100, 5, COLOR_DARK); rect(8, 15, p1_hp, 5, COLOR_GREEN);
        rect(132, 15, 100, 5, COLOR_DARK); rect(132 + (100 - p2_hp), 15, p2_hp, 5, COLOR_RED);
        draw_fighter(p1_x, p1_y + p1_jump, roster[p1].color, p1_facing_right, attack_timer > 0);
        draw_fighter(p2_x, p2_y, roster[p2].color, false, enemy_attack_timer > 0);
        centered(149, "A LIGHT B HEAVY L GUARD START MENU", COLOR_WHITE);
        if(p1_hp == 0 || p2_hp == 0)
        {
            rect(40, 58, 160, 34, COLOR_DARK);
            centered(67, p1_hp ? "PLAYER 1 WINS" : "PLAYER 2 WINS", COLOR_GOLD);
            centered(81, "PRESS START", COLOR_WHITE);
        }
    }

    void update_select()
    {
        int visible[10];
        int visible_count = 0;
        for(int i = 0; i < ROSTER_COUNT && visible_count < 10; ++i)
            if(roster[i].part[5] == char('0' + current_part)) visible[visible_count++] = i;
        int slot = 0;
        for(int i = 0; i < visible_count; ++i) if(visible[i] == selected) slot = i;
        if(bn::keypad::left_pressed() && slot > 0) selected = visible[slot - 1];
        if(bn::keypad::right_pressed() && slot + 1 < visible_count) selected = visible[slot + 1];
        if(bn::keypad::up_pressed() && slot >= 5) selected = visible[slot - 5];
        if(bn::keypad::down_pressed() && slot + 5 < visible_count) selected = visible[slot + 5];
        if(bn::keypad::l_pressed() || bn::keypad::r_pressed())
        {
            current_part += bn::keypad::r_pressed() ? 1 : -1;
            if(current_part < 1) current_part = 8;
            if(current_part > 8) current_part = 1;
            for(int i = 0; i < ROSTER_COUNT; ++i)
                if(roster[i].part[5] == char('0' + current_part)) { selected = i; break; }
        }
        if(bn::keypad::b_pressed())
        {
            if(selection_stage == 1) { selection_stage = 0; selected = p1; current_part = roster[p1].part[5] - '0'; }
            else screen = Screen::TITLE;
        }
        if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            if(! roster[selected].playable) return;
            if(selection_stage == 0) { p1 = selected; selection_stage = 1; current_part = 1; selected = (p1 == 0 ? 1 : 0); }
            else { p2 = selected; screen = Screen::BATTLE; p1_hp = p2_hp = 100; p1_x = 58; p2_x = 180; }
        }
        draw_select();
    }

    void update_battle()
    {
        ++frame;
        if(attack_timer > 0) --attack_timer;
        if(enemy_attack_timer > 0) --enemy_attack_timer;
        if(bn::keypad::start_pressed()) { screen = Screen::SELECT; selection_stage = 0; selected = p1; return; }
        if(p1_hp <= 0 || p2_hp <= 0) { if(bn::keypad::a_pressed()) { p1_hp = p2_hp = 100; p1_x = 58; p2_x = 180; } draw_battle(); return; }
        if(bn::keypad::left_held()) { if(p1_x > 15) p1_x -= 2; p1_facing_right = false; }
        if(bn::keypad::right_held()) { if(p1_x < 225) p1_x += 2; p1_facing_right = true; }
        if(bn::keypad::up_pressed() && p1_jump == 0) p1_jump = -25;
        if(p1_jump < 0) { ++p1_jump; if(p1_jump == 0) p1_jump = 0; }
        guarding = bn::keypad::l_held();
        if(bn::keypad::a_pressed() || bn::keypad::b_pressed())
        {
            attack_timer = 8;
            int distance = p2_x - p1_x; if(distance < 0) distance = -distance;
            if(distance < 48 && p1_facing_right)
            {
                int damage = bn::keypad::b_pressed() ? 18 : 10;
                p2_hp -= damage; if(p2_hp < 0) p2_hp = 0;
            }
        }
        // Simple rival AI: approach and occasionally strike; guard reduces damage.
        if(p2_x > p1_x + 28) --p2_x;
        else if(p2_x < p1_x + 25) ++p2_x;
        if((frame % 55) == 0 && p2_x - p1_x < 42)
        {
            enemy_attack_timer = 8;
            int damage = guarding ? 2 : 8;
            p1_hp -= damage; if(p1_hp < 0) p1_hp = 0;
        }
        draw_battle();
    }
}

int main()
{
    bn::core::init();
    // Switch to GBA Mode 3 (240x160, 15-bit color framebuffer).
    REG_DISPCNT = 3 | (1 << 10);
    while(true)
    {
        if(screen == Screen::TITLE)
        {
            draw_title();
            if(bn::keypad::start_pressed() || bn::keypad::a_pressed()) { screen = Screen::SELECT; selected = 0; selection_stage = 0; }
        }
        else if(screen == Screen::SELECT) update_select();
        else update_battle();
        bn::core::update();
    }
}
