/*******************************************************************************************
* KESHIPIN BATTLE: SIMPLE EDITION (Ver 119.0)
* -----------------------------------------------------------------------------------------
* [UPDATES Ver 119.0]
* - REMOVED: STATIONERY, DELAY FLOOR skills.
* - NEW SKILL: REPULSION (Cost 5) -> Push enemy 300px away if range > 700px (Every 3 turns).
* - NEW SKILL: INTIMIDATION (Cost 10) -> Opponent's Power & Speed x0.65 (Passive).
* - UI: Added "BACK TO EDIT" button in Battle Scene (Top-Right).
*
* [COMPILE COMMAND]
* gcc keshipin_simple.c -o keshipin.exe -O2 -I raylib/include -L raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm
********************************************************************************************/

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

// =========================================================================================
//  1. CONFIGURATION & CONSTANTS
// =========================================================================================
#define GAME_W 1280
#define GAME_H 720
#define TARGET_FPS 60

// Color Palette
#define COL_BG_DARK     (Color){ 15, 15, 25, 255 }
#define COL_GRID        (Color){ 40, 50, 80, 60 }
#define COL_UI_PANEL    (Color){ 10, 12, 18, 245 }
#define COL_P1_MAIN     (Color){ 0, 140, 255, 255 }
#define COL_P2_MAIN     (Color){ 255, 60, 60, 255 }
#define COL_ABS_DEF     (Color){ 220, 0, 255, 255 }
#define COL_SPARK       (Color){ 255, 255, 220, 255 }
#define COL_DIMENSION   (Color){ 0, 255, 200, 255 }
#define COL_HEAVY       (Color){ 255, 100, 0, 255 }
#define COL_HOMING      (Color){ 0, 255, 100, 255 }
#define COL_ZERO        (Color){ 255, 0, 100, 255 }
#define COL_UTURN       (Color){ 100, 100, 255, 255 }
#define COL_CLIFF       (Color){ 139, 69, 19, 255 }
#define COL_SNIPER      (Color){ 0, 200, 255, 255 }
#define COL_REPULSE     (Color){ 200, 200, 0, 255 }
#define COL_INTIM       (Color){ 100, 0, 0, 255 }

#define DESK_RECT (Rectangle){ 100, 80, 1080, 560 }
#define OBS_COUNT 6

#define MAX_BONUS_POINTS 45
#define MAX_STAT_VAL 20
#define MAX_FLIGHT 20

// CHARGE
#define MAX_CHARGE_LVL 10
#define CHARGE_COST 1

// ABSOLUTE DEFENSE
#define ABS_DEFENSE_MAX_LVL 3
#define ABS_DEFENSE_COST 15
#define ABS_STOCKS_PER_LVL 10
#define ABS_INVULN_TIME 60

#define PURSUIT_CAP 10.0f
#define REFLECT_PER_LVL 0.50f
#define MAX_POWER_MULT 8.0f 

// HOMING SETTINGS
#define HOMING_MIN_DIST 0.0f
#define HOMING_MAX_DIST 400.0f 
#define HOMING_TURN_RATE 0.15f

#define ZERO_DIST_THRESHOLD 50.0f

// CLIFF WARRIOR
#define CLIFF_MARGIN 120.0f
#define CLIFF_DURATION 3
#define CLIFF_COOLDOWN 3

// U-TURN
#define UTURN_RECOIL_SPEED 1500.0f 

// REPULSION
#define REPULSION_DIST_REQ 700.0f
#define REPULSION_PUSH 300.0f
#define REPULSION_INTERVAL 3

// INTIMIDATION
#define INTIMIDATION_FACTOR 0.65f

// Visual Limits
#define MAX_PARTICLES 15000
#define MAX_TRAILS 600
#define MAX_SHOCKWAVES 80
#define MAX_SLASHES 30
#define MAX_TEXT_FX 60
#define MAX_SKILL_WAVES 10

#define WAVE_BASE_RADIUS 150.0f
#define WAVE_SIZE_FACTOR 12.0f
#define WAVE_SPEED 22.0f
#define WAVE_BASE_FORCE 220.0f
#define WAVE_COOLDOWN 3

#define WARP_COOLDOWN 3
#define CHARGE_BREAK_PER_TURN 1000.0f
#define CHARGE_BREAK_MAX 30000.0f

#define MAX_PHYSICS_STEPS 16

// =========================================================================================
//  2. DATA STRUCTURES
// =========================================================================================

typedef enum { SCENE_TITLE, SCENE_CUSTOMIZE, SCENE_BATTLE, SCENE_RESULT } Scene;
typedef enum { OBS_BOX, OBS_CIRCLE } ObstacleType;

typedef struct {
    ObstacleType type;
    Rectangle rect;
    Vector2 pos;
    float radius;
    float rotation;
    Color color;
    float bounce;
    const char* label;
} Obstacle;

typedef struct {
    Vector2 pos;
    float currentRadius;
    float maxRadius;
    int ownerId;
    bool active;
    Color color;
} SkillWave;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float rotation;
    float life;
    int ownerId;
    bool active;
    Color color;
    float damage;
} SlashObj;

typedef struct {
    // Basic Stats
    int power, defense, flight;
    int slip, sizeRank, maxCharge;
    int recoil, critRate, pursuit, reflection;
    int critPowerLvl, longBattleLvl;

    // Toggle / Special Stats
    bool wallThrough, bounceEnabled, defenseCharge, antiDestruct;
    bool shockwaveSkill, reActSkill, warpSkill;
    
    // New Skills
    bool heavyStance; // Cost 5
    bool homingSkill; // Cost 10
    bool zeroDistSkill; // Cost 10
    bool uTurnSkill; // Cost 5
    bool cliffWarrior; // Cost 10
    bool sniperSkill; // Cost 10
    bool repulsionSkill; // Cost 5 (NEW)
    bool intimidationSkill; // Cost 10 (NEW)

    // Special Skills
    bool chargeBreakSkill;
    bool fullThrottle;

    // Absolute Defense
    int absoluteDefenseLvl;
    int absoluteDefenseCount;

    // Physics State
    Vector2 pos, vel;
    float mass, radius, rotation;

    // Battle State
    int pursuitStack, defChargeStack;
    bool isDead;
    
    // Cooldowns / Timers
    int reActCooldown;
    int shockwaveCooldown;
    int warpCooldown;
    
    int uTurnDebuffTimer; // Flight = 0
    int cliffActiveTimer;
    int cliffCoolTimer;
    int repulsionTurnCounter;

    float chargeBreakStored;
    int invulnTimer;

    int turnCount;

    // Turn Flags
    bool turnHitOpponent, turnWasHit;
    float turnMaxImpactTaken;
    
    bool isHomingActive; 

    float defenseMultiplier;

    // Visuals
    Color bodyColor, sleeveColor;
    char name[32];
    Vector2 trail[MAX_TRAILS];
    int trailHead;
} Eraser;

typedef struct {
    Vector2 pos, vel; Color color;
    float size, decay, life;
    bool active, additive, gravity;
    float rotation;
    float rotSpeed;
} Particle;

typedef struct {
    Vector2 pos; float radius, maxRadius, thick, life;
    bool active; Color color;
    bool isHexagon;
} Shockwave;

typedef struct {
    Vector2 pos; Vector2 vel; char text[32]; int size; Color color; float life; bool active;
} TextFX;

// =========================================================================================
//  3. FUNCTION PROTOTYPES
// =========================================================================================

void InitGameSystem(void);
void InitMatch(void); 
void ResetRoundState(void);
void ResetBattleState(void);
void InitEraserStats(Eraser* e, int id);
void RecalculateEraserPhysics(Eraser* e);
void InitObstacles(void);

void UpdatePhysics(void);
void UpdateInput(void);
float CalculateDrag(Eraser* e);
void ResolveEraserCollision(void);
bool CheckWallCollisionCCD(Vector2 start, Vector2 end, float radius, Vector2* hitPos, Vector2* hitNormal, float* hitBounce, int* obsIndex);
bool IsNearCliff(Eraser* e);

void SpawnSkillWave(Eraser* e, int ownerId);
void UpdateSkillWaves(void);
void SpawnSlash(Eraser* e, Vector2 targetPos, float power);
void UpdateSlashes(void);

void InitVisuals(void);
void UpdateVisuals(void);
void UpdateTrail(Eraser* e);
void SpawnParticle(Vector2 pos, Vector2 vel, Color col, float size, float decay, bool additive, bool gravity);
void SpawnSpark(Vector2 pos, Vector2 normal, int count, Color c);
void SpawnExplosion(Vector2 pos, Color c, int count, float speed);
void SpawnShockwave(Vector2 pos, float maxRadius, float thick, Color c, bool isHexagon);
void SpawnTextFX(Vector2 pos, const char* text, Color c, int size);
void SpawnWarpVisuals(Vector2 pos, Color c, bool isAppear);
void SpawnSlashImpact(Vector2 pos, Color c, float angle, float power);
void SpawnShieldBreakEffect(Vector2 pos, float radius);
void SpawnBeamEffect(Vector2 start, Vector2 end, Color c);

void DrawGameScene(void);
void DrawSceneCustomize(void);
void DrawFlatEraser(Eraser* e);
void DrawFlatObstacles(void);
void DrawSkillWaves(void);
void DrawSlashes(void);
void DrawWarpTarget(Eraser* e);
void DrawStatUI(int x, int y, const char* label, int* val, int min, int max, int* points, int cost);
void DrawToggleUI(int x, int y, const char* label, bool* val, int* points, int cost);
void DrawNegativeToggleUI(int x, int y, const char* label, bool* val, int* points, int cost);

Vector2 GetVirtualMousePos(void);
Vector2 Vector2MidPoint(Vector2 v1, Vector2 v2);
float FloatRand(float min, float max);
Color AdjustBrightness(Color c, float factor);
void SetMessage(const char* text, int time);
void ApplyShake(float intensity);
void TriggerHitStop(int frames, float zoom);

// =========================================================================================
//  4. GLOBALS
// =========================================================================================

Scene currentScene = SCENE_TITLE;
Camera2D gameCamera = { 0 };
Eraser p1, p2;
int turnPlayer = 1, winnerID = 0;
bool isTurnProcessing = false;

// Round System
int currentRound = 1;
int p1Wins = 0;
int p2Wins = 0;
int roundStarter = 1; // 1 for P1, 2 for P2

bool isDragging = false;
Vector2 dragStart = {0}, dragEnd = {0};
int rouletteValue = 1;
bool rouletteUp = true;
int frameCounter = 0;

bool isWarpSelectMode = false;
int warpActivePlayerId = 0;
float customizeScrollY = 0.0f;
bool isScrollBarDragging = false;

int p1Points = MAX_BONUS_POINTS, p2Points = MAX_BONUS_POINTS;

Obstacle obstacles[OBS_COUNT];
SkillWave skillWaves[MAX_SKILL_WAVES];
SlashObj slashes[MAX_SLASHES];
Particle particles[MAX_PARTICLES];
Shockwave shockwaves[MAX_SHOCKWAVES];
TextFX textEffects[MAX_TEXT_FX];

float shakeIntensity = 0.0f;
float zoomTarget = 1.0f;
int hitStopFrames = 0;
char messageBuf[128] = {0};
int messageTimer = 0;

float scale = 1.0f;
Vector2 offset = { 0.0f, 0.0f };
float gridScroll = 0.0f;

// =========================================================================================
//  5. IMPLEMENTATIONS
// =========================================================================================

