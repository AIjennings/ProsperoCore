#pragma once

#include "prospero/common/types.h"
#include <vector>
#include <string>

namespace prospero::game {

struct Star {
    float x, y, z;
    float ox, oy;
    u32 color;
};

struct Particle {
    float x, y;
    float vx, vy;
    float life;
    float max_life;
    u32 color;
};

struct Laser {
    float x, y;
    float vx, vy;
    bool is_enemy;
    bool active;
};

struct Enemy {
    float x, y;
    float vx, vy;
    float hp;
    float max_hp;
    int type; // 0 = drone, 1 = interceptor, 2 = heavy
    float shoot_timer;
    bool active;
};

struct Asteroid {
    float x, y;
    float vx, vy;
    float rot;
    float rot_speed;
    float radius;
    std::vector<std::pair<float, float>> vertices;
    bool active;
};

struct PowerUp {
    float x, y;
    float vx, vy;
    float rot;
    int type; // 0 = shield, 1 = score
    bool active;
};

class ProsperoGame {
public:
    ProsperoGame();
    ~ProsperoGame();

    bool init();
    void run();
    void shutdown();

private:
    void resetGame();
    void handleInput();
    void update(float dt);
    void render();
    void dispatchGpuCommands();

    // Drawing primitives (direct into PS5 GDDR6 direct memory)
    void clear(u32 color);
    void drawPixel(int x, int y, u32 color);
    void drawLine(int x0, int y0, int x1, int y1, u32 color);
    void drawCircle(int cx, int cy, int r, u32 color, bool fill = false);
    void drawRect(int x, int y, int w, int h, u32 color, bool fill = false);
    void drawChar(int x, int y, char c, u32 color, int scale = 1);
    void drawText(int x, int y, const std::string& text, u32 color, int scale = 1);
    void drawPolygon(const std::vector<std::pair<float, float>>& pts, u32 color, bool fill = false);

    // Render components
    void renderStarfield();
    void renderPlayer();
    void renderEnemies();
    void renderAsteroids();
    void renderLasers();
    void renderParticles();
    void renderPowerUps();
    void renderHud();
    void renderControllerWidget(int x, int y);

    // Framebuffer & Dimensions
    u32 m_width{1280};
    u32 m_height{720};
    vaddr_t m_fb_vaddr{0};
    u32* m_framebuffer{nullptr};

    // Subsystem handles
    s32 m_video_handle{0};
    s32 m_pad_handle{0};
    off_t m_dmem_paddr{0};

    // State
    bool m_running{false};
    bool m_game_over{false};
    u32 m_score{0};
    float m_shield{100.0f};
    float m_boost{100.0f};
    bool m_is_boosting{false};
    u32 m_wave{1};
    float m_screen_shake{0.0f};
    float m_player_x{640.0f};
    float m_player_y{560.0f};
    float m_player_vx{0.0f};
    float m_player_vy{0.0f};
    float m_player_angle{0.0f};
    float m_laser_cooldown{0.0f};

    // DualSense input cache
    u8 m_left_stick_x{128};
    u8 m_left_stick_y{128};
    u8 m_r2_trigger{0};
    u32 m_buttons{0};
    u8 m_large_motor{0};
    u8 m_small_motor{0};

    // Game objects
    std::vector<Star> m_stars;
    std::vector<Particle> m_particles;
    std::vector<Laser> m_lasers;
    std::vector<Enemy> m_enemies;
    std::vector<Asteroid> m_asteroids;
    std::vector<PowerUp> m_powerups;

    float m_enemy_spawn_timer{0.0f};
    float m_asteroid_spawn_timer{0.0f};
};

} // namespace prospero::game
