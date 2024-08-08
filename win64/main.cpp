#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cmath>


// A component is a data structure that stores information about an entity.
struct PositionComponent {
    float x;
    float y;
};

struct VelocityComponent {
    float x;
    float y;
};

struct RotationComponent {
    float angle;
};

struct InputComponent {
    bool up;
    bool down;
    bool left;
    bool right;
    bool spacebar;
    bool shoot;
    bool restart;
    bool quit;
};

struct RenderComponent {
    SDL_Texture* texture;
    SDL_Rect spriteRect;
};

struct SoundComponent {
    Mix_Chunk* sfx_shoot;
    Mix_Chunk* sfx_hit;
    Mix_Chunk* sfx_explosion;
};

struct AIComponent {
    PositionComponent* playerPosition;
    float chaseRange; // The range at which the enemy stops chasing the player
    float attackRange;

    float attackCooldown;
    float attackCooldownDuration;
    float attack_time;
    float attack_duration;
    float shoot_cooldown;
    float shoot_cooldown_duration;
};

struct ProjectileComponent {
    bool active;
    float x;
    float y;
    float velocityX;
    float velocityY;
    int damage;
};

struct HealthComponent {
    int current;
    int maxHealth;
};

struct UIComponent {
    SDL_Rect healthBarBG;
    SDL_Rect healthBar;
    HealthComponent* health;
};

struct MenuComponent {
    SDL_Rect* title_rect;
    SDL_Texture* title_texture;
    SDL_Rect* game_over_rect;
    SDL_Texture* game_over_texture;
    SDL_Rect* restart_rect;
    SDL_Texture* restart_texture;
};


struct MovementSystem {
    std::unordered_map<uint32_t, PositionComponent>* positions;
    std::unordered_map<uint32_t, VelocityComponent>* velocities;
    std::unordered_map<uint32_t, InputComponent>* inputs;
    std::unordered_map<uint32_t, RotationComponent>* rotations;

    void update(float deltaTime) {
      // Iterate over all entities with a position, velocity, and input component
      for (auto& [entity, position] : *positions) {
        if (!velocities->count(entity) || !inputs->count(entity)) {
          continue;
        }

        // Update the velocity based on the input
        auto& velocity = (*velocities)[entity];
        auto& rotation = (*rotations)[entity];
        auto& input = (*inputs)[entity];

        rotation.angle += (input.right - input.left) * 175.0f * deltaTime;

        float direction_x = cos(rotation.angle * (M_PI / 180));
        float direction_y = sin(rotation.angle * (M_PI / 180));

        velocity.x = 0.0f;
        velocity.y = 0.0f;
        if (input.up) {
          velocity.x = 350.0f * direction_x;
          velocity.y = 350.0f * direction_y;
        }
        if (input.down) {
          velocity.x = -350.0f * direction_x;
          velocity.y = -350.0f * direction_y;
        }

        // Update the position based on the velocity
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
      }
    }
};

struct HealthSystem {
    std::unordered_map<uint32_t, HealthComponent>* healths;
    std::unordered_map<uint32_t, RenderComponent>* renders;
    std::unordered_map<uint32_t, PositionComponent>* positions;
    std::unordered_map<uint32_t, SoundComponent>* sfx;
    std::unordered_map<uint32_t, std::vector<ProjectileComponent>>* projectiles;

    void update(float deltaTime) {
      // Iterate over all entities with a health component
      for (auto& [entity, health] : *healths) {
        // Get the position of the entity
        auto render = (*renders)[entity];
        auto position = (*positions)[entity];

        // Iterate over all projectiles
        for (auto& [entity2, projectile_vector] : *projectiles) {
          if (entity == entity2)
            continue;

          for (auto& projectile : projectile_vector) {
            if (!projectile.active)
              continue;

            // Check if the projectile collides with the entity
            if (collides(projectile, position, render)) {
              projectile.active = false;

              // Reduce the health of the entity
              health.current -= projectile.damage;

              auto& sound = (*sfx)[entity];
              Mix_PlayChannel(-1, sound.sfx_hit, 0);

              if (health.current <= 0)
              {
                health.current = 0;

                Mix_PlayChannel(-1, sound.sfx_explosion, 0);
              }
            }
          }
        }
      }
    }