// --- Utils ---
Vector2 GetVirtualMousePos(void) {
    Vector2 mouse = GetMousePosition();
    Vector2 virtualMouse = { 0 };
    virtualMouse.x = (mouse.x - offset.x) / scale;
    virtualMouse.y = (mouse.y - offset.y) / scale;
    return virtualMouse;
}
Vector2 Vector2MidPoint(Vector2 v1, Vector2 v2) {
    return (Vector2){ (v1.x + v2.x) / 2.0f, (v1.y + v2.y) / 2.0f };
}
float FloatRand(float min, float max) {
    return min + ((float)GetRandomValue(0, 10000) / 10000.0f) * (max - min);
}
Color AdjustBrightness(Color c, float factor) {
    int r = (int)(c.r * factor); int g = (int)(c.g * factor); int b = (int)(c.b * factor);
    if(r>255)r=255; if(g>255)g=255; if(b>255)b=255;
    return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, c.a};
}
void SetMessage(const char* text, int time) {
    snprintf(messageBuf, sizeof(messageBuf), "%s", text);
    messageTimer = time;
}
void ApplyShake(float intensity) {
    if (intensity > shakeIntensity) shakeIntensity = intensity;
    if (shakeIntensity > 150.0f) shakeIntensity = 150.0f;
}
void TriggerHitStop(int frames, float zoom) {
    hitStopFrames = frames;
    zoomTarget = zoom;
}
bool IsNearCliff(Eraser* e) {
    if (e->pos.x < DESK_RECT.x + CLIFF_MARGIN) return true;
    if (e->pos.x > DESK_RECT.x + DESK_RECT.width - CLIFF_MARGIN) return true;
    if (e->pos.y < DESK_RECT.y + CLIFF_MARGIN) return true;
    if (e->pos.y > DESK_RECT.y + DESK_RECT.height - CLIFF_MARGIN) return true;
    return false;
}

// --- Visuals ---
void InitVisuals(void) {
    for(int i=0; i<MAX_PARTICLES; i++) particles[i].active = false;
    for(int i=0; i<MAX_SHOCKWAVES; i++) shockwaves[i].active = false;
    for(int i=0; i<MAX_TEXT_FX; i++) textEffects[i].active = false;
    for(int i=0; i<MAX_SLASHES; i++) slashes[i].active = false;
}

void SpawnParticle(Vector2 pos, Vector2 vel, Color col, float size, float decay, bool additive, bool gravity) {
    for(int i=0; i<MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].active = true;
            particles[i].pos = pos;
            particles[i].vel = vel;
            particles[i].color = col;
            particles[i].size = size;
            particles[i].decay = decay;
            particles[i].life = 1.0f;
            particles[i].additive = additive;
            particles[i].gravity = gravity;
            particles[i].rotation = FloatRand(0, 360);
            particles[i].rotSpeed = FloatRand(-10, 10);
            return;
        }
    }
}

void SpawnSpark(Vector2 pos, Vector2 normal, int count, Color c) {
    for(int i=0; i<count; i++) {
        float angleBase = atan2f(normal.y, normal.x);
        float angleSpread = FloatRand(-1.2f, 1.2f);
        float spd = FloatRand(200.0f, 600.0f);
        Vector2 v = { cosf(angleBase + angleSpread) * spd, sinf(angleBase + angleSpread) * spd };
        SpawnParticle(pos, v, c, FloatRand(2, 4), 0.08f, true, true);
    }
}

void SpawnExplosion(Vector2 pos, Color c, int count, float speed) {
    for(int i=0; i<count; i++) {
        float angle = FloatRand(0, 360) * DEG2RAD;
        float spd = FloatRand(speed*0.1f, speed);
        Vector2 v = { cosf(angle)*spd, sinf(angle)*spd };
        bool add = (i % 2 == 0);
        SpawnParticle(pos, v, c, FloatRand(3, 8), FloatRand(0.02f, 0.05f), add, false);
    }
}

void SpawnShieldBreakEffect(Vector2 pos, float radius) {
    SpawnShockwave(pos, radius * 2.0f, 5.0f, COL_ABS_DEF, true);
    SpawnExplosion(pos, COL_ABS_DEF, 15, 200.0f);

    for(int i=0; i<6; i++) {
        float angle = (60.0f * i) * DEG2RAD;
        Vector2 v = { cosf(angle)*100.0f, sinf(angle)*100.0f };
        SpawnParticle(pos, v, WHITE, 6.0f, 0.05f, true, false);
    }
}

void SpawnBeamEffect(Vector2 start, Vector2 end, Color c) {
    SpawnShockwave(start, 200.0f, 10.0f, c, false);
    int pCount = (int)(Vector2Distance(start, end) / 10.0f);
    for(int i=0; i<pCount; i++) {
        Vector2 pos = Vector2Lerp(start, end, (float)i/pCount);
        SpawnParticle(pos, (Vector2){FloatRand(-20,20), FloatRand(-20,20)}, c, 8.0f, 0.1f, true, false);
    }
}

void SpawnShockwave(Vector2 pos, float maxRadius, float thick, Color c, bool isHexagon) {
    for(int i=0; i<MAX_SHOCKWAVES; i++) {
        if (!shockwaves[i].active) {
            shockwaves[i].active = true;
            shockwaves[i].pos = pos;
            shockwaves[i].radius = 10.0f;
            shockwaves[i].maxRadius = maxRadius;
            shockwaves[i].thick = thick;
            shockwaves[i].life = 1.0f;
            shockwaves[i].color = c;
            shockwaves[i].isHexagon = isHexagon;
            return;
        }
    }
}

void SpawnTextFX(Vector2 pos, const char* text, Color c, int size) {
    for(int i=0; i<MAX_TEXT_FX; i++) {
        if (!textEffects[i].active) {
            textEffects[i].active = true;
            textEffects[i].pos = pos;
            textEffects[i].vel = (Vector2){0, -2.0f};
            snprintf(textEffects[i].text, 32, "%s", text);
            textEffects[i].size = size;
            textEffects[i].color = c;
            textEffects[i].life = 1.0f;
            return;
        }
    }
}

void SpawnWarpVisuals(Vector2 pos, Color c, bool isAppear) {
    if (isAppear) {
        SpawnShockwave(pos, 300.0f, 15.0f, c, false);
        SpawnExplosion(pos, WHITE, 40, 600.0f);
        SpawnExplosion(pos, c, 30, 400.0f);
    } else {
        SpawnShockwave(pos, 100.0f, 5.0f, c, false);
    }
}

void SpawnSlashImpact(Vector2 pos, Color c, float angle, float power) {
    float scaleMod = power / 1000.0f;
    if(scaleMod < 1.0f) scaleMod = 1.0f;

    SpawnShockwave(pos, 250.0f * scaleMod, 20.0f * scaleMod, c, false);
    SpawnExplosion(pos, WHITE, (int)(50 * scaleMod), 700.0f * scaleMod);
    SpawnSpark(pos, (Vector2){cosf(angle), sinf(angle)}, (int)(30 * scaleMod), COL_SPARK);

    if(power > 5000.0f) {
        SpawnShockwave(pos, 500.0f * scaleMod, 50.0f, WHITE, true);
    }
}

void UpdateVisuals(void) {
    gridScroll += 0.5f;
    if(gridScroll > 40.0f) gridScroll -= 40.0f;

    for(int i=0; i<MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].pos = Vector2Add(particles[i].pos, particles[i].vel);
            particles[i].life -= particles[i].decay;
            particles[i].vel = Vector2Scale(particles[i].vel, 0.95f);
            if(particles[i].gravity) particles[i].vel.y += 0.5f;
            particles[i].rotation += particles[i].rotSpeed;
            if (particles[i].life <= 0) particles[i].active = false;
        }
    }
    for(int i=0; i<MAX_SHOCKWAVES; i++) {
        if (shockwaves[i].active) {
            shockwaves[i].radius += (shockwaves[i].maxRadius - shockwaves[i].radius) * 0.15f;
            shockwaves[i].life -= 0.05f;
            if (shockwaves[i].life <= 0) shockwaves[i].active = false;
        }
    }
    for(int i=0; i<MAX_TEXT_FX; i++) {
        if (textEffects[i].active) {
            textEffects[i].pos = Vector2Add(textEffects[i].pos, textEffects[i].vel);
            textEffects[i].life -= 0.02f;
            if (textEffects[i].life <= 0) textEffects[i].active = false;
        }
    }
}

void UpdateTrail(Eraser* e) {
    if (Vector2Length(e->vel) > 10.0f) {
        e->trail[e->trailHead] = e->pos;
        e->trailHead = (e->trailHead + 1) % MAX_TRAILS;
    }
}

// --- Skill Logic ---
void SpawnSkillWave(Eraser* e, int ownerId) {
    for(int i=0; i<MAX_SKILL_WAVES; i++) {
        if (!skillWaves[i].active) {
            skillWaves[i].active = true;
            skillWaves[i].pos = e->pos;
            skillWaves[i].currentRadius = 10.0f;
            float sizeBonus = (float)e->sizeRank * WAVE_SIZE_FACTOR;
            skillWaves[i].maxRadius = WAVE_BASE_RADIUS + sizeBonus;
            skillWaves[i].ownerId = ownerId;
            skillWaves[i].color = e->bodyColor;
            SpawnTextFX(e->pos, "SHOCKWAVE!", e->bodyColor, 40);
            return;
        }
    }
}

void UpdateSkillWaves(void) {
    for(int i=0; i<MAX_SKILL_WAVES; i++) {
        if (skillWaves[i].active) {
            skillWaves[i].currentRadius += WAVE_SPEED;
            Eraser* owner = (skillWaves[i].ownerId == 1) ? &p1 : &p2;
            Eraser* target = (skillWaves[i].ownerId == 1) ? &p2 : &p1;

            if (!target->isDead) {
                float dist = Vector2Distance(skillWaves[i].pos, target->pos);
                if (fabsf(dist - skillWaves[i].currentRadius) < WAVE_SPEED + target->radius) {

                    bool defended = false;
                    if (target->invulnTimer > 0) defended = true;
                    else if (target->absoluteDefenseCount > 0) {
                        target->absoluteDefenseCount--;
                        target->invulnTimer = ABS_INVULN_TIME;
                        SpawnShieldBreakEffect(target->pos, target->radius);
                        SpawnTextFX(target->pos, "BLOCKED", COL_ABS_DEF, 20);
                        ApplyShake(5.0f);
                        defended = true;
                    }

                    if (!defended) {
                        Vector2 dir = Vector2Normalize(Vector2Subtract(target->pos, skillWaves[i].pos));
                        float multiplier = (float)(owner->power + owner->defense) / 2.0f;
                        float finalForce = WAVE_BASE_FORCE * multiplier;
                        
                        // CLIFF WARRIOR BUFF CHECK
                        if (target->cliffActiveTimer > 0) {
                            finalForce *= 0.05f; // 95% Cut
                            SpawnTextFX(target->pos, "CLIFF GUARD", COL_CLIFF, 20);
                        }

                        target->vel = Vector2Add(target->vel, Vector2Scale(dir, finalForce));
                        SpawnExplosion(target->pos, skillWaves[i].color, 15, 300.0f);
                        ApplyShake(15.0f);
                    }
                }
            }
            if (skillWaves[i].currentRadius >= skillWaves[i].maxRadius) {
                skillWaves[i].active = false;
            }
        }
    }
}

// DIMENSIONAL SLASH
void SpawnSlash(Eraser* e, Vector2 targetPos, float power) {
    for(int i=0; i<MAX_SLASHES; i++) {
        if (!slashes[i].active) {
            slashes[i].active = true;
            slashes[i].pos = e->pos;
            Vector2 delta = Vector2Subtract(targetPos, e->pos);
            slashes[i].vel = Vector2Scale(Vector2Normalize(delta), 50.0f);
            slashes[i].rotation = atan2f(delta.y, delta.x) * RAD2DEG;
            slashes[i].life = 1.0f;
            slashes[i].ownerId = (e == &p1) ? 1 : 2;
            slashes[i].color = COL_DIMENSION;
            slashes[i].damage = power;
            return;
        }
    }
}

