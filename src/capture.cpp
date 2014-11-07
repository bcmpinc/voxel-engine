/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013  B.J. Conijn <bcmpinc@users.sourceforge.net>

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


#define __STDC_CONSTANT_MACROS
extern "C" { 
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cassert>
#include <cstdio>

#define STREAM_FRAME_RATE 15 // FPS

#include "art.h"

static AVFormatContext *oc;
static AVStream *video_st;
static AVFrame *picture;
static int video_outbuf_size;
static unsigned char *video_outbuf;
static SwsContext *img_convert_ctx;
static int frame_count;
static AVCodecContext * c;
static GLuint pfbo, prbo;
static unsigned char * buffer;

bool checkFramebufferStatus()
{
    // check FBO status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
        fprintf(stderr, "Framebuffer complete.\n");
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        fprintf(stderr, "[ERROR] Framebuffer incomplete: Attachment is NOT complete.\n");
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        fprintf(stderr, "[ERROR] Framebuffer incomplete: No image is attached to FBO.\n");
        return false;
/*
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        fprintf(stderr, "[ERROR] Framebuffer incomplete: Attached images have different dimensions.\n");
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
        fprintf(stderr, "[ERROR] Framebuffer incomplete: Color attached images have different internal formats.\n");
        return false;
*/
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        fprintf(stderr, "[ERROR] Framebuffer incomplete: Draw buffer.\n");
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        fprintf(stderr, "[ERROR] Framebuffer incomplete: Read buffer.\n");
        return false;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        fprintf(stderr, "[ERROR] Framebuffer incomplete: Unsupported by FBO implementation.\n");
        return false;

    default:
        fprintf(stderr, "[ERROR] Framebuffer incomplete: Unknown error.\n");
        return false;
    }
}

void capture_start(const char* filename) {
    av_register_all();
    AVOutputFormat * fmt = av_guess_format("mp4", NULL, NULL);
    assert(fmt);
    oc = avformat_alloc_context();
    assert(oc);
    oc->oformat = fmt;
    snprintf(oc->filename, sizeof(oc->filename), "%s", filename);
    
    // add video stream 
    video_st  = av_new_stream(oc, 0);
    assert(video_st);
    c = video_st->codec;
    c->codec_id = fmt->video_codec;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    c->bit_rate = 4000000;
    c->width = SCREEN_WIDTH;
    c->height = SCREEN_HEIGHT;
    c->time_base.den = STREAM_FRAME_RATE;
    c->time_base.num = 1;
    c->gop_size = STREAM_FRAME_RATE; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = PIX_FMT_YUV420P;
    c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    av_dump_format(oc, 0, oc->filename, 1);

    /* now that all the parameters are set, we can open the 
       video codec and allocate the necessary encode buffers */
    /* find the video encoder */
    AVCodec *codec = avcodec_find_encoder(c->codec_id);
    assert(codec);
    
    /* open the codec */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "capture.cpp:could not open codec\n");
        exit(1);
    }

    /* allocate output buffer */
    video_outbuf_size = 200000;
    video_outbuf = (uint8_t*)av_malloc(video_outbuf_size);
    
    /* allocate the encoded raw picture */
    picture = avcodec_alloc_frame();
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
    buffer = (uint8_t*)av_malloc(4*c->width*c->height);
    
    /* open the output file, if needed */
#ifndef AVIO_FLAG_WRITE
#define AVIO_FLAG_WRITE 2
#endif
    if (avio_open(&oc->pb, oc->filename, AVIO_FLAG_WRITE) < 0) {
        fprintf(stderr, "capture.cpp:Could not open %s\n", filename);
        exit(1);
    }

    /* write the stream header, if any */
    av_write_header(oc);
    
    frame_count=0;
    glGenFramebuffers(1, &pfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, pfbo);
    glGenRenderbuffers(1, &prbo);
    glBindRenderbuffer(GL_RENDERBUFFER, prbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, c->width, c->height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, prbo);
    
    checkFramebufferStatus();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
    printf("capture.cpp: Movie capture started: %s\n", filename);
}

static const glm::dmat4 frustum_matrix = glm::scale(glm::frustum<double>(frustum::left, frustum::right, frustum::bottom, frustum::top, frustum::near, frustum::far),glm::dvec3(1,1,-1));
void capture_shoot(uint32_t cubemap) {
    const uint8_t * const myrgb[4]={buffer,0,0,0};
    int mylinesize[4]={c->width*4,0,0,0};

    // Do attribute stuff
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    // Bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, pfbo);

    // OpenGL Viewport stuff
    glViewport(0,0,c->width,c->height);
    
    glMatrixMode(GL_PROJECTION);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // update context viewport size
    glLoadMatrixd(glm::value_ptr(frustum_matrix));
    glMatrixMode(GL_MODELVIEW);
    
    // Rendering
    glClear(GL_COLOR_BUFFER_BIT);
    draw_cubemap(cubemap);
    
    // Collect pixels
    glReadPixels(0,0,c->width,c->height,GL_BGRA,GL_UNSIGNED_BYTE,buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Undo attribute stuff
    glPopAttrib();
    
    // Flip image upside down (from OpenGL to av row order).
    int row = mylinesize[0];
    char t[row];
    for (int y=0; y<c->height/2; y++) {
        memcpy(t,&buffer[row*y],row);
        memcpy(&buffer[row*y],&buffer[row*(c->height-y-1)],row);
        memcpy(&buffer[row*(c->height-y-1)],t,row);
    }

    /* color convert picture */
    sws_scale(img_convert_ctx, (uint8_t**)myrgb, mylinesize,
                0, c->height, picture->data, picture->linesize);

    /* encode the image */
    int out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
    /* if zero size, it means the image was buffered */
    if (out_size > 0) {
        AVPacket pkt;
        av_init_packet(&pkt);

        if(c->coded_frame->key_frame)
            pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index= video_st->index;
        pkt.data= video_outbuf;
        pkt.size= out_size;

        /* write the compressed frame in the media file */
        int ret = av_interleaved_write_frame(oc, &pkt);
        assert(ret>=0);
    }
    printf("capture.cpp: Written frame %d, size=%d\n",frame_count, out_size);
    frame_count++;
}

void capture_end()
{
    glDeleteRenderbuffers(1, &prbo);
    glDeleteFramebuffers(1, &pfbo);
    
    sws_freeContext(img_convert_ctx);
    
    /* write the trailer, if any.  the trailer must be written
     * before you close the CodecContexts open when you wrote the
     * header; otherwise write_trailer may try to use memory that
     * was freed on av_codec_close() */
    av_write_trailer(oc);

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
