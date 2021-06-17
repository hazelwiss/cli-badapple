#include<video_decoder.h>
#include<libavformat/avformat.h>
#include<libavcodec/avcodec.h>
#include<libswscale/swscale.h>
#include<libswresample/swresample.h>
#include<libavutil/imgutils.h>
#include<sys/ioctl.h>
#include<stdio.h>
#include<unistd.h>

struct __decode_context{
    AVFormatContext* fc;
    AVCodecContext* ccv;
    AVCodecContext* cca;
    AVCodec* v_codec;
    AVCodec* a_codec;
    int v_stream_index;
    int a_stream_index;
    char file[MAX_INPUT];
    AVFrame* frame;
};

static void _abort(decode_context c, int error_code, char* error_str){
    printf("Aborting process with values:\n "
        "fc %p\n cc %p\n stream %d\n file %s\n", 
        c->fc, c->ccv, c->v_stream_index, c->file);
    unsigned long long int abort_str;
    abort_str = (int)-error_code;
    endwin();
    printf("%s\n", error_str);
    printf("Application aborted with error code: %d and error string: %s\n", error_code, (char*)&abort_str);
    exit(EXIT_FAILURE);
}

decode_context vd_alloc_context(){
    decode_context rtrn     = malloc(sizeof(*rtrn));
    rtrn->fc                = avformat_alloc_context();
    rtrn->ccv               = NULL;
    rtrn->cca               = NULL;
    rtrn->frame             = av_frame_alloc();
    rtrn->v_stream_index    = 0;
    rtrn->a_stream_index    = 0;
    return rtrn;
}
void vd_free_context(decode_context* c){
    if(*c){
        if((*c)->fc)
            avformat_close_input(&(*c)->fc);
        if((*c)->ccv)
            avcodec_free_context(&(*c)->ccv);
        if((*c)->cca)
            avcodec_free_context(&(*c)->cca);
        if((*c)->frame)
            av_frame_free(&(*c)->frame);
        free(*c);
    }
}

int vd_open_context_file(decode_context c, const char* filename){
    int result = 0;
    memset(c->file, 0, sizeof(c->file));
    memcpy(c->file, filename, strlen(filename));
    if((result=avformat_open_input(&c->fc, filename, NULL, NULL)) < 0){
        _abort(c, result, "error opening format context from the opened file. Aborting...\n");
    }
    if((c->v_stream_index=av_find_best_stream(c->fc, VIDEO, -1, -1, &c->v_codec, 0)) == AVERROR_STREAM_NOT_FOUND){
        _abort(c, AVERROR_STREAM_NOT_FOUND, "no suitable video decoding stream found. Aborting...\n");
    }
    if((c->a_stream_index=av_find_best_stream(c->fc, AUDIO, -1, -1, &c->a_codec, 0)) == AVERROR_STREAM_NOT_FOUND){
        _abort(c, AVERROR_STREAM_NOT_FOUND, "no suitable audio decoding stream found. Aborting...\n");
    }
    c->ccv = avcodec_alloc_context3(c->v_codec);
    c->cca = avcodec_alloc_context3(c->a_codec);
    if(!c->ccv){
        _abort(c, -1, "couldn't allocate a codec context from given video codec. Aborting...\n");
    }
    if(!c->cca){
        _abort(c, -1, "couldn't allocate a codec context from given audio codec. Aborting...\n");
    }
    if((result=avcodec_parameters_to_context(c->ccv, c->fc->streams[c->v_stream_index]->codecpar)) < 0){
        _abort(c, result, "failed to deduce parameters from the given video stream. Aborting...\n");
    }
    if((result=avcodec_parameters_to_context(c->cca, c->fc->streams[c->a_stream_index]->codecpar)) < 0){
        _abort(c, result, "failed to deduce parameters from the given audio stream. Aborting...\n");
    }
    if((result=avcodec_open2(c->ccv, c->v_codec, NULL))){
        _abort(c, result, "couldn't open the video codec context with the given codec. Aborting...\n");
    }
    if((result=avcodec_open2(c->cca, c->a_codec, NULL))){
        _abort(c, result, "couldn't open the audio coded context with the given codec. Aborting...\n");
    }
    return 0;
}