void UpdateSlashes(void) {
    for(int i=0; i<MAX_SLASHES; i++) {
        if (slashes[i].active) {
            slashes[i].pos = Vector2Add(slashes[i].pos, slashes[i].vel);
            slashes[i].life -= 0.02f;

            Eraser* target = (slashes[i].ownerId == 1) ? &p2 : &p1;

            if (!target->isDead) {
                if (CheckCollisionCircleRec(slashes[i].pos, 30.0f,
                    (Rectangle){target->pos.x - target->radius, target->pos.y - target->radius, target->radius*2, target->radius*2})) {

                    bool defended = false;
                    if (target->invulnTimer > 0) defended = true;
                    else if (target->absoluteDefenseCount > 0) {
                        target->absoluteDefenseCount--;
                        target->invulnTimer = ABS_INVULN_TIME;
                        SpawnShieldBreakEffect(target->pos, target->radius);
                        SpawnTextFX(target->pos, "NULLIFIED", COL_ABS_DEF, 30);
                        ApplyShake(10.0f);
                        defended = true;
                    }

                    if (!defended) {
                        Vector2 forceDir = Vector2Normalize(slashes[i].vel);
                        float dmg = slashes[i].damage;
                        
                        // CLIFF WARRIOR GUARD
                        if (target->cliffActiveTimer > 0) {
                            dmg *= 0.05f;
                            SpawnTextFX(target->pos, "CLIFF GUARD", COL_CLIFF, 20);
                        }

                        target->vel = Vector2Add(target->vel, Vector2Scale(forceDir, dmg));

                        float impactScale = dmg / 500.0f;
                        if(impactScale < 1.0f) impactScale = 1.0f;

                        SpawnSlashImpact(target->pos, slashes[i].color, slashes[i].rotation * DEG2RAD, dmg);
                        SpawnTextFX(target->pos, "JUDGMENT CUT!", RED, 60 + (int)impactScale);

                        ApplyShake(40.0f * (impactScale * 0.5f));
                        TriggerHitStop(20, 1.3f);
                    }
                    slashes[i].active = false;
                }
            }

            if (slashes[i].life <= 0) slashes[i].active = false;
        }
    }
}

// --- System & Physics ---
void InitObstacles(void) {
    obstacles[0] = (Obstacle){ OBS_BOX, (Rectangle){200, 150, 120, 160}, {0}, 0, 0.0f, (Color){60, 40, 100, 255}, 0.5f, "DICT" };
    obstacles[1] = (Obstacle){ OBS_BOX, (Rectangle){900, 500, 200, 80}, {0}, 0, 0.0f, (Color){40, 40, 40, 255}, 1.2f, "CASE" };
    obstacles[2] = (Obstacle){ OBS_BOX, (Rectangle){500, 120, 300, 40}, {0}, 0, 0.0f, (Color){200, 200, 200, 255}, 0.8f, "RULER" };
    obstacles[3] = (Obstacle){ OBS_CIRCLE, {0}, (Vector2){300, 500}, 60.0f, 0, (Color){200, 200, 255, 150}, 0.7f, "" };
    obstacles[4] = (Obstacle){ OBS_BOX, (Rectangle){600, 350, 30, 30}, {0}, 0, 45.0f, WHITE, 0.5f, "" };
    obstacles[5] = (Obstacle){ OBS_CIRCLE, {0}, (Vector2){1000, 200}, 40.0f, 0, (Color){100, 255, 100, 255}, 1.5f, "GUM" };
}

void InitEraserStats(Eraser* e, int id) {
    memset(e, 0, sizeof(Eraser));
    e->power = 5; e->defense = 5; e->flight = 5;

    e->slip = 0; e->sizeRank = 0; e->maxCharge = 0;
    e->recoil = 0; e->critRate = 0; e->pursuit = 0; e->reflection = 0;
    e->critPowerLvl = 0; e->longBattleLvl = 0;

    // Skills
    e->absoluteDefenseLvl = 0;
    e->absoluteDefenseCount = 0;
    e->fullThrottle = false;
    e->chargeBreakSkill = false;
    e->heavyStance = false;
    e->homingSkill = false;
    e->zeroDistSkill = false;
    e->uTurnSkill = false;
    e->cliffWarrior = false;
    e->sniperSkill = false;
    e->repulsionSkill = false;
    e->intimidationSkill = false;

    e->wallThrough = false; e->bounceEnabled = true;
    e->defenseCharge = false; e->defChargeStack = 0;
    e->antiDestruct = false;
    e->shockwaveSkill = false;
    e->reActSkill = false; e->reActCooldown = 0;
    e->warpSkill = false; e->warpCooldown = 0;

    e->uTurnDebuffTimer = 0;
    e->cliffActiveTimer = 0;
    e->cliffCoolTimer = 0;
    e->repulsionTurnCounter = 0;
    
    e->chargeBreakStored = 0.0f;

    e->shockwaveCooldown = 0;
    e->defenseMultiplier = 1.0f;
    e->turnHitOpponent = false; e->turnWasHit = false; e->turnMaxImpactTaken = 0.0f;
    e->turnCount = 0;
    e->invulnTimer = 0;
    e->isHomingActive = false;

    if (id == 1) {
        e->bodyColor = COL_P1_MAIN; e->sleeveColor = RAYWHITE;
        snprintf(e->name, 32, "PLAYER 1");
    } else {
        e->bodyColor = COL_P2_MAIN; e->sleeveColor = (Color){ 20, 20, 20, 255 };
        snprintf(e->name, 32, "PLAYER 2");
    }
}

void RecalculateEraserPhysics(Eraser* e) {
    e->radius = 12.0f + (e->sizeRank * 1.75f);
    e->mass = e->radius * (1.0f + (float)e->defense * 0.5f);
}

void InitMatch(void) {
    p1Points = MAX_BONUS_POINTS;
    p2Points = MAX_BONUS_POINTS;
    InitEraserStats(&p1, 1);
    InitEraserStats(&p2, 2);
    // CRITICAL FIX: Ensure physics are calculated even if match is just initialized
    RecalculateEraserPhysics(&p1);
    RecalculateEraserPhysics(&p2);
    p1Wins = 0;
    p2Wins = 0;
    currentRound = 1;
    roundStarter = 1;
    ResetBattleState();
}

void ResetRoundState(void) {
    ResetBattleState();
}

void ResetBattleState(void) {
    p1.pos = (Vector2){ DESK_RECT.x + 150, DESK_RECT.y + DESK_RECT.height/2 };
    p1.vel = Vector2Zero(); p1.isDead = false;
    p1.pursuitStack = 0; p1.defChargeStack = 0;
    p1.reActCooldown = 0; p1.shockwaveCooldown = 0; p1.warpCooldown = 0;
    p1.uTurnDebuffTimer = 0; p1.cliffActiveTimer = 0; p1.cliffCoolTimer = 0;
    p1.chargeBreakStored = 0.0f;
    p1.turnCount = 0; p1.invulnTimer = 0;
    p1.repulsionTurnCounter = 0;
    p1.defenseMultiplier = 1.0f;
    p1.turnHitOpponent = false; p1.turnWasHit = false; p1.turnMaxImpactTaken = 0;
    p1.trailHead = 0; memset(p1.trail, 0, sizeof(p1.trail));
    p1.absoluteDefenseCount = p1.absoluteDefenseLvl * ABS_STOCKS_PER_LVL;
    p1.isHomingActive = false;

    p2.pos = (Vector2){ DESK_RECT.x + DESK_RECT.width - 150, DESK_RECT.y + DESK_RECT.height/2 };
    p2.vel = Vector2Zero(); p2.isDead = false;
    p2.pursuitStack = 0; p2.defChargeStack = 0;
    p2.reActCooldown = 0; p2.shockwaveCooldown = 0; p2.warpCooldown = 0;
    p2.uTurnDebuffTimer = 0; p2.cliffActiveTimer = 0; p2.cliffCoolTimer = 0;
    p2.chargeBreakStored = 0.0f;
    p2.turnCount = 0; p2.invulnTimer = 0;
    p2.repulsionTurnCounter = 0;
    p2.defenseMultiplier = 1.0f;
    p2.turnHitOpponent = false; p2.turnWasHit = false; p2.turnMaxImpactTaken = 0;
    p2.trailHead = 0; memset(p2.trail, 0, sizeof(p2.trail));
    p2.absoluteDefenseCount = p2.absoluteDefenseLvl * ABS_STOCKS_PER_LVL;
    p2.isHomingActive = false;

    for(int i=0; i<MAX_SKILL_WAVES; i++) skillWaves[i].active = false;
    for(int i=0; i<MAX_SLASHES; i++) slashes[i].active = false;

    turnPlayer = roundStarter;
    isTurnProcessing = false; messageTimer = 0;
    isWarpSelectMode = false;
    customizeScrollY = 0.0f;

    gameCamera.target = (Vector2){ GAME_W/2.0f, GAME_H/2.0f };
    gameCamera.offset = gameCamera.target;
    gameCamera.zoom = 1.0f; zoomTarget = 1.0f;
    hitStopFrames = 0; shakeIntensity = 0;
    InitVisuals();
    
    SetMessage(TextFormat("ROUND %d START!", currentRound), 120);
}

void InitGameSystem(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(GAME_W, GAME_H, "Keshi-Pin SIMPLE (Ver 119.0)");
    SetWindowMinSize(640, 360);
    SetTargetFPS(60);
    srand(time(NULL));

    InitObstacles();
    InitMatch();
}

float CalculateDrag(Eraser* e) {
    // If Flight 0 debuff from U-Turn is active, return massive drag
    if (e->uTurnDebuffTimer > 0) return 0.70f; // Extremely sticky

    float airRes = 0.90f + ((float)e->flight / 15.0f) * 0.099f;
    float slipFactor = 0.94f + ((float)e->slip * 0.0028f);
    if(slipFactor > 0.999f) slipFactor = 0.999f;
    float totalDrag = airRes * slipFactor;

    // FULL THROTTLE: FLIGHT BOOST (+30%)
    if (e->fullThrottle) {
        float loss = 1.0f - totalDrag;
        loss *= 0.7f;
        totalDrag = 1.0f - loss;
    }

    return totalDrag;
}

// Raycast Collision Check for CCD
bool CheckWallCollisionCCD(Vector2 start, Vector2 end, float radius, Vector2* hitPos, Vector2* hitNormal, float* hitBounce, int* obsIndex) {
    for(int k=0; k<OBS_COUNT; k++) {
        if(obstacles[k].type == OBS_BOX) {
            Rectangle r = obstacles[k].rect;
            if(CheckCollisionLines(start, end, (Vector2){r.x, r.y}, (Vector2){r.x, r.y+r.height}, NULL) ||
               CheckCollisionLines(start, end, (Vector2){r.x, r.y}, (Vector2){r.x+r.width, r.y}, NULL) ||
               CheckCollisionLines(start, end, (Vector2){r.x+r.width, r.y}, (Vector2){r.x+r.width, r.y+r.height}, NULL) ||
               CheckCollisionLines(start, end, (Vector2){r.x, r.y+r.height}, (Vector2){r.x+r.width, r.y+r.height}, NULL)) {

                   float cX = Clamp(start.x, r.x, r.x + r.width);
                   float cY = Clamp(start.y, r.y, r.y + r.height);
                   Vector2 close = {cX, cY};
                   Vector2 delta = Vector2Subtract(start, close);
                   if (Vector2Length(delta) == 0) delta = Vector2Subtract(start, (Vector2){r.x + r.width/2, r.y + r.height/2});
                   *hitNormal = Vector2Normalize(delta);
                   *hitPos = Vector2Add(close, Vector2Scale(*hitNormal, radius));
                   *hitBounce = obstacles[k].bounce;
                   *obsIndex = k;
                   return true;
            }
        }
    }
    return false;
}

void ResolveCircleRect(Eraser* e, Rectangle rect, float wallBounce) {
    float closestX = Clamp(e->pos.x, rect.x, rect.x + rect.width);
    float closestY = Clamp(e->pos.y, rect.y, rect.y + rect.height);
    Vector2 closest = { closestX, closestY };
    Vector2 delta = Vector2Subtract(e->pos, closest);
    float dist = Vector2Length(delta);

    if (dist > 0 && dist < e->radius) {
        float overlap = e->radius - dist;
        Vector2 normal = Vector2Scale(delta, 1.0f/dist);
        e->pos = Vector2Add(e->pos, Vector2Scale(normal, overlap + 0.1f));

        if (!e->bounceEnabled) {
            e->vel = Vector2Zero();
            SpawnTextFX(e->pos, "STOP", WHITE, 20);
        } else {
            Vector2 reflection = Vector2Reflect(e->vel, normal);
            float reflectMult = 1.0f + ((float)e->reflection * REFLECT_PER_LVL);
            e->vel = Vector2Scale(reflection, wallBounce * 0.8f * reflectMult);

            if (e->chargeBreakSkill && e->chargeBreakStored > 0) {
                Eraser* target = (e == &p1) ? &p2 : &p1;
                SpawnSlash(e, target->pos, e->chargeBreakStored);
                e->chargeBreakStored = 0;
                SpawnTextFX(e->pos, "BREAK FIRE!", COL_DIMENSION, 30);
            }
        }
    }
}

