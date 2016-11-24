//
// db.c: Mock database implementation
// 
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//
#include "db.h"
#include "string.h"

#include "clib/mem.h"


struct Post_t {
    dstring id;
    int instance;
    unsigned int rate;
    double freq;
    double wet;
    int buf_size;
    double **buf;
};

Post* Post_new(dstring id, int instance, unsigned int rate, int buf_size,double **buf) {
    Post* post = mem_alloc(sizeof(*post));
    post->id = id;
    post->instance = instance;
    post->rate = rate;
    post->buf_size = buf_size;
    post->buf = buf;
    post->freq = 0.0;
    post->wet = 0.0;
    return post;
}

void Post_free(Post* post) {
    dstring_free(post->id);
    mem_free(post);
}

dstring Post_get_id(Post* post) {
    return post->id;
}

int Post_get_instance(Post* post) {
    return post->instance;
}

unsigned int Post_get_rate(Post* post) {
    return post->rate;
}

int Post_get_buf_size(Post* post){
	return post->buf_size;
}

double** Post_get_buf(Post* post){
	return post->buf;
}

void Post_set_freq(Post* post, double freq){
	post->freq = freq;
}

double Post_get_freq(Post* post){
	return post->freq;
}

void Post_set_wet(Post* post, double wet){
	post->wet = wet;
}

double Post_get_wet(Post* post){
	return post->wet;
}
