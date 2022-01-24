#include <SDL.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

size_t const FREK = 48000;
float const SAMPLE_DELTA_TIME = 1.0 / FREK;
float const STEP_TIME_MIN = 1.0f;
float const STEP_TIME_MAX = 200.0f;

//SLIDER BODY
int const STEP_TIME_SLIDER_X = 100.0f;
int const STEP_TIME_SLIDER_Y = 100.0f;
float const STEP_TIME_SLIDER_LEN = 500.0f;
int const STEP_TIME_SLIDER_THICC = 5.f;
int const STEP_TIME_SLIDER_COLOR = 0x00FF00FF;
int const STEP_TIME_SLIDER_BACKGROUND = 0x181818FF;

//GRIP
int const STEP_TIME_GRIP_SIZE = 10;
int const STEP_TIME_GRIP_COLOR = 0xFF0000FF;

//0xRRGGBBAA
#define HEXCOLOR(code)              \
    ((code) >> (8 * 3)) & 0xFF,     \
        ((code) >> (8 * 2)) & 0xFF, \
        ((code) >> (8 * 1)) & 0xFF, \
        ((code) >> (8 * 0)) & 0xFF

Sint16 sgn(int16_t x)
{
    return ((x > 0) - (x < 0));
}

bool rect_contains_point(SDL_Rect rect, int x, int y)
{

    return (x > rect.x && x < rect.w && y > rect.y && y < rect.h);
}

typedef struct
{
    Sint16 current;
    Sint16 next;
    float step_time;
    float a;
} Gen;

void white_noise(Gen *gen, Sint16 *stream, size_t stream_len)
{
    for (size_t i = 0; i < stream_len; ++i)
    {
        float gen_step = 1.0 / (gen->step_time * SAMPLE_DELTA_TIME);
        stream[i] = gen->current + (gen->next - gen->current) * gen->a;
        // printf("%d\n", stream[i]);
        gen->a += gen_step * SAMPLE_DELTA_TIME;
        if (gen->a >= 1)
        {
            Sint16 value = rand() % (1 << 10);
            Sint16 sign = (rand() % 2) * 2 - 1;
            gen->current = gen->next;
            gen->next = value * sign;
            gen->a = 0.0;
        }
    }
}

void white_noise_callback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata;
    assert(len % 2 == 0);
    white_noise((Gen *)userdata, (Sint16 *)stream, len / 2);
}

int main(void)
{

    SDL_Window *window = SDL_CreateWindow("Whine",
                                          0, 0, 800,
                                          600, SDL_WINDOW_RESIZABLE);
    if (window == NULL)
    {
        fprintf(stderr, "ERROR: cant create window: %s ", SDL_GetError());
        exit(1);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window,
                                                -1, SDL_RENDERER_ACCELERATED);

    if (renderer == NULL)
    {
        fprintf(stderr, "ERROR: cant create renderer: %s ", SDL_GetError());
        exit(1);
    }
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    Gen gen = {
        .step_time = 1,
    };

    SDL_AudioSpec desired = {
        .freq = 48000,
        .format = AUDIO_S16LSB,
        .channels = 1,
        .callback = white_noise_callback,
        .userdata = &gen,
    };

    if (SDL_OpenAudio(&desired, NULL) < 0)
    {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    bool quite = false;
    bool dragging_grip = false;

    SDL_PauseAudio(0);

    while (!quite)
    {

        // printf("Mause cursor at %d, %d\n", x, y);
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quite = true;
                printf("quite");
                break;

            case SDL_MOUSEBUTTONDOWN:

                break;
            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, HEXCOLOR(STEP_TIME_SLIDER_BACKGROUND));
        SDL_RenderClear(renderer);

        //SLIDER BODY
        {
            SDL_SetRenderDrawColor(renderer, HEXCOLOR(STEP_TIME_SLIDER_COLOR));

            SDL_Rect rect = {
                .x = STEP_TIME_SLIDER_X,
                .y = STEP_TIME_SLIDER_Y - 0.5 * STEP_TIME_SLIDER_THICC,
                .w = STEP_TIME_SLIDER_LEN,
                .h = STEP_TIME_SLIDER_THICC,
            };
            SDL_RenderFillRect(renderer, &rect);
        }
        //GRIP
        {
            float grip_value = gen.step_time / (STEP_TIME_MAX - STEP_TIME_MIN) * STEP_TIME_SLIDER_LEN;

            SDL_SetRenderDrawColor(renderer, HEXCOLOR(STEP_TIME_GRIP_COLOR));

            SDL_Rect rect = {
                .x = STEP_TIME_SLIDER_X - STEP_TIME_GRIP_SIZE + grip_value,
                .y = STEP_TIME_SLIDER_Y - STEP_TIME_GRIP_SIZE,
                .w = STEP_TIME_GRIP_SIZE * 2,
                .h = STEP_TIME_GRIP_SIZE * 2,
            };
            SDL_RenderFillRect(renderer, &rect);

            int x, y, button;
            button = SDL_GetMouseState(&x, &y);
            if (!dragging_grip)
            {
                SDL_Point point = {x, y};
                if ((SDL_PointInRect(&point, &rect) && (button & SDL_BUTTON_LMASK) != 0))
                    dragging_grip = true;
            }
            else
            {
                if ((button & SDL_BUTTON_LMASK) == 0)
                    dragging_grip = false;
                else
                {
                    float offset_max = STEP_TIME_SLIDER_X - STEP_TIME_GRIP_SIZE + STEP_TIME_SLIDER_LEN;
                    float offset_min = STEP_TIME_SLIDER_X - STEP_TIME_GRIP_SIZE;
                    float xf = x - STEP_TIME_GRIP_SIZE;
                    if (xf < offset_min)
                        xf = offset_min;
                    if (xf > offset_max)
                        xf = offset_max;
                    gen.step_time = (xf - offset_min) / STEP_TIME_SLIDER_LEN * (STEP_TIME_MAX - STEP_TIME_MIN) + STEP_TIME_MIN;
                    printf("%f\n", gen.step_time);
                }
            }
        }

        SDL_RenderPresent(renderer);
    }
    printf("OK\n");

    SDL_Quit();
    return 0;
}