void ResolveEraserCollision(void) {
    if (p1.isDead || p2.isDead) return;

    Vector2 delta = Vector2Subtract(p1.pos, p2.pos);
    float dist = Vector2Length(delta);
    float combinedRadius = p1.radius + p2.radius;

    if (dist < 0.001f) {
        delta = (Vector2){ 1.0f, 0.0f };
        dist = 1.0f;
    }

    if (dist < combinedRadius) {
        Vector2 normal = Vector2Scale(delta, 1.0f/dist);
        float overlap = combinedRadius - dist;
        p1.pos = Vector2Add(p1.pos, Vector2Scale(normal, overlap * 0.5f));
        p2.pos = Vector2Add(p2.pos, Vector2Scale(normal, -overlap * 0.5f));

        Vector2 relVel = Vector2Subtract(p1.vel, p2.vel);
        float velAlongNormal = Vector2DotProduct(relVel, normal);

        if (velAlongNormal > 0) return;

        Eraser* atk = (Vector2Length(p1.vel) > Vector2Length(p2.vel)) ? &p1 : &p2;
        Eraser* def = (atk == &p1) ? &p2 : &p1;

        // DEF CHARGE CALCULATION (1.2x per stack)
        float defenseFactor = 1.0f;
        if (def->defenseCharge) {
            float reduction = powf(1.20f, (float)def->defChargeStack);
            defenseFactor = 1.0f / reduction;
        }

        // --- ZERO DISTANCE SKILL CHECK ---
        bool forceCrit = false;
        if (atk->zeroDistSkill) {
            float gap = Vector2Distance(p1.pos, p2.pos) - (p1.radius + p2.radius);
            if (gap < ZERO_DIST_THRESHOLD) {
                forceCrit = true;
                SpawnTextFX(atk->pos, "ZERO DIST!!", COL_ZERO, 30);
            }
        }

        bool isCrit = (GetRandomValue(1, 100) <= atk->critRate * 5);
        if (forceCrit) isCrit = true; // FORCE CRIT

        float critMult = isCrit ? 3.0f : 1.0f;
        if (isCrit) {
            float critDmgBoost = (float)atk->critPowerLvl * 0.65f;
            critMult += critDmgBoost;
        }

        if (!atk->turnHitOpponent) {
            atk->turnHitOpponent = true;
            atk->isHomingActive = false; // Disable Homing immediately after hit
            
            SpawnTextFX(atk->pos, "HIT!", atk->bodyColor, 30);
            if (isCrit) {
                ApplyShake(35.0f); TriggerHitStop(15, 1.25f);
                SpawnExplosion(Vector2MidPoint(p1.pos, p2.pos), GOLD, 100, 900.0f);
                SetMessage("CRITICAL SMASH!!", 60);
            } else {
                ApplyShake(12.0f); TriggerHitStop(5, 1.05f);
                SpawnExplosion(Vector2MidPoint(p1.pos, p2.pos), WHITE, 25, 400.0f);
            }
            SpawnSpark(Vector2MidPoint(p1.pos, p2.pos), normal, 20, COL_SPARK);
        }

        float critPassive = (float)atk->critRate * 0.10f;
        float powerFactor = 1.0f + ((float)atk->power * 0.2f) + critPassive;

        // Full Throttle Bonus (+30% Power)
        if (atk->fullThrottle) powerFactor *= 1.3f;
        
        // HEAVY STANCE BONUS (x3.0 Power)
        if (atk->heavyStance) powerFactor *= 3.0f;

        // HOMING BONUS (x1.5 Power)
        if (atk->isHomingActive) powerFactor *= 1.5f;

        // REFLECT STAT HIDDEN BONUS
        powerFactor *= (1.0f + (float)atk->reflection * 0.10f);

        // INTIMIDATION CHECK (If Defender has Intimidation, Attacker is weakened)
        if (def->intimidationSkill) {
            powerFactor *= INTIMIDATION_FACTOR;
            SpawnTextFX(atk->pos, "INTIMIDATED", COL_INTIM, 20);
        }

        // Power Cap
        if (powerFactor > MAX_POWER_MULT) powerFactor = MAX_POWER_MULT;

        float baseImpact = powerFactor * 2.0f * critMult * defenseFactor;
        
        // Long Battle defense scaling
        if (def->defenseMultiplier > 1.0f) baseImpact /= def->defenseMultiplier;
        
        // --- SNIPER SKILL: PENETRATION ---
        if (atk->sniperSkill) {
            float dist = Vector2Distance(p1.pos, p2.pos);
            float penetration = (dist / 10.0f) * 0.01f; // 1% per 10px
            if(penetration > 1.0f) penetration = 1.0f;
            baseImpact *= (1.0f + penetration); 
            SpawnTextFX(atk->pos, "SNIPER", COL_SNIPER, 10);
        }

        // --- MAX DEFENSE CAP ---
        float totalDefReduc = defenseFactor / def->defenseMultiplier;
        if (totalDefReduc < 0.2f) totalDefReduc = 0.2f;

        float rawImpact = powerFactor * 2.0f * critMult;
        float finalImpact = rawImpact * totalDefReduc;
        
        if (atk->sniperSkill && baseImpact > finalImpact) finalImpact = baseImpact;

        // ZERO DISTANCE FINAL MULTIPLIER (x1.3)
        if (forceCrit && atk->zeroDistSkill) {
            finalImpact *= 1.3f;
        }

        float j = -(1.0f + 0.9f) * velAlongNormal;
        j /= (1.0f/p1.mass + 1.0f/p2.mass);

        Vector2 impulse = Vector2Scale(normal, j * finalImpact);

        // Absolute Defense (Defender)
        bool defended = false;
        
        if (def->invulnTimer > 0) {
            defended = true;
        }
        else if (def->absoluteDefenseCount > 0) {
            def->absoluteDefenseCount--;
            def->invulnTimer = ABS_INVULN_TIME; 
            defended = true;
            SpawnShieldBreakEffect(def->pos, def->radius);
            SpawnTextFX(def->pos, "A.DEFENSE", COL_ABS_DEF, 25);
            ApplyShake(5.0f);
        }

        if (!defended) {
            if (!def->turnWasHit) {
                def->turnWasHit = true;
            }
            if (finalImpact > def->turnMaxImpactTaken) {
                def->turnMaxImpactTaken = finalImpact;
            }
            
            // CLIFF WARRIOR: REDUCED FLIGHT
            if (def->cliffActiveTimer > 0) {
                impulse = Vector2Scale(impulse, 0.7f);
            }

            p1.vel = Vector2Add(p1.vel, Vector2Scale(impulse, 1.0f/p1.mass));
            p2.vel = Vector2Subtract(p2.vel, Vector2Scale(impulse, 1.0f/p2.mass));
        }

        // --- U-TURN RECOIL LOGIC ---
        if (atk->uTurnSkill) {
            Vector2 recoilDir = (atk == &p1) ? normal : Vector2Scale(normal, -1.0f);
            atk->vel = Vector2Scale(recoilDir, UTURN_RECOIL_SPEED); 
            if (!defended) {
                def->uTurnDebuffTimer = 2; // Debuff Enemy FLIGHT to 0
                SpawnTextFX(def->pos, "FLIGHT 0!", COL_UTURN, 30);
            }
            SpawnTextFX(atk->pos, "U-TURN!", COL_UTURN, 30);
            return; 
        }
        // ---------------------------

        float stability = 0.0f;
        stability += (float)atk->recoil * 0.04f;
        stability += (float)atk->critRate * 0.005f;
        if (isCrit) { stability += (float)atk->critPowerLvl * 0.25f; }
        
        // SNIPER RECOIL REDUCTION
        if (atk->sniperSkill) {
            float dist = Vector2Distance(p1.pos, p2.pos);
            float red = (dist / 10.0f) * 0.02f;
            stability += red;
        }

        if (stability > 1.0f) stability = 1.0f;
        if (stability < 0.0f) stability = 0.0f;

        // Full Throttle Bonus (-30% Recoil)
        if (atk->fullThrottle) stability *= 0.7f;

        // CLIFF WARRIOR: ENEMY RECOIL INCREASE (+50%)
        if (def->cliffActiveTimer > 0) {
            stability -= 0.5f; 
            if(stability < 0.0f) stability = 0.0f;
        }

        // Absolute Defense (Attacker)
        bool noRecoil = false;
        if (atk->invulnTimer > 0) {
            noRecoil = true;
        }
        else if (atk->absoluteDefenseCount > 0) {
            atk->absoluteDefenseCount--;
            atk->invulnTimer = ABS_INVULN_TIME;
            noRecoil = true;
            SpawnShieldBreakEffect(atk->pos, atk->radius);
            SpawnTextFX(atk->pos, "NO RECOIL", COL_ABS_DEF, 25);
        }

        if (!noRecoil) {
            atk->vel = Vector2Scale(atk->vel, 1.0f - stability);
        }
    }
}