    bool collides(const ProjectileComponent& projectile, const PositionComponent& position, const RenderComponent& render) {
      // Check if the projectile position is inside the sprite rectangle
      return projectile.x > position.x && projectile.x < position.x + render.spriteRect.w &&
             projectile.y > position.y && projectile.y < position.y + render.spriteRect.h;
    }
};

struct InputSystem {
    std::unordered_map<uint32_t, InputComponent>* inputs;

    void handleEvent(const SDL_Event& event) {
      if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        // Update the input component for the entity
        uint32_t entity = 0;
        if (!inputs->count(entity)) {
          (*inputs)[entity] = {false, false, false, false, false, false, false};
        }
        switch (event.key.keysym.sym) {
          case SDLK_UP:
            (*inputs)[entity].up = event.type == SDL_KEYDOWN;
            break;
          case SDLK_DOWN:
            (*inputs)[entity].down = event.type == SDL_KEYDOWN;
            break;
          case SDLK_LEFT:
            (*inputs)[entity].left = event.type == SDL_KEYDOWN;
            break;
          case SDLK_RIGHT:
            (*inputs)[entity].right = event.type == SDL_KEYDOWN;
            break;
          case SDLK_RETURN:
            (*inputs)[entity].restart = event.type == SDL_KEYDOWN;
            break;
          case SDLK_SPACE:
            (*inputs)[entity].shoot = (event.type == SDL_KEYDOWN && !(*inputs)[entity].spacebar);
            (*inputs)[entity].spacebar = (event.type == SDL_KEYDOWN);
            break;
          case SDLK_ESCAPE:
            (*inputs)[entity].quit = event.type == SDL_KEYDOWN;
            break;
        }
      }
    }
};

struct ShootingSystem {
    std::unordered_map<uint32_t, InputComponent>* inputs;
    std::unordered_map<uint32_t, PositionComponent>* positions;
    std::unordered_map<uint32_t, RotationComponent>* rotations;
    std::unordered_map<uint32_t, RenderComponent>* renders;
    std::unordered_map<uint32_t, SoundComponent>* sfx;
    std::unordered_map<uint32_t, std::vector<ProjectileComponent>>* projectiles;

    float attackCooldown = 0.0f;
    float attackCooldownDuration = 20.0f;

    void update(float deltaTime) {
      // Iterate over all entities with an input component
      for (auto& [entity, input] : *inputs) {
        attackCooldown -= 90.0f * deltaTime;
        if (attackCooldown <= 0) {
          // Check if the spacebar key is pressed
          if (input.shoot) {
            // Spawn a new projectile at the player position
            auto position = (*positions)[entity];
            auto rotation = (*rotations)[entity];

            float direction_x = cos(rotation.angle * (M_PI / 180));
            float direction_y = sin(rotation.angle * (M_PI / 180));

            const auto &render = (*renders)[entity];

            ProjectileComponent projectile;
            projectile.active = true;
            projectile.x = position.x + render.spriteRect.w * 0.5 - 5;
            projectile.y = position.y + render.spriteRect.h * 0.5 - 5;
            projectile.velocityX = direction_x * 450.0;
            projectile.velocityY = direction_y * 450.0;
            projectile.damage = 10;
            (*projectiles)[entity].push_back(projectile);

            input.shoot = false;

            auto &sound = (*sfx)[entity];
            Mix_PlayChannel(-1, sound.sfx_shoot, 0);

            attackCooldown = attackCooldownDuration;
          }
        }
      }
    }
};

#if defined(__WIN32__)
std::string res_path = "res\\";
#else
std::string res_path = "res/";
#endif

struct RenderSystem {
    std::unordered_map<uint32_t, PositionComponent>* positions;
    std::unordered_map<uint32_t, RotationComponent>* rotations;
    std::unordered_map<uint32_t, UIComponent>* uis;
    std::unordered_map<uint32_t, MenuComponent>* menus;
    std::unordered_map<uint32_t, std::vector<ProjectileComponent>>* projectiles;
    std::unordered_map<uint32_t, RenderComponent>* renders;

