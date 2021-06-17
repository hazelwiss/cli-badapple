#include<libavformat/avformat.h>
#include<libavcodec/avcodec.h>
#include<assert.h>
#include<sys/ioctl.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<video_decoder.h>
#include<audio_player.h>

typedef struct{
    int width; 
    int height;
} terminal_info;

static decode_video_frame frame;
static decode_audio_frame audio;
static decode_context context;
static WINDOW* win;
static audio_linked_list audio_list = {0,0,0};
static video_linked_list video_list = {0,0,0};
static int height, width;
static double milisec_per_frame;

static void init(terminal_info* info){
    initscr();
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    info->height = w.ws_row;
    info->width = w.ws_col;
}

static void video_player_func(decode_video_frame* frame){
    werase(win);
    for(int h = 0; h < height; ++h){
        for(int w = 0; w < width; ++w){
            decode_pixel cur_p = frame->frame[h*frame->linesize/sizeof(*frame->frame)+w];
            int colour_attr = cur_p.red > 0x88 && cur_p.green > 0x88 && cur_p.blue > 0x88 ? '#' : ' ';
            mvwaddch(win, h, w, colour_attr);
        }
    }
    wrefresh(win);
}

static void frame_loader(){
    int is_video;
    while(vd_read_frame(context, &frame, &audio, width, height, &is_video)){
        if(is_video == VIDEO){
            add_video_to_linked_list(&video_list, frame);
        } else{
            add_audio_to_linked_list(&audio_list, audio);
        }
    }
}

static const char* filename = "badapple.mp4";
int main(){
    terminal_info info;
    double frame_rate;
    init(&info);
    win = newwin(info.height,info.width,0,0);
    context = vd_alloc_context();
    vd_open_context_file(context, filename);
    frame_rate = vd_get_framerate_from_context(context);
    height = info.height;
    width = info.width;
    frame.frame = malloc(height*width*sizeof(*frame.frame)*2);  //  allocates double of what's necessary to not risk overflow.
    milisec_per_frame = 1000/frame_rate;
    add_audio_to_linked_list(&audio_list, (decode_audio_frame){{0,0}});
    init_audio(44100, 2, 1024, BUFSIZ, &audio_list, audio_list.beg);
    stop_audio();
    add_video_to_linked_list(&video_list, (decode_video_frame){.frame=malloc(info.width*info.height)});
    frame_loader();
    play_audio();
    video_linked_list_entry* entry = video_list.beg;
    while(video_list.count > 0){
        clock_t ticks = clock();
        double miliseconds_lapsed;
        if(entry->next){
            video_player_func(&entry->entry);
            entry = entry->next;
            remove_first_from_linked_video_list(&video_list);
        }
        ticks = clock()-ticks;
        miliseconds_lapsed = ((double)ticks*1000)/CLOCKS_PER_SEC;
        usleep(((int)(milisec_per_frame-miliseconds_lapsed))*1000);
    }
    free(frame.frame);
    endwin();
    destroy_audio();
    exit(EXIT_SUCCESS);
}