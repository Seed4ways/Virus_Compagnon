#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "tinyexpr.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRAPH_X 50
#define GRAPH_Y 60
#define GRAPH_W 700
#define GRAPH_H 430

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
} App;

int init_app(App* app) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 0;
    if (TTF_Init() == -1) return 0;
    app->window = SDL_CreateWindow("Courbe",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT,
                                   SDL_WINDOW_SHOWN);
    if (!app->window) return 0;
    app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_ACCELERATED);
    if (!app->renderer) return 0;
    app->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
    if (!app->font) return 0;
    return 1;
}

void cleanup(App* app) {
    TTF_CloseFont(app->font);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    TTF_Quit();
    SDL_Quit();
}

void render_text(App* app, const char* text, int x, int y, SDL_Color color) {
    if (!text || strlen(text) == 0) return;
    SDL_Surface* surface = TTF_RenderText_Solid(app->font, text, color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(app->renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(app->renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void render_rect(App* app, SDL_Rect* rect, SDL_Color border, SDL_Color fill) {
    SDL_SetRenderDrawColor(app->renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(app->renderer, rect);
    SDL_SetRenderDrawColor(app->renderer, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(app->renderer, rect);
}

// Convertit ** en ^ pour tinyexpr
void preprocess(char* str) {
    char result[256] = {0};
    int j = 0;
    for (int i = 0; str[i] && j < 250; ) {
        if (str[i] == '*' && str[i+1] == '*') {
            result[j++] = '^';
            i += 2;
        } else {
            result[j++] = str[i++];
        }
    }
    result[j] = '\0';
    strcpy(str, result);
}

void draw_graph(App* app, te_expr* expr, double* x_var,
                double x_min, double x_max,
                double y_min, double y_max) {
    // Fond blanc du graphe
    SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);
    SDL_Rect graph_rect = {GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H};
    SDL_RenderFillRect(app->renderer, &graph_rect);

    // Axes
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);

    // Axe X (y=0)
    int y_zero = GRAPH_Y + (int)((y_max / (y_max - y_min)) * GRAPH_H);
    if (y_zero >= GRAPH_Y && y_zero <= GRAPH_Y + GRAPH_H)
        SDL_RenderDrawLine(app->renderer, GRAPH_X, y_zero, GRAPH_X + GRAPH_W, y_zero);

    // Axe Y (x=0)
    int x_zero = GRAPH_X + (int)((-x_min / (x_max - x_min)) * GRAPH_W);
    if (x_zero >= GRAPH_X && x_zero <= GRAPH_X + GRAPH_W)
        SDL_RenderDrawLine(app->renderer, x_zero, GRAPH_Y, x_zero, GRAPH_Y + GRAPH_H);

    // Graduations X
    SDL_Color black = {0, 0, 0, 255};
    for (int v = (int)x_min; v <= (int)x_max; v++) {
        int px = GRAPH_X + (int)(((double)(v - x_min) / (x_max - x_min)) * GRAPH_W);
        SDL_RenderDrawLine(app->renderer, px, y_zero - 4, px, y_zero + 4);
        if (v != 0) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", v);
            render_text(app, buf, px - 5, y_zero + 6, black);
        }
    }

    // Graduations Y
    for (int v = (int)y_min; v <= (int)y_max; v++) {
        int py = GRAPH_Y + (int)(((y_max - v) / (y_max - y_min)) * GRAPH_H);
        SDL_RenderDrawLine(app->renderer, x_zero - 4, py, x_zero + 4, py);
        if (v != 0) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", v);
            render_text(app, buf, x_zero + 6, py - 7, black);
        }
    }

    // Bordure graphe
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(app->renderer, &graph_rect);

    // Courbe
    if (!expr) return;
    SDL_SetRenderDrawColor(app->renderer, 220, 0, 0, 255);
    int prev_px = -1, prev_py = -1;
    for (int px = 0; px < GRAPH_W; px++) {
        *x_var = x_min + ((double)px / GRAPH_W) * (x_max - x_min);
        double y = te_eval(expr);
        if (isnan(y) || isinf(y)) { prev_px = -1; continue; }
        if (y < y_min || y > y_max) { prev_px = -1; continue; }
        int py = GRAPH_Y + (int)(((y_max - y) / (y_max - y_min)) * GRAPH_H);
        int screen_px = GRAPH_X + px;
        if (prev_px != -1)
            SDL_RenderDrawLine(app->renderer, prev_px, prev_py, screen_px, py);
        prev_px = screen_px;
        prev_py = py;
    }
}

int main(void) {
    App app;
    if (!init_app(&app)) { cleanup(&app); return 1; }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color black = {0,   0,   0,   255};
    SDL_Color blue  = {50,  100, 200, 255};
    SDL_Color dark  = {30,  70,  150, 255};
    SDL_Color gray  = {120, 120, 120, 255};
    SDL_Color red   = {220, 0,   0,   255};

    // Zone de saisie et bouton
    SDL_Rect input_box = {GRAPH_X, GRAPH_Y + GRAPH_H + 20, 500, 30};
    SDL_Rect btn       = {GRAPH_X + 520, GRAPH_Y + GRAPH_H + 20, 120, 30};

    char func_text[128] = {0};
    int  active = 0;

    double x_var = 0;
    double x_min = -10, x_max = 10;
    double y_min = -10, y_max = 10;

    te_variable vars[] = {{"x", &x_var, 0, 0}};
    te_expr* expr = NULL;

    // Explications affichées en haut
    const char* expl1 = "Entrez une fonction de x. Exemple : x**2 + 2*x - 1 ou sin(x)";
    const char* expl2 = "Utilisez ** pour la puissance (ex: x**3), * pour multiplier";

    SDL_StartTextInput();
    int running = 1;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;

            // Clic
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;

                // Clic sur champ de saisie
                active = (mx >= input_box.x && mx <= input_box.x + input_box.w &&
                          my >= input_box.y && my <= input_box.y + input_box.h);

                // Clic sur bouton Tracer
                if (mx >= btn.x && mx <= btn.x + btn.w &&
                    my >= btn.y && my <= btn.y + btn.h) {
                    char tmp[128];
                    strncpy(tmp, func_text, sizeof(tmp));
                    preprocess(tmp);
                    if (expr) te_free(expr);
                    expr = te_compile(tmp, vars, 1, NULL);
                }
            }

            // Saisie texte
            if (e.type == SDL_TEXTINPUT && active) {
                char* t = e.text.text;
                // On n'accepte que les caractères ASCII imprimables
                if ((unsigned char)t[0] >= 0x20 && (unsigned char)t[0] < 0x80) {
                    if (strlen(func_text) + strlen(t) < sizeof(func_text) - 1)
                        strncat(func_text, t, sizeof(func_text) - strlen(func_text) - 1);
                }
            }

            if (e.type == SDL_KEYDOWN && active) {
                // Backspace
                if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    int l = strlen(func_text);
                    if (l > 0) func_text[l - 1] = '\0';
                }
                // Entrée = tracer
                if (e.key.keysym.sym == SDLK_RETURN) {
                    char tmp[128];
                    strncpy(tmp, func_text, sizeof(tmp));
                    preprocess(tmp);
                    if (expr) te_free(expr);
                    expr = te_compile(tmp, vars, 1, NULL);
                }
            }
        }

        // Rendu
        SDL_SetRenderDrawColor(app.renderer, 50, 50, 50, 255);
        SDL_RenderClear(app.renderer);

        // Explications
        render_text(&app, expl1, GRAPH_X, 10, white);
        render_text(&app, expl2, GRAPH_X, 30, white);

        // Graphe
        draw_graph(&app, expr, &x_var, x_min, x_max, y_min, y_max);

        // Champ de saisie
        render_rect(&app, &input_box, white, active ? blue : gray);
        if (strlen(func_text) > 0)
            render_text(&app, func_text, input_box.x + 5, input_box.y + 7, white);

        // Bouton
        render_rect(&app, &btn, blue, dark);
        render_text(&app, "Tracer", btn.x + 30, btn.y + 7, white);

        // Message erreur si fonction invalide
        if (strlen(func_text) > 0 && !expr)
            render_text(&app, "Fonction invalide", input_box.x, input_box.y - 22, red);

        SDL_RenderPresent(app.renderer);
        SDL_Delay(16);
    }

    if (expr) te_free(expr);
    SDL_StopTextInput();
    cleanup(&app);
    return 0;
}