    void render(SDL_Renderer* renderer) {
      // Iterate over all entities with a position and render component
      for (auto& [entity, position] : *positions) {
        if (!renders->count(entity)) {
          continue;
        }

        // Get the render and rotation components
        auto& render = (*renders)[entity];
        auto& rotation = (*rotations)[entity];

        // Create a destination rectangle at the position of the entity
        SDL_Rect dstRect;
        dstRect.x = static_cast<int>(position.x);
        dstRect.y = static_cast<int>(position.y);
        dstRect.w = render.spriteRect.w;
        dstRect.h = render.spriteRect.h;

        // Create a center point for the rotation
        SDL_Point center;
        center.x = dstRect.w / 2;
        center.y = dstRect.h / 2;

        // Render the sprite using the destination rectangle and rotation angle
        SDL_RenderCopyEx(renderer, render.texture, NULL, &dstRect, rotation.angle, &center, SDL_FLIP_NONE);
      }

      // Render projectiles
      for (auto& [entity, projectile_vector] : *projectiles) {
        for (auto& projectile : projectile_vector) {
          if (projectile.active == false)
            continue;

          // Create a destination rectangle at the position of the entity
          SDL_Rect dstRect;
          dstRect.x = static_cast<int>(projectile.x);
          dstRect.y = static_cast<int>(projectile.y);
          dstRect.w = 15;
          dstRect.h = 15;

          // Draw the rectangle using the destination rectangle
          SDL_SetRenderDrawColor(renderer, 255, 90, 30, 255);
          SDL_RenderFillRect(renderer, &dstRect);
        }
      }

      // Iterate over all entities with a UI component
      for (auto& [entity, ui] : *uis) {
        auto& position = (*positions)[entity];
        auto& render = (*renders)[entity];

        // Initialize the background rectangle
        ui.healthBarBG.x = position.x + render.spriteRect.w * 0.5 - ui.healthBarBG.w * 0.5;
        ui.healthBarBG.y = position.y + std::max(render.spriteRect.h, render.spriteRect.w) + ui.healthBarBG.h;

        // Set the color of the background
        SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);

        // Draw the background
        SDL_RenderFillRect(renderer, &ui.healthBarBG);

        // Set the color of the health bar based on the current health
        if (ui.health->current > 50) {
          SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        } else if (ui.health->current > 25) {
          SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
        } else {
          SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
        }

        float percentage = (float)ui.health->current / (float)ui.health->maxHealth;
        int fillWidth = percentage * ui.healthBarBG.w;

        // Initialize the health bar rectangle
        ui.healthBar.x = ui.healthBarBG.x;
        ui.healthBar.y = ui.healthBarBG.y;
        ui.healthBar.w = fillWidth;
        ui.healthBar.h = ui.healthBarBG.h;

        // Draw the health bar
        SDL_RenderFillRect(renderer, &ui.healthBar);
      }

      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

      auto& menu = (*menus)[0];

      SDL_RenderCopy(renderer, menu.title_texture, nullptr, menu.title_rect);

      for (auto& [_, ui] : *uis) {
        if (ui.health->current == 0)
        {
          SDL_RenderCopy(renderer, menu.game_over_texture, nullptr, menu.game_over_rect);
          SDL_RenderCopy(renderer, menu.restart_texture, nullptr, menu.restart_rect);
        }
      }
    }
};

struct AISystem {
    std::unordered_map<uint32_t, AIComponent>* ais;
    std::unordered_map<uint32_t, PositionComponent>* positions;
    std::unordered_map<uint32_t, RotationComponent>* rotations;
    std::unordered_map<uint32_t, VelocityComponent>* velocities;
    std::unordered_map<uint32_t, RenderComponent>* renders;
    std::unordered_map<uint32_t, SoundComponent>* sfx;
    std::unordered_map<uint32_t, std::vector<ProjectileComponent>>* projectiles;

