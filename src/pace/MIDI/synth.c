//
// main.c: The main program
//
// Joël Duet (joel.duet@gmail.com)
// This code is in the public domain
//
#include "clib/die.h"
#include "clib/mem.h"
#include "db.h"
#include "plugin_discovery.h"
#include "plugin_manager.h"
#include "pace.h"
#include <signal.h>   
#include <stdio.h>   
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <alloca.h>
#include <pthread.h>

#define POLY 10
#define ORC 1
#define GAIN 1.0
#define BUFSIZE 512
#define NCHNLS 2
#define BYTEPERSAMP 2

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif
#define INT_MAX (32767)
#define max( a, b ) ( ( a > b) ? a : b )
#define min( a, b ) ( ( a < b) ? a : b )

snd_seq_t *seq_synth_handle;
snd_pcm_t *playback_handle;
double pitch;
int note[POLY], gate[POLY], note_active[POLY];
int channel[POLY];
float onset[POLY];
unsigned int rate;
Post* post[ORC][POLY];
PluginManager* pm;
void* pdstate;
double env_time[POLY], env_level[POLY], velocity[POLY];
short* bufcard;
int port_in_id;
bool isRecording = 0;
    

    double attack = 0.01;
    double decay = 0.1;
    double sustain = 0.2;
    double release = 0.5;
    

