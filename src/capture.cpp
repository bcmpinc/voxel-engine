/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013,2014  B.J. Conijn <bcmpinc@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef IN_IDE_PARSER
# define FOUND_LIBAV
#endif

#include <cstdio>

#include "capture.h"

#ifdef FOUND_LIBAV

#define __STDC_CONSTANT_MACROS
extern "C" { 
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

static void check_return(int ret, const char * desc) {
    if (ret < 0) {
        char error[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(error, AV_ERROR_MAX_STRING_SIZE, ret);
        fprintf(stderr, "%s: %s\n", desc, error);
        exit(1);
    }
}

struct CaptureData {
    AVFormatContext *oc;
    AVStream *video_st;
    AVFrame *picture;
    int video_outbuf_size;
    unsigned char *video_outbuf;
    SwsContext *img_convert_ctx;
    int frame_count;
    AVCodecContext * c;
    uint8_t * buffer;
    AVPacket pkt;

    CaptureData(const char* filename, uint8_t* data, int width, int height) { 
        av_register_all();
        AVOutputFormat * fmt = av_guess_format("mp4", NULL, NULL);
        if (fmt == nullptr) {
            fprintf(stderr, "Failed to select mp4 format.\n");
            exit(1);
        }
        oc = avformat_alloc_context();
        if (oc == nullptr) {
            fprintf(stderr, "Failed to allocate format context.\n");
            exit(1);
        }
        oc->oformat = fmt;
        snprintf(oc->filename, sizeof(oc->filename), "%s", filename);
        
        // add video stream 
        video_st  = avformat_new_stream(oc, 0);
        if (video_st == nullptr) {
            fprintf(stderr, "Failed to create video stream.\n");
            exit(1);
        }
        c = video_st->codec;
        c->codec_id = fmt->video_codec;
        c->codec_type = AVMEDIA_TYPE_VIDEO;
        c->bit_rate = 4000000;
        c->width = width;
        c->height = height;
        c->gop_size = 25;
        c->pix_fmt = PIX_FMT_YUV420P;
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
        c->time_base.den = video_st->time_base.den = 10;
        c->time_base.num = video_st->time_base.num = 1;
        
        av_dump_format(oc, 0, oc->filename, 1);

        /* now that all the parameters are set, we can open the 
        video codec and allocate the necessary encode buffers */
        /* find the video encoder */
        AVCodec *codec = avcodec_find_encoder(c->codec_id);
        if (codec == nullptr) {
            fprintf(stderr, "Failed to find video codec.\n");
            exit(1);
        }
        
        /* open the codec */
        int ret = avcodec_open2(c, codec, nullptr);
        check_return(ret, "Could not open audio codec");

        /* allocate output buffer */
        video_outbuf_size = 1000000;
        video_outbuf = (uint8_t*)av_malloc(video_outbuf_size);
        
        /* allocate the encoded raw picture */
        picture = av_frame_alloc();
        picture->format = c->pix_fmt;
        picture->width = c->width;
        picture->height = c->height;
        int size = c->width * c->height;
        picture->data[0] = (uint8_t*)av_malloc((size * 3) / 2); /* size for YUV 420 */
        picture->data[1] = picture->data[0] + size;
        picture->data[2] = picture->data[1] + size / 4;
        picture->linesize[0] = c->width;
        picture->linesize[1] = c->width / 2;
        picture->linesize[2] = c->width / 2;
        
        img_convert_ctx = sws_getContext(
            c->width, c->height, PIX_FMT_BGRA, // <- from
            c->width, c->height, c->pix_fmt, // <- to
            SWS_BICUBIC, NULL, NULL, NULL);
        if (img_convert_ctx == NULL) {
            fprintf(stderr, "capture.cpp:Cannot initialize the conversion context\n");
            exit(1);
        }
        buffer = data;
        
        /* open the output file, if needed */
#ifndef AVIO_FLAG_WRITE
#define AVIO_FLAG_WRITE 2
#endif
        ret = avio_open(&oc->pb, oc->filename, AVIO_FLAG_WRITE);
        check_return(ret, "Could not open file");

        /* write the stream header, if any */
        ret = avformat_write_header(oc, nullptr);
        check_return(ret, "Failed to write header");
        
        frame_count=0;
        printf("capture.cpp: Movie capture started: %s\n", filename);

        pkt.data= video_outbuf;
        pkt.size= video_outbuf_size;
    }

    void shoot() {
        const uint8_t * const myrgb[4]={buffer,0,0,0};
        int mylinesize[4]={c->width*4,0,0,0};

        /* color convert picture */
        sws_scale(img_convert_ctx, (uint8_t**)myrgb, mylinesize,
                    0, c->height, picture->data, picture->linesize);

        /* encode the image */
        //int out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
        int got_packet=0;
        av_init_packet(&pkt);
        int ret = avcodec_encode_video2(c, &pkt, picture, &got_packet);
        check_return(ret, "Error encoding video frame");
        /* if no packet, it means the image was buffered */
        if (got_packet) {
            /* write the compressed frame in the media file */
            pkt.pts = pkt.dts = picture->pts = AV_NOPTS_VALUE; // Somehow necessary to fix the fps.
            pkt.stream_index= video_st->index;
            ret = av_interleaved_write_frame(oc, &pkt);
            check_return(ret, "Error while writing video frame");
        }
        //printf("capture.cpp: Written frame %d, size=%d\n",frame_count, out_size);
        frame_count++;
    }

    void flush() {
        for(;;) {
            int got_packet=0;
            av_init_packet(&pkt);
            int ret = avcodec_encode_video2(c, &pkt, nullptr, &got_packet);
            check_return(ret, "Error encoding video frame");
            if (!got_packet) break;
            /* write the compressed frame in the media file */
            pkt.pts = pkt.dts = picture->pts = AV_NOPTS_VALUE; // Somehow necessary to fix the fps.
            pkt.stream_index= video_st->index;
            ret = av_interleaved_write_frame(oc, &pkt);
            check_return(ret, "Error while writing video frame");
        }
    }
    
    void end()
    {
        sws_freeContext(img_convert_ctx);
        
        flush();
        
        /* write the trailer, if any.  the trailer must be written
         * before you close the CodecContexts open when you wrote the
         * header; otherwise write_trailer may try to use memory that
         * was freed on avcodec_close() */
        av_write_trailer(oc);
        av_free_packet(&pkt);

        /* close each codec */
        avcodec_close(video_st->codec);
        av_free(picture->data[0]);
        av_free(picture);
        av_free(video_outbuf);

        /* free the streams */
        for(uint i = 0; i < oc->nb_streams; i++) {
            av_freep(&oc->streams[i]->codec);
            av_freep(&oc->streams[i]);
        }

        avio_close(oc->pb);

        /* free the stream */
        av_free(oc);
        printf("capture.cpp: Movie saved! frames: %d\n",frame_count);
    }
};

Capture::Capture(const char* filename, uint8_t* data, int width, int height)
    : data(new CaptureData(filename, data, width, height)) {
}

void Capture::shoot() {
    if (data) {
        data->shoot();
    }
}
    
void Capture::end() {
    if (data) {
        data->end();
        delete data;
        data = nullptr;
    }
}

Capture::~Capture() {
    end();
}

#else
// FFMPEG is not available, so provide dummy implementations.
Capture::Capture(const char*, uint8_t*, int, int) : data(nullptr) {}
void Capture::shoot() {}
void Capture::end() {}
Capture::~Capture() {}

#endif
