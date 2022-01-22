#include <SDL.h>
#include <stdio.h>
#include <assert.h>

size_t const FREK = 48000;
float const SAMPLE_DELTA_TIME = 1.0 / FREK;
size_t const GEN_STEP = 1.0 / (20.0 * SAMPLE_DELTA_TIME);

int16_t sgn(int16_t x)
{
    return ((x > 0) - (x < 0));
}

struct Gen
{
    int16_t current;
    int16_t next;
    float a;
};

void white_noise(Gen *gen, Sint16 *stream, size_t stream_len)
{
    for (size_t i = 0; i < stream_len; ++i)
    {
        stream[i] = gen->current + int16_t(gen->next - gen->current) * gen->a;
        printf("%d\n", stream[i]);
        gen->a += GEN_STEP * SAMPLE_DELTA_TIME;
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
    white_noise(reinterpret_cast<Gen *>(userdata), reinterpret_cast<Sint16 *>(stream), len / 2);
}

int main(void)
{

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    Gen gen{};

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
    while (!quite)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quite = true;
                printf("quite");
                break;

            default:
                break;
            }
        }
    }

    printf("OK\n");

    SDL_Quit();
    return 0;
}