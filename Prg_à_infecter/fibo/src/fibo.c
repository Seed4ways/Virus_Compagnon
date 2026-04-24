#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdlib.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 400
#define MARGIN 20

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
} App;

int calcul(int input) {
	if (input == 0 || input == 1) return input;
	return calcul(input - 1) + calcul(input - 2);
}

int init_app(App* app) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 0;
    if (TTF_Init() == -1) return 0;

    app->window = SDL_CreateWindow("Fibonacci",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT,
                                   SDL_WINDOW_SHOWN);
    if (!app->window) return 0;

    app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_ACCELERATED);
    if (!app->renderer) return 0;

    app->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (!app->font) return 0;

    return 1;
}

void render_text(App* app, const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(app->font, text, color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(app->renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(app->renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_rect(App* app, SDL_Rect* r, SDL_Color fill, SDL_Color border) {
    SDL_SetRenderDrawColor(app->renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(app->renderer, r);
    SDL_SetRenderDrawColor(app->renderer, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(app->renderer, r);
}

void cleanup(App* app) {
    if (app->font) TTF_CloseFont(app->font);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    TTF_Quit();
    SDL_Quit();
}

int main() {
    App app = {0};
    if (!init_app(&app)) {
        cleanup(&app);
        return 1;
    }

    const char* expl1 = "Calcul de Fibonacci pour un n choisi";
    const char* expl2 = "dans la zone de saisie ci-dessous";


    char input_text[64] = "";
    char result_text[64] = "";
    int input_active = 0;

    // Zones de l'interface
    SDL_Rect input_box  = {MARGIN, 120, WINDOW_WIDTH - MARGIN*2, 35};
    SDL_Rect button     = {MARGIN, 190, 150, 35};
    SDL_Rect result_box = {MARGIN, 260, WINDOW_WIDTH - MARGIN*2, 35};

    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color black  = {0,   0,   0,   255};
    SDL_Color gray   = {200, 200, 200, 255};
    SDL_Color blue   = {70,  130, 180, 255};
    SDL_Color dark   = {50,  50,  50,  255};

    SDL_StartTextInput();
    int running = 1;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {

            if (e.type == SDL_QUIT) {
                running = 0;
            }

            // Clic souris
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;

                // Activer/désactiver la saisie
                input_active = (mx >= input_box.x && mx <= input_box.x + input_box.w &&
                                my >= input_box.y && my <= input_box.y + input_box.h);

                // Clic sur le bouton
                if (mx >= button.x && mx <= button.x + button.w &&
                    my >= button.y && my <= button.y + button.h) {
			int val = atoi(input_text);
			int res = calcul(val);
			snprintf(result_text, sizeof(result_text), "%d", res);
                }
            }

            // Saisie clavier
            if (input_active) {
                if (e.type == SDL_TEXTINPUT) {
                    // Accepte chiffres, point et signe moins
                    char c = e.text.text[0];
                    if (c >= '0' && c <= '9') {
                        if (strlen(input_text) < sizeof(input_text) - 1)
                            strncat(input_text, e.text.text, 1);
                    }
                }
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE) {
                    int len = strlen(input_text);
                    if (len > 0) input_text[len - 1] = '\0';
                }
            }
        }

        // Rendu
        SDL_SetRenderDrawColor(app.renderer, 50, 50, 50, 255);
        SDL_RenderClear(app.renderer);

        // Texte d'explication
        render_text(&app, expl1, MARGIN, MARGIN, white);
	render_text(&app, expl2, MARGIN, MARGIN + 22, white);


        // Zone de saisie
        render_rect(&app, &input_box, white, input_active ? blue : gray);
        if (strlen(input_text) > 0)
            render_text(&app, input_text, input_box.x + 5, input_box.y + 8, black);

        // Bouton
        render_rect(&app, &button, blue, dark);
        render_text(&app, "Calculer", button.x + 20, button.y + 8, white);

        // Zone résultat
        render_rect(&app, &result_box, white, gray);
        if (strlen(result_text) > 0)
            render_text(&app, result_text, result_box.x + 5, result_box.y + 8, black);

        SDL_RenderPresent(app.renderer);
        SDL_Delay(16);
    }

    SDL_StopTextInput();
    cleanup(&app);
    return 0;
}

