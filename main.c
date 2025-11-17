#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

typedef enum {
    STATE_MENU,
    STATE_GAME,
    STATE_GAMEOVER
} GameState;

typedef struct {
    Rectangle rect;
    Vector2 velocity;
    bool active;
} Projectiles;

typedef struct {
    Rectangle rect;
    Vector2 velocity;
    int lives;
    float shootCooldown;
    float shootTimer;
} Player;

typedef struct {
    Rectangle rect;
    Vector2 velocity;
    int lives;
    float shootCooldown;
    float shootTimer;
    bool active;
    float moveTimer;
} Mobs;

typedef struct {
    Rectangle rect;
    Vector2 velocity;
    int lives;
    float shootCooldown;
    int phase;
    bool active;
    float shootTimer;
    int patternIndex;
    float moveTimer;
} Boss;

typedef enum {
    PICKUP_HEALTH, 
    PICKUP_SHOT 
} PickupType;

typedef struct {
    Rectangle rect;
    bool active;
    PickupType type;
} Pickup;

#define MAX_BULLETS 300
#define MAX_MOBS 5
#define MAX_PICKUPS 4

void ResetGame(Player *player, Boss *boss, Projectiles bossBullets[], Projectiles playerBullets[],
               float screenWidth, float screenHeight, Rectangle frameRecBoss, float scale, 
               int *bombCounter, Pickup pickups[], float *playerShootTimer, float *weaponUpgradeTimer, bool *weaponUpgraded){

                // Reset player
                player->lives = 3;
                player->rect.x = 100;
                player->rect.y = screenHeight - 50;
                player->shootCooldown = 0.3f;   // restore default
                *playerShootTimer = 0.0f;

                // Reset boss
                boss->lives = 100;
                boss->rect.x = screenWidth / 2 - (frameRecBoss.width * scale) / 2;
                boss->rect.y = 10;
                boss->phase = 0;
                boss->shootTimer = 0;
                boss->patternIndex = 0;
                boss->moveTimer = 0.0f;
                boss->active = true;
                boss->shootCooldown = 1.0f;
                
                *bombCounter = 3;

                // Reset bullets
                for (int i = 0; i < MAX_BULLETS; i++) {
                    bossBullets[i].active = false;
                    playerBullets[i].active = false;
                }
                // Reset pickups
                for (int i = 0; i < MAX_PICKUPS; i++) {
                    pickups[i].active = false;
                }
                // Reset weapon upgrade
                *weaponUpgradeTimer = 0.0f;
                *weaponUpgraded = false;
}

void SpawnPickup(Pickup pickups[], float x, float y){
    for (int i = 0; i < MAX_PICKUPS; i++){
        if (!pickups[i].active){
            pickups[i].active = true;
            pickups[i].rect = (Rectangle){x, y, 16, 16};
            pickups[i].type = (GetRandomValue(0, 1) == 0) ? PICKUP_HEALTH : PICKUP_SHOT;
            break;
        }
    }
}

