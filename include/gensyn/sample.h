#ifndef H_GENSYN_SAMPLE__INCLUDED
#define H_GENSYN_SAMPLE__INCLUDED

#include <stdint.h>
#include <stddef.h>
#ifndef NULL 
#define NULL ((void*)0x0)
#endif


#define gensyn_sample_min 0.f
#define gensyn_sample_max 1.f
typedef float gensyn_sample_t;


// C0
#define gensyn_min_pitch_hz 16.35 

// B8
#define gensyn_max_pitch_hz 7902.08




#define gensyn_pitch_sample_to_hz(__P__) ((__P__)*(gensyn_max_pitch_hz - gensyn_min_pitch_hz) + gensyn_min_pitch_hz)

#define gensyn_pitch_hz_to_sample(__P__) (((__P__)-gensyn_min_pitch_hz)/(gensyn_max_pitch_hz - gensyn_min_pitch_hz))








#endif
