#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdlib.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 450
#define MARGIN 20

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
} App;

int calcul(int a, int b) {
	if (b > a) {
	    int tmp = a;
	    a = b;
	    b = tmp;
	}
	
	while (b != 0) {
            int tmp = a % b;
            a = b;
            b = tmp;
        }
        return a;
}

int init_app(App* app) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 0;
    if (TTF_Init() == -1) return 0;
    app->window = SDL_CreateWindow("PGCD",
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
    if (!init_app(&app)) { cleanup(&app); return 1; }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color black = {0,   0,   0,   255};
    SDL_Color gray  = {200, 200, 200, 255};
    SDL_Color blue  = {70,  130, 180, 255};
    SDL_Color dark  = {50,  50,  50,  255};

    const char* explication = "Calcul PGCD (ordre indifferent)";

    char input_text1[64] = "";
    char input_text2[64] = "";
    char result_text[64] = "";
    int input_active = 0;

    SDL_Rect input_box1 = {MARGIN, 140, WINDOW_WIDTH - MARGIN*2, 35};
    SDL_Rect input_box2 = {MARGIN, 210, WINDOW_WIDTH - MARGIN*2, 35};
    SDL_Rect button     = {MARGIN, 270, 150, 35};
    SDL_Rect result_box = {MARGIN, 340, WINDOW_WIDTH - MARGIN*2, 35};

    SDL_StartTextInput();
    int running = 1;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;

                input_active = 0;
                if (mx >= input_box1.x && mx <= input_box1.x + input_box1.w &&
                    my >= input_box1.y && my <= input_box1.y + input_box1.h)
                    input_active = 1;
                if (mx >= input_box2.x && mx <= input_box2.x + input_box2.w &&
                    my >= input_box2.y && my <= input_box2.y + input_box2.h)
                    input_active = 2;

                if (mx >= button.x && mx <= button.x + button.w &&
                    my >= button.y && my <= button.y + button.h) {
                    int a = atoi(input_text1);
                    int b = atoi(input_text2);
                    snprintf(result_text, sizeof(result_text), "%d", calcul(a, b));
                }
            }

            if (input_active) {
                char* active_text = (input_active == 1) ? input_text1 : input_text2;
                if (e.type == SDL_TEXTINPUT) {
                    char c = e.text.text[0];
                    if (c >= '0' && c <= '9') {
                        if (strlen(active_text) < 63)
                            strncat(active_text, e.text.text, 1);
                    }
                }
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE) {
                    int len = strlen(active_text);
                    if (len > 0) active_text[len - 1] = '\0';
                }
            }
        }

        SDL_SetRenderDrawColor(app.renderer, 50, 50, 50, 255);
        SDL_RenderClear(app.renderer);

        render_text(&app, explication, MARGIN, MARGIN, white);

        render_text(&app, "Entree 1 :", MARGIN, 115, white);
        render_rect(&app, &input_box1, white, input_active == 1 ? blue : gray);
        if (strlen(input_text1) > 0)
            render_text(&app, input_text1, input_box1.x + 5, input_box1.y + 8, black);

        render_text(&app, "Entree 2 :", MARGIN, 185, white);
        render_rect(&app, &input_box2, white, input_active == 2 ? blue : gray);
        if (strlen(input_text2) > 0)
            render_text(&app, input_text2, input_box2.x + 5, input_box2.y + 8, black);

        render_rect(&app, &button, blue, dark);
        render_text(&app, "Calculer", button.x + 20, button.y + 8, white);

        render_text(&app, "PGCD :", MARGIN, 315, white);
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