    void update(float deltaTime) {
      // Iterate over all enemies with an AIComponent and position and velocity components
      for (auto& [entity, ai] : *ais) {
        if (entity == 0) {
          continue;
        }

        if (!positions->count(entity) || !velocities->count(entity)) {
          continue;
        }

        // Get the position and velocity of the enemy
        auto& position = (*positions)[entity];
        auto& velocity = (*velocities)[entity];
        auto& rotation = (*rotations)[entity];

        // Calculate the direction to the player
        float dx = ai.playerPosition->x - position.x;
        float dy = ai.playerPosition->y - position.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        float direction_x = (dx / distance);
        float direction_y = (dy / distance);

        rotation.angle = std::atan2(dy, dx) * 180.0f / M_PI;

        if (distance < ai.chaseRange) {
          velocity.x = 0;
          velocity.y = 0;
        } else {
          velocity.x = direction_x * 200.0f;
          velocity.y = direction_y * 200.0f;
        }

        // Update the position based on the velocity
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;

        // Update the render component with the new position
        auto& render = (*renders)[entity];
        render.spriteRect.x = position.x;
        render.spriteRect.y = position.y;


        // If the player is in range, shoot a projectile
        if (distance <= ai.attackRange) {
          ai.attackCooldown -= 120.0f * deltaTime;
          if (ai.attackCooldown <= 0) {
            ai.attack_time += 90.0f * deltaTime;

            ai.shoot_cooldown -= 90.0f * deltaTime;
            if (ai.shoot_cooldown <= 0) {

              ProjectileComponent projectile;
              projectile.active = true;
              projectile.x = position.x + render.spriteRect.w * 0.5 - 5;
              projectile.y = position.y + render.spriteRect.h * 0.5 - 5;
              projectile.velocityX = direction_x * 750.0;
              projectile.velocityY = direction_y * 750.0;
              projectile.damage = 25;
              (*projectiles)[entity].push_back(projectile);

              auto &sound = (*sfx)[entity];
              Mix_PlayChannel(-1, sound.sfx_shoot, 0);

              // Reset the shoot cooldown.
              ai.shoot_cooldown = ai.shoot_cooldown_duration;
            }
            if (ai.attack_time >= ai.attack_duration) {
              ai.attackCooldown = ai.attackCooldownDuration;
              ai.attack_time = 0;
            }
          }
        }
      }
    }
};

struct ProjectileSystem {
    std::unordered_map<uint32_t, std::vector<ProjectileComponent>>* projectiles;

    void update(float deltaTime) {
      if (projectiles->empty())
        return;

      // Iterate over all entities with a position and projectile component
      for (auto& [entity, projectile_vector] : *projectiles) {
        for (auto& projectile : projectile_vector) {
          if (!projectile.active)
            continue;

          // Update the position based on the velocity
          projectile.x += projectile.velocityX * deltaTime;
          projectile.y += projectile.velocityY * deltaTime;
        }
      }
    }
};