void UpdatePhysics(void) {
    if (isWarpSelectMode) return;

    float dt = 1.0f / (float)TARGET_FPS;
    bool someoneMoving = false;
    Eraser* players[] = { &p1, &p2 };

    for(int i=0; i<2; i++) {
        Eraser* e = players[i];
        Eraser* target = (i == 0) ? &p2 : &p1;

        if (e->invulnTimer > 0) e->invulnTimer--;

        if (e->isDead) continue;

        if (Vector2Length(e->vel) > 1.0f) {
            someoneMoving = true;

            // --- HOMING LOGIC ---
            if (e->isHomingActive && !target->isDead) {
                Vector2 toTarget = Vector2Subtract(target->pos, e->pos);
                Vector2 desired = Vector2Normalize(toTarget);
                Vector2 currentDir = Vector2Normalize(e->vel);
                float speed = Vector2Length(e->vel);
                Vector2 newDir = Vector2Add(currentDir, Vector2Scale(desired, HOMING_TURN_RATE));
                newDir = Vector2Normalize(newDir);
                e->vel = Vector2Scale(newDir, speed);
                if (GetRandomValue(0, 5) == 0) {
                    SpawnParticle(e->pos, Vector2Zero(), COL_HOMING, 4.0f, 0.1f, true, false);
                }
            }

            int steps = 1 + (int)(Vector2Length(e->vel) * dt / 5.0f);
            if(steps > MAX_PHYSICS_STEPS) steps = MAX_PHYSICS_STEPS;
            float subDt = dt / (float)steps;

            for(int s=0; s<steps; s++) {
                Vector2 nextPos = Vector2Add(e->pos, Vector2Scale(e->vel, subDt));

                Vector2 hitPos, hitNormal;
                float hitBounce;
                int obsIdx;
                if (!e->wallThrough && CheckWallCollisionCCD(e->pos, nextPos, e->radius, &hitPos, &hitNormal, &hitBounce, &obsIdx)) {
                    // CCD Collision
                    e->pos = hitPos;
                    Vector2 reflection = Vector2Reflect(e->vel, hitNormal);
                    float reflectMult = 1.0f + ((float)e->reflection * REFLECT_PER_LVL);
                    e->vel = Vector2Scale(reflection, hitBounce * reflectMult);

                    // CHARGE BREAK TRIGGER (CCD)
                    if (e->chargeBreakSkill && e->chargeBreakStored > 0) {
                        SpawnSlash(e, target->pos, e->chargeBreakStored);
                        e->chargeBreakStored = 0; // Reset
                        SpawnTextFX(e->pos, "BREAK FIRE!", COL_DIMENSION, 30);
                    }

                } else {
                    e->pos = nextPos;
                    // Discrete Check
                    if (!e->wallThrough) {
                        for(int k=0; k<OBS_COUNT; k++) {
                            if (obstacles[k].type == OBS_BOX) {
                                ResolveCircleRect(e, obstacles[k].rect, obstacles[k].bounce);
                            } else {
                                Vector2 delta = Vector2Subtract(e->pos, obstacles[k].pos);
                                if(Vector2Length(delta) < e->radius + obstacles[k].radius) {
                                    Vector2 norm = Vector2Normalize(delta);
                                    if (!e->bounceEnabled) {
                                        e->vel = Vector2Zero();
                                        e->pos = Vector2Add(obstacles[k].pos, Vector2Scale(norm, e->radius + obstacles[k].radius + 0.1f));
                                    } else {
                                        Vector2 reflection = Vector2Reflect(e->vel, norm);
                                        float reflectMult = 1.0f + ((float)e->reflection * REFLECT_PER_LVL);
                                        e->vel = Vector2Scale(reflection, obstacles[k].bounce * reflectMult);
                                        e->pos = Vector2Add(obstacles[k].pos, Vector2Scale(norm, e->radius + obstacles[k].radius + 0.1f));
                                    }
                                }
                            }
                        }
                    }
                }
            }

            e->rotation += e->vel.x * dt * 5.0f;
            UpdateTrail(e);
            e->vel = Vector2Scale(e->vel, CalculateDrag(e));
        } else {
            e->vel = Vector2Zero();
        }

        if (!CheckCollisionPointRec(e->pos, DESK_RECT)) {
            bool saved = false;
            
            // --- ANTI-DESTRUCT (Only save if self-inflicted) ---
            bool isMyTurn = (turnPlayer == (i + 1));
            if (e->antiDestruct && isMyTurn) { 
                Vector2 n = {0,0};
                if (e->pos.x < DESK_RECT.x) n.x = 1.0f; 
                else if (e->pos.x > DESK_RECT.x + DESK_RECT.width) n.x = -1.0f;
                
                if (e->pos.y < DESK_RECT.y) n.y = 1.0f; 
                else if (e->pos.y > DESK_RECT.y + DESK_RECT.height) n.y = -1.0f;

                if (n.x != 0 || n.y != 0) {
                    if (n.x != 0) e->vel.x = fabsf(e->vel.x) * n.x;
                    if (n.y != 0) e->vel.y = fabsf(e->vel.y) * n.y;
                    
                    e->vel = Vector2Scale(e->vel, 1.0f + ((float)e->reflection * REFLECT_PER_LVL));
                    e->pos = Vector2Add(e->pos, Vector2Scale(n, e->radius + 5.0f));
                    
                    SpawnTextFX(e->pos, "SAFE!", GREEN, 40);
                    SpawnExplosion(e->pos, GREEN, 15, 300.0f);
                    ApplyShake(10.0f);
                    saved = true;
                }
            }
            // -------------------------------------

            if (!saved) {
                e->isDead = true;
                ApplyShake(50.0f); TriggerHitStop(10, 1.0f); SetMessage("FALL OUT!", 120);
                SpawnShockwave(e->pos, 800.0f, 40.0f, RED, false);
                
                // ROUND LOGIC
                int winner = (i == 0) ? 2 : 1;
                if (winner == 1) p1Wins++; else p2Wins++;
                
                if (p1Wins >= 2 || p2Wins >= 2) {
                    winnerID = winner;
                    currentScene = SCENE_RESULT;
                } else {
                    currentRound++;
                    roundStarter = (roundStarter == 1) ? 2 : 1;
                    ResetRoundState();
                }
            }
        }
    }

    ResolveEraserCollision();
    UpdateSkillWaves();
    UpdateSlashes();

    if (!someoneMoving && isTurnProcessing) {
        Eraser* curr = (turnPlayer == 1) ? &p1 : &p2;
        Eraser* enemy = (turnPlayer == 1) ? &p2 : &p1;

        if (curr->warpSkill && curr->warpCooldown == 0 && !curr->isDead && !isWarpSelectMode) {
            isWarpSelectMode = true;
            warpActivePlayerId = turnPlayer;
            SetMessage("WARP SELECT", 0);
            return;
        }

        if (curr->turnHitOpponent) {
            curr->pursuitStack++;
        }

        if (enemy->turnWasHit) {
            enemy->defChargeStack++;
        }

        if (curr->shockwaveSkill && !curr->isDead) {
            if(curr->shockwaveCooldown == 0) {
                SpawnSkillWave(curr, turnPlayer);
                curr->shockwaveCooldown = WAVE_COOLDOWN;
            }
        }

        if (curr->longBattleLvl > 0) {
            float growth = 1.0f + ((float)curr->longBattleLvl * 0.025f);
            curr->defenseMultiplier *= growth;
            SpawnTextFX(curr->pos, "DEF UP", GREEN, 20);
        }
        
        // CLIFF WARRIOR CHECK
        if (curr->cliffWarrior) {
            if (curr->cliffActiveTimer > 0) curr->cliffActiveTimer--;
            else if (curr->cliffCoolTimer > 0) curr->cliffCoolTimer--;
            else if (IsNearCliff(curr)) {
                curr->cliffActiveTimer = CLIFF_DURATION;
                curr->cliffCoolTimer = CLIFF_COOLDOWN; 
                SpawnTextFX(curr->pos, "CLIFF ACT", COL_CLIFF, 30);
            }
        }

        bool extraTurn = false;
        if (curr->reActSkill && curr->reActCooldown == 0) {
            extraTurn = true;
            curr->reActCooldown = 2;
            SetMessage("RE-ACT!!", 60);
            SpawnTextFX(curr->pos, "AGAIN!", PURPLE, 40);
            ApplyShake(20.0f);
        } else {
            if (curr->reActCooldown > 0) curr->reActCooldown--;
        }

        if (!extraTurn) {
            curr->turnCount++;
            curr->repulsionTurnCounter++; // Repulsion tick
            
            // REPULSION SKILL (Every 3 turns)
            if (curr->repulsionSkill && curr->repulsionTurnCounter % REPULSION_INTERVAL == 0) {
                if (!enemy->isDead) {
                    float dist = Vector2Distance(curr->pos, enemy->pos);
                    if (dist >= REPULSION_DIST_REQ) {
                        Vector2 dir = Vector2Normalize(Vector2Subtract(enemy->pos, curr->pos));
                        enemy->pos = Vector2Add(enemy->pos, Vector2Scale(dir, REPULSION_PUSH));
                        SpawnShockwave(curr->pos, 1000.0f, 20.0f, COL_REPULSE, true);
                        SpawnTextFX(curr->pos, "REPULSION!", COL_REPULSE, 40);
                        SetMessage("DISTANCE KEEPER ACTIVATED!", 60);
                    }
                }
            }

            // U-TURN DEBUFF COUNTDOWN
            if (curr->uTurnDebuffTimer > 0) curr->uTurnDebuffTimer--;

            // CHARGE BREAK ACCUMULATION
            if (curr->chargeBreakSkill) {
                curr->chargeBreakStored += CHARGE_BREAK_PER_TURN;
                if (curr->chargeBreakStored > CHARGE_BREAK_MAX) curr->chargeBreakStored = CHARGE_BREAK_MAX;
                SpawnTextFX(curr->pos, "CHARGING...", COL_DIMENSION, 10);
            }
            
            turnPlayer = (turnPlayer == 1) ? 2 : 1;
            Eraser* next = (turnPlayer == 1) ? &p1 : &p2;
            if(next->shockwaveCooldown > 0) next->shockwaveCooldown--;
            if(next->warpCooldown > 0) next->warpCooldown--;
            
            // RESET FLAGS
            curr->isHomingActive = false; 
            next->isHomingActive = false;
        }

        p1.turnHitOpponent = false; p1.turnWasHit = false; p1.turnMaxImpactTaken = 0;
        p2.turnHitOpponent = false; p2.turnWasHit = false; p2.turnMaxImpactTaken = 0;

        isTurnProcessing = false;
    }

    if (someoneMoving) isTurnProcessing = true;
}

void UpdateInput(void) {
    Eraser* curr = (turnPlayer == 1) ? &p1 : &p2;
    Eraser* target = (turnPlayer == 1) ? &p2 : &p1;
    Vector2 virtualMouse = GetVirtualMousePos();

    if (isWarpSelectMode) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Eraser* warpSubject = (warpActivePlayerId == 1) ? &p1 : &p2;

            Vector2 targetPos = GetScreenToWorld2D(virtualMouse, gameCamera);
            targetPos.x = Clamp(targetPos.x, DESK_RECT.x + 20, DESK_RECT.x + DESK_RECT.width - 20);
            targetPos.y = Clamp(targetPos.y, DESK_RECT.y + 20, DESK_RECT.y + DESK_RECT.height - 20);

            SpawnWarpVisuals(warpSubject->pos, warpSubject->bodyColor, false);
            warpSubject->pos = targetPos;
            warpSubject->vel = Vector2Zero();
            SpawnWarpVisuals(warpSubject->pos, warpSubject->bodyColor, true);
            ApplyShake(30.0f);

            warpSubject->warpCooldown = WARP_COOLDOWN;
            isWarpSelectMode = false;

            isTurnProcessing = true;
        }
        return;
    }

    if (isTurnProcessing || messageTimer > 0) return;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 camMouse = GetScreenToWorld2D(virtualMouse, gameCamera);
        if (CheckCollisionPointCircle(camMouse, curr->pos, curr->radius * 2.5f)) {
            isDragging = true;
            dragStart = camMouse;
            rouletteValue = 1;
        }
    }

    if (isDragging) {
        Vector2 camMouse = GetScreenToWorld2D(virtualMouse, gameCamera);
        dragEnd = camMouse;

        int effectiveMaxCharge = 10 + curr->maxCharge;
        if (effectiveMaxCharge > 10 + MAX_CHARGE_LVL) effectiveMaxCharge = 10 + MAX_CHARGE_LVL;

        // FULL THROTTLE: Skip Roulette
        if (curr->fullThrottle) {
            rouletteValue = effectiveMaxCharge;
        } else {
            if (frameCounter % 4 == 0) {
                if (rouletteUp) {
                    rouletteValue++;
                    if (rouletteValue >= effectiveMaxCharge) rouletteUp = false;
                } else {
                    rouletteValue--;
                    if (rouletteValue <= 1) rouletteUp = true;
                }
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            isDragging = false;
            Vector2 pull = Vector2Subtract(dragStart, dragEnd);
            if (Vector2Length(pull) > 20.0f) {
                Vector2 dir = Vector2Normalize(pull);
                float pursuitBonus = (float)curr->pursuitStack * (float)curr->pursuit * 0.1f;
                float multiplier = 1.0f + pursuitBonus;
                if(multiplier > PURSUIT_CAP) multiplier = PURSUIT_CAP;

                float power = rouletteValue * 120.0f * multiplier;
                curr->vel = Vector2Scale(dir, power);

                // --- NEW SKILL LOGIC ---
                
                // HEAVY STANCE: Speed 0.5x
                if (curr->heavyStance) {
                    curr->vel = Vector2Scale(curr->vel, 0.5f);
                    SpawnTextFX(curr->pos, "HEAVY", COL_HEAVY, 30);
                }

                // INTIMIDATION (If Opponent has Intimidation, I am slowed)
                if (target->intimidationSkill) {
                    curr->vel = Vector2Scale(curr->vel, INTIMIDATION_FACTOR);
                    SpawnTextFX(curr->pos, "INTIMIDATED", COL_INTIM, 20);
                }

                // U-TURN DEBUFF: FLIGHT 0
                if (curr->uTurnDebuffTimer > 0) {
                    SpawnTextFX(curr->pos, "FLIGHT DOWN", DARKGRAY, 20);
                }

                // HOMING: Check Range & Activate
                if (curr->homingSkill) {
                    float dist = Vector2Distance(curr->pos, target->pos);
                    if (dist >= HOMING_MIN_DIST && dist <= HOMING_MAX_DIST) {
                        curr->isHomingActive = true;
                        SpawnTextFX(curr->pos, "HOMING!!", COL_HOMING, 30);
                    } else {
                        curr->isHomingActive = false;
                        SpawnTextFX(curr->pos, "NO TGT", DARKGRAY, 20);
                    }
                }

                // -----------------------

                if (curr->pursuitStack > 0) { SpawnTextFX(curr->pos, "BURST!!", GOLD, 40); curr->pursuitStack = 0; }

                // Full Throttle Visual
                if (curr->fullThrottle) {
                    SpawnBeamEffect(curr->pos, Vector2Add(curr->pos, Vector2Scale(dir, 300)), curr->bodyColor);
                    SpawnTextFX(curr->pos, "MAX POWER!", curr->bodyColor, 40);
                    ApplyShake(10.0f);
                }

                isTurnProcessing = true;
            }
        }
    }
}

