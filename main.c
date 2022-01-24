#include <SDL.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#define HEXCOLOR(code)              \
    ((code) >> (8 * 3)) & 0xFF,     \
        ((code) >> (8 * 2)) & 0xFF, \
        ((code) >> (8 * 1)) & 0xFF, \
        ((code) >> (8 * 0)) & 0xFF

size_t const FREK = 48000;
float const SAMPLE_DELTA_TIME = 1.0 / FREK;
float const STEP_TIME_MIN = 1.0f;
float const STEP_TIME_MAX = 200.0f;

//SLIDER BODY
float const STEP_TIME_SLIDER_THICC = 5.f;
int const STEP_TIME_SLIDER_COLOR = 0x00FF00FF;
int const STEP_TIME_SLIDER_BACKGROUND = 0x181818FF;

//GRIP
float const STEP_TIME_GRIP_SIZE = 10;
int const STEP_TIME_GRIP_COLOR = 0xFF0000FF;

float clampf(float value, float min, float max)
{
    if (value < min)
        value = min;
    if (value > max)
        value = max;
    return value;
}
float ilerpf(float value, float min, float max)
{
    return (value - min) / (max - min);
}

float lerpf(float value, float min, float max)
{
    return (max - min) * value + min;
}

int active_id = -1;

void slider(SDL_Renderer *renderer, float slider_x, float slider_y, float len, float id,
            float *value, float min, float max)
{
    //SLIDER BODY
    {
        SDL_SetRenderDrawColor(renderer, HEXCOLOR(STEP_TIME_SLIDER_COLOR));

        SDL_Rect rect = {
            .x = slider_x,
            .y = slider_y - 0.5 * STEP_TIME_SLIDER_THICC,
            .w = len,
            .h = STEP_TIME_SLIDER_THICC,
        };
        SDL_RenderFillRect(renderer, &rect);
    }
    //GRIP
    {
        assert(min <= max);
        float grip_value = ilerpf(*value, min, max) * len;

        SDL_SetRenderDrawColor(renderer, HEXCOLOR(STEP_TIME_GRIP_COLOR));

        SDL_Rect rect = {
            .x = slider_x - STEP_TIME_GRIP_SIZE + grip_value,
            .y = slider_y - STEP_TIME_GRIP_SIZE,
            .w = STEP_TIME_GRIP_SIZE * 2,
            .h = STEP_TIME_GRIP_SIZE * 2,
        };
        SDL_RenderFillRect(renderer, &rect);

        int x, y, button;
        button = SDL_GetMouseState(&x, &y);
        if (active_id < 0)
        {
            SDL_Point point = {x, y};
            if ((SDL_PointInRect(&point, &rect) && (button & SDL_BUTTON_LMASK) != 0))
            {
                active_id = id;
            }
        }
        else
        {
            if (active_id == id)
            {
                if ((button & SDL_BUTTON_LMASK) == 0)
                {
                    active_id = -1;
                }
                else
                {
                    float offset_min = slider_x - STEP_TIME_GRIP_SIZE;
                    float offset_max = offset_min + len;
                    float xf = clampf(x - STEP_TIME_GRIP_SIZE, offset_min, offset_max);
                    xf = ilerpf(xf, offset_min, offset_max);
                    xf = lerpf(xf, min, max);
                    *value = xf;
                }
            }
        }
        // printf("%f, %d,%d, %d \n", *value, x, y, *dragging);
    }
}
//0xRRGGBBAA
Sint16 sgn(int16_t x)
{
    return ((x > 0) - (x < 0));
}

typedef struct
{
    Sint16 current;
    Sint16 next;
    float step_time;
    float volume;
    float a;
} Gen;

typedef enum
{
    SLIDE_FREQ = 0,
    SLIDE_VOLUME,
    SLIDE_COUNT,
} Slider;

void white_noise(Gen *gen, Sint16 *stream, size_t stream_len)
{
    for (size_t i = 0; i < stream_len; ++i)
    {
        float gen_step = 1.0 / (gen->step_time * SAMPLE_DELTA_TIME);
        stream[i] = (gen->current + (gen->next - gen->current) * gen->a) * gen->volume;
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
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

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

    Gen gen = {
        .step_time = 10,
        .volume = 0.5,
    };
    static_assert(SLIDE_COUNT == 2, " Exhaustive defenition of sliders");
    float *fields[SLIDE_COUNT] = {
        [SLIDE_FREQ] = &gen.step_time,
        [SLIDE_VOLUME] = &gen.volume,
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
    SDL_PauseAudio(0);
    // bool dragging_freq = false;
    // bool dragging_volume = false;

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

        slider(renderer, 100, 100, 500, SLIDE_FREQ, fields[0], 1, 200);
        slider(renderer, 100, 200, 500, SLIDE_VOLUME, fields[1], 0, 1);
        SDL_RenderPresent(renderer);
    }
    printf("OK\n");

    SDL_Quit();
    return 0;
}