int main(int argc, char** argv) {
  // Initialize SDL and SDL_image
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "Error: SDL_Init failed: " << SDL_GetError() << std::endl;
    return 1;
  }
  if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
    std::cerr << "Error: IMG_Init failed: " << IMG_GetError() << std::endl;
    return 1;
  }

  // Initialize the SDL_ttf library
  if (TTF_Init() != 0) {
    std::cerr << "Error: TTF_Init() failed: " << TTF_GetError() << std::endl;
    return 1;
  }

  // Initialize the SDL_mixer library
  if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
    std::cerr << "Error: Mix_OpenAudio() failed: " << Mix_GetError() << std::endl;
    return 1;
  }

  SDL_Rect display_bounds;
  if (SDL_GetDisplayBounds(0, &display_bounds) != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error getting display bounds: %s", Mix_GetError());
    return 1;
  }
  int display_width = display_bounds.w;
  int display_height = display_bounds.h;

  // Create the window
  SDL_Window* window = SDL_CreateWindow("My Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, display_width, display_height, 0);
  if (window == nullptr) {
    std::cerr << "Error: SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
    return 1;
  }
  SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

  // Create the renderer
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == nullptr) {
    std::cerr << "Error: SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Get the base path
  char* basePath = SDL_GetBasePath();
  if (basePath == nullptr) {
    std::cerr << "Error: SDL_GetBasePath failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Load the sound effect
  std::string soundPath = basePath;
  soundPath += res_path + "shoot2.wav";
  Mix_Chunk* sfx_shoot_player = Mix_LoadWAV(soundPath.c_str());
  if (!sfx_shoot_player) {
    std::cerr << "Error: Mix_LoadWAV() failed: " << Mix_GetError() << std::endl;
    return 1;
  }
  soundPath = basePath;
  soundPath += res_path + "shoot1.wav";
  Mix_Chunk* sfx_shoot_enemy = Mix_LoadWAV(soundPath.c_str());
  if (sfx_shoot_enemy == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mix_LoadWAV error: %s", Mix_GetError());
    return -1;
  }
  soundPath = basePath;
  soundPath += res_path + "hit1.wav";
  Mix_Chunk* sfx_hit_player = Mix_LoadWAV(soundPath.c_str());
  if (sfx_hit_player == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mix_LoadWAV error: %s", Mix_GetError());
    return -1;
  }
  soundPath = basePath;
  soundPath += res_path + "hit2.wav";
  Mix_Chunk* sfx_hit_enemy = Mix_LoadWAV(soundPath.c_str());
  if (sfx_hit_enemy == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mix_LoadWAV error: %s", Mix_GetError());
    return -1;
  }
  soundPath = basePath;
  soundPath += res_path + "explosion1.wav";
  Mix_Chunk* sfx_explosion_player = Mix_LoadWAV(soundPath.c_str());
  if (sfx_explosion_player == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mix_LoadWAV error: %s", Mix_GetError());
    return -1;
  }
  soundPath = basePath;
  soundPath += res_path + "explosion2.wav";
  Mix_Chunk* sfx_explosion_enemy = Mix_LoadWAV(soundPath.c_str());
  if (sfx_explosion_enemy == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mix_LoadWAV error: %s", Mix_GetError());
    return -1;
  }
  soundPath = basePath;
  soundPath += res_path + "win.wav";
  Mix_Chunk* sfx_win = Mix_LoadWAV(soundPath.c_str());
  if (sfx_win == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mix_LoadWAV error: %s", Mix_GetError());
    return -1;
  }

  Mix_VolumeChunk(sfx_shoot_player, 64);
  Mix_VolumeChunk(sfx_shoot_enemy, 64);
  Mix_VolumeChunk(sfx_hit_player, 96);
  Mix_VolumeChunk(sfx_win, 64);
  Mix_VolumeChunk(sfx_explosion_enemy, 32);

  // Load the font
  std::string fontPath = basePath;
  fontPath += res_path + "orange-kid.regular.ttf";
  TTF_Font* font_large = TTF_OpenFont(fontPath.c_str(), 64);
  if (!font_large) {
    std::cerr << "Error: TTF_OpenFont() failed: " << TTF_GetError() << std::endl;
    return 1;
  }
  TTF_Font* font_small = TTF_OpenFont(fontPath.c_str(), 48);
  if (!font_small) {
    std::cerr << "Error: TTF_OpenFont() failed: " << TTF_GetError() << std::endl;
    return 1;
  }

  // Initialize the menu text textures
  SDL_Surface* font_surface = TTF_RenderText_Blended(font_large, "GAME OVER", SDL_Color{255, 255, 255, 255});
  SDL_Texture* game_over_texture = SDL_CreateTextureFromSurface(renderer, font_surface);
  font_surface = TTF_RenderText_Blended(font_small, "Press RETURN to restart", SDL_Color{255, 255, 255, 255});
  SDL_Texture* restart_texture = SDL_CreateTextureFromSurface(renderer, font_surface);
  font_surface = TTF_RenderText_Blended(font_large, "ChatGPT Game", SDL_Color{255, 255, 255, 255});
  SDL_Texture* title_texture = SDL_CreateTextureFromSurface(renderer, font_surface);
  SDL_FreeSurface(font_surface);

  // Set the position of the "game over" text
  SDL_Rect game_over_rect;
  SDL_QueryTexture(game_over_texture, nullptr, nullptr, &game_over_rect.w, &game_over_rect.h);
  game_over_rect.x = display_width * 0.5 - game_over_rect.w * 0.5;
  game_over_rect.y = display_height * 0.5 - game_over_rect.h * 0.5;

  SDL_Rect restart_rect;
  SDL_QueryTexture(restart_texture, nullptr, nullptr, &restart_rect.w, &restart_rect.h);
  restart_rect.x = display_width * 0.5 - restart_rect.w * 0.5;
  restart_rect.y = display_height * 0.5 + restart_rect.h * 0.5;

  SDL_Rect title_rect;
  SDL_QueryTexture(title_texture, nullptr, nullptr, &title_rect.w, &title_rect.h);
  title_rect.x = display_width * 0.5 - title_rect.w * 0.5;
  title_rect.y = title_rect.h * 0.25;

  // Create player texture
  std::string imagePath = basePath;
  imagePath += res_path + "player.png";
  SDL_Surface* player_surface = IMG_Load(imagePath.c_str());
  if (player_surface == nullptr) {
    std::cerr << "Error: IMG_Load failed: " << IMG_GetError() << std::endl;
    return 1;
  }

  SDL_Texture* player_texture = SDL_CreateTextureFromSurface(renderer, player_surface);
  if (player_texture == nullptr) {
    std::cerr << "Error: SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Rect player_rect;
  player_rect.x = 0;
  player_rect.y = 0;
  player_rect.w = player_surface->w; // Use the width of the surface as the width of the rect
  player_rect.h = player_surface->h; // Use the height of the surface as the height of the rect
  SDL_FreeSurface(player_surface);

  // Create player texture
  std::string imagePath2 = basePath;
  imagePath2 += res_path + "enemy.png";
  SDL_Surface* enemy_surface = IMG_Load(imagePath2.c_str());
  if (enemy_surface == nullptr) {
    std::cerr << "Error: IMG_Load failed: " << IMG_GetError() << std::endl;
    return 1;
  }

  SDL_Texture* enemy_texture = SDL_CreateTextureFromSurface(renderer, enemy_surface);
  if (enemy_texture == nullptr) {
    std::cerr << "Error: SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Rect enemy_rect;
  enemy_rect.x = 0;
  enemy_rect.y = 0;
  enemy_rect.w = enemy_surface->w; // Use the width of the surface as the width of the rect
  enemy_rect.h = enemy_surface->h; // Use the height of the surface as the height of the rect
  SDL_FreeSurface(enemy_surface);

  // Create the maps of components
  std::unordered_map<uint32_t, PositionComponent> positions;
  std::unordered_map<uint32_t, VelocityComponent> velocities;
  std::unordered_map<uint32_t, RotationComponent> rotations;
  std::unordered_map<uint32_t, InputComponent> inputs;
  std::unordered_map<uint32_t, RenderComponent> renders;
  std::unordered_map<uint32_t, AIComponent> ais;
  std::unordered_map<uint32_t, HealthComponent> healths;
  std::unordered_map<uint32_t, UIComponent> uis;
  std::unordered_map<uint32_t, MenuComponent> menus;
  std::unordered_map<uint32_t, SoundComponent> sfx;
  std::unordered_map<uint32_t, std::vector<ProjectileComponent>> projectiles;

  // Add the player entity
  uint32_t playerEntity = 0;
  positions[playerEntity] = {100, 100};
  velocities[playerEntity] = {0, 0};
  rotations[playerEntity] = {0.0};
  inputs[playerEntity] = {false, false, false, false};
  renders[playerEntity] = {player_texture, player_rect};
  healths[playerEntity] = {100, 100};
  uis[playerEntity] = {SDL_Rect{0, 0, 100, 8}, SDL_Rect{0, 0, 100, 8}, &healths[playerEntity]};
  menus[playerEntity] = {&title_rect, title_texture, &game_over_rect, game_over_texture, &restart_rect, restart_texture};
  sfx[playerEntity] = {sfx_shoot_player, sfx_hit_player, sfx_explosion_player};

  // Create an AIComponent for each enemy
  uint32_t enemy1 = 1;
  positions[enemy1] = {float(display_width - 200), float(display_height - 200)};
  velocities[enemy1] = {0, 0};
  rotations[enemy1] = {0.0};
  ais[enemy1] = {&positions[playerEntity], 150.0f, (float)(display_width * 0.7), 0, 180, 0, 80, 0, 20};
  healths[enemy1] = {100, 100};
  renders[enemy1] = {enemy_texture, enemy_rect};
  uis[enemy1] = {SDL_Rect{0, 0, 100, 8}, SDL_Rect{0, 0, 100, 8}, &healths[enemy1]};
  sfx[enemy1] = {sfx_shoot_enemy, sfx_hit_enemy, sfx_explosion_enemy};

  // Create the movement system
  MovementSystem movementSystem;
  RenderSystem renderSystem;
  InputSystem inputSystem;
  AISystem aiSystem;
  ProjectileSystem projectileSystem;
  ShootingSystem shootingSystem;
  HealthSystem healthSystem;

  // Store the references to the component maps
  movementSystem.positions = &positions;
  movementSystem.velocities = &velocities;
  movementSystem.rotations = &rotations;
  movementSystem.inputs = &inputs;
  renderSystem.positions = &positions;
  renderSystem.rotations = &rotations;
  renderSystem.renders = &renders;
  renderSystem.projectiles = &projectiles;
  renderSystem.uis = &uis;
  renderSystem.menus = &menus;
  inputSystem.inputs = &inputs;
  aiSystem.ais = &ais;
  aiSystem.positions = &positions;
  aiSystem.rotations = &rotations;
  aiSystem.velocities = &velocities;
  aiSystem.renders = &renders;
  aiSystem.projectiles = &projectiles;
  aiSystem.sfx = &sfx;
  projectileSystem.projectiles = &projectiles;
  shootingSystem.positions = &positions;
  shootingSystem.rotations = &rotations;
  shootingSystem.projectiles = &projectiles;
  shootingSystem.inputs = &inputs;
  shootingSystem.sfx = &sfx;
  shootingSystem.renders = &renders;
  healthSystem.projectiles = &projectiles;
  healthSystem.positions = &positions;
  healthSystem.renders = &renders;
  healthSystem.healths = &healths;
  healthSystem.sfx = &sfx;

  // Initialize the previous time
  uint32_t previousTime = SDL_GetTicks();

  bool won = false;
  bool game_over = false;
  // Game loop
  while (true) {
    // Handle events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT || inputs[playerEntity].quit) {
        goto cleanup;
      }
      inputSystem.handleEvent(event);
    }

    // Calculate delta time
    uint32_t currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - previousTime) / 1000.0f;
    previousTime = currentTime;

    if (!game_over && !won)
    {
      // Update the MovementSystem
      movementSystem.update(deltaTime);
      aiSystem.update(deltaTime);
      shootingSystem.update(deltaTime);
      projectileSystem.update(deltaTime);
      healthSystem.update(deltaTime);

      game_over = healths[playerEntity].current == 0;
      won = healths[enemy1].current == 0;

      if (won)
        Mix_PlayChannel(-1, sfx_win, 0);
    }
    else if (inputs[playerEntity].restart)
    {
      positions[playerEntity].x = 100;
      positions[playerEntity].y = 100;
      rotations[playerEntity].angle = 0;
      healths[playerEntity].current = 100;

      positions[enemy1].x = display_width - 200;
      positions[enemy1].y = display_height - 200;
      rotations[enemy1].angle = 0;
      healths[enemy1].current = 100;

      for (auto& [_, projectile_vector] : projectiles) {
        for (auto &projectile: projectile_vector) {
          projectile.active = false;
        }
      }

      game_over = false;
      won = false;
    }

    // Clear the screen
    SDL_RenderClear(renderer);

    // Render the entities
    renderSystem.render(renderer);

    // Update the screen
    SDL_RenderPresent(renderer);

    SDL_Delay(16.666f - deltaTime);
  }

  cleanup:
  // Clean up resources
  SDL_free(basePath);
  SDL_DestroyTexture(player_texture);
  SDL_DestroyTexture(enemy_texture);
  SDL_DestroyTexture(game_over_texture);
  SDL_DestroyTexture(restart_texture);
  TTF_CloseFont(font_large);
  TTF_CloseFont(font_small);
  Mix_FreeChunk(sfx_shoot_player);
  Mix_CloseAudio();
  TTF_Quit();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