// =========================================================================================
//  6. DRAWING FUNCTIONS
// =========================================================================================

void DrawFlatObstacles(void) {
    for(int i=0; i<OBS_COUNT; i++) {
        Obstacle o = obstacles[i];
        if (o.type == OBS_BOX) {
            DrawRectanglePro(o.rect, (Vector2){0,0}, o.rotation, Fade(BLACK, 0.4f));
            Rectangle top = {o.rect.x - 4, o.rect.y - 4, o.rect.width, o.rect.height};
            DrawRectanglePro(top, (Vector2){0,0}, o.rotation, o.color);
            DrawRectangleLinesEx(top, 2, AdjustBrightness(o.color, 1.3f));
            DrawText(o.label, top.x + 10, top.y + 10, 10, Fade(WHITE, 0.8f));
        } else {
            DrawCircleV(o.pos, o.radius, Fade(BLACK, 0.4f));
            DrawCircleV((Vector2){o.pos.x-3, o.pos.y-3}, o.radius, o.color);
            DrawCircleLines((int)o.pos.x-3, (int)o.pos.y-3, o.radius, AdjustBrightness(o.color, 1.5f));
        }
    }
}

void DrawSkillWaves(void) {
    for(int i=0; i<MAX_SKILL_WAVES; i++) {
        if (skillWaves[i].active) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleLines((int)skillWaves[i].pos.x, (int)skillWaves[i].pos.y, skillWaves[i].currentRadius, skillWaves[i].color);
            DrawCircleLines((int)skillWaves[i].pos.x, (int)skillWaves[i].pos.y, skillWaves[i].currentRadius - 5, Fade(skillWaves[i].color, 0.5f));
            EndBlendMode();
        }
    }
}

void DrawSlashes(void) {
    BeginBlendMode(BLEND_ADDITIVE);
    for(int i=0; i<MAX_SLASHES; i++) {
        if(slashes[i].active) {
            Vector2 p = slashes[i].pos;
            float rot = slashes[i].rotation;

            // SCALED VISUALS
            float intensity = slashes[i].damage / 500.0f; // Scale visual by power
            if(intensity < 0.5f) intensity = 0.5f;

            float radius = 40.0f * intensity;
            if (radius > 3000.0f) radius = 3000.0f;

            DrawCircleSector(p, radius, rot - 60, rot + 60, 10, Fade(slashes[i].color, 0.8f));
            DrawCircleSector(p, radius * 0.75f, rot - 50, rot + 50, 10, Fade(WHITE, 0.9f));
        }
    }
    EndBlendMode();
}

void DrawWarpTarget(Eraser* e) {
    Vector2 mouse = GetScreenToWorld2D(GetVirtualMousePos(), gameCamera);
    mouse.x = Clamp(mouse.x, DESK_RECT.x + 20, DESK_RECT.x + DESK_RECT.width - 20);
    mouse.y = Clamp(mouse.y, DESK_RECT.y + 20, DESK_RECT.y + DESK_RECT.height - 20);

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV(mouse, e->radius, Fade(e->bodyColor, 0.3f));
    DrawCircleLines((int)mouse.x, (int)mouse.y, e->radius + 10 + sinf(GetTime()*15)*5, e->bodyColor);

    float rot = GetTime() * 90.0f;
    Rectangle rectH = {mouse.x, mouse.y, 60, 4};
    Rectangle rectV = {mouse.x, mouse.y, 4, 60};
    DrawRectanglePro(rectH, (Vector2){30,2}, rot, Fade(e->bodyColor, 0.8f));
    DrawRectanglePro(rectV, (Vector2){2,30}, -rot, Fade(e->bodyColor, 0.8f));

    DrawText("CLICK DESTINATION", mouse.x - 50, mouse.y - 50, 10, WHITE);
    EndBlendMode();
}

void DrawFlatEraser(Eraser* e) {
    if(e->isDead) return;
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_TRAILS; i++) {
        int idx = (e->trailHead - 1 - i + MAX_TRAILS) % MAX_TRAILS;
        Vector2 p = e->trail[idx];
        if (p.x == 0 && p.y == 0) continue;
        float alpha = 1.0f - ((float)i / 20.0f);
        if (alpha < 0) break;
        Color trailCol = e->wallThrough ? WHITE : e->bodyColor;
        DrawCircleV(p, e->radius * alpha * 0.8f, Fade(trailCol, alpha * 0.4f));
    }
    EndBlendMode();

    float w = e->radius * 2.2f; float h = e->radius * 1.5f;
    Vector2 origin = { w/2, h/2 };
    float alpha = e->wallThrough ? 0.6f : 1.0f;
    DrawRectanglePro((Rectangle){ e->pos.x + 8, e->pos.y + 8, w, h }, origin, e->rotation, Fade(BLACK, 0.3f * alpha));
    DrawRectanglePro((Rectangle){ e->pos.x, e->pos.y, w, h }, origin, e->rotation, Fade(e->bodyColor, alpha));
    DrawRectanglePro((Rectangle){ e->pos.x, e->pos.y, w * 0.6f, h + 2.0f }, (Vector2){ w * 0.3f, h/2 + 1.0f }, e->rotation, Fade(e->sleeveColor, alpha));

    if (e->absoluteDefenseCount > 0) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawPolyLines(e->pos, 6, e->radius * 1.4f, GetTime()*30.0f, Fade(COL_ABS_DEF, 0.8f));
        DrawPoly(e->pos, 6, e->radius * 1.4f, GetTime()*30.0f, Fade(COL_ABS_DEF, 0.1f));
        DrawText(TextFormat("A.DEF: %d", e->absoluteDefenseCount), e->pos.x - 20, e->pos.y - 50, 10, COL_ABS_DEF);
        EndBlendMode();
    }

    if (e->pursuitStack > 0) {
        float pulse = (sinf(GetTime() * 10.0f) + 1.0f) * 0.5f;
        DrawCircleLines((int)e->pos.x, (int)e->pos.y, e->radius * (1.2f + pulse*0.3f), Fade(GOLD, 0.6f));
        DrawText(TextFormat("x%d", e->pursuitStack), e->pos.x - 10, e->pos.y - 40, 20, GOLD);
    }
    if (e->defChargeStack > 0) DrawCircleLines((int)e->pos.x, (int)e->pos.y, e->radius * 1.1f, SKYBLUE);
    if (e->antiDestruct) { DrawCircleLines((int)e->pos.x, (int)e->pos.y, e->radius * 1.5f, LIME); }
    if (e->shockwaveSkill) {
        if(e->shockwaveCooldown > 0) DrawText("WAVE CD", e->pos.x-20, e->pos.y+30, 10, GRAY);
        else DrawCircleLines((int)e->pos.x, (int)e->pos.y, e->radius * 1.3f, Fade(e->bodyColor, 0.5f));
    }
    if (e->critPowerLvl > 0) { DrawCircleLines((int)e->pos.x, (int)e->pos.y, e->radius * 1.2f, Fade(RED, 0.7f)); }
    if (e->reActSkill) { DrawText("RE-ACT", e->pos.x-20, e->pos.y+40, 10, (e->reActCooldown==0)?GREEN:GRAY); }
    if (e->warpSkill) {
        if(e->warpCooldown > 0) DrawText("WARP CD", e->pos.x-20, e->pos.y+50, 10, GRAY);
        else DrawText("WARP READY", e->pos.x-30, e->pos.y+50, 10, MAGENTA);
    }
    if (e->heavyStance) {
        DrawText("HEAVY", e->pos.x-20, e->pos.y+20, 10, COL_HEAVY);
        DrawCircleLines(e->pos.x, e->pos.y, e->radius * 1.6f, COL_HEAVY);
    }
    if (e->homingSkill) {
        DrawText("HOMING", e->pos.x-20, e->pos.y+60, 10, COL_HOMING);
    }
    if (e->zeroDistSkill) {
        DrawText("ZERO", e->pos.x-20, e->pos.y+70, 10, COL_ZERO);
    }
    if (e->uTurnSkill) {
        DrawText("U-TURN", e->pos.x-20, e->pos.y+80, 10, COL_UTURN);
    }
    if (e->sniperSkill) {
        DrawText("SNIPER", e->pos.x-20, e->pos.y+90, 10, COL_SNIPER);
    }
    if (e->repulsionSkill) {
        DrawText("REPULSE", e->pos.x-20, e->pos.y+100, 10, COL_REPULSE);
    }
    if (e->intimidationSkill) {
        DrawText("INTIMIDATE", e->pos.x-20, e->pos.y+110, 10, COL_INTIM);
    }
    if (e->cliffActiveTimer > 0) {
        DrawText("CLIFF", e->pos.x-20, e->pos.y-60, 10, COL_CLIFF);
        DrawPolyLines(e->pos, 3, e->radius * 2.0f, GetTime()*100, COL_CLIFF);
    }
    if (e->uTurnDebuffTimer > 0) {
        DrawText("FLIGHT 0", e->pos.x+20, e->pos.y, 10, COL_UTURN);
    }

    // CHARGE BREAK DISPLAY
    if (e->chargeBreakSkill) {
        float pct = e->chargeBreakStored / CHARGE_BREAK_MAX;
        DrawRectangle(e->pos.x - 20, e->pos.y + 30, 40, 5, BLACK);
        DrawRectangle(e->pos.x - 20, e->pos.y + 30, 40 * pct, 5, COL_DIMENSION);
        DrawText(TextFormat("%.0f", e->chargeBreakStored), e->pos.x-10, e->pos.y+40, 10, COL_DIMENSION);
    }

    if (e->longBattleLvl > 0 && e->defenseMultiplier > 1.2f) {
        DrawCircleLines((int)e->pos.x, (int)e->pos.y, e->radius * 1.8f, Fade(BLUE, 0.5f));
    }
}

void DrawStatUI(int x, int y, const char* label, int* val, int min, int max, int* points, int cost) {
    DrawText(label, x, y, 20, WHITE);
    Rectangle btnMinus = { x + 110, y - 2, 30, 30 };
    Rectangle barRect = { x + 150, y + 5, 120, 20 };
    Rectangle btnPlus = { x + 320, y - 2, 30, 30 };

    bool hM = CheckCollisionPointRec(GetVirtualMousePos(), btnMinus);
    bool hP = CheckCollisionPointRec(GetVirtualMousePos(), btnPlus);

    DrawRectangleRec(btnMinus, hM ? WHITE : LIGHTGRAY); DrawText("-", btnMinus.x + 11, btnMinus.y + 5, 20, BLACK);
    DrawRectangleRec(barRect, Fade(BLACK, 0.5f));
    float pct = (float)(*val)/max;
    DrawRectangle(barRect.x, barRect.y, barRect.width * pct, barRect.height, (*val>=max)?GOLD:GREEN);
    DrawText(TextFormat("%2d", *val), x + 280, y, 20, (*val==max)?GOLD:WHITE);
    DrawRectangleRec(btnPlus, hP ? WHITE : LIGHTGRAY); DrawText("+", btnPlus.x + 9, btnPlus.y + 5, 20, BLACK);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hM && *val > min) {
            *val -= 1;
            *points += cost;
        }
        if (hP && *val < max && *points >= cost) {
            *val += 1;
            *points -= cost;
        }
    }
}

void DrawToggleUI(int x, int y, const char* label, bool* val, int* points, int cost) {
    DrawText(label, x, y, 20, (*val) ? GOLD : WHITE);
    Rectangle btn = { x + 150, y - 2, 100, 30 };
    bool hover = CheckCollisionPointRec(GetVirtualMousePos(), btn);

    DrawRectangleRec(btn, *val ? GREEN : RED); DrawRectangleLinesEx(btn, 2, WHITE);
    DrawText(*val ? "ON" : "OFF", btn.x + 35, btn.y + 5, 20, WHITE);
    DrawText(TextFormat("(%dpt)", cost), btn.x + 110, y + 5, 10, LIGHTGRAY);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
        if (*val) { *val = false; *points += cost; }
        else if (*points >= cost) { *val = true; *points -= cost; }
    }
}