static int decode_video(decode_context c, AVPacket* packet, decode_video_frame* input, int width, int height){
    int result = 0;
    AVFrame* frame_rgb = av_frame_alloc();
    struct SwsContext* ctx;
    decode_dimensions dimensions = vd_get_dimensions_from_context(c);
    if((result=avcodec_send_packet(c->ccv, packet)) < 0){
        _abort(c, result, "unable to send video packet to be decoded. Aborting...\n");
    }
    if((result=avcodec_receive_frame(c->ccv, c->frame)) < 0){
        if(result == AVERROR(EAGAIN) || result == AVERROR_EOF)
            return 0;
        if(result < 0){
            _abort(c, result, "error receiving frame. Aborting...\n");
        }
    }
    av_image_alloc(frame_rgb->data, frame_rgb->linesize, width, height, AV_PIX_FMT_RGB32, 16);
    input->width = frame_rgb->width = width;
    input->height = frame_rgb->height = height;
    input->linesize = frame_rgb->linesize[0];
    ctx = sws_getContext(dimensions.width, dimensions.height, 
        get_pixel_format_from_context(c), width, height, AV_PIX_FMT_RGB32, SWS_POINT, NULL, NULL, NULL);
    sws_scale(ctx, (const uint8_t**)c->frame->data, c->frame->linesize, 0, c->frame->height, frame_rgb->data, frame_rgb->linesize); 
    memcpy(input->frame, frame_rgb->data[0], frame_rgb->linesize[0]*frame_rgb->height);
    sws_freeContext(ctx);
    av_freep(frame_rgb->data);
    av_frame_unref(frame_rgb);  //  maybe uneccessary?
    av_frame_free(&frame_rgb);
    return 1;
}

static int decode_audio(decode_context c, AVPacket* packet, decode_audio_frame* input){
    int result = 0;
    SwrContext* ctx;
    AVFrame* decoded_audio = av_frame_alloc();
    if((result=avcodec_send_packet(c->cca, packet)) < 0){
        _abort(c, result, "unable to send audio packet to be decoded. Aborting...\n");
    }
    if((result=avcodec_receive_frame(c->cca, c->frame)) < 0){
        _abort(c, result, "error receiving audio frame. Aborting...\n");
    }
    ctx = swr_alloc_set_opts(NULL, c->frame->channel_layout, AV_SAMPLE_FMT_S16, c->frame->sample_rate, 
        c->frame->channel_layout, c->frame->format, c->frame->sample_rate, 0, NULL);
    swr_init(ctx);
    decoded_audio->data[0] = malloc(c->frame->linesize[0]*3);
    swr_convert(ctx, decoded_audio->data, 
        c->frame->nb_samples, (const uint8_t**)c->frame->data, c->frame->nb_samples);
    memcpy(input->buffer, *decoded_audio->data, BUFSIZ);
    av_frame_unref(decoded_audio);
    av_frame_free(&decoded_audio);
    return 1;
}

int vd_read_frame(decode_context c, decode_video_frame* v_input, decode_audio_frame* a_input, int width, int height, int* decoded_type){
    AVPacket packet; 
    int result = 0;
    packet.data = NULL;
    packet.size = 0;
    result = av_read_frame(c->fc, &packet);
    if(packet.stream_index == VIDEO){
        *decoded_type = VIDEO;
        result = decode_video(c, &packet, v_input, width, height);
    } else if(packet.stream_index == AUDIO){
        *decoded_type = AUDIO;
        result = decode_audio(c, &packet, a_input);
    } else{
        endwin();
        printf("invalid streaming index %d.\n", packet.stream_index);
        abort();
    }
    av_frame_unref(c->frame);
    av_packet_unref(&packet);
    return result;
}

double vd_get_framerate_from_context(decode_context c){
    return av_q2d(c->fc->streams[c->v_stream_index]->r_frame_rate);
}

decode_dimensions vd_get_dimensions_from_context(decode_context c){
    decode_dimensions rtrn;
    rtrn.height = c->ccv->height;
    rtrn.width = c->ccv->width;
    return rtrn;
}

int get_pixel_format_from_context(decode_context c){
    return c->ccv->pix_fmt;
}

decode_audio_info get_audio_info_from_context(decode_context c){
    return (decode_audio_info){ 
        .channels = c->cca->channels,
        .freq = c->cca->sample_rate
    };
}

void add_video_to_linked_list(video_linked_list* list, decode_video_frame entry){
    video_linked_list_entry* new_entry = malloc(sizeof(video_linked_list_entry));
    new_entry->entry = entry;
    new_entry->entry.frame = malloc(entry.linesize*entry.height*sizeof(entry.frame));
    memcpy(new_entry->entry.frame, entry.frame, entry.linesize*entry.height);
    new_entry->next = NULL;
    if(!list->beg)
        list->beg = new_entry;
    else
        list->end->next = new_entry;
    list->end = new_entry; 
    ++list->count;
}

void remove_first_from_linked_video_list(video_linked_list* list){
    video_linked_list_entry* tmp = list->beg;
    list->beg = list->beg->next;
    free(tmp->entry.frame);
    free(tmp);
    --list->count;
    ++list->removed;
}