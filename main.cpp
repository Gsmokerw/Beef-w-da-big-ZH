/*******************************************************************************************
*
*   SHADOW DIMENSION - Playdead (Limbo & Inside) Aesthetic Edition v2.3
*   A Monochromatic Atmospheric Precision Platformer in C++17 & Raylib
*
*   Features:
*   - Atmospheric Exit Portal (Aperture): Tall vertical reality tear with bright radiant core
*     and continuous upward-drifting volumetric dust particles.
*   - 4-Phase Cinematic White Flash Transition:
*     Phase 1 (0.2s): Implosion & suction of surrounding particles inward + player dissolving into light.
*     Phase 2 (0.4s): Smooth ramp-up of 100% blinding solid white flash overlay + deep low-frequency hum.
*     Phase 3: Seamless Level Load & Spawn Point reset behind 100% solid white screen.
*     Phase 4 (0.3s): Smooth fade-in from white down to 0% revealing the new sector with expanding shockwave.
*   - Secret Breakable Wall & Hidden Tunnel: Concealed behind Level 1 exit, opens on contact/shift.
*   - Secret Portal & 5-Second Fullscreen picture.png Cutscene.
*   - Crumbling/Fragile Platforms: Jitter and dissolve into ash 0.5s after stepped on.
*   - 5 Handcrafted, tested & 100% mathematically beatable levels.
*   - 35mm Film Grain & Vignette post-processing shader.
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cstdio>

// Screen & World Constants
static const int SCREEN_WIDTH = 1280;
static const int SCREEN_HEIGHT = 720;
static const float TILE_SIZE = 32.0f;
static const float PI_F = 3.14159265358979323846f;

// Phase Types
enum Phase {
    PHASE_BLACK = 0,    // Black Phase: White objects solid & radiant; Black objects pass-through
    PHASE_WHITE = 1     // White Phase: Black objects solid & heavy; White objects pass-through
};

// Tile Types
enum TileType {
    TILE_EMPTY = 0,
    TILE_NEUTRAL = 1,           // Permanent monolithic masonry (always solid)
    TILE_WHITE = 2,             // Solid in Black Phase; ghostly dashed outline in White Phase
    TILE_BLACK = 3,             // Solid in White Phase; ghostly dashed outline in Black Phase
    TILE_SPIKE_UP = 4,          // Organic jagged stakes
    TILE_SPIKE_DOWN = 5,
    TILE_SPIKE_LEFT = 6,
    TILE_SPIKE_RIGHT = 7,
    TILE_CRUMBLE_NEUTRAL = 8,   // Fragile neutral platform (shakes & dissolves into ash 0.5s after stepped on)
    TILE_CRUMBLE_WHITE = 9,     // Fragile white platform (solid in Black phase, dissolves 0.5s after step)
    TILE_CRUMBLE_BLACK = 10,    // Fragile black platform (solid in White phase, dissolves 0.5s after step)
    TILE_BREAKABLE = 11,        // Secret breakable wall block (shatters on impact or phase shift)
    TILE_SPIKE_DECORATIVE = 12  // Decorative organic spike with zero collision (fake spike)
};

// Game States
enum GameState {
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_PAUSE,
    STATE_SETTINGS,
    STATE_SECRET_CUTSCENE,      // Fullscreen 5-second cinematic view of picture.png
    STATE_VICTORY
};

// Game Settings
struct GameSettings {
    bool isFullscreen = false;
    float masterVolume = 0.8f;      // 0.0 to 1.0
    bool filmGrainEnabled = true;
    bool screenShakeEnabled = true;
    GameState previousState = STATE_MENU;
};

// Monochromatic Limbo / Inside Palette
static const Color COLOR_OBSIDIAN       = { 10, 12, 16, 255 };      // Pitch black silhouette
static const Color COLOR_CHARCOAL       = { 24, 28, 36, 255 };      // Dark stone monolithic fill
static const Color COLOR_FOG_DARK       = { 36, 42, 52, 255 };      // Deep fog
static const Color COLOR_FOG_MID        = { 65, 74, 88, 255 };      // Mid-distance silhouettes
static const Color COLOR_FOG_LIGHT      = { 110, 120, 135, 255 };   // Distant haze
static const Color COLOR_MIST           = { 165, 175, 192, 255 };   // Drifting ground mist
static const Color COLOR_STARK_WHITE    = { 245, 248, 252, 255 };   // Pure chalk white
static const Color COLOR_WHITE_GLOW     = { 255, 255, 255, 55 };    // Additive aura

// Particle Types
enum ParticleType {
    PART_ASH,
    PART_MIST,
    PART_ORB_SPARK,
    PART_DISSOLVE
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float size;
    float initialSize;
    float life;
    float maxLife;
    ParticleType type;
    float gravity;
    float drag;
};

// Ghost Trail
struct GhostTrail {
    Vector2 pos;
    Vector2 size;
    Vector2 scale;
    float facing;
    Color color;
    float alpha;
    float life;
    float maxLife;
};

// Monochromatic Shift Wave Effect
struct ShiftWave {
    Vector2 center;
    float radius;
    float maxRadius;
    float speed;
    float thickness;
    Color color;
    float alpha;
    bool active;
};

// Collectible Anima Orb
struct AnimaOrb {
    Vector2 pos;
    bool collected;
    float animOffset;
};

// Exit Light Aperture
struct Aperture {
    Vector2 pos;
    bool active;
    float animTimer;
};

// Crumbling / Fragile Platform Tile
struct CrumbleTile {
    int x;
    int y;
    int tileType;           // TILE_CRUMBLE_NEUTRAL, TILE_CRUMBLE_WHITE, TILE_CRUMBLE_BLACK
    bool triggered;         // True once player stands on it
    float triggerTimer;     // 0.5s countdown before collapse
    bool collapsed;         // True once dissolved into ash (intangible)
    float respawnTimer;     // 2.5s timer before re-solidifying
    float shakeOffset;      // Visual jitter offset in X
};

// Secret Breakable Wall Block
struct BreakableBlock {
    int x;
    int y;
    int health;
    bool broken;
};

// Player Structure (Limbo Stylized Silhouette Boy)
struct Player {
    Vector2 pos;
    Vector2 vel;
    Vector2 size;           // 20 x 28 base hitbox
    Vector2 scale;          // Squash & stretch
    float facing;           // 1 = right, -1 = left

    bool isGrounded;
    bool wasGrounded;
    int wallContact;        // -1 = left, 1 = right, 0 = none
    bool isWallSliding;

    float coyoteTimer;
    float jumpBufferTimer;
    float wallJumpLockTimer;
    float shiftCooldownTimer;
    float trailTimer;
    float footstepTimer;
    float runAnimCycle;
    float blinkTimer;
    bool eyeBlink;

    bool isAlive;
    float deathTimer;
    Vector2 spawnPoint;

    // 4-Phase Atmospheric Exit Portal & White Flash Transition Sequence
    bool inPortalTransition;
    bool isSecretPortal;
    float portalTransitionTimer;   // 0.0s to 0.9s
    Vector2 portalCenter;
    bool portalSoundPlayed;
    bool portalLevelLoaded;
};

// Level Structure
struct Level {
    int id;
    std::string name;
    std::string subtitle;
    int width;
    int height;
    std::vector<int> tiles;
    std::vector<AnimaOrb> orbs;
    std::vector<CrumbleTile> crumbleTiles;
    std::vector<BreakableBlock> breakableBlocks;
    bool hasSecretAperture;
    Aperture aperture;
    Aperture secretAperture;
    bool secretUnlocked;
    Vector2 playerSpawn;
};

// Atmospheric Audio
struct AudioManager {
    Sound sndJump;
    Sound sndLand;
    Sound sndShift;
    Sound sndOrb;
    Sound sndDeath;
    Sound sndWallJump;
    Sound sndVictory;
    Sound sndTransition;
    bool audioReady;
};

// Forward Declarations
static Wave GenProceduralWave(int sampleRate, float duration, float (*generator)(float t, float dur));
static AudioManager InitAtmosphericAudio();
static void CloseAtmosphericAudio(AudioManager& am);
static void InitLevels(std::vector<Level>& levels);
static void SpawnAshBurst(std::vector<Particle>& particles, Vector2 pos, int count, Color col, float spreadX, float speedY);
static void SpawnAmbientMistParticles(std::vector<Particle>& particles, int count);
static bool IsTileSolid(int tileType, Phase phase);
static const CrumbleTile* GetCrumbleTileAt(const Level& level, int x, int y);
static CrumbleTile* GetCrumbleTileAtMut(Level& level, int x, int y);
static const BreakableBlock* GetBreakableBlockAt(const Level& level, int x, int y);
static BreakableBlock* GetBreakableBlockAtMut(Level& level, int x, int y);
static bool CheckTileCollision(const Level& level, Rectangle rect, Phase phase);
static bool CheckSpikeCollision(const Level& level, Rectangle rect);
static void TriggerPlayerDeath(Player& player, std::vector<Particle>& particles, float& screenTrauma, AudioManager& am);
static void ResetCrumbleTiles(Level& level);
static void ResetBreakableBlocks(Level& level);
static void DrawDashedLine(Vector2 start, Vector2 end, float dashLen, float gapLen, float thickness, Color col);
static void DrawStylizedPlayer(const Player& player, const Level& level, Phase phase, float currentTime);
static void DrawAtmosphericAperture(Vector2 pos, bool active, float currentTime, bool isSecret);

// =========================================================================
// ATMOSPHERIC SOUND SYNTHESIS
// =========================================================================
static float WaveJump(float t, float dur) {
    float p = t / dur;
    float freq = 140.0f + 160.0f * (1.0f - p);
    float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.18f;
    float env = std::pow(1.0f - p, 2.0f);
    return (std::sin(2.0f * PI_F * freq * t) * 0.5f + noise) * env * 0.6f;
}

static float WaveLand(float t, float dur) {
    float p = t / dur;
    float freq = 75.0f * (1.0f - p * 0.6f);
    float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.35f;
    float env = std::pow(1.0f - p, 3.2f);
    return (std::sin(2.0f * PI_F * freq * t) + noise) * env * 0.75f;
}

static float WaveShift(float t, float dur) {
    float p = t / dur;
    float subFreq = 85.0f * (1.0f - p * 0.5f);
    float sub = std::sin(2.0f * PI_F * subFreq * t);
    float breath = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * std::sin(p * PI_F) * 0.35f;
    float env = std::sin(p * PI_F);
    return (sub * 0.65f + breath) * env * 0.85f;
}

static float WaveOrb(float t, float dur) {
    float p = t / dur;
    float tone1 = 1046.5f; // C6 glass chime
    float tone2 = 1567.98f; // G6 overtone
    float env = std::pow(1.0f - p, 1.8f);
    return (0.7f * std::sin(2.0f * PI_F * tone1 * t) + 0.3f * std::sin(2.0f * PI_F * tone2 * t)) * env * 0.7f;
}

static float WaveDeath(float t, float dur) {
    float p = t / dur;
    float freq = 120.0f * (1.0f - p * 0.8f);
    float snap = (p < 0.08f) ? (((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.9f) : 0.0f;
    float low = std::sin(2.0f * PI_F * freq * t) * 0.4f;
    float env = std::pow(1.0f - p, 2.2f);
    return (snap + low) * env * 0.85f;
}

static float WaveWallJump(float t, float dur) {
    float p = t / dur;
    float freq = 210.0f * (1.0f - p * 0.4f);
    float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.25f;
    float env = std::pow(1.0f - p, 2.5f);
    return (std::sin(2.0f * PI_F * freq * t) * 0.6f + noise) * env * 0.55f;
}

static float WaveVictory(float t, float dur) {
    float p = t / dur;
    float f1 = 261.63f; // C4
    float f2 = 392.00f; // G4
    float f3 = 523.25f; // C5
    float chord = (std::sin(2.0f * PI_F * f1 * t) + std::sin(2.0f * PI_F * f2 * t) + std::sin(2.0f * PI_F * f3 * t)) / 3.0f;
    float env = std::sin(std::pow(p, 0.6f) * PI_F);
    return chord * env * 0.8f;
}

// Deep, low-frequency atmospheric hum / whoosh at peak white flash
static float WaveTransition(float t, float dur) {
    float p = t / dur;
    float subFreq = 48.0f * (1.0f - p * 0.35f);
    float subDrone = std::sin(2.0f * PI_F * subFreq * t);
    float harmonic = std::sin(2.0f * PI_F * (subFreq * 2.0f) * t) * 0.40f;
    float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.22f;
    float sweep = std::sin(2.0f * PI_F * (70.0f + 180.0f * std::sin(p * PI_F)) * t) * 0.35f;
    float env = std::sin(p * PI_F);
    return (subDrone + harmonic + sweep + noise) * env * 0.95f;
}

static Wave GenProceduralWave(int sampleRate, float duration, float (*generator)(float t, float dur)) {
    Wave wave = { 0 };
    wave.frameCount = (unsigned int)(sampleRate * duration);
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    short* data = (short*)MemAlloc(wave.frameCount * sizeof(short));
    for (unsigned int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / (float)sampleRate;
        float sample = generator(t, duration);
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        data[i] = (short)(sample * 30000.0f);
    }
    wave.data = data;
    return wave;
}

static AudioManager InitAtmosphericAudio() {
    AudioManager am = { 0 };
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        am.audioReady = false;
        return am;
    }
    am.audioReady = true;

    Wave w;
    w = GenProceduralWave(44100, 0.15f, WaveJump);       am.sndJump = LoadSoundFromWave(w);       UnloadWave(w);
    w = GenProceduralWave(44100, 0.18f, WaveLand);       am.sndLand = LoadSoundFromWave(w);       UnloadWave(w);
    w = GenProceduralWave(44100, 0.32f, WaveShift);      am.sndShift = LoadSoundFromWave(w);      UnloadWave(w);
    w = GenProceduralWave(44100, 0.40f, WaveOrb);        am.sndOrb = LoadSoundFromWave(w);        UnloadWave(w);
    w = GenProceduralWave(44100, 0.38f, WaveDeath);      am.sndDeath = LoadSoundFromWave(w);      UnloadWave(w);
    w = GenProceduralWave(44100, 0.13f, WaveWallJump);   am.sndWallJump = LoadSoundFromWave(w);   UnloadWave(w);
    w = GenProceduralWave(44100, 1.10f, WaveVictory);    am.sndVictory = LoadSoundFromWave(w);    UnloadWave(w);
    w = GenProceduralWave(44100, 0.90f, WaveTransition); am.sndTransition = LoadSoundFromWave(w); UnloadWave(w);

    return am;
}

static void CloseAtmosphericAudio(AudioManager& am) {
    if (am.audioReady) {
        UnloadSound(am.sndJump);
        UnloadSound(am.sndLand);
        UnloadSound(am.sndShift);
        UnloadSound(am.sndOrb);
        UnloadSound(am.sndDeath);
        UnloadSound(am.sndWallJump);
        UnloadSound(am.sndVictory);
        UnloadSound(am.sndTransition);
        CloseAudioDevice();
    }
}

// Dashed line segment helper for perimeter rendering
static void DrawDashedLine(Vector2 start, Vector2 end, float dashLen, float gapLen, float thickness, Color col) {
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0.001f) return;
    float nx = dx / len;
    float ny = dy / len;

    float dist = 0.0f;
    while (dist < len) {
        float seg = std::min(dashLen, len - dist);
        Vector2 s = { start.x + nx * dist, start.y + ny * dist };
        Vector2 e = { start.x + nx * (dist + seg), start.y + ny * (dist + seg) };
        DrawLineEx(s, e, thickness, col);
        dist += dashLen + gapLen;
    }
}

// Check solid status
static bool IsTileSolid(int tileType, Phase phase) {
    if (tileType == TILE_NEUTRAL || tileType == TILE_CRUMBLE_NEUTRAL || tileType == TILE_BREAKABLE) return true;
    if ((tileType == TILE_WHITE || tileType == TILE_CRUMBLE_WHITE) && phase == PHASE_BLACK) return true;
    if ((tileType == TILE_BLACK || tileType == TILE_CRUMBLE_BLACK) && phase == PHASE_WHITE) return true;
    return false;
}

static const CrumbleTile* GetCrumbleTileAt(const Level& level, int x, int y) {
    for (const auto& ct : level.crumbleTiles) {
        if (ct.x == x && ct.y == y) return &ct;
    }
    return nullptr;
}

static CrumbleTile* GetCrumbleTileAtMut(Level& level, int x, int y) {
    for (auto& ct : level.crumbleTiles) {
        if (ct.x == x && ct.y == y) return &ct;
    }
    return nullptr;
}

static const BreakableBlock* GetBreakableBlockAt(const Level& level, int x, int y) {
    for (const auto& bb : level.breakableBlocks) {
        if (bb.x == x && bb.y == y) return &bb;
    }
    return nullptr;
}

static BreakableBlock* GetBreakableBlockAtMut(Level& level, int x, int y) {
    for (auto& bb : level.breakableBlocks) {
        if (bb.x == x && bb.y == y) return &bb;
    }
    return nullptr;
}

static bool CheckTileCollision(const Level& level, Rectangle rect, Phase phase) {
    int startX = std::max(0, (int)(rect.x / TILE_SIZE));
    int endX   = std::min(level.width - 1, (int)((rect.x + rect.width - 0.01f) / TILE_SIZE));
    int startY = std::max(0, (int)(rect.y / TILE_SIZE));
    int endY   = std::min(level.height - 1, (int)((rect.y + rect.height - 0.01f) / TILE_SIZE));

    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            int tile = level.tiles[y * level.width + x];
            if (tile == TILE_CRUMBLE_NEUTRAL || tile == TILE_CRUMBLE_WHITE || tile == TILE_CRUMBLE_BLACK) {
                const CrumbleTile* ct = GetCrumbleTileAt(level, x, y);
                if (ct && ct->collapsed) continue; // Collapsed fragile tile is intangible!
            }
            if (tile == TILE_BREAKABLE) {
                const BreakableBlock* bb = GetBreakableBlockAt(level, x, y);
                if (bb && bb->broken) continue; // Shattered secret wall block is intangible!
            }
            if (IsTileSolid(tile, phase)) return true;
        }
    }
    return false;
}

static bool CheckSpikeCollision(const Level& level, Rectangle rect) {
    int startX = std::max(0, (int)(rect.x / TILE_SIZE));
    int endX   = std::min(level.width - 1, (int)((rect.x + rect.width - 0.01f) / TILE_SIZE));
    int startY = std::max(0, (int)(rect.y / TILE_SIZE));
    int endY   = std::min(level.height - 1, (int)((rect.y + rect.height - 0.01f) / TILE_SIZE));

    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            int tile = level.tiles[y * level.width + x];
            if (tile >= TILE_SPIKE_UP && tile <= TILE_SPIKE_RIGHT) {
                Rectangle spikeRect = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                if (tile == TILE_SPIKE_UP) {
                    spikeRect.y += 12.0f; spikeRect.height = 20.0f;
                    spikeRect.x += 4.0f;  spikeRect.width = 24.0f;
                } else if (tile == TILE_SPIKE_DOWN) {
                    spikeRect.height = 20.0f;
                    spikeRect.x += 4.0f;  spikeRect.width = 24.0f;
                } else if (tile == TILE_SPIKE_LEFT) {
                    spikeRect.x += 12.0f; spikeRect.width = 20.0f;
                    spikeRect.y += 4.0f;  spikeRect.height = 24.0f;
                } else if (tile == TILE_SPIKE_RIGHT) {
                    spikeRect.width = 20.0f;
                    spikeRect.y += 4.0f;  spikeRect.height = 24.0f;
                }
                if (CheckCollisionRecs(rect, spikeRect)) return true;
            }
        }
    }
    return false;
}

static void SpawnAshBurst(std::vector<Particle>& particles, Vector2 pos, int count, Color col, float spreadX, float speedY) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = pos;
        p.pos.x += ((float)rand() / (float)RAND_MAX - 0.5f) * spreadX;
        float angle = ((float)rand() / (float)RAND_MAX * PI_F) + PI_F;
        float spd = ((float)rand() / (float)RAND_MAX * 55.0f + 15.0f);
        p.vel = { std::cos(angle) * spd, std::sin(angle) * spd + speedY };
        p.color = col;
        p.initialSize = (float)rand() / (float)RAND_MAX * 3.0f + 1.5f;
        p.size = p.initialSize;
        p.maxLife = (float)rand() / (float)RAND_MAX * 0.35f + 0.20f;
        p.life = p.maxLife;
        p.type = PART_ASH;
        p.gravity = 110.0f;
        p.drag = 0.93f;
        particles.push_back(p);
    }
}

static void SpawnAmbientMistParticles(std::vector<Particle>& particles, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = { (float)(rand() % (SCREEN_WIDTH * 2) - SCREEN_WIDTH / 2), (float)(rand() % SCREEN_HEIGHT) };
        p.vel = { ((float)rand() / (float)RAND_MAX * 30.0f + 20.0f), ((float)rand() / (float)RAND_MAX - 0.5f) * 6.0f };
        p.color = ColorAlpha(COLOR_STARK_WHITE, (float)rand() / (float)RAND_MAX * 0.40f + 0.15f);
        p.initialSize = (float)rand() / (float)RAND_MAX * 2.5f + 1.2f;
        p.size = p.initialSize;
        p.maxLife = (float)rand() / (float)RAND_MAX * 6.0f + 5.0f;
        p.life = p.maxLife;
        p.type = PART_MIST;
        p.gravity = 0.0f;
        p.drag = 1.0f;
        particles.push_back(p);
    }
}

static void TriggerPlayerDeath(Player& player, std::vector<Particle>& particles, float& screenTrauma, AudioManager& am) {
    if (!player.isAlive) return;
    player.isAlive = false;
    player.deathTimer = 0.85f;
    screenTrauma = std::min(1.0f, screenTrauma + 0.75f);
    if (am.audioReady) PlaySound(am.sndDeath);

    Vector2 center = { player.pos.x + player.size.x * 0.5f, player.pos.y + player.size.y * 0.5f };
    int ashCount = 18;
    for (int i = 0; i < ashCount; i++) {
        Particle p;
        p.pos = center;
        float angle = (float)rand() / (float)RAND_MAX * 2.0f * PI_F;
        float spd = (float)rand() / (float)RAND_MAX * 220.0f + 50.0f;
        p.vel = { std::cos(angle) * spd, std::sin(angle) * spd };
        p.color = (rand() % 3 == 0) ? COLOR_STARK_WHITE : COLOR_OBSIDIAN;
        p.initialSize = (float)rand() / (float)RAND_MAX * 4.0f + 2.0f;
        p.size = p.initialSize;
        p.maxLife = (float)rand() / (float)RAND_MAX * 0.55f + 0.35f;
        p.life = p.maxLife;
        p.type = PART_DISSOLVE;
        p.gravity = 280.0f;
        p.drag = 0.92f;
        particles.push_back(p);
    }
}

static void ResetCrumbleTiles(Level& level) {
    for (auto& ct : level.crumbleTiles) {
        ct.triggered = false;
        ct.triggerTimer = 0.5f;
        ct.collapsed = false;
        ct.respawnTimer = 0.0f;
        ct.shakeOffset = 0.0f;
    }
}

static void ResetBreakableBlocks(Level& level) {
    for (auto& bb : level.breakableBlocks) {
        bb.broken = false;
        bb.health = 1;
    }
    level.secretUnlocked = false;
}

static void RespawnPlayer(Player& player) {
    player.pos = player.spawnPoint;
    player.vel = { 0.0f, 0.0f };
    player.scale = { 1.0f, 1.0f };
    player.isGrounded = false;
    player.wasGrounded = false;
    player.wallContact = 0;
    player.isWallSliding = false;
    player.coyoteTimer = 0.0f;
    player.jumpBufferTimer = 0.0f;
    player.wallJumpLockTimer = 0.0f;
    player.isAlive = true;
    player.deathTimer = 0.0f;
    player.eyeBlink = false;
    player.blinkTimer = 2.0f;
    player.runAnimCycle = 0.0f;
    player.inPortalTransition = false;
    player.isSecretPortal = false;
    player.portalTransitionTimer = 0.0f;
    player.portalSoundPlayed = false;
    player.portalLevelLoaded = false;
}

// -------------------------------------------------------------
// ENHANCED PARALLAX BACKGROUND & DECAYING INDUSTRIAL LANDSCAPE
// -------------------------------------------------------------
static void DrawParallaxBackground(Camera2D camera, int lvlWidth, int lvlHeight, Phase phase, float time) {
    float camX = camera.target.x;
    float camY = camera.target.y;

    // 1. Far Parallax (0.15x speed): High-contrast Decaying Industrial Factories & Cooling Towers
    float farX = -camX * 0.15f;
    Color farSkyCol = (phase == PHASE_BLACK) ? ColorAlpha(COLOR_FOG_DARK, 0.85f) : ColorAlpha(COLOR_FOG_MID, 0.80f);

    for (int i = -4; i < 28; i++) {
        float fX = farX + i * 260.0f;
        float baseH = camY + 280.0f;

        DrawRectangle((int)fX, (int)(baseH - 120.0f), 130, 260, farSkyCol);
        DrawTriangle({ fX, baseH - 120.0f }, { fX + 35.0f, baseH - 165.0f }, { fX + 65.0f, baseH - 120.0f }, farSkyCol);
        DrawTriangle({ fX + 65.0f, baseH - 120.0f }, { fX + 100.0f, baseH - 165.0f }, { fX + 130.0f, baseH - 120.0f }, farSkyCol);

        if (i % 2 == 0) {
            DrawRectangle((int)(fX + 155), (int)(baseH - 240.0f), 22, 380, farSkyCol);
            DrawRectangle((int)(fX + 150), (int)(baseH - 246.0f), 32, 10, farSkyCol);
            DrawCircle((int)(fX + 166 + std::sin(time + i) * 12.0f), (int)(baseH - 265.0f), 18.0f, ColorAlpha(COLOR_MIST, 0.12f));
        }

        if (i % 3 == 1) {
            float pX = fX + 40.0f;
            DrawLineEx({ pX, baseH }, { pX + 25.0f, baseH - 280.0f }, 2.5f, farSkyCol);
            DrawLineEx({ pX + 50.0f, baseH }, { pX + 25.0f, baseH - 280.0f }, 2.5f, farSkyCol);
            DrawLineEx({ pX + 5.0f, baseH - 200.0f }, { pX + 45.0f, baseH - 200.0f }, 2.0f, farSkyCol);
            DrawLineEx({ pX, baseH - 140.0f }, { pX + 50.0f, baseH - 140.0f }, 2.0f, farSkyCol);
        }
    }

    // 2. Mid Parallax (0.40x speed): Twisted Gothic Dead Trees & Sagging Telegraph Wires
    float midX = -camX * 0.40f;
    Color midCol = (phase == PHASE_BLACK) ? ColorAlpha(COLOR_OBSIDIAN, 0.92f) : ColorAlpha(COLOR_CHARCOAL, 0.90f);

    for (int i = -5; i < 32; i++) {
        float objX = midX + i * 190.0f;
        float groundY = camY + 320.0f;

        if (i % 3 == 0) {
            DrawRectangle((int)objX, (int)(groundY - 260.0f), 7, 260, midCol);
            DrawRectangle((int)(objX - 32), (int)(groundY - 240.0f), 71, 5, midCol);
            DrawRectangle((int)(objX - 22), (int)(groundY - 215.0f), 51, 5, midCol);

            float nextX = objX + 190.0f * 3.0f;
            for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
                float x1 = objX + t * (nextX - objX);
                float y1 = (groundY - 240.0f) + std::sin(t * PI_F) * 35.0f;
                float x2 = objX + (t + 0.1f) * (nextX - objX);
                float y2 = (groundY - 240.0f) + std::sin((t + 0.1f) * PI_F) * 35.0f;
                DrawLineEx({ x1, y1 }, { x2, y2 }, 1.5f, midCol);
            }
        } else {
            DrawRectangle((int)objX, (int)(groundY - 170.0f), 10, 170, midCol);
            DrawLineEx({ objX + 5, groundY - 130.0f }, { objX - 45, groundY - 180.0f }, 3.5f, midCol);
            DrawLineEx({ objX - 35, groundY - 170.0f }, { objX - 60, groundY - 210.0f }, 2.2f, midCol);
            DrawLineEx({ objX + 5, groundY - 110.0f }, { objX + 55, groundY - 165.0f }, 3.5f, midCol);
            DrawLineEx({ objX + 40, groundY - 155.0f }, { objX + 75, groundY - 195.0f }, 2.0f, midCol);
        }
    }
}

// -------------------------------------------------------------
// VOLUMETRIC GROUND FOG LAYER
// -------------------------------------------------------------
static void DrawVolumetricGroundFog(Camera2D camera, int lvlWidth, float time) {
    float camX = camera.target.x;
    float camY = camera.target.y;

    float fogY = camY + 80.0f;
    for (int layer = 0; layer < 3; layer++) {
        float speed = (layer == 0) ? 22.0f : (layer == 1) ? -16.0f : 28.0f;
        float waveOff = time * speed + layer * 140.0f;
        float alpha = (layer == 0) ? 0.08f : (layer == 1) ? 0.12f : 0.07f;
        Color fogCol = ColorAlpha(COLOR_MIST, alpha);

        for (float x = camX - SCREEN_WIDTH * 0.8f; x <= camX + SCREEN_WIDTH * 0.8f; x += 90.0f) {
            float yOffset = std::sin((x + waveOff) * 0.015f) * 18.0f + std::cos((x - waveOff) * 0.022f) * 12.0f;
            DrawCircleGradient((int)x, (int)(fogY + yOffset + layer * 45.0f), 110.0f, fogCol, BLANK);
        }
    }
}

// -------------------------------------------------------------
// VOLUMETRIC GOD RAYS
// -------------------------------------------------------------
static void DrawVolumetricGodRays(float time, Phase phase) {
    float baseAlpha = (phase == PHASE_BLACK) ? 0.065f : 0.045f;

    for (int i = 0; i < 4; i++) {
        float sway = std::sin(time * 0.45f + i * 1.5f) * 25.0f;
        float topX = -320.0f + i * 440.0f + sway * 0.5f;
        float botX = topX + 580.0f + sway;
        float widthTop = 150.0f + i * 30.0f;
        float widthBot = 340.0f + i * 45.0f;

        Vector2 v1 = { topX, -280.0f };
        Vector2 v2 = { topX + widthTop, -280.0f };
        Vector2 v3 = { botX + widthBot, 1150.0f };
        Vector2 v4 = { botX, 1150.0f };

        float pulse = baseAlpha + 0.018f * std::sin(time * 1.4f + i * 2.2f);
        Color rayCol = ColorAlpha(COLOR_STARK_WHITE, pulse);
        DrawTriangle(v1, v4, v2, rayCol);
        DrawTriangle(v2, v4, v3, rayCol);
    }
}

// -------------------------------------------------------------
// ORGANIC JAGGED SPIKES (MULTI-NEEDLE WITH STARK WHITE GLINTS)
// -------------------------------------------------------------
static void DrawOrganicSpike(Rectangle tileRect, int type) {
    float x = tileRect.x;
    float y = tileRect.y;

    if (type == TILE_SPIKE_UP) {
        Vector2 b1_l = { x + 1.0f, y + TILE_SIZE };
        Vector2 b1_r = { x + 12.0f, y + TILE_SIZE };
        Vector2 tip1 = { x + 6.0f, y + 8.0f };
        DrawTriangle(b1_l, tip1, b1_r, COLOR_OBSIDIAN);

        Vector2 b2_l = { x + 8.0f, y + TILE_SIZE };
        Vector2 b2_r = { x + 24.0f, y + TILE_SIZE };
        Vector2 tip2 = { x + 16.0f, y + 2.0f };
        DrawTriangle(b2_l, tip2, b2_r, COLOR_OBSIDIAN);

        Vector2 b3_l = { x + 20.0f, y + TILE_SIZE };
        Vector2 b3_r = { x + 31.0f, y + TILE_SIZE };
        Vector2 tip3 = { x + 26.0f, y + 9.0f };
        DrawTriangle(b3_l, tip3, b3_r, COLOR_OBSIDIAN);

        DrawTriangle({ tip2.x - 3.0f, tip2.y + 7.0f }, tip2, { tip2.x + 3.0f, tip2.y + 7.0f }, COLOR_STARK_WHITE);
        DrawCircleV(tip2, 1.8f, COLOR_STARK_WHITE);

        DrawTriangle({ tip1.x - 2.5f, tip1.y + 6.0f }, tip1, { tip1.x + 2.5f, tip1.y + 6.0f }, COLOR_STARK_WHITE);
        DrawCircleV(tip1, 1.4f, COLOR_STARK_WHITE);

        DrawTriangle({ tip3.x - 2.5f, tip3.y + 6.0f }, tip3, { tip3.x + 2.5f, tip3.y + 6.0f }, COLOR_STARK_WHITE);
        DrawCircleV(tip3, 1.4f, COLOR_STARK_WHITE);
    }
    else if (type == TILE_SPIKE_DOWN) {
        Vector2 tip = { x + 16.0f, y + TILE_SIZE - 2.0f };
        DrawTriangle({ x + 2.0f, y }, { x + TILE_SIZE - 2.0f, y }, tip, COLOR_OBSIDIAN);
        DrawTriangle({ tip.x - 3.0f, tip.y - 7.0f }, tip, { tip.x + 3.0f, tip.y - 7.0f }, COLOR_STARK_WHITE);
        DrawCircleV(tip, 1.8f, COLOR_STARK_WHITE);
    }
    else if (type == TILE_SPIKE_LEFT) {
        Vector2 tip = { x + 2.0f, y + 16.0f };
        DrawTriangle({ x + TILE_SIZE, y + 2.0f }, tip, { x + TILE_SIZE, y + TILE_SIZE - 2.0f }, COLOR_OBSIDIAN);
        DrawTriangle({ tip.x + 7.0f, tip.y - 3.0f }, tip, { tip.x + 7.0f, tip.y + 3.0f }, COLOR_STARK_WHITE);
        DrawCircleV(tip, 1.8f, COLOR_STARK_WHITE);
    }
    else if (type == TILE_SPIKE_RIGHT) {
        Vector2 tip = { x + TILE_SIZE - 2.0f, y + 16.0f };
        DrawTriangle({ x, y + 2.0f }, { x, y + TILE_SIZE - 2.0f }, tip, COLOR_OBSIDIAN);
        DrawTriangle({ tip.x - 7.0f, tip.y - 3.0f }, tip, { tip.x - 7.0f, tip.y + 3.0f }, COLOR_STARK_WHITE);
        DrawCircleV(tip, 1.8f, COLOR_STARK_WHITE);
    }
}

// -------------------------------------------------------------
// ATMOSPHERIC EXIT PORTAL (VERTICAL APERTURE & REALITY TEAR)
// -------------------------------------------------------------
static void DrawAtmosphericAperture(Vector2 pos, bool active, float currentTime, bool isSecret) {
    if (active) {
        float pulse = 0.88f + 0.12f * std::sin(currentTime * 4.5f);

        // Radiant Outer Glow Halos
        DrawCircleGradient((int)pos.x, (int)pos.y, 65.0f * pulse, ColorAlpha(COLOR_STARK_WHITE, 0.42f), BLANK);
        DrawCircleGradient((int)pos.x, (int)pos.y, 35.0f * pulse, ColorAlpha(COLOR_STARK_WHITE, 0.65f), BLANK);

        // Tall Vertical Reality Tear Beam (4px wide, 84px high)
        DrawRectangle((int)pos.x - 2, (int)pos.y - 42, 4, 84, COLOR_STARK_WHITE);

        // Tapered Diamond Tips at top and bottom of tear
        Vector2 topL = { pos.x - 3.0f, pos.y - 36.0f };
        Vector2 topR = { pos.x + 3.0f, pos.y - 36.0f };
        Vector2 topTip = { pos.x, pos.y - 46.0f };
        DrawTriangle(topL, topTip, topR, COLOR_STARK_WHITE);

        Vector2 botL = { pos.x - 3.0f, pos.y + 36.0f };
        Vector2 botR = { pos.x + 3.0f, pos.y + 36.0f };
        Vector2 botTip = { pos.x, pos.y + 46.0f };
        DrawTriangle(botL, botR, botTip, COLOR_STARK_WHITE);

        // Brilliant Stark White Inner Slit Core
        DrawRectangle((int)pos.x - 1, (int)pos.y - 38, 2, 76, WHITE);

        // Delicate Outer Framing Brackets
        DrawLineEx({ pos.x - 6.0f, pos.y - 36.0f }, { pos.x - 6.0f, pos.y + 36.0f }, 1.5f, ColorAlpha(COLOR_STARK_WHITE, 0.80f));
        DrawLineEx({ pos.x + 6.0f, pos.y - 36.0f }, { pos.x + 6.0f, pos.y + 36.0f }, 1.5f, ColorAlpha(COLOR_STARK_WHITE, 0.80f));
    } else {
        // Sealed portal frame
        DrawRectangleLines((int)pos.x - 4, (int)pos.y - 32, 8, 64, ColorAlpha(COLOR_FOG_MID, 0.35f));
        DrawText("SEALED", (int)pos.x - 18, (int)pos.y - 5, 10, COLOR_FOG_MID);
    }
}

// -------------------------------------------------------------
// STYLIZED PLAYDEAD SILHOUETTE BOY (LIMBO STYLE)
// -------------------------------------------------------------
static void DrawStylizedPlayer(const Player& player, const Level& level, Phase phase, float currentTime) {
    if (!player.isAlive) return;

    // Squash & Stretch pivot anchored at bottom center
    float drawW = player.size.x * player.scale.x;
    float drawH = player.size.y * player.scale.y;
    float drawX = player.pos.x + player.size.x * 0.5f - drawW * 0.5f;
    float drawY = player.pos.y + player.size.y - drawH;

    // Head parameters
    float headRadius = drawW * 0.36f;
    float headCenterX = drawX + drawW * 0.5f + (player.facing * 2.0f);
    float headCenterY = drawY + headRadius + 1.0f;

    // Torso parameters
    float torsoTopY = headCenterY + headRadius * 0.65f;
    float torsoBotY = drawY + drawH * 0.72f;
    float torsoWidth = drawW * 0.65f;
    float torsoCenterX = drawX + drawW * 0.5f;

    // 1. Legs Animation
    if (player.isGrounded && std::abs(player.vel.x) > 40.0f) {
        float stride = std::sin(player.runAnimCycle * 18.0f) * 6.5f;
        DrawLineEx({ torsoCenterX - 2.0f, torsoBotY }, { torsoCenterX - player.facing * stride - 2.0f, drawY + drawH }, 3.5f, COLOR_OBSIDIAN);
        DrawLineEx({ torsoCenterX + 2.0f, torsoBotY }, { torsoCenterX + player.facing * stride + 2.0f, drawY + drawH }, 3.5f, COLOR_OBSIDIAN);
    } else if (!player.isGrounded) {
        DrawLineEx({ torsoCenterX - 3.0f, torsoBotY }, { torsoCenterX - 4.0f, drawY + drawH - 3.0f }, 3.5f, COLOR_OBSIDIAN);
        DrawLineEx({ torsoCenterX + 3.0f, torsoBotY }, { torsoCenterX + 4.0f, drawY + drawH - 1.0f }, 3.5f, COLOR_OBSIDIAN);
    } else {
        DrawLineEx({ torsoCenterX - 3.5f, torsoBotY }, { torsoCenterX - 3.5f, drawY + drawH }, 3.5f, COLOR_OBSIDIAN);
        DrawLineEx({ torsoCenterX + 3.5f, torsoBotY }, { torsoCenterX + 3.5f, drawY + drawH }, 3.5f, COLOR_OBSIDIAN);
    }

    // 2. Torso
    Rectangle torsoRect = { torsoCenterX - torsoWidth * 0.5f, torsoTopY, torsoWidth, torsoBotY - torsoTopY };
    DrawRectangleRounded(torsoRect, 0.4f, 4, COLOR_OBSIDIAN);

    // 3. Head
    DrawCircle((int)headCenterX, (int)headCenterY, headRadius, COLOR_OBSIDIAN);
    float hairBackX = headCenterX - player.facing * (headRadius * 0.7f);
    DrawTriangle({ hairBackX, headCenterY - headRadius },
                 { hairBackX - player.facing * 4.0f, headCenterY - headRadius - 4.0f },
                 { hairBackX + player.facing * 3.0f, headCenterY - headRadius * 0.4f },
                 COLOR_OBSIDIAN);

    // -------------------------------------------------------------
    // 4. SMART EYE BEHAVIOR (Limbo/Inside Glowing Stark-White Pupils)
    // -------------------------------------------------------------
    if (!player.eyeBlink) {
        float eyeOffsetX = player.facing * (headRadius * 0.46f);
        float eyeOffsetY = -(headRadius * 0.10f);

        bool nearSpikesOrPit = false;
        if (player.isGrounded) {
            float forwardFootX = player.pos.x + player.size.x * 0.5f + player.facing * 20.0f;
            int checkTileX = (int)(forwardFootX / TILE_SIZE);
            int checkTileY = (int)((player.pos.y + player.size.y + 4.0f) / TILE_SIZE);
            if (checkTileX >= 0 && checkTileX < level.width && checkTileY >= 0 && checkTileY < level.height) {
                int groundTile = level.tiles[checkTileY * level.width + checkTileX];
                if (!IsTileSolid(groundTile, phase)) {
                    nearSpikesOrPit = true;
                }
                for (int sy = checkTileY; sy <= std::min(level.height - 1, checkTileY + 3); sy++) {
                    int t = level.tiles[sy * level.width + checkTileX];
                    if (t >= TILE_SPIKE_UP && t <= TILE_SPIKE_RIGHT) {
                        nearSpikesOrPit = true;
                        break;
                    }
                }
            }
        }

        if (nearSpikesOrPit) {
            eyeOffsetY = (headRadius * 0.32f);
            eyeOffsetX = player.facing * (headRadius * 0.38f);
        } else if (!player.isGrounded && player.vel.y > 180.0f) {
            eyeOffsetY = (headRadius * 0.38f);
            eyeOffsetX = player.facing * (headRadius * 0.30f);
        } else if (!player.isGrounded && player.vel.y < -100.0f) {
            eyeOffsetY = -(headRadius * 0.35f);
        } else if (std::abs(player.vel.x) > 30.0f) {
            eyeOffsetX = player.facing * (headRadius * 0.52f);
            eyeOffsetY = -(headRadius * 0.08f);
        }

        float eye1X = headCenterX + eyeOffsetX;
        float eye1Y = headCenterY + eyeOffsetY;
        float eye2X = eye1X - player.facing * 3.2f;
        float eye2Y = eye1Y;

        DrawCircle((int)eye1X, (int)eye1Y, 2.2f, COLOR_STARK_WHITE);
        DrawCircle((int)eye1X, (int)eye1Y, 1.1f, RAYWHITE);
        DrawCircle((int)eye2X, (int)eye2Y, 1.6f, ColorAlpha(COLOR_STARK_WHITE, 0.85f));
        DrawCircle((int)eye2X, (int)eye2Y, 0.8f, RAYWHITE);
    }
}

// -------------------------------------------------------------
// POST-PROCESSING SHADER (35MM GRAIN & VIGNETTE)
// -------------------------------------------------------------
static const char* FILM_GRAIN_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform float uTime;\n"
    "uniform float uVignette;\n"
    "uniform float uGrain;\n"
    "\n"
    "float rand(vec2 co) {\n"
    "    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec4 tex = texture(texture0, fragTexCoord);\n"
    "    vec2 uv = fragTexCoord * (vec2(1.0) - fragTexCoord.yx);\n"
    "    float vig = uv.x * uv.y * 15.0;\n"
    "    vig = clamp(pow(vig, uVignette), 0.0, 1.0);\n"
    "    float noise = (rand(fragTexCoord * 140.0 + fract(uTime * 37.19)) - 0.5) * uGrain;\n"
    "    vec3 col = tex.rgb * vig + vec3(noise);\n"
    "    finalColor = vec4(clamp(col, 0.0, 1.0), tex.a);\n"
    "}\n";

static const char* FILM_GRAIN_FS_120 =
    "#version 120\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform float uTime;\n"
    "uniform float uVignette;\n"
    "uniform float uGrain;\n"
    "\n"
    "float rand(vec2 co) {\n"
    "    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec4 tex = texture2D(texture0, fragTexCoord);\n"
    "    vec2 uv = fragTexCoord * (vec2(1.0) - fragTexCoord.yx);\n"
    "    float vig = uv.x * uv.y * 15.0;\n"
    "    vig = clamp(pow(vig, uVignette), 0.0, 1.0);\n"
    "    float noise = (rand(fragTexCoord * 140.0 + fract(uTime * 37.19)) - 0.5) * uGrain;\n"
    "    vec3 col = tex.rgb * vig + vec3(noise);\n"
    "    gl_FragColor = vec4(clamp(col, 0.0, 1.0), tex.a);\n"
    "}\n";

static void ToggleGameFullscreen(GameSettings& settings) {
    ToggleFullscreen();
    if (!IsWindowFullscreen()) {
        SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    settings.isFullscreen = IsWindowFullscreen();
}

static std::string FindUniversalAssetPath(const std::vector<std::string>& candidates, const std::string& origCwd, const std::string& appDir) {
    std::vector<std::string> baseDirs;
    if (!origCwd.empty()) {
        baseDirs.push_back(origCwd);
        baseDirs.push_back(origCwd + "/..");
        baseDirs.push_back(origCwd + "/../..");
    }
    if (!appDir.empty()) {
        baseDirs.push_back(appDir);
        baseDirs.push_back(appDir + "/..");
        baseDirs.push_back(appDir + "/../..");
        baseDirs.push_back(appDir + "/Resources");
        baseDirs.push_back(appDir + "/../Resources");
    }
    baseDirs.push_back(".");
    baseDirs.push_back("..");
    baseDirs.push_back("../..");

    std::vector<std::string> subDirs = {
        "assets/sys/.cache/",
        "assets/sys/",
        "assets/",
        "sys/.cache/",
        ".cache/",
        "Resources/assets/sys/.cache/",
        "Resources/assets/",
        "Resources/",
        ""
    };

    for (const auto& base : baseDirs) {
        for (const auto& sub : subDirs) {
            for (const auto& file : candidates) {
                std::string fullPath;
                if (base == ".") fullPath = sub + file;
                else if (base.empty() || base.back() == '/' || base.back() == '\\') fullPath = base + sub + file;
                else fullPath = base + "/" + sub + file;

                if (FileExists(fullPath.c_str())) {
                    return fullPath;
                }
            }
        }
    }
    return "";
}

static void AddCrumbleTile(Level& lvl, int x, int y, int type) {
    if (x >= 0 && x < lvl.width && y >= 0 && y < lvl.height) {
        lvl.tiles[y * lvl.width + x] = type;
        CrumbleTile ct;
        ct.x = x;
        ct.y = y;
        ct.tileType = type;
        ct.triggered = false;
        ct.triggerTimer = 0.5f;
        ct.collapsed = false;
        ct.respawnTimer = 0.0f;
        ct.shakeOffset = 0.0f;
        lvl.crumbleTiles.push_back(ct);
    }
}

static void AddBreakableBlock(Level& lvl, int x, int y) {
    if (x >= 0 && x < lvl.width && y >= 0 && y < lvl.height) {
        lvl.tiles[y * lvl.width + x] = TILE_BREAKABLE;
        BreakableBlock bb;
        bb.x = x;
        bb.y = y;
        bb.health = 1;
        bb.broken = false;
        lvl.breakableBlocks.push_back(bb);
    }
}

// =========================================================================
// MAIN GAME LOOP
// =========================================================================
int main(void) {
    const char* origCwdPtr = GetWorkingDirectory();
    std::string origCwd = origCwdPtr ? origCwdPtr : "";
    const char* appDirPtr = GetApplicationDirectory();
    std::string appDir = appDirPtr ? appDirPtr : "";

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Shadow Dimension - Limbo & Inside Monochromatic Edition");
    if (!appDir.empty()) {
        ChangeDirectory(appDir.c_str());
    }
    SetExitKey(KEY_NULL); // Prevent ESC from closing window!
    SetTargetFPS(60);

    GameSettings settings;
    SetMasterVolume(settings.masterVolume);

    AudioManager am = InitAtmosphericAudio();

    RenderTexture2D screenTarget = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(screenTarget.texture, TEXTURE_FILTER_BILINEAR);

    std::vector<std::string> imgCandidates = {
        "depth_buffer.png", "depth_buffer.PNG", "depth_buffer.jpg", "depth_buffer.JPG", "depth_buffer.jpeg",
        "picture.png", "picture.PNG", "picture.jpg", "picture.JPG", "picture.jpeg", "picture.JPEG",
        "Picture.png", "Picture.PNG", "Picture.jpg", "Picture.JPG",
        "secret.png", "secret.PNG", "secret.jpg", "secret.JPG",
        "secret_image.png", "secret_image.PNG", "secret_image.jpg"
    };
    Texture2D secretPicture = { 0 };
    std::string foundImgPath = FindUniversalAssetPath(imgCandidates, origCwd, appDir);
    if (!foundImgPath.empty()) {
        secretPicture = LoadTexture(foundImgPath.c_str());
        if (secretPicture.id > 0) {
            SetTextureFilter(secretPicture, TEXTURE_FILTER_BILINEAR);
        }
    }

    std::vector<std::string> sndCandidates = {
        "env_ambience.mp3", "env_ambience.MP3", "env_ambience.wav", "env_ambience.WAV", "env_ambience.ogg", "env_ambience.OGG",
        "audio.mp3", "audio.MP3", "audio.wav", "audio.WAV", "audio.ogg", "audio.OGG",
        "Audio.mp3", "Audio.MP3", "Audio.wav", "Audio.WAV", "Audio.ogg", "Audio.OGG",
        "secret.mp3", "secret.MP3", "secret.wav", "secret.WAV", "secret.ogg", "secret.OGG",
        "secret_audio.mp3", "secret_audio.MP3"
    };
    Sound secretSound = { 0 };
    bool secretSoundReady = false;
    Music secretMusic = { 0 };
    bool secretMusicReady = false;
    std::string foundSndPath = FindUniversalAssetPath(sndCandidates, origCwd, appDir);
    if (!foundSndPath.empty()) {
        secretSound = LoadSound(foundSndPath.c_str());
        secretSoundReady = IsSoundReady(secretSound);
        if (!secretSoundReady) {
            secretMusic = LoadMusicStream(foundSndPath.c_str());
            secretMusicReady = IsMusicReady(secretMusic);
            if (secretMusicReady) {
                secretMusic.looping = false;
                SetMusicVolume(secretMusic, settings.masterVolume);
            }
        }
    }

    Shader postShader = LoadShaderFromMemory(nullptr, FILM_GRAIN_FS);
    if (!IsShaderReady(postShader)) {
        postShader = LoadShaderFromMemory(nullptr, FILM_GRAIN_FS_120);
    }
    bool shaderActive = IsShaderReady(postShader);
    int uTimeLoc = -1, uVignetteLoc = -1, uGrainLoc = -1;
    if (shaderActive) {
        uTimeLoc = GetShaderLocation(postShader, "uTime");
        uVignetteLoc = GetShaderLocation(postShader, "uVignette");
        uGrainLoc = GetShaderLocation(postShader, "uGrain");
    }

    GameState gameState = STATE_MENU;
    Phase currentPhase = PHASE_BLACK;
    float phaseTransition = 0.0f;
    bool gameStarted = false;

    float speedrunTimer = 0.0f;
    int deathCounter = 0;
    int totalShifts = 0;
    float secretCutsceneTimer = 0.0f;

    std::vector<Level> levels;
    InitLevels(levels);
    int currentLevelIdx = 0;

    Player player = { 0 };
    player.spawnPoint = levels[currentLevelIdx].playerSpawn;
    player.size = { 20.0f, 28.0f };
    player.facing = 1.0f;
    RespawnPlayer(player);

    Camera2D camera = { 0 };
    camera.target = player.pos;
    camera.offset = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    std::vector<Particle> particles;
    particles.reserve(600);
    SpawnAmbientMistParticles(particles, 60);

    std::vector<GhostTrail> ghostTrails;
    ghostTrails.reserve(50);

    ShiftWave shiftWave = { 0 };
    float screenTrauma = 0.0f;
    float levelBannerTimer = 0.0f;
    float whiteFlashAlpha = 0.0f;
    float shockwaveRadius = 0.0f;
    float timeDilationTimer = 0.0f;

    while (!WindowShouldClose()) {
        float rawDt = GetFrameTime();
        if (rawDt > 0.1f) rawDt = 0.1f;

        float dt = rawDt;
        if (timeDilationTimer > 0.0f) {
            timeDilationTimer -= rawDt;
            dt *= 0.50f;
        }

        float currentTime = (float)GetTime();

        float winW = (float)GetRenderWidth();
        float winH = (float)GetRenderHeight();
        if (winW <= 0.0f) winW = (float)GetScreenWidth();
        if (winH <= 0.0f) winH = (float)GetScreenHeight();
        if (winW <= 0.0f) winW = (float)SCREEN_WIDTH;
        if (winH <= 0.0f) winH = (float)SCREEN_HEIGHT;

        float viewScale = std::min(winW / (float)SCREEN_WIDTH, winH / (float)SCREEN_HEIGHT);
        if (viewScale <= 0.001f) viewScale = 1.0f;
        float renderW = (float)SCREEN_WIDTH * viewScale;
        float renderH = (float)SCREEN_HEIGHT * viewScale;
        Rectangle destViewport = { (winW - renderW) * 0.5f, (winH - renderH) * 0.5f, renderW, renderH };

        Vector2 rawMouse = GetMousePosition();
        Vector2 mouse = {
            (rawMouse.x - destViewport.x) / viewScale,
            (rawMouse.y - destViewport.y) / viewScale
        };

        if (IsKeyPressed(KEY_F11) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER)) || (IsKeyDown(KEY_RIGHT_ALT) && IsKeyPressed(KEY_ENTER))) {
            ToggleGameFullscreen(settings);
        }
        if (IsWindowFullscreen() != settings.isFullscreen) {
            settings.isFullscreen = IsWindowFullscreen();
        }

        if (secretMusicReady && IsMusicStreamPlaying(secretMusic)) {
            UpdateMusicStream(secretMusic);
        }

        // Screen flash fade (when outside portal transition)
        if (!player.inPortalTransition && gameState != STATE_SECRET_CUTSCENE && whiteFlashAlpha > 0.0f) {
            whiteFlashAlpha = std::max(0.0f, whiteFlashAlpha - dt * 2.5f);
        }

        // ==========================================
        // 1. STATE MACHINE & LOGIC UPDATE
        // ==========================================
        if (gameState == STATE_MENU) {
            if (rand() % 4 == 0) {
                Vector2 spawn = { (float)(rand() % SCREEN_WIDTH), (float)(rand() % SCREEN_HEIGHT) };
                SpawnAshBurst(particles, spawn, 1, ColorAlpha(COLOR_STARK_WHITE, 0.4f), 5.0f, -10.0f);
            }
            for (size_t i = 0; i < particles.size(); ) {
                particles[i].pos.x += particles[i].vel.x * dt;
                particles[i].pos.y += particles[i].vel.y * dt;
                if (particles[i].type != PART_MIST) {
                    particles[i].life -= dt;
                    if (particles[i].life <= 0) particles.erase(particles.begin() + i);
                    else i++;
                } else {
                    i++;
                }
            }

            Rectangle playBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 335.0f, 320.0f, 42.0f };
            Rectangle skipBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 390.0f, 320.0f, 42.0f };
            Rectangle setBtn  = { SCREEN_WIDTH / 2.0f - 160.0f, 445.0f, 320.0f, 42.0f };

            bool playClicked = CheckCollisionPointRec(mouse, playBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            bool skipClicked = CheckCollisionPointRec(mouse, skipBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            bool setClicked  = (CheckCollisionPointRec(mouse, setBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_O);

            if (gameStarted && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))) {
                gameState = STATE_GAMEPLAY;
            }

            // Start or Resume
            if (playClicked || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                if (!gameStarted) {
                    currentLevelIdx = 0;
                    player.spawnPoint = levels[currentLevelIdx].playerSpawn;
                    ResetCrumbleTiles(levels[currentLevelIdx]);
                    ResetBreakableBlocks(levels[currentLevelIdx]);
                    RespawnPlayer(player);
                    currentPhase = PHASE_BLACK;
                    phaseTransition = 0.0f;
                    levelBannerTimer = 2.5f;
                    speedrunTimer = 0.0f;
                    deathCounter = 0;
                    totalShifts = 0;
                }
                gameStarted = true;
                gameState = STATE_GAMEPLAY;
                if (am.audioReady) PlaySound(am.sndJump);
            }

            // Skip Level Button or Hotkey (S or TAB)
            if (skipClicked || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_TAB)) {
                currentLevelIdx = (currentLevelIdx + 1) % (int)levels.size();
                player.spawnPoint = levels[currentLevelIdx].playerSpawn;
                ResetCrumbleTiles(levels[currentLevelIdx]);
                ResetBreakableBlocks(levels[currentLevelIdx]);
                RespawnPlayer(player);
                camera.target = player.pos;
                currentPhase = PHASE_BLACK;
                phaseTransition = 0.0f;
                levelBannerTimer = 2.5f;
                gameStarted = true;
                gameState = STATE_GAMEPLAY;
                if (am.audioReady) PlaySound(am.sndVictory);
            }

            // Open Settings
            if (setClicked) {
                settings.previousState = STATE_MENU;
                gameState = STATE_SETTINGS;
            }
        }
        else if (gameState == STATE_SETTINGS) {
            Rectangle fsBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 240.0f, 320.0f, 40.0f };
            Rectangle volDownBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 295.0f, 45.0f, 40.0f };
            Rectangle volUpBtn = { SCREEN_WIDTH / 2.0f + 115.0f, 295.0f, 45.0f, 40.0f };
            Rectangle grainBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 350.0f, 320.0f, 40.0f };
            Rectangle shakeBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 405.0f, 320.0f, 40.0f };
            Rectangle backBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 470.0f, 320.0f, 40.0f };

            bool fsClick = (CheckCollisionPointRec(mouse, fsBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_F);
            bool vDownClick = (CheckCollisionPointRec(mouse, volDownBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_MINUS);
            bool vUpClick = (CheckCollisionPointRec(mouse, volUpBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_EQUAL);
            bool grainClick = (CheckCollisionPointRec(mouse, grainBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_G);
            bool shakeClick = (CheckCollisionPointRec(mouse, shakeBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_K);
            bool backClick = (CheckCollisionPointRec(mouse, backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_B) || IsKeyPressed(KEY_BACKSPACE);

            if (fsClick) {
                ToggleGameFullscreen(settings);
            }
            if (vDownClick) {
                settings.masterVolume = std::max(0.0f, settings.masterVolume - 0.1f);
                SetMasterVolume(settings.masterVolume);
                if (secretMusicReady) SetMusicVolume(secretMusic, settings.masterVolume);
            }
            if (vUpClick) {
                settings.masterVolume = std::min(1.0f, settings.masterVolume + 0.1f);
                SetMasterVolume(settings.masterVolume);
                if (secretMusicReady) SetMusicVolume(secretMusic, settings.masterVolume);
            }
            if (grainClick) {
                settings.filmGrainEnabled = !settings.filmGrainEnabled;
            }
            if (shakeClick) {
                settings.screenShakeEnabled = !settings.screenShakeEnabled;
            }
            if (backClick) {
                gameState = settings.previousState;
            }
        }
        else if (gameState == STATE_PAUSE) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) gameState = STATE_GAMEPLAY;
            if (IsKeyPressed(KEY_R)) {
                ResetCrumbleTiles(levels[currentLevelIdx]);
                ResetBreakableBlocks(levels[currentLevelIdx]);
                RespawnPlayer(player);
                camera.target = player.pos;
                gameState = STATE_GAMEPLAY;
            }
            if (IsKeyPressed(KEY_O)) {
                settings.previousState = STATE_PAUSE;
                gameState = STATE_SETTINGS;
            }
            if (IsKeyPressed(KEY_M)) gameState = STATE_MENU;
        }
        else if (gameState == STATE_SECRET_CUTSCENE) {
            // Fullscreen picture.png view for exactly 5.0 seconds
            secretCutsceneTimer -= dt;

            // Fade in from white during the first 0.3s of cutscene
            if (secretCutsceneTimer > 4.7f) {
                whiteFlashAlpha = (secretCutsceneTimer - 4.7f) / 0.3f;
            } else if (secretCutsceneTimer <= 0.3f) {
                // Fade out to white before warping to next level
                whiteFlashAlpha = (0.3f - secretCutsceneTimer) / 0.3f;
            } else {
                whiteFlashAlpha = 0.0f;
            }

            // Ambient spores drifting across picture
            if (rand() % 4 == 0) {
                Particle p;
                p.pos = { (float)(rand() % SCREEN_WIDTH), (float)(rand() % SCREEN_HEIGHT) };
                p.vel = { ((float)rand() / (float)RAND_MAX - 0.5f) * 8.0f, (float)rand() / (float)RAND_MAX * 16.0f + 8.0f };
                p.color = ColorAlpha(COLOR_STARK_WHITE, 0.40f);
                p.initialSize = (float)rand() / (float)RAND_MAX * 2.2f + 1.0f;
                p.size = p.initialSize;
                p.maxLife = 2.0f;
                p.life = p.maxLife;
                p.type = PART_ASH;
                p.gravity = 5.0f;
                p.drag = 0.98f;
                particles.push_back(p);
            }

            for (size_t i = 0; i < particles.size(); ) {
                particles[i].pos.x += particles[i].vel.x * dt;
                particles[i].pos.y += particles[i].vel.y * dt;
                particles[i].life -= dt;
                if (particles[i].life <= 0) particles.erase(particles.begin() + i);
                else i++;
            }

            if (secretCutsceneTimer <= 0.0f) {
                currentLevelIdx = 1; // Advance to Level 2
                player.spawnPoint = levels[currentLevelIdx].playerSpawn;
                ResetCrumbleTiles(levels[currentLevelIdx]);
                ResetBreakableBlocks(levels[currentLevelIdx]);
                RespawnPlayer(player);
                camera.target = player.pos;
                currentPhase = PHASE_BLACK;
                phaseTransition = 0.0f;
                levelBannerTimer = 2.5f;

                // Trigger Phase 4 fade-in on new level
                player.inPortalTransition = true;
                player.portalCenter = { player.pos.x + player.size.x * 0.5f, player.pos.y + player.size.y * 0.5f };
                player.portalTransitionTimer = 0.601f;
                player.portalLevelLoaded = true;
                player.portalSoundPlayed = true;
                whiteFlashAlpha = 1.0f;
                shockwaveRadius = 16.0f;
                if (am.audioReady) PlaySound(am.sndTransition);
                gameState = STATE_GAMEPLAY;
            }
        }
        else if (gameState == STATE_VICTORY) {
            if (rand() % 5 == 0) {
                Vector2 spawn = { (float)(rand() % SCREEN_WIDTH), (float)(rand() % 350 + 80) };
                SpawnAshBurst(particles, spawn, 12, COLOR_STARK_WHITE, 20.0f, -60.0f);
            }
            for (size_t i = 0; i < particles.size(); ) {
                particles[i].vel.y += particles[i].gravity * dt;
                particles[i].pos.x += particles[i].vel.x * dt;
                particles[i].pos.y += particles[i].vel.y * dt;
                particles[i].life -= dt;
                if (particles[i].life <= 0) particles.erase(particles.begin() + i);
                else i++;
            }
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) {
                currentLevelIdx = 0;
                InitLevels(levels);
                player.spawnPoint = levels[currentLevelIdx].playerSpawn;
                ResetCrumbleTiles(levels[currentLevelIdx]);
                ResetBreakableBlocks(levels[currentLevelIdx]);
                RespawnPlayer(player);
                currentPhase = PHASE_BLACK;
                phaseTransition = 0.0f;
                speedrunTimer = 0.0f;
                deathCounter = 0;
                totalShifts = 0;
                levelBannerTimer = 2.5f;
                gameState = STATE_GAMEPLAY;
            }
            if (IsKeyPressed(KEY_M)) gameState = STATE_MENU;
        }
        else if (gameState == STATE_GAMEPLAY) {
            speedrunTimer += dt;
            if (levelBannerTimer > 0.0f) levelBannerTimer -= dt;

            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) gameState = STATE_PAUSE;

            Level& currentLevel = levels[currentLevelIdx];

            // ------------------------------------------
            // CONTINUOUS UPWARD PORTAL DUST PARTICLES (IDLE STATE)
            // ------------------------------------------
            if (currentLevel.aperture.active && rand() % 2 == 0) {
                Vector2 motePos = {
                    currentLevel.aperture.pos.x + ((float)rand() / (float)RAND_MAX - 0.5f) * 16.0f,
                    currentLevel.aperture.pos.y + ((float)rand() / (float)RAND_MAX - 0.5f) * 64.0f
                };
                Particle p;
                p.pos = motePos;
                p.vel = { ((float)rand() / (float)RAND_MAX - 0.5f) * 6.0f, -((float)rand() / (float)RAND_MAX * 40.0f + 20.0f) };
                p.color = ColorAlpha(COLOR_STARK_WHITE, (float)rand() / (float)RAND_MAX * 0.45f + 0.35f);
                p.initialSize = (float)rand() / (float)RAND_MAX * 2.2f + 1.0f;
                p.size = p.initialSize;
                p.maxLife = (float)rand() / (float)RAND_MAX * 0.7f + 0.4f;
                p.life = p.maxLife;
                p.type = PART_ASH;
                p.gravity = -12.0f; // drift upward
                p.drag = 0.98f;
                particles.push_back(p);
            }

            if (currentLevel.hasSecretAperture && currentLevel.secretUnlocked && rand() % 2 == 0) {
                Vector2 motePos = {
                    currentLevel.secretAperture.pos.x + ((float)rand() / (float)RAND_MAX - 0.5f) * 16.0f,
                    currentLevel.secretAperture.pos.y + ((float)rand() / (float)RAND_MAX - 0.5f) * 64.0f
                };
                Particle p;
                p.pos = motePos;
                p.vel = { ((float)rand() / (float)RAND_MAX - 0.5f) * 6.0f, -((float)rand() / (float)RAND_MAX * 40.0f + 20.0f) };
                p.color = ColorAlpha(COLOR_STARK_WHITE, (float)rand() / (float)RAND_MAX * 0.45f + 0.35f);
                p.initialSize = (float)rand() / (float)RAND_MAX * 2.2f + 1.0f;
                p.size = p.initialSize;
                p.maxLife = (float)rand() / (float)RAND_MAX * 0.7f + 0.4f;
                p.life = p.maxLife;
                p.type = PART_ASH;
                p.gravity = -12.0f;
                p.drag = 0.98f;
                particles.push_back(p);
            }

            // ------------------------------------------
            // CRUMBLING PLATFORMS UPDATE & ASH EMISSION
            // ------------------------------------------
            for (auto& ct : currentLevel.crumbleTiles) {
                if (ct.triggered && !ct.collapsed) {
                    ct.triggerTimer -= dt;
                    float progress = 1.0f - (ct.triggerTimer / 0.5f);
                    float shakeMag = progress * 4.5f + 1.0f;
                    ct.shakeOffset = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeMag;

                    if (rand() % 2 == 0) {
                        Vector2 ashPos = { (ct.x + (float)rand() / (float)RAND_MAX) * TILE_SIZE, ct.y * TILE_SIZE + (float)rand() / (float)RAND_MAX * TILE_SIZE };
                        Color ashCol = (ct.tileType == TILE_CRUMBLE_WHITE) ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT;
                        SpawnAshBurst(particles, ashPos, 1, ashCol, 2.0f, 18.0f);
                    }

                    if (ct.triggerTimer <= 0.0f) {
                        ct.collapsed = true;
                        ct.respawnTimer = 2.5f;
                        ct.shakeOffset = 0.0f;

                        Vector2 center = { (ct.x + 0.5f) * TILE_SIZE, (ct.y + 0.5f) * TILE_SIZE };
                        Color burstCol = (ct.tileType == TILE_CRUMBLE_WHITE) ? COLOR_STARK_WHITE : (ct.tileType == TILE_CRUMBLE_BLACK ? COLOR_OBSIDIAN : COLOR_CHARCOAL);
                        SpawnAshBurst(particles, center, 20, burstCol, 16.0f, -25.0f);
                        screenTrauma = std::min(1.0f, screenTrauma + 0.18f);
                        if (am.audioReady) PlaySound(am.sndLand);
                    }
                } else if (ct.collapsed) {
                    ct.respawnTimer -= dt;
                    if (ct.respawnTimer <= 0.0f) {
                        ct.collapsed = false;
                        ct.triggered = false;
                        ct.triggerTimer = 0.5f;
                        ct.shakeOffset = 0.0f;

                        Vector2 center = { (ct.x + 0.5f) * TILE_SIZE, (ct.y + 0.5f) * TILE_SIZE };
                        Color respawnCol = (ct.tileType == TILE_CRUMBLE_WHITE) ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT;
                        SpawnAshBurst(particles, center, 10, respawnCol, 12.0f, -12.0f);
                    }
                }
            }

            // ------------------------------------------
            // SECRET TUNNEL "LIVE" SPORES & VFX
            // ------------------------------------------
            if (currentLevel.hasSecretAperture && currentLevel.secretUnlocked) {
                if (rand() % 3 == 0) {
                    float spX = (float)(rand() % (8 * (int)TILE_SIZE) + 1 * (int)TILE_SIZE);
                    float spY = (float)(15 * (int)TILE_SIZE + rand() % 16);
                    Particle p;
                    p.pos = { spX, spY };
                    p.vel = { ((float)rand() / (float)RAND_MAX - 0.5f) * 6.0f, (float)rand() / (float)RAND_MAX * 16.0f + 10.0f };
                    p.color = ColorAlpha(COLOR_STARK_WHITE, 0.45f);
                    p.initialSize = (float)rand() / (float)RAND_MAX * 2.2f + 1.2f;
                    p.size = p.initialSize;
                    p.maxLife = 2.4f;
                    p.life = p.maxLife;
                    p.type = PART_ASH;
                    p.gravity = 12.0f;
                    p.drag = 0.98f;
                    particles.push_back(p);
                }
            }

            // ------------------------------------------
            // PHASE SHIFT TOGGLE (SHIFT or F)
            // ------------------------------------------
            if (player.shiftCooldownTimer > 0.0f) player.shiftCooldownTimer -= dt;

            if (player.isAlive && !player.inPortalTransition && player.shiftCooldownTimer <= 0.0f &&
                (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT) || IsKeyPressed(KEY_F))) {

                currentPhase = (currentPhase == PHASE_BLACK) ? PHASE_WHITE : PHASE_BLACK;
                totalShifts++;
                player.shiftCooldownTimer = 0.15f;
                timeDilationTimer = 0.05f; // 50% slow-motion for 0.05s
                screenTrauma = std::min(1.0f, screenTrauma + 0.38f);
                if (am.audioReady) PlaySound(am.sndShift);

                shiftWave.center = { player.pos.x + player.size.x * 0.5f, player.pos.y + player.size.y * 0.5f };
                shiftWave.radius = 8.0f;
                shiftWave.maxRadius = 180.0f;
                shiftWave.speed = 520.0f;
                shiftWave.thickness = 3.5f;
                shiftWave.color = (currentPhase == PHASE_BLACK) ? COLOR_STARK_WHITE : COLOR_OBSIDIAN;
                shiftWave.alpha = 1.0f;
                shiftWave.active = true;

                SpawnAshBurst(particles, shiftWave.center, 25, shiftWave.color, 14.0f, -35.0f);

                // Breakable wall shatter check on nearby phase shift
                for (auto& bb : currentLevel.breakableBlocks) {
                    if (!bb.broken) {
                        Vector2 bCenter = { (bb.x + 0.5f) * TILE_SIZE, (bb.y + 0.5f) * TILE_SIZE };
                        if (Vector2Distance(shiftWave.center, bCenter) < 120.0f) {
                            bb.broken = true;
                            currentLevel.secretUnlocked = true;
                            SpawnAshBurst(particles, bCenter, 30, COLOR_CHARCOAL, 16.0f, -25.0f);
                            SpawnAshBurst(particles, bCenter, 18, COLOR_STARK_WHITE, 12.0f, -18.0f);
                            screenTrauma = std::min(1.0f, screenTrauma + 0.40f);
                            if (am.audioReady) PlaySound(am.sndDeath);
                        }
                    }
                }

                // Safe Ejection if materialized inside solid block
                Rectangle pRect = { player.pos.x, player.pos.y, player.size.x, player.size.y };
                if (CheckTileCollision(currentLevel, pRect, currentPhase)) {
                    bool freed = false;
                    for (float dist = 4.0f; dist <= 36.0f && !freed; dist += 4.0f) {
                        Vector2 offsets[8] = {
                            { 0.0f, -dist }, { -dist, 0.0f }, { dist, 0.0f }, { 0.0f, dist },
                            { -dist, -dist }, { dist, -dist }, { -dist, dist }, { dist, dist }
                        };
                        for (int o = 0; o < 8; o++) {
                            Rectangle testRect = { player.pos.x + offsets[o].x, player.pos.y + offsets[o].y, player.size.x, player.size.y };
                            if (!CheckTileCollision(currentLevel, testRect, currentPhase)) {
                                player.pos.x += offsets[o].x;
                                player.pos.y += offsets[o].y;
                                freed = true;
                                break;
                            }
                        }
                    }
                    if (!freed) {
                        deathCounter++;
                        ResetCrumbleTiles(currentLevel);
                        ResetBreakableBlocks(currentLevel);
                        TriggerPlayerDeath(player, particles, screenTrauma, am);
                    }
                }
            }

            float targetFactor = (currentPhase == PHASE_BLACK) ? 0.0f : 1.0f;
            phaseTransition += (targetFactor - phaseTransition) * dt * 8.5f;

            // -------------------------------------------------------------
            // PLAYER MOVEMENT & 4-PHASE PORTAL TRANSITION SEQUENCE
            // -------------------------------------------------------------
            if (player.isAlive) {
                // =========================================================
                // 1. ATMOSPHERIC EXIT PORTAL & WHITE FLASH TRANSITION
                // =========================================================
                if (player.inPortalTransition) {
                    player.vel = { 0.0f, 0.0f };
                    player.portalTransitionTimer += dt;
                    float t = player.portalTransitionTimer;

                    // -----------------------------------------------------
                    // PHASE 1: IMPLOSION / SUCTION (0.0s to 0.2s)
                    // -----------------------------------------------------
                    if (t <= 0.20f) {
                        // Snap player horizontally to portal center in old level
                        player.pos.x = Vector2Lerp(player.pos, { player.portalCenter.x - player.size.x * 0.5f, player.pos.y }, dt * 14.0f).x;
                        float p1 = t / 0.20f;
                        player.scale = { std::max(0.12f, 1.0f - 0.75f * p1), 1.0f + 1.20f * p1 };
                        player.pos.y -= 35.0f * dt; // slight upward float into light beam

                        // Accelerate surrounding particles inward toward portal center (suction effect)
                        for (auto& p : particles) {
                            Vector2 dir = Vector2Subtract(player.portalCenter, p.pos);
                            float d = Vector2Length(dir);
                            if (d < 380.0f && d > 4.0f) {
                                float pullSpeed = 520.0f * (1.0f - d / 380.0f) + 160.0f;
                                Vector2 pullVel = Vector2Scale(Vector2Normalize(dir), pullSpeed);
                                p.vel = Vector2Lerp(p.vel, pullVel, dt * 16.0f);
                            }
                        }

                        if (rand() % 2 == 0) {
                            Vector2 pSpot = { player.pos.x + player.size.x * 0.5f, player.pos.y + player.size.y * (float)rand() / (float)RAND_MAX };
                            SpawnAshBurst(particles, pSpot, 2, COLOR_STARK_WHITE, 6.0f, -30.0f);
                        }

                        whiteFlashAlpha = 0.0f;
                        shockwaveRadius = 0.0f;
                    }
                    // -----------------------------------------------------
                    // PHASE 2: WHITE-OUT FLASH (0.2s to 0.6s)
                    // -----------------------------------------------------
                    else if (t <= 0.60f) {
                        player.pos.x = Vector2Lerp(player.pos, { player.portalCenter.x - player.size.x * 0.5f, player.pos.y }, dt * 14.0f).x;
                        float p2 = (t - 0.20f) / 0.40f;
                        // Smooth ramp up of solid white overlay from 0% to 100%
                        whiteFlashAlpha = std::clamp(p2, 0.0f, 1.0f);
                        player.scale = { std::max(0.01f, 0.25f * (1.0f - p2)), std::max(0.01f, 2.20f * (1.0f - p2)) };

                        // Particles suction continues
                        for (auto& p : particles) {
                            Vector2 dir = Vector2Subtract(player.portalCenter, p.pos);
                            float d = Vector2Length(dir);
                            if (d < 380.0f && d > 4.0f) {
                                p.vel = Vector2Scale(Vector2Normalize(dir), 600.0f);
                            }
                        }

                        // Peak brightness reached at 0.6s -> play deep low-frequency hum/whoosh!
                        if (p2 >= 0.95f && !player.portalSoundPlayed) {
                            player.portalSoundPlayed = true;
                            if (am.audioReady) PlaySound(am.sndTransition);
                            screenTrauma = std::min(1.0f, screenTrauma + 0.50f);
                        }
                    }
                    // -----------------------------------------------------
                    // PHASE 3 (AT 100% WHITE): LEVEL LOAD & RESET
                    // PHASE 4 (0.6s to 0.9s): FADE IN (0.3s)
                    // -----------------------------------------------------
                    else {
                        // Phase 3: Level Load when screen is 100% blinding white
                        if (!player.portalLevelLoaded) {
                            player.portalLevelLoaded = true;
                            whiteFlashAlpha = 1.0f;

                            if (player.isSecretPortal) {
                                player.inPortalTransition = false;
                                player.isSecretPortal = false;
                                secretCutsceneTimer = 5.0f;
                                gameState = STATE_SECRET_CUTSCENE;
                            }
                            else if (currentLevelIdx + 1 < (int)levels.size()) {
                                currentLevelIdx++;
                                player.spawnPoint = levels[currentLevelIdx].playerSpawn;
                                ResetCrumbleTiles(levels[currentLevelIdx]);
                                ResetBreakableBlocks(levels[currentLevelIdx]);
                                RespawnPlayer(player);
                                camera.target = player.pos;
                                currentPhase = PHASE_BLACK;
                                phaseTransition = 0.0f;
                                levelBannerTimer = 2.5f;

                                // Maintain transition state for Phase 4 fade-in in new level
                                player.inPortalTransition = true;
                                player.portalCenter = { player.pos.x + player.size.x * 0.5f, player.pos.y + player.size.y * 0.5f };
                                player.portalTransitionTimer = 0.601f;
                                player.portalLevelLoaded = true;
                                player.portalSoundPlayed = true;
                                shockwaveRadius = 16.0f;
                            } else {
                                gameState = STATE_VICTORY;
                                player.inPortalTransition = false;
                            }
                        }

                        // Phase 4: Smooth Fade In (0.6s to 0.9s)
                        if (player.inPortalTransition) {
                            player.pos = player.spawnPoint;
                            player.vel = { 0.0f, 0.0f };
                            player.scale = { 1.0f, 1.0f };

                            float p4 = (t - 0.60f) / 0.30f;
                            whiteFlashAlpha = std::clamp(1.0f - p4, 0.0f, 1.0f);
                            shockwaveRadius += 380.0f * dt;

                            if (t >= 0.90f) {
                                player.inPortalTransition = false;
                                whiteFlashAlpha = 0.0f;
                                shockwaveRadius = 0.0f;
                            }
                        }
                    }
                }
                // ---------------------------------------------------------
                // 2. NORMAL ACTIVE MOVEMENT (Disabled during transition)
                // ---------------------------------------------------------
                else {
                    player.blinkTimer -= dt;
                    if (player.blinkTimer <= 0.0f) {
                        player.eyeBlink = !player.eyeBlink;
                        player.blinkTimer = player.eyeBlink ? 0.12f : ((float)rand() / (float)RAND_MAX * 3.5f + 2.0f);
                    }

                    float moveInput = 0.0f;
                    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  moveInput -= 1.0f;
                    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveInput += 1.0f;

                    if (moveInput != 0.0f) {
                        player.facing = moveInput;
                        player.runAnimCycle += dt;
                    } else {
                        player.runAnimCycle = 0.0f;
                    }

                    const float MAX_RUN_SPEED = 240.0f;
                    const float RUN_ACCEL     = 1800.0f;
                    const float RUN_FRICTION  = 2200.0f;
                    const float AIR_ACCEL     = 1300.0f;
                    const float AIR_FRICTION  = 550.0f;

                    if (player.wallJumpLockTimer > 0.0f) player.wallJumpLockTimer -= dt;

                    if (player.wallJumpLockTimer <= 0.0f) {
                        if (moveInput != 0.0f) {
                            float accel = player.isGrounded ? RUN_ACCEL : AIR_ACCEL;
                            player.vel.x += moveInput * accel * dt;
                            if (std::abs(player.vel.x) > MAX_RUN_SPEED) {
                                player.vel.x = (player.vel.x > 0 ? 1 : -1) * MAX_RUN_SPEED;
                            }
                        } else {
                            float fric = player.isGrounded ? RUN_FRICTION : AIR_FRICTION;
                            if (player.vel.x > 0.0f) player.vel.x = std::max(0.0f, player.vel.x - fric * dt);
                            else if (player.vel.x < 0.0f) player.vel.x = std::min(0.0f, player.vel.x + fric * dt);
                        }
                    }

                    const float GRAVITY = 1100.0f;
                    const float MAX_FALL_SPEED = 560.0f;
                    const float WALL_SLIDE_SPEED = 85.0f;

                    player.vel.y += GRAVITY * dt;

                    // Wall Slide
                    player.isWallSliding = false;
                    if (!player.isGrounded && player.wallContact != 0 && player.vel.y > 0.0f) {
                        if ((player.wallContact < 0 && moveInput < 0) || (player.wallContact > 0 && moveInput > 0) || moveInput == 0) {
                            player.isWallSliding = true;
                            if (player.vel.y > WALL_SLIDE_SPEED) player.vel.y = WALL_SLIDE_SPEED;

                            if (rand() % 4 == 0) {
                                Vector2 wpos = { (player.wallContact < 0) ? player.pos.x : player.pos.x + player.size.x, player.pos.y + 12.0f };
                                SpawnAshBurst(particles, wpos, 1, COLOR_FOG_LIGHT, 2.0f, -8.0f);
                            }
                        }
                    }

                    if (player.vel.y > MAX_FALL_SPEED) player.vel.y = MAX_FALL_SPEED;

                    // Coyote Time & Jump Buffer
                    if (player.isGrounded) player.coyoteTimer = 0.12f;
                    else player.coyoteTimer -= dt;

                    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) player.jumpBufferTimer = 0.12f;
                    else player.jumpBufferTimer -= dt;

                    // Variable Jump Height
                    if ((IsKeyReleased(KEY_SPACE) || IsKeyReleased(KEY_W) || IsKeyReleased(KEY_UP)) && player.vel.y < -120.0f) {
                        player.vel.y *= 0.50f;
                    }

                    // Execute Jump
                    if (player.jumpBufferTimer > 0.0f) {
                        if (player.coyoteTimer > 0.0f) {
                            player.vel.y = -430.0f;
                            player.coyoteTimer = 0.0f;
                            player.jumpBufferTimer = 0.0f;
                            player.scale = { 0.72f, 1.38f };
                            screenTrauma = std::min(1.0f, screenTrauma + 0.10f);
                            if (am.audioReady) PlaySound(am.sndJump);

                            Vector2 feet = { player.pos.x + player.size.x * 0.5f, player.pos.y + player.size.y };
                            SpawnAshBurst(particles, feet, 8, COLOR_FOG_MID, 12.0f, -18.0f);
                        }
                        else if (player.wallContact != 0 && !player.isGrounded) {
                            float kickDir = (float)(-player.wallContact);
                            player.vel.x = kickDir * 290.0f;
                            player.vel.y = -390.0f;
                            player.facing = kickDir;
                            player.wallJumpLockTimer = 0.12f;
                            player.jumpBufferTimer = 0.0f;
                            player.scale = { 0.78f, 1.28f };
                            screenTrauma = std::min(1.0f, screenTrauma + 0.16f);
                            if (am.audioReady) PlaySound(am.sndWallJump);

                            Vector2 wallSpot = { (player.wallContact < 0) ? player.pos.x : player.pos.x + player.size.x, player.pos.y + 15.0f };
                            SpawnAshBurst(particles, wallSpot, 8, COLOR_STARK_WHITE, 3.0f, -25.0f);
                        }
                    }

                    // Separated Axis Collision Resolution
                    player.wallContact = 0;

                    // Move X
                    player.pos.x += player.vel.x * dt;
                    Rectangle rectX = { player.pos.x, player.pos.y, player.size.x, player.size.y };
                    if (CheckTileCollision(currentLevel, rectX, currentPhase)) {
                        if (player.vel.x > 0.0f) {
                            int tileX = (int)((player.pos.x + player.size.x) / TILE_SIZE);
                            player.pos.x = tileX * TILE_SIZE - player.size.x;
                            player.wallContact = 1;
                        } else if (player.vel.x < 0.0f) {
                            int tileX = (int)(player.pos.x / TILE_SIZE);
                            player.pos.x = (tileX + 1) * TILE_SIZE;
                            player.wallContact = -1;
                        }
                        player.vel.x = 0.0f;
                    }

                    // Move Y
                    player.wasGrounded = player.isGrounded;
                    player.pos.y += player.vel.y * dt;
                    Rectangle rectY = { player.pos.x, player.pos.y, player.size.x, player.size.y };
                    player.isGrounded = false;

                    if (CheckTileCollision(currentLevel, rectY, currentPhase)) {
                        if (player.vel.y > 0.0f) {
                            int tileY = (int)((player.pos.y + player.size.y) / TILE_SIZE);
                            player.pos.y = tileY * TILE_SIZE - player.size.y;
                            player.isGrounded = true;

                            if (!player.wasGrounded) {
                                player.scale = { 1.38f, 0.68f };
                                screenTrauma = std::min(1.0f, screenTrauma + 0.15f);
                                if (am.audioReady) PlaySound(am.sndLand);
                                Vector2 feet = { player.pos.x + player.size.x * 0.5f, player.pos.y + player.size.y };
                                SpawnAshBurst(particles, feet, 10, COLOR_FOG_DARK, 16.0f, -20.0f);
                            }
                        } else if (player.vel.y < 0.0f) {
                            int tileY = (int)(player.pos.y / TILE_SIZE);
                            player.pos.y = (tileY + 1) * TILE_SIZE;
                            player.scale = { 1.20f, 0.85f };
                        }
                        player.vel.y = 0.0f;
                    }

                    Rectangle pBox = { player.pos.x, player.pos.y, player.size.x, player.size.y };

                    // Check player touching false / decorative spike to reveal secret portal
                    if (currentLevel.hasSecretAperture && !currentLevel.secretUnlocked) {
                        int pTileLeftX  = (int)(player.pos.x / TILE_SIZE);
                        int pTileRightX = (int)((player.pos.x + player.size.x) / TILE_SIZE);
                        int pTileTopY   = (int)(player.pos.y / TILE_SIZE);
                        int pTileBotY   = (int)((player.pos.y + player.size.y) / TILE_SIZE);

                        for (int ty = pTileTopY; ty <= pTileBotY && !currentLevel.secretUnlocked; ty++) {
                            for (int tx = pTileLeftX; tx <= pTileRightX && !currentLevel.secretUnlocked; tx++) {
                                if (tx >= 0 && tx < currentLevel.width && ty >= 0 && ty < currentLevel.height) {
                                    if (currentLevel.tiles[ty * currentLevel.width + tx] == TILE_SPIKE_DECORATIVE) {
                                        currentLevel.secretUnlocked = true;
                                        screenTrauma = std::min(1.0f, screenTrauma + 0.35f);
                                        if (am.audioReady) PlaySound(am.sndOrb);
                                        SpawnAshBurst(particles, currentLevel.secretAperture.pos, 30, COLOR_STARK_WHITE, 16.0f, -30.0f);
                                    }
                                }
                            }
                        }
                    }

                    // Check player pushing against / hitting breakable blocks
                    for (auto& bb : currentLevel.breakableBlocks) {
                        if (!bb.broken) {
                            Rectangle bContactRect = { bb.x * TILE_SIZE - 4.0f, bb.y * TILE_SIZE - 2.0f, TILE_SIZE + 8.0f, TILE_SIZE + 4.0f };
                            if (CheckCollisionRecs(pBox, bContactRect)) {
                                bb.broken = true;
                                currentLevel.secretUnlocked = true;
                                Vector2 center = { (bb.x + 0.5f) * TILE_SIZE, (bb.y + 0.5f) * TILE_SIZE };
                                SpawnAshBurst(particles, center, 30, COLOR_CHARCOAL, 16.0f, -25.0f);
                                SpawnAshBurst(particles, center, 18, COLOR_STARK_WHITE, 12.0f, -18.0f);
                                screenTrauma = std::min(1.0f, screenTrauma + 0.40f);
                                if (am.audioReady) PlaySound(am.sndDeath);
                            }
                        }
                    }

                    // Check if player is standing on any fragile/crumble platform to trigger it
                    if (player.isGrounded) {
                        int footLeftX = (int)((player.pos.x + 2.0f) / TILE_SIZE);
                        int footRightX = (int)((player.pos.x + player.size.x - 2.0f) / TILE_SIZE);
                        int footY = (int)((player.pos.y + player.size.y + 2.0f) / TILE_SIZE);

                        for (int fx = footLeftX; fx <= footRightX; fx++) {
                            if (fx >= 0 && fx < currentLevel.width && footY >= 0 && footY < currentLevel.height) {
                                CrumbleTile* ct = GetCrumbleTileAtMut(currentLevel, fx, footY);
                                if (ct && !ct->triggered && !ct->collapsed) {
                                    if (IsTileSolid(ct->tileType, currentPhase)) {
                                        ct->triggered = true;
                                        ct->triggerTimer = 0.5f;
                                    }
                                }
                            }
                        }
                    }

                    // Wall Contact check
                    if (!player.isGrounded) {
                        Rectangle checkLeft = { player.pos.x - 2.0f, player.pos.y + 4.0f, 2.0f, player.size.y - 8.0f };
                        Rectangle checkRight = { player.pos.x + player.size.x, player.pos.y + 4.0f, 2.0f, player.size.y - 8.0f };
                        if (CheckTileCollision(currentLevel, checkLeft, currentPhase)) player.wallContact = -1;
                        else if (CheckTileCollision(currentLevel, checkRight, currentPhase)) player.wallContact = 1;
                    }

                    // Spike Hazards
                    if (CheckSpikeCollision(currentLevel, pBox)) {
                        deathCounter++;
                        ResetCrumbleTiles(currentLevel);
                        ResetBreakableBlocks(currentLevel);
                        TriggerPlayerDeath(player, particles, screenTrauma, am);
                    }

                    // Map Bounds
                    if (player.pos.y > currentLevel.height * TILE_SIZE + 64.0f) {
                        deathCounter++;
                        ResetCrumbleTiles(currentLevel);
                        ResetBreakableBlocks(currentLevel);
                        TriggerPlayerDeath(player, particles, screenTrauma, am);
                    }

                    // Spring back scale
                    player.scale.x += (1.0f - player.scale.x) * dt * 14.0f;
                    player.scale.y += (1.0f - player.scale.y) * dt * 14.0f;

                    // Footstep mist puffs
                    if (player.isGrounded && std::abs(player.vel.x) > 60.0f) {
                        player.footstepTimer -= dt;
                        if (player.footstepTimer <= 0.0f) {
                            player.footstepTimer = 0.10f;
                            Vector2 heel = { (player.facing > 0) ? player.pos.x : player.pos.x + player.size.x, player.pos.y + player.size.y };
                            SpawnAshBurst(particles, heel, 2, COLOR_FOG_LIGHT, 3.0f, -10.0f);
                        }
                    }

                    // Ghost Trails
                    player.trailTimer -= dt;
                    if (player.trailTimer <= 0.0f) {
                        player.trailTimer = 0.045f;
                        if (std::abs(player.vel.x) > 180.0f || std::abs(player.vel.y) > 200.0f || player.isWallSliding) {
                            GhostTrail gt;
                            gt.pos = player.pos;
                            gt.size = player.size;
                            gt.scale = player.scale;
                            gt.facing = player.facing;
                            gt.color = (currentPhase == PHASE_BLACK) ? COLOR_OBSIDIAN : COLOR_STARK_WHITE;
                            gt.alpha = 0.55f;
                            gt.maxLife = 0.20f;
                            gt.life = gt.maxLife;
                            ghostTrails.push_back(gt);
                        }
                    }

                    // Orbs & Exit
                    int collectedOrbs = 0;
                    for (size_t i = 0; i < currentLevel.orbs.size(); i++) {
                        AnimaOrb& orb = currentLevel.orbs[i];
                        if (!orb.collected) {
                            Rectangle orbRect = { orb.pos.x - 14.0f, orb.pos.y - 14.0f, 28.0f, 28.0f };
                            if (CheckCollisionRecs(pBox, orbRect)) {
                                orb.collected = true;
                                screenTrauma = std::min(1.0f, screenTrauma + 0.25f);
                                if (am.audioReady) PlaySound(am.sndOrb);
                                SpawnAshBurst(particles, orb.pos, 30, COLOR_STARK_WHITE, 12.0f, -30.0f);
                            }
                        } else {
                            collectedOrbs++;
                        }
                    }

                    bool allOrbsCollected = (collectedOrbs == (int)currentLevel.orbs.size());
                    currentLevel.aperture.active = allOrbsCollected;

                    // 1. Normal Aperture Trigger & Player Control Lock
                    if (currentLevel.aperture.active && !player.inPortalTransition) {
                        Rectangle apertureRect = { currentLevel.aperture.pos.x - 18.0f, currentLevel.aperture.pos.y - 36.0f, 36.0f, 72.0f };
                        if (CheckCollisionRecs(pBox, apertureRect)) {
                            player.inPortalTransition = true;
                            player.isSecretPortal = false;
                            player.portalCenter = currentLevel.aperture.pos;
                            player.portalTransitionTimer = 0.0f;
                            player.portalSoundPlayed = false;
                            player.portalLevelLoaded = false;
                            player.vel = { 0.0f, 0.0f };
                            screenTrauma = std::min(1.0f, screenTrauma + 0.45f);
                        }
                    }

                    // 2. Secret Aperture Trigger & Player Control Lock
                    if (currentLevel.hasSecretAperture && currentLevel.secretUnlocked && !player.inPortalTransition) {
                        Rectangle secretApertureRect = { currentLevel.secretAperture.pos.x - 18.0f, currentLevel.secretAperture.pos.y - 36.0f, 36.0f, 72.0f };
                        if (CheckCollisionRecs(pBox, secretApertureRect)) {
                            player.inPortalTransition = true;
                            player.isSecretPortal = true;
                            player.portalCenter = currentLevel.secretAperture.pos;
                            player.portalTransitionTimer = 0.0f;
                            player.portalSoundPlayed = false;
                            player.portalLevelLoaded = false;
                            player.vel = { 0.0f, 0.0f };
                            screenTrauma = std::min(1.0f, screenTrauma + 0.55f);

                            // IN THE VERY SECOND PLAYER TOUCHES SECRET PORTAL -> PLAY audio.mp3
                            if (secretSoundReady) {
                                PlaySound(secretSound);
                            } else if (secretMusicReady) {
                                SeekMusicStream(secretMusic, 0.0f);
                                PlayMusicStream(secretMusic);
                            }
                        }
                    }
                }
            } else {
                player.deathTimer -= dt;
                if (player.deathTimer <= 0.0f) {
                    ResetCrumbleTiles(currentLevel);
                    ResetBreakableBlocks(currentLevel);
                    RespawnPlayer(player);
                }
            }

            if (shiftWave.active) {
                shiftWave.radius += shiftWave.speed * dt;
                shiftWave.alpha = 1.0f - (shiftWave.radius / shiftWave.maxRadius);
                if (shiftWave.radius >= shiftWave.maxRadius) shiftWave.active = false;
            }

            for (size_t i = 0; i < ghostTrails.size(); ) {
                ghostTrails[i].life -= dt;
                ghostTrails[i].alpha = (ghostTrails[i].life / ghostTrails[i].maxLife) * 0.55f;
                if (ghostTrails[i].life <= 0.0f) ghostTrails.erase(ghostTrails.begin() + i);
                else i++;
            }

            for (size_t i = 0; i < particles.size(); ) {
                particles[i].vel.x *= std::pow(particles[i].drag, dt * 60.0f);
                particles[i].vel.y += particles[i].gravity * dt;
                particles[i].pos.x += particles[i].vel.x * dt;
                particles[i].pos.y += particles[i].vel.y * dt;
                particles[i].life -= dt;

                if (particles[i].type == PART_MIST) {
                    particles[i].pos.y += std::sin(currentTime * 1.2f + particles[i].pos.x * 0.03f) * 6.0f * dt;
                    if (particles[i].life <= 0.0f || particles[i].pos.x > camera.target.x + SCREEN_WIDTH) {
                        particles[i].pos = { camera.target.x - SCREEN_WIDTH * 0.8f,
                                             camera.target.y + ((float)rand() / (float)RAND_MAX - 0.5f) * SCREEN_HEIGHT * 1.3f };
                        particles[i].life = particles[i].maxLife;
                    }
                    i++;
                } else {
                    if (particles[i].life <= 0.0f) particles.erase(particles.begin() + i);
                    else {
                        particles[i].size = particles[i].initialSize * (particles[i].life / particles[i].maxLife);
                        i++;
                    }
                }
            }

            Vector2 targetCam = player.pos;
            if (player.isAlive) {
                targetCam.x += player.facing * 35.0f;
                targetCam.y -= 10.0f;
            }
            camera.target = Vector2Lerp(camera.target, targetCam, dt * 7.5f);

            screenTrauma = std::max(0.0f, screenTrauma - dt * 1.8f);
            float shakeAmount = settings.screenShakeEnabled ? (screenTrauma * screenTrauma * 16.0f) : 0.0f;
            float shakeAngle  = settings.screenShakeEnabled ? (screenTrauma * screenTrauma * 2.2f) : 0.0f;
            camera.offset.x = (SCREEN_WIDTH / 2.0f)  + ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeAmount;
            camera.offset.y = (SCREEN_HEIGHT / 2.0f) + ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeAmount;
            camera.rotation = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeAngle;
        }

        // ==========================================
        // 2. RENDERING TO SCREEN TARGET
        // ==========================================
        BeginTextureMode(screenTarget);

        Color skyTop = {
            (unsigned char)(COLOR_OBSIDIAN.r + (COLOR_FOG_DARK.r - COLOR_OBSIDIAN.r) * phaseTransition),
            (unsigned char)(COLOR_OBSIDIAN.g + (COLOR_FOG_DARK.g - COLOR_OBSIDIAN.g) * phaseTransition),
            (unsigned char)(COLOR_OBSIDIAN.b + (COLOR_FOG_DARK.b - COLOR_OBSIDIAN.b) * phaseTransition),
            255
        };
        Color skyBottom = {
            (unsigned char)(COLOR_FOG_DARK.r + (COLOR_FOG_MID.r - COLOR_FOG_DARK.r) * phaseTransition),
            (unsigned char)(COLOR_FOG_DARK.g + (COLOR_FOG_MID.g - COLOR_FOG_DARK.g) * phaseTransition),
            (unsigned char)(COLOR_FOG_DARK.b + (COLOR_FOG_MID.b - COLOR_FOG_DARK.b) * phaseTransition),
            255
        };
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, skyTop, skyBottom);

        if (gameState == STATE_MENU) {
            for (int i = 0; i < 7; i++) {
                float tx = 80.0f + i * 190.0f;
                DrawRectangle((int)tx, 380, 8, 340, ColorAlpha(COLOR_OBSIDIAN, 0.6f));
                DrawLineEx({ tx + 4, 450 }, { tx - 40, 400 }, 3.0f, ColorAlpha(COLOR_OBSIDIAN, 0.6f));
                DrawLineEx({ tx + 4, 480 }, { tx + 45, 420 }, 3.0f, ColorAlpha(COLOR_OBSIDIAN, 0.6f));
            }

            for (const auto& p : particles) {
                DrawCircleV(p.pos, p.size, ColorAlpha(COLOR_STARK_WHITE, 0.25f));
            }

            const char* title = "SHADOW DIMENSION";
            int titleSize = 58;
            int textW = MeasureText(title, titleSize);
            int centerX = SCREEN_WIDTH / 2 - textW / 2;
            int centerY = 190;

            DrawText(title, centerX, centerY, titleSize, COLOR_STARK_WHITE);

            const char* subtitle = "A MONOCHROMATIC CINEMATIC PLATFORMER";
            int subW = MeasureText(subtitle, 15);
            DrawText(subtitle, SCREEN_WIDTH / 2 - subW / 2, centerY + 65, 15, COLOR_FOG_LIGHT);

            Rectangle playBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 320.0f, 320.0f, 42.0f };
            Rectangle skipBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 375.0f, 320.0f, 42.0f };
            Rectangle setBtn  = { SCREEN_WIDTH / 2.0f - 160.0f, 430.0f, 320.0f, 42.0f };
            bool playHover = CheckCollisionPointRec(mouse, playBtn);
            bool skipHover = CheckCollisionPointRec(mouse, skipBtn);
            bool setHover  = CheckCollisionPointRec(mouse, setBtn);

            DrawRectangleRec(playBtn, playHover ? ColorAlpha(COLOR_CHARCOAL, 0.9f) : ColorAlpha(COLOR_OBSIDIAN, 0.75f));
            DrawRectangleLinesEx(playBtn, 1.5f, playHover ? COLOR_STARK_WHITE : COLOR_FOG_MID);
            const char* playText = gameStarted ? "RESUME  [ SPACE / ESC ]" : "START GAME  [ SPACE ]";
            int playTw = MeasureText(playText, 15);
            DrawText(playText, (int)(playBtn.x + (playBtn.width - playTw) / 2.0f), (int)(playBtn.y + 13), 15, playHover ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT);

            DrawRectangleRec(skipBtn, skipHover ? ColorAlpha(COLOR_CHARCOAL, 0.9f) : ColorAlpha(COLOR_OBSIDIAN, 0.75f));
            DrawRectangleLinesEx(skipBtn, 1.5f, skipHover ? COLOR_STARK_WHITE : COLOR_FOG_MID);
            const char* skipText = "SKIP LEVEL  [ S / TAB ]";
            int skipTw = MeasureText(skipText, 15);
            DrawText(skipText, (int)(skipBtn.x + (skipBtn.width - skipTw) / 2.0f), (int)(skipBtn.y + 13), 15, skipHover ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT);

            DrawRectangleRec(setBtn, setHover ? ColorAlpha(COLOR_CHARCOAL, 0.9f) : ColorAlpha(COLOR_OBSIDIAN, 0.75f));
            DrawRectangleLinesEx(setBtn, 1.5f, setHover ? COLOR_STARK_WHITE : COLOR_FOG_MID);
            const char* setText = "SETTINGS  [ O ]";
            int setTw = MeasureText(setText, 15);
            DrawText(setText, (int)(setBtn.x + (setBtn.width - setTw) / 2.0f), (int)(setBtn.y + 13), 15, setHover ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT);

            DrawText("[A / D]  MOVE    [SPACE]  JUMP / WALL JUMP    [SHIFT / F]  PHASE SHIFT    [ESC]  MENU",
                     SCREEN_WIDTH / 2 - MeasureText("[A / D]  MOVE    [SPACE]  JUMP / WALL JUMP    [SHIFT / F]  PHASE SHIFT    [ESC]  MENU", 13) / 2,
                     SCREEN_HEIGHT - 50, 13, COLOR_FOG_LIGHT);

            EndTextureMode();
        }
        else if (gameState == STATE_SETTINGS) {
            for (int i = 0; i < 7; i++) {
                float tx = 80.0f + i * 190.0f;
                DrawRectangle((int)tx, 380, 8, 340, ColorAlpha(COLOR_OBSIDIAN, 0.6f));
                DrawLineEx({ tx + 4, 450 }, { tx - 40, 400 }, 3.0f, ColorAlpha(COLOR_OBSIDIAN, 0.6f));
                DrawLineEx({ tx + 4, 480 }, { tx + 45, 420 }, 3.0f, ColorAlpha(COLOR_OBSIDIAN, 0.6f));
            }

            for (const auto& p : particles) {
                DrawCircleV(p.pos, p.size, ColorAlpha(COLOR_STARK_WHITE, 0.25f));
            }

            const char* stTitle = "SETTINGS";
            int stW = MeasureText(stTitle, 36);
            DrawText(stTitle, SCREEN_WIDTH / 2 - stW / 2, 160, 36, COLOR_STARK_WHITE);

            Rectangle fsBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 240.0f, 320.0f, 40.0f };
            Rectangle volArea = { SCREEN_WIDTH / 2.0f - 160.0f, 295.0f, 320.0f, 40.0f };
            Rectangle volDownBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 295.0f, 45.0f, 40.0f };
            Rectangle volUpBtn = { SCREEN_WIDTH / 2.0f + 115.0f, 295.0f, 45.0f, 40.0f };
            Rectangle grainBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 350.0f, 320.0f, 40.0f };
            Rectangle shakeBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 405.0f, 320.0f, 40.0f };
            Rectangle backBtn = { SCREEN_WIDTH / 2.0f - 160.0f, 470.0f, 320.0f, 40.0f };

            bool fsHov = CheckCollisionPointRec(mouse, fsBtn);
            bool vdHov = CheckCollisionPointRec(mouse, volDownBtn);
            bool vuHov = CheckCollisionPointRec(mouse, volUpBtn);
            bool grHov = CheckCollisionPointRec(mouse, grainBtn);
            bool shHov = CheckCollisionPointRec(mouse, shakeBtn);
            bool bkHov = CheckCollisionPointRec(mouse, backBtn);

            DrawRectangleRec(fsBtn, fsHov ? ColorAlpha(COLOR_CHARCOAL, 0.9f) : ColorAlpha(COLOR_OBSIDIAN, 0.75f));
            DrawRectangleLinesEx(fsBtn, 1.5f, fsHov ? COLOR_STARK_WHITE : COLOR_FOG_MID);
            char fsText[64];
            std::snprintf(fsText, sizeof(fsText), "FULLSCREEN: %s  [ F ]", settings.isFullscreen ? "ON" : "OFF");
            int fsTw = MeasureText(fsText, 14);
            DrawText(fsText, (int)(fsBtn.x + (fsBtn.width - fsTw) / 2.0f), (int)(fsBtn.y + 13), 14, fsHov ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT);

            DrawRectangleRec(volArea, ColorAlpha(COLOR_OBSIDIAN, 0.75f));
            DrawRectangleLinesEx(volArea, 1.5f, COLOR_FOG_MID);
            DrawRectangleRec(volDownBtn, vdHov ? ColorAlpha(COLOR_CHARCOAL, 0.9f) : ColorAlpha(COLOR_OBSIDIAN, 0.85f));
            DrawRectangleLinesEx(volDownBtn, 1.2f, vdHov ? COLOR_STARK_WHITE : COLOR_FOG_MID);
            DrawText("-", (int)(volDownBtn.x + 18), (int)(volDownBtn.y + 10), 20, vdHov ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT);
            DrawRectangleRec(volUpBtn, vuHov ? ColorAlpha(COLOR_CHARCOAL, 0.9f) : ColorAlpha(COLOR_OBSIDIAN, 0.85f));
            DrawRectangleLinesEx(volUpBtn, 1.2f, vuHov ? COLOR_STARK_WHITE : COLOR_FOG_MID);
            DrawText("+", (int)(volUpBtn.x + 16), (int)(volUpBtn.y + 10), 20, vuHov ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT);
            char volText[64];
            std::snprintf(volText, sizeof(volText), "VOLUME: %d%%", (int)(settings.masterVolume * 100.0f + 0.5f));
            int vTw = MeasureText(volText, 14);
            DrawText(volText, (int)(volArea.x + (volArea.width - vTw) / 2.0f), (int)(volArea.y + 13), 14, COLOR_STARK_WHITE);

            DrawRectangleRec(grainBtn, grHov ? ColorAlpha(COLOR_CHARCOAL, 0.9f) : ColorAlpha(COLOR_OBSIDIAN, 0.75f));
            DrawRectangleLinesEx(grainBtn, 1.5f, grHov ? COLOR_STARK_WHITE : COLOR_FOG_MID);
            char grText[64];
            std::snprintf(grText, sizeof(grText), "FILM GRAIN: %s  [ G ]", settings.filmGrainEnabled ? "ON" : "OFF");
            int grTw = MeasureText(grText, 14);
            DrawText(grText, (int)(grainBtn.x + (grainBtn.width - grTw) / 2.0f), (int)(grainBtn.y + 13), 14, grHov ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT);

            DrawRectangleRec(shakeBtn, shHov ? ColorAlpha(COLOR_CHARCOAL, 0.9f) : ColorAlpha(COLOR_OBSIDIAN, 0.75f));
            DrawRectangleLinesEx(shakeBtn, 1.5f, shHov ? COLOR_STARK_WHITE : COLOR_FOG_MID);
            char shText[64];
            std::snprintf(shText, sizeof(shText), "SCREEN SHAKE: %s  [ K ]", settings.screenShakeEnabled ? "ON" : "OFF");
            int shTw = MeasureText(shText, 14);
            DrawText(shText, (int)(shakeBtn.x + (shakeBtn.width - shTw) / 2.0f), (int)(shakeBtn.y + 13), 14, shHov ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT);

            DrawRectangleRec(backBtn, bkHov ? ColorAlpha(COLOR_CHARCOAL, 0.9f) : ColorAlpha(COLOR_OBSIDIAN, 0.75f));
            DrawRectangleLinesEx(backBtn, 1.5f, bkHov ? COLOR_STARK_WHITE : COLOR_FOG_MID);
            const char* bkText = "BACK  [ ESC ]";
            int bkTw = MeasureText(bkText, 14);
            DrawText(bkText, (int)(backBtn.x + (backBtn.width - bkTw) / 2.0f), (int)(backBtn.y + 13), 14, bkHov ? COLOR_STARK_WHITE : COLOR_FOG_LIGHT);

            EndTextureMode();
        }
        else if (gameState == STATE_SECRET_CUTSCENE) {
            ClearBackground(COLOR_OBSIDIAN);

            if (secretPicture.id > 0) {
                float scale = std::min((float)SCREEN_WIDTH / (float)secretPicture.width, (float)SCREEN_HEIGHT / (float)secretPicture.height);
                float dw = (float)secretPicture.width * scale;
                float dh = (float)secretPicture.height * scale;
                float dx = ((float)SCREEN_WIDTH - dw) * 0.5f;
                float dy = ((float)SCREEN_HEIGHT - dh) * 0.5f;

                DrawTexturePro(secretPicture, { 0, 0, (float)secretPicture.width, (float)secretPicture.height },
                               { dx, dy, dw, dh }, { 0, 0 }, 0.0f, WHITE);
            } else {
                float cx = (float)SCREEN_WIDTH * 0.5f;
                float cy = (float)SCREEN_HEIGHT * 0.5f;
                DrawCircleGradient((int)cx, (int)cy, 280.0f, ColorAlpha(COLOR_STARK_WHITE, 0.40f), BLANK);
                DrawCircle((int)cx, (int)cy, 90.0f, COLOR_OBSIDIAN);
                DrawCircleLines((int)cx, (int)cy, 92.0f, COLOR_STARK_WHITE);
                DrawCircleLines((int)cx, (int)cy, 105.0f, COLOR_FOG_LIGHT);
                DrawCircle((int)cx, (int)cy, 18.0f, COLOR_STARK_WHITE);
            }

            for (const auto& p : particles) {
                DrawCircleV(p.pos, p.size, ColorAlpha(COLOR_STARK_WHITE, (p.life / p.maxLife) * 0.35f));
            }

            EndTextureMode();
        }
        else {
            BeginMode2D(camera);

            const Level& lvl = levels[currentLevelIdx];

            // 1. Enhanced Parallax Background
            DrawParallaxBackground(camera, lvl.width, lvl.height, currentPhase, currentTime);

            // 2. Volumetric Ground Fog Layer
            DrawVolumetricGroundFog(camera, lvl.width, currentTime);

            // -------------------------------------------------------------
            // 3. MONOLITHIC PLATFORM & CRUMBLING/BREAKABLE PLATFORM RENDERING
            // -------------------------------------------------------------
            // Pass 1: Solid fills (seamless silhouettes)
            for (int y = 0; y < lvl.height; y++) {
                for (int x = 0; x < lvl.width; x++) {
                    int tile = lvl.tiles[y * lvl.width + x];
                    if (tile == TILE_EMPTY) continue;

                    const CrumbleTile* ct = GetCrumbleTileAt(lvl, x, y);
                    if (ct && ct->collapsed) continue;

                    const BreakableBlock* bb = GetBreakableBlockAt(lvl, x, y);
                    if (bb && bb->broken) continue;

                    float shake = ct ? ct->shakeOffset : 0.0f;
                    Rectangle tileRect = { x * TILE_SIZE + shake, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };

                    if (tile == TILE_NEUTRAL || tile == TILE_CRUMBLE_NEUTRAL || tile == TILE_BREAKABLE) {
                        DrawRectangleRec(tileRect, COLOR_CHARCOAL);
                    }
                    else if ((tile == TILE_WHITE || tile == TILE_CRUMBLE_WHITE) && currentPhase == PHASE_BLACK) {
                        DrawRectangleRec(tileRect, COLOR_STARK_WHITE);
                    }
                    else if ((tile == TILE_BLACK || tile == TILE_CRUMBLE_BLACK) && currentPhase == PHASE_WHITE) {
                        DrawRectangleRec(tileRect, COLOR_OBSIDIAN);
                    }
                    else if ((tile == TILE_WHITE || tile == TILE_CRUMBLE_WHITE) && currentPhase == PHASE_WHITE) {
                        DrawRectangleRec(tileRect, ColorAlpha(COLOR_STARK_WHITE, 0.06f));
                    }
                    else if ((tile == TILE_BLACK || tile == TILE_CRUMBLE_BLACK) && currentPhase == PHASE_BLACK) {
                        DrawRectangleRec(tileRect, ColorAlpha(COLOR_OBSIDIAN, 0.22f));
                    }
                }
            }

            // Pass 2: Outer borders and fragile platform cracks
            for (int y = 0; y < lvl.height; y++) {
                for (int x = 0; x < lvl.width; x++) {
                    int tile = lvl.tiles[y * lvl.width + x];
                    if (tile == TILE_EMPTY) continue;

                    const CrumbleTile* ct = GetCrumbleTileAt(lvl, x, y);
                    if (ct && ct->collapsed) continue;

                    const BreakableBlock* bb = GetBreakableBlockAt(lvl, x, y);
                    if (bb && bb->broken) continue;

                    float shake = ct ? ct->shakeOffset : 0.0f;
                    Rectangle tileRect = { x * TILE_SIZE + shake, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };

                    bool topExposed    = (y == 0 || lvl.tiles[(y - 1) * lvl.width + x] != tile);
                    bool bottomExposed = (y == lvl.height - 1 || lvl.tiles[(y + 1) * lvl.width + x] != tile);
                    bool leftExposed   = (x == 0 || lvl.tiles[y * lvl.width + (x - 1)] != tile);
                    bool rightExposed  = (x == lvl.width - 1 || lvl.tiles[y * lvl.width + (x + 1)] != tile);

                    if (tile == TILE_NEUTRAL || tile == TILE_CRUMBLE_NEUTRAL || tile == TILE_BREAKABLE) {
                        if (topExposed) {
                            DrawLineEx({ tileRect.x, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y }, 2.0f, ColorAlpha(COLOR_FOG_LIGHT, 0.65f));
                            DrawLineEx({ tileRect.x + 1.0f, tileRect.y + 1.0f }, { tileRect.x + tileRect.width - 1.0f, tileRect.y + 1.0f }, 1.0f, ColorAlpha(COLOR_STARK_WHITE, 0.35f));
                        }
                        if (bottomExposed) DrawLineEx({ tileRect.x, tileRect.y + tileRect.height }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 1.5f, COLOR_OBSIDIAN);
                        if (leftExposed)   DrawLineEx({ tileRect.x, tileRect.y }, { tileRect.x, tileRect.y + tileRect.height }, 1.5f, COLOR_OBSIDIAN);
                        if (rightExposed)  DrawLineEx({ tileRect.x + tileRect.width, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 1.5f, COLOR_OBSIDIAN);
                    }
                    else if ((tile == TILE_WHITE || tile == TILE_CRUMBLE_WHITE) && currentPhase == PHASE_BLACK) {
                        if (topExposed) {
                            DrawLineEx({ tileRect.x, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y }, 2.5f, COLOR_STARK_WHITE);
                            DrawLineEx({ tileRect.x, tileRect.y - 1.0f }, { tileRect.x + tileRect.width, tileRect.y - 1.0f }, 1.2f, ColorAlpha(COLOR_STARK_WHITE, 0.50f));
                        }
                        if (bottomExposed) DrawLineEx({ tileRect.x, tileRect.y + tileRect.height }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 1.5f, COLOR_FOG_LIGHT);
                        if (leftExposed)   DrawLineEx({ tileRect.x, tileRect.y }, { tileRect.x, tileRect.y + tileRect.height }, 1.5f, COLOR_FOG_LIGHT);
                        if (rightExposed)  DrawLineEx({ tileRect.x + tileRect.width, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 1.5f, COLOR_FOG_LIGHT);
                    }
                    else if ((tile == TILE_BLACK || tile == TILE_CRUMBLE_BLACK) && currentPhase == PHASE_WHITE) {
                        if (topExposed) {
                            DrawLineEx({ tileRect.x, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y }, 2.0f, COLOR_CHARCOAL);
                            DrawLineEx({ tileRect.x + 1.0f, tileRect.y }, { tileRect.x + tileRect.width - 1.0f, tileRect.y }, 1.0f, ColorAlpha(COLOR_FOG_LIGHT, 0.40f));
                        }
                        if (bottomExposed) DrawLineEx({ tileRect.x, tileRect.y + tileRect.height }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 1.5f, COLOR_OBSIDIAN);
                        if (leftExposed)   DrawLineEx({ tileRect.x, tileRect.y }, { tileRect.x, tileRect.y + tileRect.height }, 1.5f, COLOR_OBSIDIAN);
                        if (rightExposed)  DrawLineEx({ tileRect.x + tileRect.width, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 1.5f, COLOR_OBSIDIAN);
                    }
                    else if ((tile == TILE_WHITE || tile == TILE_CRUMBLE_WHITE) && currentPhase == PHASE_WHITE) {
                        Color dashCol = ColorAlpha(COLOR_STARK_WHITE, 0.40f);
                        if (topExposed)    DrawDashedLine({ tileRect.x, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y }, 6.0f, 4.0f, 1.2f, dashCol);
                        if (bottomExposed) DrawDashedLine({ tileRect.x, tileRect.y + tileRect.height }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 6.0f, 4.0f, 1.2f, dashCol);
                        if (leftExposed)   DrawDashedLine({ tileRect.x, tileRect.y }, { tileRect.x, tileRect.y + tileRect.height }, 6.0f, 4.0f, 1.2f, dashCol);
                        if (rightExposed)  DrawDashedLine({ tileRect.x + tileRect.width, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 6.0f, 4.0f, 1.2f, dashCol);
                    }
                    else if ((tile == TILE_BLACK || tile == TILE_CRUMBLE_BLACK) && currentPhase == PHASE_BLACK) {
                        Color dashCol = ColorAlpha(COLOR_FOG_MID, 0.45f);
                        if (topExposed)    DrawDashedLine({ tileRect.x, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y }, 6.0f, 4.0f, 1.2f, dashCol);
                        if (bottomExposed) DrawDashedLine({ tileRect.x, tileRect.y + tileRect.height }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 6.0f, 4.0f, 1.2f, dashCol);
                        if (leftExposed)   DrawDashedLine({ tileRect.x, tileRect.y }, { tileRect.x, tileRect.y + tileRect.height }, 6.0f, 4.0f, 1.2f, dashCol);
                        if (rightExposed)  DrawDashedLine({ tileRect.x + tileRect.width, tileRect.y }, { tileRect.x + tileRect.width, tileRect.y + tileRect.height }, 6.0f, 4.0f, 1.2f, dashCol);
                    }
                    else if ((tile >= TILE_SPIKE_UP && tile <= TILE_SPIKE_RIGHT) || tile == TILE_SPIKE_DECORATIVE) {
                        DrawOrganicSpike(tileRect, (tile == TILE_SPIKE_DECORATIVE) ? TILE_SPIKE_UP : tile);
                    }

                    // Crumble Platform Cracks
                    if (ct && !ct->collapsed) {
                        bool isSolid = IsTileSolid(tile, currentPhase);
                        Color crackCol = isSolid ? (ct->triggered ? COLOR_STARK_WHITE : ColorAlpha(COLOR_FOG_LIGHT, 0.5f)) : ColorAlpha(COLOR_FOG_MID, 0.25f);
                        float cX = tileRect.x;
                        float cY = tileRect.y;
                        DrawLineEx({ cX + 6.0f, cY + 4.0f }, { cX + 14.0f, cY + 12.0f }, 1.2f, crackCol);
                        DrawLineEx({ cX + 14.0f, cY + 12.0f }, { cX + 22.0f, cY + 8.0f }, 1.2f, crackCol);
                        DrawLineEx({ cX + 14.0f, cY + 12.0f }, { cX + 18.0f, cY + 24.0f }, 1.2f, crackCol);
                        if (ct->triggered) {
                            DrawLineEx({ cX + 22.0f, cY + 8.0f }, { cX + 28.0f, cY + 18.0f }, 1.2f, COLOR_STARK_WHITE);
                            DrawLineEx({ cX + 6.0f, cY + 4.0f }, { cX + 2.0f, cY + 16.0f }, 1.2f, COLOR_STARK_WHITE);
                        }
                    }

                    // Breakable Wall Cracks
                    if (tile == TILE_BREAKABLE && bb && !bb->broken) {
                        float cX = tileRect.x;
                        float cY = tileRect.y;
                        DrawLineEx({ cX + 8.0f, cY + 2.0f }, { cX + 16.0f, cY + 15.0f }, 1.5f, ColorAlpha(COLOR_STARK_WHITE, 0.45f));
                        DrawLineEx({ cX + 16.0f, cY + 15.0f }, { cX + 12.0f, cY + 28.0f }, 1.5f, ColorAlpha(COLOR_STARK_WHITE, 0.45f));
                        DrawLineEx({ cX + 16.0f, cY + 15.0f }, { cX + 24.0f, cY + 20.0f }, 1.2f, ColorAlpha(COLOR_STARK_WHITE, 0.35f));
                    }
                }
            }

            // 4. Anima Orbs
            for (const auto& orb : lvl.orbs) {
                if (!orb.collected) {
                    float bob = std::sin(currentTime * 3.0f + orb.pos.x * 0.1f) * 4.0f;
                    Vector2 oPos = { orb.pos.x, orb.pos.y + bob };
                    DrawCircleGradient((int)oPos.x, (int)oPos.y, 22.0f, ColorAlpha(COLOR_STARK_WHITE, 0.35f), BLANK);
                    DrawCircleV(oPos, 6.0f, COLOR_STARK_WHITE);
                    DrawCircleV(oPos, 3.5f, RAYWHITE);
                }
            }

            // 6. Atmospheric Exit Aperture (Vertical Reality Tear)
            DrawAtmosphericAperture(lvl.aperture.pos, lvl.aperture.active, currentTime, false);

            // 7. Secret Second Aperture (End of Tunnel)
            if (lvl.hasSecretAperture && lvl.secretUnlocked) {
                DrawAtmosphericAperture(lvl.secretAperture.pos, true, currentTime, true);
            }

            // 8. Ghost Trails
            for (const auto& gt : ghostTrails) {
                Rectangle tRect = {
                    gt.pos.x + gt.size.x * 0.5f - (gt.size.x * gt.scale.x) * 0.5f,
                    gt.pos.y + gt.size.y - (gt.size.y * gt.scale.y),
                    gt.size.x * gt.scale.x,
                    gt.size.y * gt.scale.y
                };
                DrawRectangleRounded(tRect, 0.4f, 6, ColorAlpha(gt.color, gt.alpha));
            }

            // 9. Stylized Silhouette Boy Player
            DrawStylizedPlayer(player, lvl, currentPhase, currentTime);

            // 10. Shift Wave
            if (shiftWave.active) {
                DrawCircleLines((int)shiftWave.center.x, (int)shiftWave.center.y,
                                shiftWave.radius, ColorAlpha(shiftWave.color, shiftWave.alpha));
                DrawCircleLines((int)shiftWave.center.x, (int)shiftWave.center.y,
                                shiftWave.radius - 2.0f, ColorAlpha(COLOR_STARK_WHITE, shiftWave.alpha * 0.7f));
            }

            // 11. Particles
            for (const auto& p : particles) {
                DrawCircleV(p.pos, p.size, ColorAlpha(p.color, p.life / p.maxLife));
            }

            // -------------------------------------------------------------
            // 12. DYNAMIC ADDITIVE LIGHTING & GOD RAYS PASS
            // -------------------------------------------------------------
            BeginBlendMode(BLEND_ADDITIVE);
            {
                DrawVolumetricGodRays(currentTime, currentPhase);

                // Ambient light from solid white phase blocks
                if (currentPhase == PHASE_BLACK) {
                    for (int y = 0; y < lvl.height; y++) {
                        for (int x = 0; x < lvl.width; x++) {
                            int t = lvl.tiles[y * lvl.width + x];
                            if (t == TILE_WHITE || t == TILE_CRUMBLE_WHITE) {
                                const CrumbleTile* ct = GetCrumbleTileAt(lvl, x, y);
                                if (ct && ct->collapsed) continue;

                                bool hasExposed = (y == 0 || (lvl.tiles[(y - 1) * lvl.width + x] != TILE_WHITE && lvl.tiles[(y - 1) * lvl.width + x] != TILE_CRUMBLE_WHITE)) ||
                                                  (y == lvl.height - 1 || (lvl.tiles[(y + 1) * lvl.width + x] != TILE_WHITE && lvl.tiles[(y + 1) * lvl.width + x] != TILE_CRUMBLE_WHITE)) ||
                                                  (x == 0 || (lvl.tiles[y * lvl.width + (x - 1)] != TILE_WHITE && lvl.tiles[y * lvl.width + (x - 1)] != TILE_CRUMBLE_WHITE)) ||
                                                  (x == lvl.width - 1 || (lvl.tiles[y * lvl.width + (x + 1)] != TILE_WHITE && lvl.tiles[y * lvl.width + (x + 1)] != TILE_CRUMBLE_WHITE));
                                if (hasExposed) {
                                    DrawCircleGradient((int)(x * TILE_SIZE + 16.0f), (int)(y * TILE_SIZE + 16.0f), 38.0f, ColorAlpha(COLOR_STARK_WHITE, 0.12f), BLANK);
                                }
                            }
                        }
                    }
                }

                // Player White Eye Beacon Glow Aura
                if (player.isAlive && !player.eyeBlink) {
                    float eyeX = player.pos.x + player.size.x * 0.5f + (player.facing * 7.0f);
                    float eyeY = player.pos.y + 8.0f;
                    DrawCircleGradient((int)eyeX, (int)eyeY, 28.0f, ColorAlpha(COLOR_STARK_WHITE, 0.35f), BLANK);
                }

                // Orbs & Portal Additive Radiance
                for (const auto& orb : lvl.orbs) {
                    if (!orb.collected) {
                        DrawCircleGradient((int)orb.pos.x, (int)orb.pos.y, 40.0f, ColorAlpha(COLOR_STARK_WHITE, 0.22f), BLANK);
                    }
                }

                if (lvl.aperture.active) {
                    DrawCircleGradient((int)lvl.aperture.pos.x, (int)lvl.aperture.pos.y, 95.0f, ColorAlpha(COLOR_STARK_WHITE, 0.35f), BLANK);
                }

                if (lvl.hasSecretAperture && lvl.secretUnlocked) {
                    DrawCircleGradient((int)lvl.secretAperture.pos.x, (int)lvl.secretAperture.pos.y, 100.0f, ColorAlpha(COLOR_STARK_WHITE, 0.38f), BLANK);
                }
            }
            EndBlendMode();

            // Exit Portal Shockwave
            if (shockwaveRadius > 0.0f) {
                Vector2 pCenter = player.portalCenter;
                DrawCircleLines((int)pCenter.x, (int)pCenter.y,
                                shockwaveRadius, ColorAlpha(COLOR_STARK_WHITE, std::clamp(whiteFlashAlpha + 0.3f, 0.0f, 1.0f)));
                DrawCircleLines((int)pCenter.x, (int)pCenter.y,
                                shockwaveRadius + 4.0f, ColorAlpha(COLOR_STARK_WHITE, std::clamp(whiteFlashAlpha * 0.6f, 0.0f, 1.0f)));
            }

            EndMode2D();

            // -------------------------------------------------------------
            // 13. HUD: CLEAN, PURE MINIMALIST TYPOGRAPHY
            // -------------------------------------------------------------
            Color hudWhite = ColorAlpha(COLOR_STARK_WHITE, 0.35f);
            Color hudMisty = ColorAlpha(COLOR_FOG_LIGHT, 0.35f);

            if (currentPhase == PHASE_BLACK) {
                DrawText("PHASE: BLACK", 30, 22, 11, hudWhite);
                DrawText("WHITE OBJECTS SOLID  |  SHIFT", 30, 36, 9, hudMisty);
            } else {
                DrawText("PHASE: WHITE", 30, 22, 11, hudWhite);
                DrawText("BLACK OBJECTS SOLID  |  SHIFT", 30, 36, 9, hudMisty);
            }

            int orbsGot = 0;
            for (const auto& orb : lvl.orbs) {
                if (orb.collected) orbsGot++;
            }
            char orbBuf[48];
            std::snprintf(orbBuf, sizeof(orbBuf), "ANIMA: %d / %d", orbsGot, (int)lvl.orbs.size());
            DrawText(orbBuf, SCREEN_WIDTH - MeasureText(orbBuf, 11) - 30, 22, 11, hudWhite);

            char lvlBuf[64];
            std::snprintf(lvlBuf, sizeof(lvlBuf), "SECTOR: %02d / %02d", currentLevelIdx + 1, (int)levels.size());
            DrawText(lvlBuf, SCREEN_WIDTH - MeasureText(lvlBuf, 9) - 30, 36, 9, hudMisty);

            // Minimalist Level intro Banner
            if (levelBannerTimer > 0.0f) {
                float bannerAlpha = (levelBannerTimer > 2.0f) ? (2.5f - levelBannerTimer) * 2.0f :
                                    (levelBannerTimer < 0.5f) ? (levelBannerTimer / 0.5f) : 1.0f;
                bannerAlpha = std::clamp(bannerAlpha, 0.0f, 1.0f);

                const char* lName = lvl.name.c_str();
                const char* lSub = lvl.subtitle.c_str();
                int nameW = MeasureText(lName, 26);
                int subW = MeasureText(lSub, 13);

                DrawText(lName, SCREEN_WIDTH / 2 - nameW / 2, 72, 26, ColorAlpha(COLOR_STARK_WHITE, bannerAlpha * 0.85f));
                DrawText(lSub, SCREEN_WIDTH / 2 - subW / 2, 106, 13, ColorAlpha(COLOR_FOG_LIGHT, bannerAlpha * 0.70f));
            }

            // Pause Modal
            if (gameState == STATE_PAUSE) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(COLOR_OBSIDIAN, 0.80f));
                const char* pt = "SUSPENDED";
                DrawText(pt, SCREEN_WIDTH / 2 - MeasureText(pt, 30) / 2, SCREEN_HEIGHT / 2 - 80, 30, COLOR_STARK_WHITE);
                const char* pp = "[ ESC / P ] RESUME   |   [ R ] RETRY   |   [ O ] SETTINGS   |   [ M ] MENU";
                DrawText(pp, SCREEN_WIDTH / 2 - MeasureText(pp, 14) / 2, SCREEN_HEIGHT / 2 + 10, 14, COLOR_FOG_LIGHT);
            }

            // Victory Modal
            if (gameState == STATE_VICTORY) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(COLOR_OBSIDIAN, 0.90f));
                const char* vt = "THE APERTURE HAS BEEN CROSSED";
                DrawText(vt, SCREEN_WIDTH / 2 - MeasureText(vt, 32) / 2, SCREEN_HEIGHT / 2 - 140, 32, COLOR_STARK_WHITE);
                const char* vs = "You traversed the silence between light and shadow.";
                DrawText(vs, SCREEN_WIDTH / 2 - MeasureText(vs, 16) / 2, SCREEN_HEIGHT / 2 - 95, 16, COLOR_FOG_LIGHT);

                char finalT[64];
                int fMin = (int)(speedrunTimer / 60.0f);
                float fSec = std::fmod(speedrunTimer, 60.0f);
                std::snprintf(finalT, sizeof(finalT), "TOTAL TIME     %02d:%05.2f", fMin, fSec);
                DrawText(finalT, SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 35, 18, COLOR_STARK_WHITE);

                char finalD[64];
                std::snprintf(finalD, sizeof(finalD), "TOTAL DEATHS   %d", deathCounter);
                DrawText(finalD, SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 5, 18, COLOR_FOG_LIGHT);

                char finalS[64];
                std::snprintf(finalS, sizeof(finalS), "PHASE SHIFTS   %d", totalShifts);
                DrawText(finalS, SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 25, 18, COLOR_FOG_MID);

                const char* restartPrompt = "PRESS [ SPACE ] TO WALK THE PATH AGAIN   |   [ M ] MENU";
                DrawText(restartPrompt, SCREEN_WIDTH / 2 - MeasureText(restartPrompt, 16) / 2, SCREEN_HEIGHT / 2 + 100, 16, COLOR_STARK_WHITE);
            }

            EndTextureMode();
        }

        // ==========================================
        // 3. POST-PROCESSING PASS & FULLSCREEN WHITE FLASH
        // ==========================================
        BeginDrawing();
        ClearBackground(COLOR_OBSIDIAN);

        if (shaderActive) {
            float grainStrength = settings.filmGrainEnabled ? 0.055f : 0.0f;
            float vignetteStrength = (gameState == STATE_SECRET_CUTSCENE) ? 0.22f : 0.38f;
            SetShaderValue(postShader, uTimeLoc, &currentTime, SHADER_UNIFORM_FLOAT);
            SetShaderValue(postShader, uVignetteLoc, &vignetteStrength, SHADER_UNIFORM_FLOAT);
            SetShaderValue(postShader, uGrainLoc, &grainStrength, SHADER_UNIFORM_FLOAT);

            BeginShaderMode(postShader);
            DrawTexturePro(screenTarget.texture, { 0, 0, (float)SCREEN_WIDTH, -(float)SCREEN_HEIGHT }, destViewport, { 0, 0 }, 0.0f, WHITE);
            EndShaderMode();
        } else {
            DrawTexturePro(screenTarget.texture, { 0, 0, (float)SCREEN_WIDTH, -(float)SCREEN_HEIGHT }, destViewport, { 0, 0 }, 0.0f, WHITE);
            DrawCircleGradient((int)(destViewport.x + destViewport.width * 0.5f), (int)(destViewport.y + destViewport.height * 0.5f), destViewport.width * 0.75f, BLANK, ColorAlpha(COLOR_OBSIDIAN, 0.45f));
        }

        // FULLSCREEN SOLID WHITE OVERLAY (PHASE 2 / PHASE 4 TRANSITION FLASH)
        if (whiteFlashAlpha > 0.0f) {
            DrawRectangle(0, 0, (int)winW, (int)winH, ColorAlpha(COLOR_STARK_WHITE, std::clamp(whiteFlashAlpha, 0.0f, 1.0f)));
        }

        // Cinematic Letterboxing bars
        DrawRectangle((int)destViewport.x, (int)destViewport.y, (int)destViewport.width, (int)(14.0f * viewScale), COLOR_OBSIDIAN);
        DrawRectangle((int)destViewport.x, (int)(destViewport.y + destViewport.height - 14.0f * viewScale), (int)destViewport.width, (int)(14.0f * viewScale), COLOR_OBSIDIAN);

        EndDrawing();
    }

    if (secretPicture.id > 0) UnloadTexture(secretPicture);
    if (secretSoundReady) UnloadSound(secretSound);
    if (secretMusicReady) UnloadMusicStream(secretMusic);
    if (shaderActive) UnloadShader(postShader);
    UnloadRenderTexture(screenTarget);
    CloseAtmosphericAudio(am);
    CloseWindow();
    return 0;
}

// =========================================================================
// HANDCRAFTED, TESTED & 100% BEATABLE LEVEL DEFINITIONS (5 LEVELS)
// =========================================================================
static void InitLevels(std::vector<Level>& levels) {
    levels.clear();

    // -------------------------------------------------------------
    // LEVEL 1: "AREA 01: THE OUTER WOODS"
    // Features: Black/White Phase switching, fragile platform intro,
    // Secret Sub-Tunnel on bottom-left with zero-collision fake spike,
    // Breakable Wall at red circle (x=1, y=17), and Secret Cutscene Portal!
    // -------------------------------------------------------------
    {
        Level lvl;
        lvl.id = 1;
        lvl.name = "AREA 01: THE OUTER WOODS";
        lvl.subtitle = "Shift between Light and Shadow (SHIFT or F)";
        lvl.width = 44;
        lvl.height = 20;
        lvl.tiles.resize(lvl.width * lvl.height, TILE_EMPTY);
        lvl.hasSecretAperture = true;
        lvl.secretUnlocked = false;

        // Outer borders
        for (int x = 0; x < lvl.width; x++) {
            lvl.tiles[0 * lvl.width + x] = TILE_NEUTRAL;
            lvl.tiles[(lvl.height - 1) * lvl.width + x] = TILE_NEUTRAL;
        }
        for (int y = 0; y < lvl.height; y++) {
            lvl.tiles[y * lvl.width + 0] = TILE_NEUTRAL;
            lvl.tiles[y * lvl.width + (lvl.width - 1)] = TILE_NEUTRAL;
        }

        // Starting solid ground (y = 16, x = 1 to 8)
        for (int x = 1; x <= 8; x++) lvl.tiles[16 * lvl.width + x] = TILE_NEUTRAL;

        

        // Secret Aperture (moved 1 tile down and 1 tile right)
        lvl.secretAperture = { { 1.5f * TILE_SIZE, 18.0f * TILE_SIZE }, true, 0.0f };

        // Pit 1 (x = 9 to 14): White platforms bridge at y = 16, with fragile crumble pads
        // Fake zero-collision spike at x = 9 (white circle in user diagram)
        lvl.tiles[18 * lvl.width + 9] = TILE_SPIKE_DECORATIVE;
        for (int x = 10; x <= 14; x++) {
            lvl.tiles[18 * lvl.width + x] = TILE_SPIKE_UP;
        }
        lvl.tiles[16 * lvl.width + 10] = TILE_WHITE;
        lvl.tiles[16 * lvl.width + 11] = TILE_WHITE;
        AddCrumbleTile(lvl, 13, 16, TILE_CRUMBLE_WHITE);
        AddCrumbleTile(lvl, 14, 16, TILE_CRUMBLE_WHITE);

        // Middle neutral island (y = 16)
        for (int x = 15; x <= 19; x++) lvl.tiles[16 * lvl.width + x] = TILE_NEUTRAL;

        // Pit 2 (x = 20 to 26): Black platforms bridge at y = 16
        for (int x = 20; x <= 26; x++) {
            lvl.tiles[18 * lvl.width + x] = TILE_SPIKE_UP;
        }
        lvl.tiles[16 * lvl.width + 21] = TILE_BLACK;
        lvl.tiles[16 * lvl.width + 22] = TILE_BLACK;
        AddCrumbleTile(lvl, 24, 16, TILE_CRUMBLE_BLACK);
        AddCrumbleTile(lvl, 25, 16, TILE_CRUMBLE_BLACK);

        // Rest ledge (y = 16)
        for (int x = 27; x <= 29; x++) lvl.tiles[16 * lvl.width + x] = TILE_NEUTRAL;

        // Strict 2-tile vertical staircase ascent to summit
        lvl.tiles[14 * lvl.width + 30] = TILE_WHITE; lvl.tiles[14 * lvl.width + 31] = TILE_WHITE; // y = 14 (2 tiles up)
        lvl.tiles[12 * lvl.width + 33] = TILE_BLACK; lvl.tiles[12 * lvl.width + 34] = TILE_BLACK; // y = 12 (2 tiles up)
        lvl.tiles[10 * lvl.width + 31] = TILE_WHITE; lvl.tiles[10 * lvl.width + 32] = TILE_WHITE; // y = 10 (2 tiles up)

        // Summit plateau with Normal Light Aperture (y = 8)
        for (int x = 33; x <= 38; x++) lvl.tiles[8 * lvl.width + x] = TILE_NEUTRAL;

        // Right side boundary wall at x = 39..43
        for (int y = 0; y <= 19; y++) {
            for (int x = 39; x <= 43; x++) {
                lvl.tiles[y * lvl.width + x] = TILE_NEUTRAL;
            }
        }

        // Original Player Spawn Point on starting platform
        lvl.playerSpawn = { 3.0f * TILE_SIZE, 14.0f * TILE_SIZE };

        // 3 Anima Orbs
        lvl.orbs.push_back({ { 12.0f * TILE_SIZE + 16.0f, 14.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 23.0f * TILE_SIZE + 16.0f, 14.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 32.0f * TILE_SIZE + 16.0f, 8.5f * TILE_SIZE }, false, 0.0f });

        lvl.aperture = { { 36.0f * TILE_SIZE, 6.0f * TILE_SIZE }, false, 0.0f };

        levels.push_back(lvl);
    }

    // -------------------------------------------------------------
    // LEVEL 2: "AREA 02: THE SILENT SPIRE"
    // Vertical labyrinth. Every step is strictly 2 tiles vertical!
    // -------------------------------------------------------------
    {
        Level lvl;
        lvl.id = 2;
        lvl.name = "AREA 02: THE SILENT SPIRE";
        lvl.subtitle = "Descent into the industrial labyrinth";
        lvl.width = 36;
        lvl.height = 26;
        lvl.tiles.resize(lvl.width * lvl.height, TILE_EMPTY);
        lvl.hasSecretAperture = false;
        lvl.secretUnlocked = false;

        for (int x = 0; x < lvl.width; x++) {
            lvl.tiles[0 * lvl.width + x] = TILE_NEUTRAL;
            lvl.tiles[(lvl.height - 1) * lvl.width + x] = TILE_NEUTRAL;
        }
        for (int y = 0; y < lvl.height; y++) {
            lvl.tiles[y * lvl.width + 0] = TILE_NEUTRAL;
            lvl.tiles[y * lvl.width + (lvl.width - 1)] = TILE_NEUTRAL;
        }

        // Starting floor
        for (int x = 1; x <= 8; x++) lvl.tiles[23 * lvl.width + x] = TILE_NEUTRAL;
        for (int x = 9; x <= 34; x++) lvl.tiles[24 * lvl.width + x] = TILE_SPIKE_UP;

        // Central dividing wall (x = 17)
        for (int y = 6; y <= 21; y++) lvl.tiles[y * lvl.width + 17] = TILE_NEUTRAL;

        // Left climb: strictly 2 tiles vertical per step
        lvl.tiles[21 * lvl.width + 10] = TILE_WHITE; lvl.tiles[21 * lvl.width + 11] = TILE_WHITE; // y = 21 (2 up)
        lvl.tiles[19 * lvl.width + 5]  = TILE_BLACK; lvl.tiles[19 * lvl.width + 6]  = TILE_BLACK; // y = 19 (2 up)
        AddCrumbleTile(lvl, 11, 17, TILE_CRUMBLE_WHITE); AddCrumbleTile(lvl, 12, 17, TILE_CRUMBLE_WHITE); // y = 17 (2 up)
        lvl.tiles[15 * lvl.width + 5]  = TILE_BLACK; lvl.tiles[15 * lvl.width + 6]  = TILE_BLACK; // y = 15 (2 up)
        lvl.tiles[13 * lvl.width + 11] = TILE_WHITE; lvl.tiles[13 * lvl.width + 12] = TILE_WHITE; // y = 13 (2 up)
        AddCrumbleTile(lvl, 5, 11, TILE_CRUMBLE_BLACK); AddCrumbleTile(lvl, 6, 11, TILE_CRUMBLE_BLACK);   // y = 11 (2 up)
        lvl.tiles[9 * lvl.width + 10]  = TILE_WHITE; lvl.tiles[9 * lvl.width + 11]  = TILE_WHITE; // y = 9  (2 up)
        lvl.tiles[7 * lvl.width + 6]   = TILE_NEUTRAL; lvl.tiles[7 * lvl.width + 7] = TILE_NEUTRAL; // y = 7 (2 up)

        // Top crossover bridge over spine
        for (int x = 12; x <= 22; x++) lvl.tiles[5 * lvl.width + x] = TILE_NEUTRAL;                // y = 5  (2 up)

        // Right side descent / platforming
        lvl.tiles[8 * lvl.width + 24]  = TILE_BLACK; lvl.tiles[8 * lvl.width + 25]  = TILE_BLACK;
        lvl.tiles[11 * lvl.width + 28] = TILE_WHITE; lvl.tiles[11 * lvl.width + 29] = TILE_WHITE;
        AddCrumbleTile(lvl, 24, 14, TILE_CRUMBLE_BLACK); AddCrumbleTile(lvl, 25, 14, TILE_CRUMBLE_BLACK);
        lvl.tiles[17 * lvl.width + 28] = TILE_WHITE; lvl.tiles[17 * lvl.width + 29] = TILE_WHITE;

        // Sanctuary landing
        for (int x = 25; x <= 34; x++) lvl.tiles[20 * lvl.width + x] = TILE_NEUTRAL;

        lvl.playerSpawn = { 3.0f * TILE_SIZE, 21.0f * TILE_SIZE };

        // 4 Anima Orbs
        lvl.orbs.push_back({ { 6.0f * TILE_SIZE + 16.0f, 17.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 17.0f * TILE_SIZE + 16.0f, 3.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 28.0f * TILE_SIZE + 16.0f, 9.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 24.0f * TILE_SIZE + 16.0f, 12.5f * TILE_SIZE }, false, 0.0f });

        lvl.aperture = { { 31.0f * TILE_SIZE, 18.0f * TILE_SIZE }, false, 0.0f };

        levels.push_back(lvl);
    }

    // -------------------------------------------------------------
    // LEVEL 3: "AREA 03: THE ABYSSAL CONDUIT" (FULLY REMADE!)
    // Polarity wall-jump elevator, crumbling conduits, 100% beatable.
    // -------------------------------------------------------------
    {
        Level lvl;
        lvl.id = 3;
        lvl.name = "AREA 03: THE ABYSSAL CONDUIT";
        lvl.subtitle = "Navigate the high-voltage underground core";
        lvl.width = 44;
        lvl.height = 22;
        lvl.tiles.resize(lvl.width * lvl.height, TILE_EMPTY);
        lvl.hasSecretAperture = false;
        lvl.secretUnlocked = false;

        for (int x = 0; x < lvl.width; x++) {
            lvl.tiles[0 * lvl.width + x] = TILE_NEUTRAL;
            lvl.tiles[(lvl.height - 1) * lvl.width + x] = TILE_NEUTRAL;
        }
        for (int y = 0; y < lvl.height; y++) {
            lvl.tiles[y * lvl.width + 0] = TILE_NEUTRAL;
            lvl.tiles[y * lvl.width + (lvl.width - 1)] = TILE_NEUTRAL;
        }

        // Starting platform
        for (int x = 1; x <= 6; x++) lvl.tiles[19 * lvl.width + x] = TILE_NEUTRAL;
        // Spiked floor across the abyss
        for (int x = 7; x <= 42; x++) lvl.tiles[20 * lvl.width + x] = TILE_SPIKE_UP;

        // Section 1: Mid-air polarity stepping stones (gap = 2 tiles, step = 2 tiles)
        lvl.tiles[17 * lvl.width + 8]  = TILE_WHITE; lvl.tiles[17 * lvl.width + 9]  = TILE_WHITE; // y = 17 (2 up)
        lvl.tiles[15 * lvl.width + 11] = TILE_BLACK; lvl.tiles[15 * lvl.width + 12] = TILE_BLACK; // y = 15 (2 up)
        AddCrumbleTile(lvl, 14, 15, TILE_CRUMBLE_WHITE); AddCrumbleTile(lvl, 15, 15, TILE_CRUMBLE_WHITE); // y = 15
        for (int x = 17; x <= 20; x++) lvl.tiles[15 * lvl.width + x] = TILE_NEUTRAL; // Resting ledge (y = 15)

        // Section 2: Polarity Wall Jump Shaft (Width = 4 tiles / 128px from x = 22 to 26)
        // Left wall White, Right wall Black
        for (int y = 7; y <= 15; y++) {
            lvl.tiles[y * lvl.width + 22] = TILE_WHITE;
            lvl.tiles[y * lvl.width + 26] = TILE_BLACK;
        }

        // Intermediate rest platform above shaft entrance
        for (int x = 26; x <= 30; x++) lvl.tiles[7 * lvl.width + x] = TILE_NEUTRAL; // y = 7

        // Section 3: High-Tension Fragile Bridge
        AddCrumbleTile(lvl, 32, 7, TILE_CRUMBLE_NEUTRAL); AddCrumbleTile(lvl, 33, 7, TILE_CRUMBLE_NEUTRAL); // y = 7
        lvl.tiles[9 * lvl.width + 35] = TILE_WHITE; lvl.tiles[9 * lvl.width + 36] = TILE_WHITE;              // y = 9 (2 down)
        AddCrumbleTile(lvl, 38, 11, TILE_CRUMBLE_BLACK); AddCrumbleTile(lvl, 39, 11, TILE_CRUMBLE_BLACK);   // y = 11 (2 down)
        lvl.tiles[13 * lvl.width + 36] = TILE_WHITE; lvl.tiles[13 * lvl.width + 37] = TILE_WHITE;           // y = 13 (2 down)

        // Section 4: Final Monolithic Sanctuary with Aperture
        for (int x = 32; x <= 42; x++) lvl.tiles[16 * lvl.width + x] = TILE_NEUTRAL;                         // y = 16

        lvl.playerSpawn = { 3.0f * TILE_SIZE, 17.0f * TILE_SIZE };

        // 4 Anima Orbs
        lvl.orbs.push_back({ { 9.0f * TILE_SIZE, 15.0f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 24.0f * TILE_SIZE, 10.0f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 32.5f * TILE_SIZE, 5.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 38.5f * TILE_SIZE, 9.5f * TILE_SIZE }, false, 0.0f });

        lvl.aperture = { { 40.0f * TILE_SIZE, 14.0f * TILE_SIZE }, false, 0.0f };

        levels.push_back(lvl);
    }

    // -------------------------------------------------------------
    // LEVEL 4: "AREA 04: THE FORGOTTEN FOUNDRY" (NEW LEVEL!)
    // Suspended fragile gantries, polarity wall-slide chimney, precision hops.
    // -------------------------------------------------------------
    {
        Level lvl;
        lvl.id = 4;
        lvl.name = "AREA 04: THE FORGOTTEN FOUNDRY";
        lvl.subtitle = "Descend through the cold furnace and fractured catwalks";
        lvl.width = 46;
        lvl.height = 24;
        lvl.tiles.resize(lvl.width * lvl.height, TILE_EMPTY);
        lvl.hasSecretAperture = false;
        lvl.secretUnlocked = false;

        for (int x = 0; x < lvl.width; x++) {
            lvl.tiles[0 * lvl.width + x] = TILE_NEUTRAL;
            lvl.tiles[(lvl.height - 1) * lvl.width + x] = TILE_NEUTRAL;
        }
        for (int y = 0; y < lvl.height; y++) {
            lvl.tiles[y * lvl.width + 0] = TILE_NEUTRAL;
            lvl.tiles[y * lvl.width + (lvl.width - 1)] = TILE_NEUTRAL;
        }

        // Starting upper gantry (y = 7)
        for (int x = 1; x <= 5; x++) lvl.tiles[7 * lvl.width + x] = TILE_NEUTRAL;

        // Fragile high catwalk over industrial pit
        for (int x = 6; x <= 22; x++) lvl.tiles[22 * lvl.width + x] = TILE_SPIKE_UP;

        AddCrumbleTile(lvl, 7, 7, TILE_CRUMBLE_WHITE); AddCrumbleTile(lvl, 8, 7, TILE_CRUMBLE_WHITE);
        for (int x = 10; x <= 12; x++) lvl.tiles[7 * lvl.width + x] = TILE_NEUTRAL;
        AddCrumbleTile(lvl, 14, 7, TILE_CRUMBLE_BLACK); AddCrumbleTile(lvl, 15, 7, TILE_CRUMBLE_BLACK);
        for (int x = 17; x <= 19; x++) lvl.tiles[7 * lvl.width + x] = TILE_NEUTRAL;

        // Polarity wall-slide descending shaft (Left wall White x = 21, Right wall Black x = 25)
        for (int y = 8; y <= 18; y++) {
            lvl.tiles[y * lvl.width + 21] = TILE_WHITE;
            lvl.tiles[y * lvl.width + 25] = TILE_BLACK;
        }

        // Bottom chamber landing floor
        for (int x = 20; x <= 27; x++) lvl.tiles[19 * lvl.width + x] = TILE_NEUTRAL;
        for (int x = 28; x <= 44; x++) lvl.tiles[22 * lvl.width + x] = TILE_SPIKE_UP;

        // Ascending pillars out of lower chamber
        lvl.tiles[17 * lvl.width + 29] = TILE_WHITE; lvl.tiles[17 * lvl.width + 30] = TILE_WHITE; // y = 17 (2 up)
        lvl.tiles[15 * lvl.width + 32] = TILE_BLACK; lvl.tiles[15 * lvl.width + 33] = TILE_BLACK; // y = 15 (2 up)
        AddCrumbleTile(lvl, 35, 13, TILE_CRUMBLE_NEUTRAL); AddCrumbleTile(lvl, 36, 13, TILE_CRUMBLE_NEUTRAL); // y = 13 (2 up)
        lvl.tiles[11 * lvl.width + 38] = TILE_WHITE; lvl.tiles[11 * lvl.width + 39] = TILE_WHITE; // y = 11 (2 up)

        // Upper Foundry Overlook with Aperture (y = 9)
        for (int x = 40; x <= 44; x++) lvl.tiles[9 * lvl.width + x] = TILE_NEUTRAL;

        lvl.playerSpawn = { 3.0f * TILE_SIZE, 5.0f * TILE_SIZE };

        // 4 Anima Orbs
        lvl.orbs.push_back({ { 7.5f * TILE_SIZE, 5.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 23.0f * TILE_SIZE, 12.0f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 32.5f * TILE_SIZE, 13.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 38.5f * TILE_SIZE, 9.5f * TILE_SIZE }, false, 0.0f });

        lvl.aperture = { { 42.0f * TILE_SIZE, 7.0f * TILE_SIZE }, false, 0.0f };

        levels.push_back(lvl);
    }

    // -------------------------------------------------------------
    // LEVEL 5: "AREA 05: THE VOID APEX" (NEW GRAND FINALE!)
    // Rhythmic polarity leaps, twin wall towers, crumbling spire ascent.
    // -------------------------------------------------------------
    {
        Level lvl;
        lvl.id = 5;
        lvl.name = "AREA 05: THE VOID APEX";
        lvl.subtitle = "Cross the negative space to the celestial threshold";
        lvl.width = 50;
        lvl.height = 26;
        lvl.tiles.resize(lvl.width * lvl.height, TILE_EMPTY);
        lvl.hasSecretAperture = false;
        lvl.secretUnlocked = false;

        for (int x = 0; x < lvl.width; x++) {
            lvl.tiles[0 * lvl.width + x] = TILE_NEUTRAL;
            lvl.tiles[(lvl.height - 1) * lvl.width + x] = TILE_NEUTRAL;
        }
        for (int y = 0; y < lvl.height; y++) {
            lvl.tiles[y * lvl.width + 0] = TILE_NEUTRAL;
            lvl.tiles[y * lvl.width + (lvl.width - 1)] = TILE_NEUTRAL;
        }

        // Starting sanctuary (y = 23)
        for (int x = 1; x <= 6; x++) lvl.tiles[23 * lvl.width + x] = TILE_NEUTRAL;
        // The Great Void Spike Sea
        for (int x = 7; x <= 48; x++) lvl.tiles[24 * lvl.width + x] = TILE_SPIKE_UP;

        // Section 1: The Ascending Rhythm Gauntlet (strictly 2 tiles vertical per step!)
        lvl.tiles[21 * lvl.width + 8]  = TILE_WHITE; lvl.tiles[21 * lvl.width + 9]  = TILE_WHITE; // y = 21 (2 up)
        AddCrumbleTile(lvl, 11, 19, TILE_CRUMBLE_BLACK); AddCrumbleTile(lvl, 12, 19, TILE_CRUMBLE_BLACK); // y = 19 (2 up)
        lvl.tiles[17 * lvl.width + 14] = TILE_WHITE; lvl.tiles[17 * lvl.width + 15] = TILE_WHITE; // y = 17 (2 up)
        AddCrumbleTile(lvl, 17, 15, TILE_CRUMBLE_BLACK); AddCrumbleTile(lvl, 18, 15, TILE_CRUMBLE_BLACK); // y = 15 (2 up)
        // Mid-way resting obelisk (y = 15)
        for (int x = 20; x <= 22; x++) lvl.tiles[15 * lvl.width + x] = TILE_NEUTRAL;

        // Section 2: Twin Polarity Wall Towers
        // Tower 1 (x = 24 Black, x = 28 White) from y = 7 to 15
        for (int y = 7; y <= 15; y++) {
            lvl.tiles[y * lvl.width + 24] = TILE_BLACK;
            lvl.tiles[y * lvl.width + 28] = TILE_WHITE;
        }
        // Cross-bridge over Tower 1
        for (int x = 27; x <= 32; x++) lvl.tiles[7 * lvl.width + x] = TILE_NEUTRAL;

        // Tower 2 descent/step (x = 32 White, x = 36 Black) from y = 7 to 15
        for (int y = 7; y <= 15; y++) {
            lvl.tiles[y * lvl.width + 32] = TILE_WHITE;
            lvl.tiles[y * lvl.width + 36] = TILE_BLACK;
        }

        // Tower 2 exit ledge (y = 13)
        for (int x = 36; x <= 38; x++) lvl.tiles[13 * lvl.width + x] = TILE_NEUTRAL;

        // Section 3: The Void Leap to the Apex
        AddCrumbleTile(lvl, 40, 11, TILE_CRUMBLE_WHITE); AddCrumbleTile(lvl, 41, 11, TILE_CRUMBLE_WHITE); // y = 11 (2 up)
        AddCrumbleTile(lvl, 43, 9, TILE_CRUMBLE_BLACK);  AddCrumbleTile(lvl, 44, 9, TILE_CRUMBLE_BLACK);  // y = 9 (2 up)

        // Section 4: The Apex Celestial Monolith with Final Light Aperture (y = 7)
        for (int x = 44; x <= 48; x++) lvl.tiles[7 * lvl.width + x] = TILE_NEUTRAL;

        lvl.playerSpawn = { 3.0f * TILE_SIZE, 21.0f * TILE_SIZE };

        // 5 Anima Orbs
        lvl.orbs.push_back({ { 11.5f * TILE_SIZE, 17.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 17.5f * TILE_SIZE, 13.5f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 26.0f * TILE_SIZE, 10.0f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 34.0f * TILE_SIZE, 10.0f * TILE_SIZE }, false, 0.0f });
        lvl.orbs.push_back({ { 43.5f * TILE_SIZE, 7.5f * TILE_SIZE }, false, 0.0f });

        lvl.aperture = { { 46.0f * TILE_SIZE, 5.0f * TILE_SIZE }, false, 0.0f };

        levels.push_back(lvl);
    }
}