void DrawNegativeToggleUI(int x, int y, const char* label, bool* val, int* points, int cost) {
    DrawText(label, x, y, 20, WHITE);
    Rectangle btn = { x + 150, y - 2, 100, 30 };
    bool hover = CheckCollisionPointRec(GetVirtualMousePos(), btn);

    DrawRectangleRec(btn, *val ? GREEN : PURPLE); DrawRectangleLinesEx(btn, 2, WHITE);
    DrawText(*val ? "ON" : "OFF", btn.x + 35, btn.y + 5, 20, WHITE);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
        if (*val) { if (*points >= cost) { *val = false; *points -= cost; } }
        else { *val = true; *points += cost; }
    }
}

void DrawSceneCustomize(void) {
    DrawRectangle(0, 0, GAME_W, GAME_H, COL_BG_DARK);
    BeginBlendMode(BLEND_ADDITIVE);
    for(int i=0; i<GAME_W; i+=60) DrawLine(i, 0, i, GAME_H, Fade(COL_GRID, 0.3f));
    for(int i=0; i<GAME_H; i+=60) DrawLine(0, i, GAME_W, i, Fade(COL_GRID, 0.3f));
    EndBlendMode();

    DrawText("KESHIPIN FACTORY: SIMPLE", 50, 20, 50, WHITE);
    DrawText("SCROLL WITH MOUSE WHEEL", 50, 70, 20, GOLD);

    // MOUSE WHEEL SCROLL
    float wheel = GetMouseWheelMove();
    customizeScrollY += wheel * 30.0f;

    // Limits
    int contentHeight = 1400; // Increased
    int viewHeight = 580;
    int maxScroll = contentHeight - viewHeight + 200; // Extra padding
    if (maxScroll < 0) maxScroll = 0;

    // SCROLL BAR LOGIC
    int scrollBarX = GAME_W - 20;
    int scrollBarY = 100;
    int scrollBarW = 12;
    Rectangle trackRect = { scrollBarX, scrollBarY, scrollBarW, viewHeight };

    Vector2 mouse = GetVirtualMousePos();
    if (CheckCollisionPointRec(mouse, trackRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isScrollBarDragging = true;
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        isScrollBarDragging = false;
    }

    if (isScrollBarDragging) {
        float ratio = (mouse.y - scrollBarY) / (float)viewHeight;
        if(ratio < 0) ratio = 0;
        if(ratio > 1) ratio = 1;
        customizeScrollY = -ratio * maxScroll;
    }

    if (customizeScrollY > 0) customizeScrollY = 0;
    if (customizeScrollY < -maxScroll) customizeScrollY = -maxScroll;

    int startY = 120 + (int)customizeScrollY;
    int gapY = 28; int px = 50;

    BeginScissorMode(0, 100, GAME_W, 580);

    // P1
    DrawRectangle(px - 10, startY - 10, 550, 950, COL_UI_PANEL);
    DrawRectangleLines(px - 10, startY - 10, 550, 950, COL_P1_MAIN);
    DrawText("PLAYER 1", px, startY - 40, 30, COL_P1_MAIN);
    DrawText(TextFormat("PTS: %d", p1Points), px + 200, startY - 40, 30, (p1Points>0)?YELLOW:GRAY);
    int row = 0;
    DrawStatUI(px, startY + gapY*row++, "POWER", &p1.power, 0, 20, &p1Points, 1);
    DrawStatUI(px, startY + gapY*row++, "DEFENSE", &p1.defense, 0, 20, &p1Points, 1);
    DrawStatUI(px, startY + gapY*row++, "FLIGHT", &p1.flight, 0, 20, &p1Points, 1);
    DrawStatUI(px, startY + gapY*row++, "SLIP", &p1.slip, 0, 20, &p1Points, 1);
    DrawStatUI(px, startY + gapY*row++, "CHARGE", &p1.maxCharge, 0, MAX_CHARGE_LVL, &p1Points, CHARGE_COST);
    // DrawStatUI(px, startY + gapY*row++, "BOUNCE", &p1.bounceLimit, 0, 10, &p1Points, 1); // REMOVED
    DrawStatUI(px, startY + gapY*row++, "RECOIL", &p1.recoil, 0, 20, &p1Points, 1);
    DrawStatUI(px, startY + gapY*row++, "CRIT %", &p1.critRate, 0, 5, &p1Points, 1);
    DrawStatUI(px, startY + gapY*row++, "CRIT DMG", &p1.critPowerLvl, 0, 5, &p1Points, 3);
    DrawStatUI(px, startY + gapY*row++, "PURSUIT", &p1.pursuit, 0, 20, &p1Points, 1);
    DrawStatUI(px, startY + gapY*row++, "REFLECT", &p1.reflection, 0, 20, &p1Points, 1);
    // DrawStatUI(px, startY + gapY*row++, "ABSORB", &p1.absorbLvl, 0, 10, &p1Points, 1); // REMOVED
    DrawStatUI(px, startY + gapY*row++, "LONG BTL", &p1.longBattleLvl, 0, 10, &p1Points, 9);
    // ABSOLUTE DEFENSE
    DrawStatUI(px, startY + gapY*row++, "ABS. DEF", &p1.absoluteDefenseLvl, 0, ABS_DEFENSE_MAX_LVL, &p1Points, ABS_DEFENSE_COST);

    row++;
    DrawNegativeToggleUI(px, startY + gapY*row++, "WALL BOUNCE", &p1.bounceEnabled, &p1Points, 5);
    DrawToggleUI(px, startY + gapY*row++, "DEF CHARGE", &p1.defenseCharge, &p1Points, 5);
    DrawToggleUI(px, startY + gapY*row++, "ANTI-DESTRUCT", &p1.antiDestruct, &p1Points, 10);
    DrawToggleUI(px, startY + gapY*row++, "FULL THROT", &p1.fullThrottle, &p1Points, 5);
    DrawToggleUI(px, startY + gapY*row++, "HEAVY STANCE", &p1.heavyStance, &p1Points, 5);
    DrawToggleUI(px, startY + gapY*row++, "HOMING", &p1.homingSkill, &p1Points, 10);
    DrawToggleUI(px, startY + gapY*row++, "ZERO DIST", &p1.zeroDistSkill, &p1Points, 10); 
    DrawToggleUI(px, startY + gapY*row++, "U-TURN", &p1.uTurnSkill, &p1Points, 5); 
    DrawToggleUI(px, startY + gapY*row++, "CLIFF WARRIOR", &p1.cliffWarrior, &p1Points, 10); 
    DrawToggleUI(px, startY + gapY*row++, "SNIPER", &p1.sniperSkill, &p1Points, 10); 
    DrawToggleUI(px, startY + gapY*row++, "REPULSION", &p1.repulsionSkill, &p1Points, 5); // NEW
    DrawToggleUI(px, startY + gapY*row++, "INTIMIDATION", &p1.intimidationSkill, &p1Points, 10); // NEW

    // P2
    px = 650 ;
    DrawRectangle(px - 10, startY - 10, 550, 950, COL_UI_PANEL);
    DrawRectangleLines(px - 10, startY - 10, 550, 950, COL_P2_MAIN);
    DrawText("PLAYER 2", px, startY - 40, 30, COL_P2_MAIN);
    DrawText(TextFormat("PTS: %d", p2Points), px + 200, startY - 40, 30, (p2Points>0)?YELLOW:GRAY);
    row = 0;
    DrawStatUI(px, startY + gapY*row++, "POWER", &p2.power, 0, 20, &p2Points, 1);
    DrawStatUI(px, startY + gapY*row++, "DEFENSE", &p2.defense, 0, 20, &p2Points, 1);
    DrawStatUI(px, startY + gapY*row++, "FLIGHT", &p2.flight, 0, 20, &p2Points, 1);
    DrawStatUI(px, startY + gapY*row++, "SLIP", &p2.slip, 0, 20, &p2Points, 1);
    DrawStatUI(px, startY + gapY*row++, "CHARGE", &p2.maxCharge, 0, MAX_CHARGE_LVL, &p2Points, CHARGE_COST);
    // DrawStatUI(px, startY + gapY*row++, "BOUNCE", &p2.bounceLimit, 0, 10, &p2Points, 1); // REMOVED
    DrawStatUI(px, startY + gapY*row++, "RECOIL", &p2.recoil, 0, 20, &p2Points, 1);
    DrawStatUI(px, startY + gapY*row++, "CRIT %", &p2.critRate, 0, 5, &p2Points, 1);
    DrawStatUI(px, startY + gapY*row++, "CRIT DMG", &p2.critPowerLvl, 0, 5, &p2Points, 3);
    DrawStatUI(px, startY + gapY*row++, "PURSUIT", &p2.pursuit, 0, 20, &p2Points, 1);
    DrawStatUI(px, startY + gapY*row++, "REFLECT", &p2.reflection, 0, 20, &p2Points, 1);
    // DrawStatUI(px, startY + gapY*row++, "ABSORB", &p2.absorbLvl, 0, 10, &p2Points, 1); // REMOVED
    DrawStatUI(px, startY + gapY*row++, "LONG BTL", &p2.longBattleLvl, 0, 10, &p2Points, 9);
    // ABSOLUTE DEFENSE
    DrawStatUI(px, startY + gapY*row++, "ABS. DEF", &p2.absoluteDefenseLvl, 0, ABS_DEFENSE_MAX_LVL, &p2Points, ABS_DEFENSE_COST);

    row++;
    DrawNegativeToggleUI(px, startY + gapY*row++, "WALL BOUNCE", &p2.bounceEnabled, &p2Points, 5);
    DrawToggleUI(px, startY + gapY*row++, "DEF CHARGE", &p2.defenseCharge, &p2Points, 5);
    DrawToggleUI(px, startY + gapY*row++, "ANTI-DESTRUCT", &p2.antiDestruct, &p2Points, 10);
    // TRACE WALL REMOVED
    DrawToggleUI(px, startY + gapY*row++, "FULL THROT", &p2.fullThrottle, &p2Points, 5);
    DrawToggleUI(px, startY + gapY*row++, "HEAVY STANCE", &p2.heavyStance, &p2Points, 5);
    DrawToggleUI(px, startY + gapY*row++, "HOMING", &p2.homingSkill, &p2Points, 10);
    DrawToggleUI(px, startY + gapY*row++, "ZERO DIST", &p2.zeroDistSkill, &p2Points, 10); 
    DrawToggleUI(px, startY + gapY*row++, "U-TURN", &p2.uTurnSkill, &p2Points, 5); 
    DrawToggleUI(px, startY + gapY*row++, "CLIFF WARRIOR", &p2.cliffWarrior, &p2Points, 10); 
    DrawToggleUI(px, startY + gapY*row++, "SNIPER", &p2.sniperSkill, &p2Points, 10); 
    DrawToggleUI(px, startY + gapY*row++, "REPULSION", &p2.repulsionSkill, &p2Points, 5); // NEW
    DrawToggleUI(px, startY + gapY*row++, "INTIMIDATION", &p2.intimidationSkill, &p2Points, 10); // NEW

    EndScissorMode();

    // DRAW SCROLL BAR (Overlay)
    DrawRectangleRec(trackRect, Fade(BLACK, 0.6f));
    float thumbH = viewHeight * (viewHeight / (float)(viewHeight + maxScroll));
    if(thumbH < 30) thumbH = 30;

    float scrollRatio = -customizeScrollY / (float)maxScroll;
    float thumbY = scrollBarY + scrollRatio * (viewHeight - thumbH);

    Rectangle thumbRect = { scrollBarX + 1, thumbY, scrollBarW - 2, thumbH };
    DrawRectangleRec(thumbRect, isScrollBarDragging ? WHITE : Fade(WHITE, 0.5f));

    Rectangle btnStart = { GAME_W/2 - 150, 680, 300, 35 };
    bool hover = CheckCollisionPointRec(GetVirtualMousePos(), btnStart);
    DrawRectangleRec(btnStart, hover ? WHITE : ORANGE);
    DrawRectangleLinesEx(btnStart, 3, BLACK);
    DrawText("BATTLE START", btnStart.x + 40, btnStart.y + 5, 30, BLACK);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
        // Recalculate physics based on the stats set in UI
        RecalculateEraserPhysics(&p1);
        RecalculateEraserPhysics(&p2);
        
        p1Wins = 0; p2Wins = 0; currentRound = 1; roundStarter = 1;
        
        ResetBattleState();
        currentScene = SCENE_BATTLE;
    }
}