int main(void) {
    const float screenWidth = 800 * 1.2;
    const float screenHeight = 640 * 1.2;

    InitWindow(screenWidth, screenHeight, "Tohou");
    InitAudioDevice(); // Initialize audio system

    Rectangle floor = { 0, screenHeight - 5, screenWidth, 5 };

    Sound hitSound = LoadSound("hitsound.wav");
    Sound powerUp = LoadSound("powerup.wav");

    Texture2D playerSprite = LoadTexture("player.png");
    Texture2D bgSprite = LoadTexture("background.png");
    Texture2D bossSprite = LoadTexture("boss.png");
    Texture2D pickupSprite = LoadTexture("pickups.png");

    Rectangle frameRecPlayerMid = { 0, 0, 30, 50};
    Rectangle frameRecPlayerLeft = { 70, 50, 30, 50};
    Rectangle frameRecPlayerRight = { 70, 94, 30, 50};
    Rectangle currentPlayerFrame = frameRecPlayerMid;
    Rectangle frameRecBg = { 0, 0, screenWidth / 7 + 200, screenHeight / 5 + 102};
    Rectangle frameRecBoss = { 20, 20, 50, 80};
    Rectangle frameRecPickupsHealth = { 0, 0, 96 / 12 * 3, 96 /12 * 3 };
    Rectangle frameRecPickupsBullets = { 96 / 12 * 4 + 5, 96 / 12 * 5, 96 / 12 * 3, 96 / 12 * 3};

    Projectiles bossBullets[MAX_BULLETS];
    //Prepares the bullets which are all inactive
    for (int i = 0; i < MAX_BULLETS; i++) {
        bossBullets[i].rect = (Rectangle){ 0, 0, 0, 0 };
        bossBullets[i].velocity = (Vector2){ 0, 0 };
        bossBullets[i].active = false;
    }
    Projectiles playerBullets[MAX_BULLETS];
    for (int i = 0; i < MAX_BULLETS; i++) {
        playerBullets[i].rect = (Rectangle){ 0, 0, 0, 0 };
        playerBullets[i].velocity = (Vector2){ 0, 0 };
        playerBullets[i].active = false;
    }

    Projectiles mobsBullets[MAX_BULLETS];
    for (int i = 0; i < MAX_BULLETS; i++) {
        mobsBullets[i].rect = (Rectangle){ 0, 0, 0, 0 };
        mobsBullets[i].velocity = (Vector2){ 0, 0 };
        mobsBullets[i].active = false;
    }

    GameState gameState = STATE_MENU;

    Player player;
    player.rect = (Rectangle){ 100, screenHeight - 50, 32, 32};
    player.velocity = (Vector2){ 0, 0 };
    player.lives = 3;
    player.rect.width = currentPlayerFrame.width; //make hitbox match sprite
    player.rect.height = currentPlayerFrame.height;
    player.shootCooldown = 0.3f;
    float playerShootTimer = 0.0f;

    SetTargetFPS(144);
    bool gameOver = false;
    bool gameWon = false;

    Boss boss;
    float scale = 3.0f; 
    float bossStartX = screenWidth / 2 - 32;
    boss.rect = (Rectangle){ bossStartX, 10, 64, 64 };
    boss.rect.y = 10;
    boss.rect.x = screenWidth / 2 - (frameRecBoss.width * scale) / 2;
    boss.rect.width = frameRecBoss.width * scale; // make hitbox match sprite
    boss.rect.height = frameRecBoss.height * scale;
    boss.velocity = (Vector2){ 0, 0 };
    boss.lives = 100;
    boss.shootCooldown = 1.0f;
    boss.active = true;
    boss.phase = 0;
    boss.shootTimer = 0.0f;
    boss.patternIndex = 0;
    boss.moveTimer = 0.0f;
    
    Mobs mobsArray[MAX_MOBS];
    for (int i = 0; i < MAX_MOBS; i++){
        mobsArray[i].rect = (Rectangle){0, 0, 15, 15};
        mobsArray[i].lives = 4;
        mobsArray[i].active = false;
        mobsArray[i].shootTimer = 0.0f;
        mobsArray[i].shootCooldown = 1.0f;
        mobsArray[i].velocity = (Vector2){0, 0};
        mobsArray[i].moveTimer = 0.0f;
    }
    
    //Boss Health Bar
    float healthBarWidth = screenWidth - 200;
    float healthBarHeight = 20;
    float healthBarX = screenWidth / 2 - healthBarWidth / 2;
    float healthBarY = 20;
    
    //Player Health Bar
    float playerHealthBarWidth = screenWidth / 3;
    float playerHealthBarHeight = 20;
    float playerHealthBarX = screenWidth / 2 - healthBarWidth / 2;
    float playerHealthBarY = screenHeight - 40 ;
    
    //PowerUp
    int bombCounter = 3;
    float bombFlashTime = 0.0f;
    Pickup pickups[MAX_PICKUPS];
    int pickupChance = 20;
    float weaponUpgradeTimer = 0.0f;
    bool weaponUpgraded = false;


    while (!WindowShouldClose()) {

        // --- Reset player velocity ---
        player.velocity.x = 0;
        player.velocity.y = 0;

        // -- Menu State --
        if (gameState == STATE_MENU){
            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("MY BULLET HELL GAME", screenWidth/2 - 200, screenHeight/2 - 100, 40, WHITE);
            DrawText("Press ENTER to Start", screenWidth/2 - 150, screenHeight/2, 20, GRAY);
            DrawText("Press ESC to Quit", screenWidth/2 - 140, screenHeight/2 + 40, 20, GRAY);
            
            if (IsKeyPressed(KEY_ENTER)) {
                gameState = STATE_GAME;
            }
            if (IsKeyPressed(KEY_ESCAPE)){
                CloseWindow();
                return 0;
            }

            EndDrawing();
            continue;
        }
        if (!gameOver && gameState == STATE_GAME) {
            //Player Movement
            if (IsKeyDown(KEY_A)) player.velocity.x -= 300;
            if (IsKeyDown(KEY_D)) player.velocity.x += 300;
            if (IsKeyDown(KEY_W)) player.velocity.y -= 300;
            if (IsKeyDown(KEY_S)) player.velocity.y += 300;
            if (IsKeyPressed(KEY_SPACE) && bombCounter > 0){
                bombCounter--;
                for (int i = 0; i < MAX_BULLETS; i++){
                    bossBullets[i].active = false;
                    mobsBullets[i].active = false;
                }
                bombFlashTime = 0.2f; 
            }
            //Bomb Flash
            float dt = GetFrameTime();
            if(bombFlashTime > 0){
                bombFlashTime -= dt;
            }

            // --- Move player ---
            player.rect.x += player.velocity.x * dt;
            player.rect.y += player.velocity.y * dt;

            // --- Boss shooting ---
            if (boss.active) {
                // --- Boss movement ---
                boss.moveTimer += dt;
                float amplitudeX = 200; // how far left/right the boss moves
                float amplitudeY = 50;
                float speedX = 1.6f; 
                float speedY = 0.5f;
                boss.rect.x = bossStartX + sinf(boss.moveTimer * speedX) * amplitudeX;
                boss.rect.y = cosf(boss.moveTimer * speedY) * amplitudeY;

                if (boss.lives <= 50 && boss.phase == 0){
                    boss.phase = 1;
                    boss.shootCooldown = 0.25f;
                }
                if (boss.lives <= 25 && boss.phase == 1){
                    boss.phase = 2;
                    boss.shootCooldown = 0.2f;
                }
                boss.shootTimer += dt;

                if (boss.shootTimer >= boss.shootCooldown) {
                    boss.shootTimer = 0.0f;
                    if (boss.phase != 2){
                        int numBullets = 30;
                        float angleStep = (2 * PI) / numBullets;
                        float randomOffset = GetRandomValue(0, 360) * DEG2RAD;

                        for (int j = 0; j < numBullets; j++) {
                            for (int i = 0; i < MAX_BULLETS; i++) {
                                if (!bossBullets[i].active) {
                                    bossBullets[i].active = true;
                                    bossBullets[i].rect = (Rectangle){
                                        boss.rect.x + boss.rect.width / 2 - 4,
                                        boss.rect.y + boss.rect.height / 2 - 4,
                                        8, 8
                                    };
                                    float angle = j * angleStep + randomOffset;
                                    bossBullets[i].velocity = (Vector2){
                                        cosf(angle) * 300,
                                        sinf(angle) * 300
                                    };
                                    break; // only spawn one bullet per slot
                                }
                            }
                        }
                    }
                    if (boss.phase == 2){
                        int numBullets = 10;
                        for(int j = 0; j < numBullets; j++){
                            for (int i = 0; i < MAX_BULLETS; i++){
                                if (!bossBullets[i].active){
                                    bossBullets[i].active = true;
                                    float xPos = GetRandomValue(0, (int)(screenWidth - 8));
                                    bossBullets[i].rect = (Rectangle){xPos, 0, 8, 8};
                                    bossBullets[i].velocity = (Vector2){0, 200};
                                    break;
                                }
                            }
                        }
                    }
                }
                if (boss.phase == 1){
                    for (int i = 0; i < MAX_MOBS; i++){
                        if(!mobsArray[i].active){
                            mobsArray[i].active = true;
                            mobsArray[i].rect.x = boss.rect.x + i * 50 - 100;
                            mobsArray[i].rect.y = 10;
                            mobsArray[i].moveTimer = 0.0f;
                            break;
                        }
                    }
                }
                // --- Update Mobs ---
                for (int i = 0; i < MAX_MOBS; i++){
                    if (mobsArray[i].active){
                        // Move the mob (simple downward movement)
                        mobsArray[i].rect.y += 30 * GetFrameTime();
                        //horizontal oscillation
                        mobsArray[i].rect.x += sinf(mobsArray[i].moveTimer * 2.0f) * 50 * GetFrameTime();
                        mobsArray[i].moveTimer += GetFrameTime();

                        //spiral shooting bullets
                        mobsArray[i].shootTimer += GetFrameTime();
                        if (mobsArray[i].shootTimer > mobsArray[i].shootCooldown){
                            mobsArray[i].shootTimer = 0.0f;

                            static float mobAngles[MAX_MOBS] = {0};
                            float angle = mobAngles[i];
                            int numBullets = 5;
                            float angleStep = (2 * PI) / numBullets;

                            for (int j = 0; j < MAX_BULLETS; j++){
                                if (!mobsBullets[j].active){
                                    mobsBullets[j].active = true;
                                    mobsBullets[j].rect = (Rectangle){
                                        mobsArray[i].rect.x + mobsArray[i].rect.width/2 - 4,
                                        mobsArray[i].rect.y + mobsArray[i].rect.height,
                                        8, 8
                                    };
                                    // Set velocity using angle
                                    mobsBullets[j].velocity = (Vector2){cosf(angle) * 200, sinf(angle) * 200};
                                    angle += angleStep; // next bullet in the spiral
                                    if (j >= numBullets - 1){ // spawn only numBullets bullets
                                        break;
                                    }
                                }
                                mobAngles[i] += 0.2f; // rotate spiral over time
                            }
                        }
                        if (mobsArray[i].rect.y > screenHeight){
                            mobsArray[i].active = false;
                        } 
                    }
                }
            }

            // --- Update boss bullets ---
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bossBullets[i].active) {
                    bossBullets[i].rect.x += bossBullets[i].velocity.x * GetFrameTime();
                    bossBullets[i].rect.y += bossBullets[i].velocity.y * GetFrameTime();

                    // deactivate if off-screen
                    if (bossBullets[i].rect.y + bossBullets[i].rect.height < 0 ||
                        bossBullets[i].rect.y > screenHeight ||
                        bossBullets[i].rect.x + bossBullets[i].rect.width < 0 ||
                        bossBullets[i].rect.x > screenWidth) {
                        bossBullets[i].active = false;
                    }
                }
            }

            // --- Player shooting ---
            playerShootTimer += GetFrameTime();
            if (player.lives > 0 && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (playerShootTimer >= player.shootCooldown) {
                    playerShootTimer = 0.0f;
                    int numBullets = 0;
                    for (int i = 0; i < MAX_BULLETS && numBullets < 2; i++) {
                        if (!playerBullets[i].active) {
                            playerBullets[i].active = true;

                            float gap = 6.0f;
                            float xOffset = (numBullets == 0) ? -gap : gap;
                            playerBullets[i].rect = (Rectangle){
                                player.rect.x + player.rect.width / 2 - 4 + xOffset,
                                player.rect.y - 8, // spawn above the player
                                8, 8
                            };
                            playerBullets[i].velocity = (Vector2){0, -400};
                            numBullets++;; // only one bullet per cooldown
                        }
                    }
                }
            }

            // --- Update player bullets ---
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (playerBullets[i].active) {
                    playerBullets[i].rect.x += playerBullets[i].velocity.x * GetFrameTime();
                    playerBullets[i].rect.y += playerBullets[i].velocity.y * GetFrameTime();

                    // deactivate if off-screen
                    if (playerBullets[i].rect.y + playerBullets[i].rect.height < 0) {
                        playerBullets[i].active = false;
                    }
                }
            }
            // --- Update Mobs bullets ---
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (mobsBullets[i].active) {
                    mobsBullets[i].rect.x += mobsBullets[i].velocity.x * GetFrameTime();
                    mobsBullets[i].rect.y += mobsBullets[i].velocity.y * GetFrameTime();

                    // deactivate if off-screen
                    if (mobsBullets[i].rect.y + mobsBullets[i].rect.height < 0 ||
                        mobsBullets[i].rect.y > screenHeight ||
                        mobsBullets[i].rect.x + mobsBullets[i].rect.width < 0 ||
                        mobsBullets[i].rect.x > screenWidth) {
                        mobsBullets[i].active = false;
                    }
                }
            }

            //Update Pickups
            for (int i = 0; i < MAX_PICKUPS; i++){
                if (pickups[i].active){
                    pickups[i].rect.y += 100 * GetFrameTime();
                    if (CheckCollisionRecs(pickups[i].rect, player.rect)){
                        pickups[i].active = false;
                        PlaySound(powerUp);
                        SetSoundVolume(powerUp, 0.5f);
                        if (pickups[i].type == PICKUP_HEALTH){
                            if (player.lives < 3){
                                player.lives++;
                            }
                        }
                        else if(pickups[i].type == PICKUP_SHOT){
                            if (!weaponUpgraded){
                                player.shootCooldown *= 0.5f;
                                weaponUpgraded = true;
                                weaponUpgradeTimer = 5.0f;
                            }
                        }
                    }
                    if (pickups[i].rect.y > screenHeight){
                        pickups[i].active = false;
                    }
                }
            }

            if (weaponUpgraded){
                weaponUpgradeTimer -= GetFrameTime();
                if (weaponUpgradeTimer <= 0){
                    player.shootCooldown /= 0.5f;
                    weaponUpgraded = false;
                }
            }
            //Collision
            for (int i = 0; i < MAX_BULLETS; i++){
                if (playerBullets[i].active){
                    if (CheckCollisionRecs(playerBullets[i].rect, boss.rect)){
                        boss.lives -= 1;
                        playerBullets[i].active = false;

                        if (GetRandomValue(0, 100) < pickupChance){
                            SpawnPickup(pickups, boss.rect.x + boss.rect.width / 2, boss.rect.y + boss.rect.height/2);
                        }
                        if (boss.lives <= 0){
                            gameWon = true;
                            gameOver = true;
                            gameState = STATE_GAMEOVER;
                        }
                    }
                    for (int j = 0; j < MAX_MOBS; j++){
                        if (mobsArray[j].active && CheckCollisionRecs(playerBullets[i].rect, mobsArray[j].rect)){
                            mobsArray[j].lives -= 1;
                            playerBullets[i].active = false;

                            if (GetRandomValue(0, 100) < pickupChance){
                                 SpawnPickup(pickups, mobsArray[j].rect.x + mobsArray[j].rect.width / 2, mobsArray[j].rect.y + mobsArray[j].rect.height / 2);
                            }
                            if (mobsArray[j].lives <= 0){
                                mobsArray[j].active = false;
                            }
                        }
                    }
                    
                }
            }
            for (int i = 0; i < MAX_BULLETS; i++){
                if (bossBullets[i].active){
                    if (CheckCollisionRecs(bossBullets[i].rect, player.rect)){
                        player.lives -= 1;
                        PlaySound(hitSound);
                        bossBullets[i].active = false;
                        if(player.lives <= 0){
                            gameOver = true;
                            gameState = STATE_GAMEOVER;
                            break;
                        }
                    }
                }
            }
            for (int i = 0; i < MAX_BULLETS; i++){
                if (mobsBullets[i].active){
                    if (CheckCollisionRecs(mobsBullets[i].rect, player.rect)){
                        player.lives -= 1;
                        PlaySound(hitSound);
                        mobsBullets[i].active = false;
                        if(player.lives <= 0){
                            gameOver = true;
                            gameState = STATE_GAMEOVER;
                            break;
                        }
                    }
                }
            }
        } 
        //Switch sprite 
        if (IsKeyDown(KEY_A)){
            currentPlayerFrame = frameRecPlayerLeft;
        }
        else if (IsKeyDown(KEY_D)){
            currentPlayerFrame = frameRecPlayerRight;
        }
        else {
            currentPlayerFrame = frameRecPlayerMid;
        }

        // --- Drawing ---
        BeginDrawing();
        if (gameState == STATE_GAME){
            // Draw background
            DrawTexturePro(
                bgSprite,
                frameRecBg,
                (Rectangle){0, 0, screenWidth, screenHeight},
                (Vector2){0, 0},
                0.0f,
                WHITE
            );
            
            DrawTexturePro(
                bossSprite,
                frameRecBoss,
                (Rectangle){boss.rect.x, boss.rect.y, frameRecBoss.width * scale, frameRecBoss.height * scale},
                (Vector2){0, 0},
                0.0f,
                WHITE
            );
            
            DrawTexturePro(
                playerSprite,
                currentPlayerFrame,
                (Rectangle){ player.rect.x, player.rect.y, currentPlayerFrame.width, currentPlayerFrame.height },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE
            );
            for(int i = 0; i < MAX_MOBS; i++){
                if (mobsArray[i].active){
                    DrawRectangleRec(mobsArray[i].rect, ORANGE);
                }
            }
            DrawRectangleRec(floor, BLACK);
            //DrawRectangleRec(player.rect, BLUE); //player hitbox
            //DrawRectangleRec(boss.rect, RED); //boss hitbox

            float healthPercentage = (float)boss.lives/100.0f;
            DrawRectangle(healthBarX, healthBarY, healthBarWidth * healthPercentage, healthBarHeight, RED);
            DrawRectangleLines(healthBarX, healthBarY, healthBarWidth, healthBarHeight, BLACK);

            float playerHealthPercentage = (float)player.lives/3.0f;
            float lifeWidth = playerHealthBarWidth / 3.0f;
            DrawRectangle(playerHealthBarX, playerHealthBarY, playerHealthBarWidth * playerHealthPercentage, playerHealthBarHeight, GREEN);
            for (int i = 1; i < 3; i++){
                float lineX = playerHealthBarX + i * lifeWidth;
                DrawLine(lineX, playerHealthBarY, lineX, playerHealthBarY + playerHealthBarHeight, BLACK);
            }
            DrawRectangleLines(playerHealthBarX, playerHealthBarY, playerHealthBarWidth, playerHealthBarHeight, BLACK);
            

            if (gameOver && !gameWon){
                DrawText("GAME OVER", screenWidth / 2 - 150, screenHeight/2 -50, 40, RED);
                DrawText("Press R to Retry", screenWidth / 2 - 150, screenHeight/2 +10, 20, BLACK);
            }
            if (gameWon){
                DrawText("YOU WON", screenWidth / 2 - 150, screenHeight/2 -50, 40, GREEN);
            }

            //Flash
            if (bombFlashTime > 0){
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(WHITE, 0.5f));
            }
            //Pickups
            for (int i = 0; i < MAX_PICKUPS; i++){
                if (pickups[i].active){
                    if (pickups[i].type == PICKUP_HEALTH){
                        DrawTexturePro(
                            pickupSprite,
                            frameRecPickupsHealth,
                            (Rectangle){ pickups[i].rect.x, pickups[i].rect.y, frameRecPickupsHealth.width, frameRecPickupsHealth.height },
                            (Vector2){ 0, 0 },
                            0.0f,
                            WHITE
                        );
                    }
                    else if (pickups[i].type == PICKUP_SHOT){
                        DrawTexturePro(
                            pickupSprite,
                            frameRecPickupsBullets,
                            (Rectangle){ pickups[i].rect.x, pickups[i].rect.y, frameRecPickupsBullets.width, frameRecPickupsBullets.height },
                            (Vector2){ 0, 0 },
                            0.0f,
                            WHITE
                        );
                    }
                }
            }

            // Draw bullets
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bossBullets[i].active) DrawRectangleRec(bossBullets[i].rect, MAROON);
                if (playerBullets[i].active) DrawRectangleRec(playerBullets[i].rect, GREEN);
                if (mobsBullets[i].active) DrawRectangleRec(mobsBullets[i].rect, YELLOW);
            }
        }
        else if (gameState == STATE_GAMEOVER){
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("GAME OVER", screenWidth / 2 - 150, screenHeight/2 -50, 40, RED);
            DrawText("Press R to Retry", screenWidth / 2 - 150, screenHeight/2 +10, 20, WHITE);
            DrawText("Press ESC for Menu", screenWidth / 2 - 130, screenHeight/2 +50, 20, WHITE);

            if (IsKeyPressed(KEY_R)){
                ResetGame(&player, &boss, bossBullets, playerBullets,
                screenWidth, screenHeight, frameRecBoss, scale, &bombCounter, pickups,
                &playerShootTimer, &weaponUpgradeTimer, &weaponUpgraded);
                gameOver = false;
                gameWon = false;
                gameState = STATE_GAME;

                for (int i = 0; i < MAX_MOBS; i++){
                    mobsArray[i].active = false;
                    mobsArray[i].rect.x = 0;
                    mobsArray[i].rect.y = 0;
                    mobsArray[i].moveTimer = 0.0f;
                    mobsArray[i].shootTimer = 0.0f;
                }
                for (int i = 0; i < MAX_BULLETS; i++){
                    mobsBullets[i].active = false;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)){
                gameState = STATE_MENU;
            }
            EndDrawing();
            continue; // skip normal game drawing
        }
        EndDrawing();
    }

    CloseAudioDevice(); // Close audio system
    CloseWindow();
    return 0;
}