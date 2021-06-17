#include<audio_player.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<memory.h>
#include <assert.h>
#include<SDL2/SDL.h>

struct tmp_cont{
    audio_linked_list* list;
    audio_linked_list_entry* entry;
} static tmp_container;
static SDL_AudioSpec audio_spec;

void fill_audio(void *ud, unsigned char *s, int l){
    audio_linked_list_entry* entry = tmp_container.entry;
    __attribute((unused)) audio_linked_list* list = tmp_container.list;
    int audio_l;
    char* audio_p;
    audio_l = l;
    audio_p = entry->entry.buffer;
    if(audio_l == 0)
        return;
    l = (l > audio_l ? audio_l: l);
    while(audio_l > 0){
        memset(s, 0, l);
        SDL_MixAudio(s, (const Uint8*)audio_p, l, 25);
        audio_p+=l;
        audio_l-=l;
    }
    while(!entry->next);
    tmp_container.entry = entry->next;
    remove_first_from_linked_audio_list(tmp_container.list);
}
void init_audio(int frequency, int channels, int samples, int size, 
    audio_linked_list* list, audio_linked_list_entry* entry)
{
    assert(SDL_Init(SDL_INIT_AUDIO) == 0);
    audio_spec.freq = frequency;
    audio_spec.channels = channels;
    audio_spec.samples = samples;
    audio_spec.size = size;
    audio_spec.callback = fill_audio;
    audio_spec.userdata = NULL;
    audio_spec.format = AUDIO_S16;
    audio_spec.silence = 0;
    assert(SDL_OpenAudio(&audio_spec, NULL) >= 0);
    SDL_PauseAudio(0);
    tmp_container.entry = entry;
    tmp_container.list = list;
}

void destroy_audio(){
    SDL_CloseAudio();
}

void add_audio_to_linked_list(audio_linked_list* list, decode_audio_frame entry){
    audio_linked_list_entry* new_entry = malloc(sizeof(audio_linked_list_entry));
    new_entry->entry = entry;
    new_entry->next = NULL;
    if(!list->beg){
        list->beg = new_entry;
    } else{
        list->end->next = new_entry;
    }
    list->end = new_entry;
    ++list->count;
};

void remove_first_from_linked_audio_list(audio_linked_list* list){
    audio_linked_list_entry* tmp = list->beg;
    list->beg = list->beg->next;
    free(tmp);
    --list->count;
    ++list->removed;
}

void stop_audio(){
    SDL_PauseAudio(1);
}

void play_audio(){
    SDL_PauseAudio(0);
}