void DrawGameScene(void) {
    DrawRectangleRec(DESK_RECT, (Color){ 40, 80, 50, 255 });
    DrawRectangleLinesEx(DESK_RECT, 10.0f, BROWN);

    DrawFlatObstacles();

    DrawFlatEraser(&p1); DrawFlatEraser(&p2);
    DrawSkillWaves();
    DrawSlashes();

    if(isWarpSelectMode) {
        Eraser* c = (warpActivePlayerId==1)?&p1:&p2;
        DrawWarpTarget(c);
    }

    BeginBlendMode(BLEND_ADDITIVE);
        for(int i=0; i<MAX_PARTICLES; i++) if(particles[i].active && particles[i].additive) DrawCircleV(particles[i].pos, particles[i].size * particles[i].life, Fade(particles[i].color, particles[i].life));
        for(int i=0; i<MAX_SHOCKWAVES; i++) {
            if(shockwaves[i].active) {
                if(shockwaves[i].isHexagon) {
                    DrawPolyLines(shockwaves[i].pos, 6, shockwaves[i].radius, 0, Fade(shockwaves[i].color, shockwaves[i].life));
                    DrawPoly(shockwaves[i].pos, 6, shockwaves[i].radius, 0, Fade(shockwaves[i].color, shockwaves[i].life * 0.2f));
                } else {
                    DrawCircleLines((int)shockwaves[i].pos.x, (int)shockwaves[i].pos.y, shockwaves[i].radius, Fade(shockwaves[i].color, shockwaves[i].life));
                }
            }
        }
    EndBlendMode();

    for(int i=0; i<MAX_PARTICLES; i++) if(particles[i].active && !particles[i].additive) DrawCircleV(particles[i].pos, particles[i].size * particles[i].life, Fade(particles[i].color, particles[i].life));
    for(int i=0; i<MAX_TEXT_FX; i++) if(textEffects[i].active) DrawText(textEffects[i].text, textEffects[i].pos.x, textEffects[i].pos.y, textEffects[i].size, Fade(textEffects[i].color, textEffects[i].life));

    if (isDragging && !isWarpSelectMode) {
        DrawLineEx(dragStart, dragEnd, 4.0f, WHITE); DrawCircleV(dragStart, 5.0f, WHITE);
        Vector2 mid = Vector2MidPoint(dragStart, dragEnd);
        Eraser* c = (turnPlayer == 1) ? &p1 : &p2;
        Eraser* target = (turnPlayer == 1) ? &p2 : &p1;

        int effectiveMaxCharge = 10 + c->maxCharge;
        if(effectiveMaxCharge > 10 + MAX_CHARGE_LVL) effectiveMaxCharge = 10 + MAX_CHARGE_LVL;

        // Full throttle visual override
        if (c->fullThrottle) effectiveMaxCharge = effectiveMaxCharge; // Keep max

        float m = 1.0f + (c->pursuitStack * c->pursuit * 0.1f); if(m>PURSUIT_CAP) m=PURSUIT_CAP;

        if (c->fullThrottle) m *= 1.3f; 
        if (c->heavyStance) m *= 3.0f; 
        if (target->intimidationSkill) m *= 0.65f; // Preview intimidation
        
        float dist = Vector2Distance(p1.pos, p2.pos) - (p1.radius + p2.radius);
        
        // ZERO DISTANCE INDICATOR
        if (c->zeroDistSkill && dist < ZERO_DIST_THRESHOLD) {
             m *= 1.3f; 
             DrawText("ZERO DIST!!", mid.x - 40, mid.y + 20, 10, COL_ZERO);
        }

        bool homingReady = false;
        if (c->homingSkill) {
            float centerDist = Vector2Distance(p1.pos, p2.pos);
            if(centerDist >= HOMING_MIN_DIST && centerDist <= HOMING_MAX_DIST) {
                m *= 1.5f;
                homingReady = true;
            }
            const char* distInfo = TextFormat("DIST: %.0f [%s]", centerDist, homingReady ? "OK" : "NG");
            DrawText(distInfo, mid.x - MeasureText(distInfo, 10)/2, mid.y + 40, 10, homingReady ? GREEN : RED);
        }
        
        // U-TURN DEBUFF INDICATOR (If USER has debuff)
        if (c->uTurnDebuffTimer > 0) {
            DrawText("FLIGHT 0 (WEAKENED)", mid.x - 40, mid.y + 60, 10, GRAY);
        }

        const char* dmgTxt = TextFormat("POW: %d (x%.1f)", rouletteValue, m);
        DrawText(dmgTxt, mid.x - MeasureText(dmgTxt, 20)/2, mid.y - 40, 20, YELLOW);
    }
    
    // ROUND UI
    const char* roundTxt = TextFormat("ROUND %d", currentRound);
    DrawText(roundTxt, GAME_W/2 - MeasureText(roundTxt, 40)/2, 20, 40, WHITE);
    
    // Win Stars
    for(int i=0; i<2; i++) {
        Color c = (i < p1Wins) ? GOLD : GRAY;
        DrawPoly((Vector2){ 450 + i*40, 40 }, 5, 10, 0, c);
    }
    for(int i=0; i<2; i++) {
        Color c = (i < p2Wins) ? GOLD : GRAY;
        DrawPoly((Vector2){ 830 - i*40, 40 }, 5, 10, 0, c);
    }

    // BACK BUTTON
    Rectangle btnBack = { GAME_W - 140, 20, 120, 40 };
    bool hBack = CheckCollisionPointRec(GetVirtualMousePos(), btnBack);
    DrawRectangleRec(btnBack, hBack ? WHITE : GRAY);
    DrawRectangleLinesEx(btnBack, 2, BLACK);
    DrawText("BACK TO EDIT", btnBack.x + 10, btnBack.y + 10, 10, BLACK);
    
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hBack) {
        currentScene = SCENE_CUSTOMIZE;
    }
}

// =========================================================================================
//  7. MAIN LOOP
// =========================================================================================

int main() {
    InitGameSystem();

    while (!WindowShouldClose()) {
        frameCounter++;

        float screenW = (float)GetScreenWidth();
        float screenH = (float)GetScreenHeight();
        scale = fminf(screenW / GAME_W, screenH / GAME_H);
        offset.x = (screenW - (GAME_W * scale)) * 0.5f;
        offset.y = (screenH - (GAME_H * scale)) * 0.5f;

        if (hitStopFrames > 0) {
            hitStopFrames--;
        } else {
            if (shakeIntensity > 0) { shakeIntensity *= 0.9f; if (shakeIntensity < 0.5f) shakeIntensity = 0; }
            zoomTarget += (1.0f - zoomTarget) * 0.1f;

            if (isnan(zoomTarget) || zoomTarget <= 0.0f) zoomTarget = 1.0f;
            if (isnan(shakeIntensity)) shakeIntensity = 0.0f;

            gameCamera.offset = (Vector2){ GAME_W/2.0f, GAME_H/2.0f };
            gameCamera.target = (Vector2){ GAME_W/2.0f, GAME_H/2.0f };
            gameCamera.zoom = zoomTarget + (shakeIntensity * 0.002f);

            Vector2 shakeOffset = { FloatRand(-shakeIntensity, shakeIntensity), FloatRand(-shakeIntensity, shakeIntensity) };
            gameCamera.target = Vector2Add(gameCamera.target, shakeOffset);

            if (messageTimer > 0) messageTimer--;

            switch (currentScene) {
                case SCENE_TITLE: if (IsKeyPressed(KEY_ENTER)) currentScene = SCENE_CUSTOMIZE; break;
                case SCENE_CUSTOMIZE: break;
                case SCENE_BATTLE: UpdatePhysics(); UpdateInput(); break;
                case SCENE_RESULT: if (IsKeyPressed(KEY_R)) currentScene = SCENE_CUSTOMIZE; break;
            }
            UpdateVisuals();
        }

        BeginDrawing();

            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), COL_BG_DARK);

            Camera2D screenCam = { 0 };
            screenCam.zoom = scale;
            screenCam.offset = offset;

            BeginMode2D(screenCam);
                if (currentScene == SCENE_BATTLE) {
                    BeginMode2D(gameCamera);
                    DrawGameScene();
                    EndMode2D();

                    if(isWarpSelectMode) {
                        Eraser* c = (warpActivePlayerId==1)?&p1:&p2;
                        DrawText("SELECT WARP DESTINATION", GAME_W/2 - 150, 50, 30, MAGENTA);
                        DrawText(c->name, GAME_W/2 - 50, 90, 20, c->bodyColor);
                    } else {
                        Eraser* c = (turnPlayer == 1) ? &p1 : &p2;
                        DrawText(TextFormat("TURN: P%d", turnPlayer), 30, 30, 40, (turnPlayer==1)?SKYBLUE:RED);

                        DrawText(TextFormat("P1 A.Def: %d", p1.absoluteDefenseCount), 30, 80, 20, (p1.absoluteDefenseCount>0)?COL_ABS_DEF:GRAY);
                        DrawText(TextFormat("P2 A.Def: %d", p2.absoluteDefenseCount), GAME_W-150, 80, 20, (p2.absoluteDefenseCount>0)?COL_ABS_DEF:GRAY);
                    }
                    if (messageTimer > 0) {
                        int w = MeasureText(messageBuf, 60);
                        DrawText(messageBuf, GAME_W/2 - w/2, GAME_H/2 - 30, 60, ORANGE);
                    }
                }
                else if (currentScene == SCENE_TITLE) {
                    DrawRectangleGradientV(0, 0, GAME_W, GAME_H, (Color){10,10,30,255}, BLACK);
                    DrawText("KESHIPIN", GAME_W/2 - 250, 200, 100, SKYBLUE);
                    DrawText("SIMPLE EDITION", GAME_W/2 - 300, 300, 60, GOLD);
                    DrawText("PRESS [ENTER] TO CUSTOMIZE", 450, 500, 30, LIGHTGRAY);
                }
                else if (currentScene == SCENE_CUSTOMIZE) {
                    DrawSceneCustomize();
                }
                else if (currentScene == SCENE_RESULT) {
                    DrawGameScene();
                    DrawRectangle(0, 0, GAME_W, GAME_H, Fade(BLACK, 0.7f));
                    const char* winTxt = (winnerID==1) ? "PLAYER 1 WINS MATCH!" : "PLAYER 2 WINS MATCH!";
                    Color winCol = (winnerID==1) ? SKYBLUE : RED;
                    int w = MeasureText(winTxt, 60);
                    DrawText(winTxt, GAME_W/2 - w/2, 300, 60, winCol);
                    
                    const char* scoreTxt = TextFormat("%d - %d", p1Wins, p2Wins);
                    DrawText(scoreTxt, GAME_W/2 - MeasureText(scoreTxt, 80)/2, 400, 80, WHITE);

                    DrawText("PRESS [R] TO CUSTOMIZE", GAME_W/2 - 150, 500, 30, WHITE);
                }
            EndMode2D();

            if (offset.x > 0) {
                DrawRectangle(0, 0, (int)offset.x, GetScreenHeight(), BLACK);
                DrawRectangle(GetScreenWidth() - (int)offset.x, 0, (int)offset.x, GetScreenHeight(), BLACK);
            }
            if (offset.y > 0) {
                DrawRectangle(0, 0, GetScreenWidth(), (int)offset.y, BLACK);
                DrawRectangle(0, GetScreenHeight() - (int)offset.y, GetScreenWidth(), (int)offset.y, BLACK);
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}