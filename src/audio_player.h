#pragma once
#include<video_decoder.h>

typedef struct audio_linked_list_entry{
    decode_audio_frame entry;
    struct audio_linked_list_entry* next;
} audio_linked_list_entry;

typedef struct{
    int count, removed;
    audio_linked_list_entry* beg;
    audio_linked_list_entry* end;
} audio_linked_list;

void init_audio(int frequency, int channels, int samples, int size, audio_linked_list* list, audio_linked_list_entry* entry);
void destroy_audio();
void add_audio_to_linked_list(audio_linked_list* list, decode_audio_frame entry);
void remove_first_from_linked_audio_list(audio_linked_list* list);
void stop_audio();
void play_audio();