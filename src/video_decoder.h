#pragma once
#include<ncurses.h>
#include<libavutil/avutil.h>

typedef struct __decode_context* decode_context;
typedef struct{
    int width, height;
} decode_dimensions;
typedef struct{
    int freq;
    int channels;
    int samples;
    int size;
} decode_audio_info;
typedef struct{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} decode_pixel;
typedef struct{
    decode_pixel* frame;
    int width, height;
    int linesize;
} decode_video_frame;
typedef struct{
    char buffer[BUFSIZ];
} decode_audio_frame;
typedef struct video_linked_list_entry{
    decode_video_frame entry;
    struct video_linked_list_entry* next;
} video_linked_list_entry;
typedef struct{
    int count, removed;
    video_linked_list_entry* beg, *end;
} video_linked_list;
typedef enum { VIDEO = AVMEDIA_TYPE_VIDEO, AUDIO = AVMEDIA_TYPE_AUDIO } DECODE_TYPE;

decode_context vd_alloc_context();
void vd_free_context(decode_context*);
int vd_open_context_file(decode_context, const char* filename);
int vd_read_frame(decode_context, decode_video_frame* v_input, decode_audio_frame* a_input, int width, int height, int* decoded_type);
double vd_get_framerate_from_context(decode_context);
decode_dimensions vd_get_dimensions_from_context(decode_context);
int get_pixel_format_from_context(decode_context);
__attribute((deprecated))
decode_audio_info get_audio_info_from_context(decode_context);
void add_video_to_linked_list(video_linked_list* list, decode_video_frame entry);
void remove_first_from_linked_video_list(video_linked_list* list);