snd_seq_t *open_seq() {

    rate = 44100;    
    if (snd_seq_open(&seq_synth_handle, "default", \
        SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        fprintf(stderr, "Error opening ALSA sequencer.\n");
        exit(1);
    }
    //snd_config_update_free_global();
    snd_seq_set_client_name(seq_synth_handle, "faustSynth");
    if ((port_in_id=snd_seq_create_simple_port(seq_synth_handle, \
        "faustSynth",
        SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
        fprintf(stderr, "Error creating sequencer port.\n");
        exit(1);
    }
    printf("\nOn client id: %d, ",snd_seq_client_id(seq_synth_handle));
    printf("\nsimple seq port created: %d\n",port_in_id);
    return(seq_synth_handle);
}

void do_claim_client(){    
	seq_synth_handle = open_seq();
}

int client_id(){
	return snd_seq_client_id(seq_synth_handle);
}

snd_pcm_t *open_pcm(char *pcm_name) {

    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
            
    if (snd_pcm_open (&playback_handle, \
        pcm_name, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        fprintf (stderr, "cannot open audio device %s\n", \
           pcm_name);
        exit (1);
    }
    //snd_config_update_free_global();
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(playback_handle, hw_params);
    snd_pcm_hw_params_set_access(playback_handle, \
              hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(playback_handle, \
              hw_params,SND_PCM_FORMAT_S16_LE); 
    snd_pcm_hw_params_set_rate_near(playback_handle, \
              hw_params, &rate, 0);
    printf("\n rate: %u\n",rate);
    snd_pcm_hw_params_set_channels(playback_handle, \
              hw_params, NCHNLS);
    snd_pcm_hw_params_set_periods(playback_handle, \
              hw_params, 2, 0);
    snd_pcm_hw_params_set_period_size(playback_handle, \
              hw_params, BUFSIZE, 0);
    snd_pcm_hw_params(playback_handle, hw_params);
   
    snd_pcm_sw_params_alloca(&sw_params);
    snd_pcm_sw_params_current(playback_handle, sw_params);
    snd_pcm_sw_params_set_avail_min(playback_handle, \
              sw_params, BUFSIZE);
    snd_pcm_sw_params(playback_handle, sw_params);
    return(playback_handle);
}

double wet[ORC][POLY];

FILE*melody;
float tick0, tick;
struct timespec tp;

int midi_callback() {

    snd_seq_event_t *ev;
    int l1;
  
    do {
        snd_seq_event_input(seq_synth_handle, &ev);
        switch (ev->type) {
            /*case SND_SEQ_EVENT_PITCHBEND:
                pitch = (double)ev->data.control.value / 8192.0;
                break;*/
	    
            /*case SND_SEQ_EVENT_CONTROLLER:
                if (ev->data.control.param == 1) {
                    modulation = \
                         (double)ev->data.control.value / 10.0;
                } 
                break;*/
            case SND_SEQ_EVENT_NOTEON:
            	clock_gettime(CLOCK_MONOTONIC,&tp);
            	tick = (float) tp.tv_sec + (float) tp.tv_nsec/1e9 - tick0;
                for (l1 = 0; l1 < POLY; l1++) {
                        if (!note_active[l1]) {
                                note[l1] = ev->data.note.note;
                                velocity[l1] = \
                                ev->data.note.velocity / 127.0;
                                env_time[l1] = 0;
                                gate[l1] = 1;
                                note_active[l1] = 1;
				onset[l1] = tick;
				channel[l1] = ev->data.note.channel;
                                wet[channel[l1]][l1]=0.93;
                                break;
                        }
                }
                break;        
            case SND_SEQ_EVENT_NOTEOFF:
            	clock_gettime(CLOCK_MONOTONIC,&tp);
            	tick = (float) tp.tv_sec + (float) tp.tv_nsec/1e9 - tick0;
            	for (l1 = 0; l1 < POLY; l1++) {
                    if (gate[l1] && \
                         note_active[l1] && \
                        (note[l1] == ev->data.note.note)) {
                        env_time[l1] = 0;
                        gate[l1] = 0;
			if(isRecording) fprintf(melody,"i %d %f %f %f %f\n", 1 + channel[l1], onset[l1], (tick - onset[l1]), (float)(note[l1]/12),(float)(note[l1]%12));
                    }
                }
                break;        
        }
        snd_seq_free_event(ev);
    } while (snd_seq_event_input_pending(seq_synth_handle, 0) > 0);
    return (0);
}

double envelope(int *note_active, int gate, double *env_level, \
                double t, \
                double attack, double decay, \
                double sustain, double release) {
    if (gate)  {
        if (t > attack + decay) return(*env_level = sustain);
        if (t > attack) return(*env_level = \
                      1.0 - (1.0 - sustain) * \
                      (t - attack) / decay);
        return(*env_level = t / attack);
    } else {
        if (t > release) {
            if (note_active) *note_active = 0;
            return(*env_level = 0);
        }
        return(*env_level * (1.0 - t / release));
    }
}
    
int playback_callback (snd_pcm_sframes_t nframes) {
    	double freq_note;
    	int l1, l2;
    
    	double **buf;
    
    	memset(bufcard, 0, BYTEPERSAMP * NCHNLS * nframes);

    	for (l1 = 0; l1 < POLY; l1++) {
        	if (note_active[l1]) {
          
	    		freq_note = 8.176 * exp((double)(note[l1]) * \
                 		log(2.0)/12.0);
	    		Post_set_freq(post[channel[l1]][l1],freq_note);
	    		wet[channel[l1]][l1] *= 0.995;
	    		Post_set_wet(post[channel[l1]][l1],wet[channel[l1]][l1]);
                	synthesize(pm, post[channel[l1]][l1]);
        		buf = Post_get_buf(post[channel[l1]][l1]);
	    
	    		for (l2 = 0; l2 < nframes; l2++) {
	        		int c=0; // left
				double sound = buf[c][l2];
		 		sound *= GAIN*envelope(&note_active[l1], \
                           		gate[l1], 
                           		&env_level[l1], 
                           		env_time[l1], 
                           		attack, decay, sustain, release)
                             		* velocity[l1]/POLY;

                		bufcard[c+l2*2] += (short)(sound*(double)INT_MAX);
                		bufcard[1+l2*2] = bufcard[c+l2*2];
 		
				env_time[l1] += 1.0 / 44100.0;
		
	    		}
		}
    	}
    	
    	return snd_pcm_writei (playback_handle, bufcard, nframes); 
}

double** createArray(int m, int n){
    double* values = calloc(m*n, sizeof(double));
    
    double** rows = malloc(n*sizeof(double*));
    for (int i=0; i<n; i++)
    {
        rows[i] = values + i*m;
    }
    return rows;
}

void startRecord(int sig){
	FILE* temp_file = fopen("temp_file","r");
	if(!fgets(record_file_name,80,temp_file))
		printf("Error record file name.\n");
	fclose(temp_file);
	melody=fopen(record_file_name,"w");
	if(!melody) printf("Error record.\n");
	isRecording = 1;
}

void stopRecord(int sig){
	fclose(melody);
	isRecording = 0;
}

void synth(char* hw, int keyboard){
    int nfds, seq_nfds, l1;
    struct pollfd *pfds;

    signal(SIGUSR1, startRecord);
    signal(SIGUSR2, stopRecord);
 
    pitch = 0;

    playback_handle = open_pcm(hw);
    // call to do_claim_client() should have occured
	snd_seq_connect_from (seq_synth_handle, port_in_id, keyboard, 0);

    seq_nfds = \
         snd_seq_poll_descriptors_count(seq_synth_handle, POLLIN);
    nfds = snd_pcm_poll_descriptors_count (playback_handle);
    pfds = (struct pollfd *)alloca(sizeof(struct pollfd) * \
         (seq_nfds + nfds));
    snd_seq_poll_descriptors(seq_synth_handle, pfds, seq_nfds, POLLIN);
    snd_pcm_poll_descriptors (playback_handle, 
                             pfds+seq_nfds, nfds);
    
    bufcard = (short*) malloc (BYTEPERSAMP * NCHNLS * BUFSIZE);
    
    dstring id;
    // The Post object takes ownership of the strings passed to it
    
    double ** buf;
    for(int instr=0; instr<ORC; instr++)
	for(int i=0; i<POLY; i++){
		id = dstring_new("logan");
            	buf= createArray(BUFSIZE,2);
            	post[instr][i] = Post_new(id,i+1,rate,BUFSIZE,buf);

            	wet[instr][i] = 0.93;
    	}

    // Perform plugin discovery in the "plugins" directory relative to the
    // working directory.
    pm = PluginManager_new();
    dstring dirname = dstring_new("src/pace/MIDI/plugin");
    pdstate = discover_plugins(dirname, pm);  
    dstring_free(dirname);

    for (l1 = 0; l1 < POLY; note_active[l1++] = 0);
    
    clock_gettime(CLOCK_MONOTONIC,&tp);
    tick0 = (float) tp.tv_sec + (float) tp.tv_nsec/1e9;
    
    while (1) {
        if (poll (pfds, seq_nfds + nfds, 1000) > 0) {
            for (l1 = 0; l1 < seq_nfds; l1++) {
               if (pfds[l1].revents > 0) midi_callback();
            }
            for (l1 = seq_nfds; l1 < seq_nfds + nfds; l1++) {    
                if (pfds[l1].revents > 0) { 
                    if (playback_callback(BUFSIZE) < BUFSIZE) {
                        fprintf (stderr, "xrun !\n");
                        snd_pcm_prepare(playback_handle);
                    }
                }
            }        
        }
    }
}
     
   
