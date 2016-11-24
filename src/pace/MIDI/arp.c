/* miniArp.c by Matthias Nagorni */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <alsa/asoundlib.h>
#include <alloca.h>
#include "pace.h"

#define TICKS_PER_QUARTER 1152
#define MAX_SEQ_LEN        640

int queue_id, port_in_id, port_out_id, \
     seq_transpose, tempo, swing, \
     sequence[4][MAX_SEQ_LEN], seq_len;
float bpm, bpm0;
char seq_filename[1024];
snd_seq_tick_time_t tick;
int midi_channel_out;

snd_seq_t *seq_arp_handle;

snd_seq_t *open_sequencer() {

  if (snd_seq_open(&seq_arp_handle, 
     "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    fprintf(stderr, "Error opening ALSA sequencer.\n");
    exit(1);
  }
  snd_seq_set_client_name(seq_arp_handle, "miniArp");
  if ((port_out_id = snd_seq_create_simple_port(seq_arp_handle, 
            "miniArpOut",
            SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
            SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
    fprintf(stderr, "Error creating sequencer port.\n");
  }
  if ((port_in_id = snd_seq_create_simple_port(seq_arp_handle, 
            "miniArpIn",
            SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
            SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
    fprintf(stderr, "Error creating sequencer port.\n");
  }

  snd_seq_connect_to(seq_arp_handle,port_out_id,midi_channel_out,0);
  return(seq_arp_handle);
}

void set_tempo() {

  snd_seq_queue_tempo_t *queue_tempo;

  snd_seq_queue_tempo_malloc(&queue_tempo);
  tempo = (int)(6e7 / bpm);
  snd_seq_queue_tempo_set_tempo(queue_tempo, tempo);
  snd_seq_queue_tempo_set_ppq(queue_tempo, TICKS_PER_QUARTER);
  snd_seq_set_queue_tempo(seq_arp_handle, queue_id, queue_tempo);
  snd_seq_queue_tempo_free(queue_tempo);
}

snd_seq_tick_time_t get_tick() {

  snd_seq_queue_status_t *status;
  snd_seq_tick_time_t current_tick;
  
  snd_seq_queue_status_malloc(&status);
  snd_seq_get_queue_status(seq_arp_handle, queue_id, status);
  current_tick = snd_seq_queue_status_get_tick_time(status);
  snd_seq_queue_status_free(status);
  return(current_tick);
}

void init_queue() {

  queue_id = snd_seq_alloc_queue(seq_arp_handle);
  snd_seq_set_client_pool_output(seq_arp_handle, (seq_len<<1) + 4);
}

void clear_queue() {

  snd_seq_remove_events_t *remove_ev;

  snd_seq_remove_events_malloc(&remove_ev);
  snd_seq_remove_events_set_queue(remove_ev, queue_id);
  // delete output / keep NOTE_OFF
  snd_seq_remove_events_set_condition(remove_ev,
            SND_SEQ_REMOVE_OUTPUT | SND_SEQ_REMOVE_IGNORE_OFF);
  snd_seq_remove_events(seq_arp_handle, remove_ev);
  snd_seq_remove_events_free(remove_ev);
}

void arpeggio() {

  snd_seq_event_t ev;
  int l1;
  double dt;
 
  for (l1 = 0; l1 < seq_len; l1++) {
    dt = (l1 % 2 == 0) ? \
         (double)swing / 16384.0 : \
         -(double)swing / 16384.0;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_note(&ev, sequence[3][l1], 
             sequence[2][l1] + seq_transpose, 
             127, 
             sequence[1][l1]);
    snd_seq_ev_schedule_tick(&ev, queue_id,  0, tick);
    snd_seq_ev_set_source(&ev, port_out_id);
    snd_seq_ev_set_subs(&ev);
    snd_seq_event_output_direct(seq_arp_handle, &ev);
    tick += (int)((double)sequence[0][l1] * (1.0 + dt));
  }
  snd_seq_ev_clear(&ev);
  
  ev.type = SND_SEQ_EVENT_ECHO; 
  snd_seq_ev_schedule_tick(&ev, queue_id,  0, tick);
  snd_seq_ev_set_dest(&ev, 
        snd_seq_client_id(seq_arp_handle), port_in_id);
  snd_seq_event_output_direct(seq_arp_handle, &ev);
}

void midi_action() {

  snd_seq_event_t *ev;

  do {
    snd_seq_event_input(seq_arp_handle, &ev);
    switch (ev->type) {
      case SND_SEQ_EVENT_ECHO:
        arpeggio(); 
        break;
      case SND_SEQ_EVENT_NOTEON:
        clear_queue();
        seq_transpose = ev->data.note.note - 60;
        tick = get_tick();
        arpeggio();
        break;        
      case SND_SEQ_EVENT_CONTROLLER:
        if (ev->data.control.param == 1) {           
          bpm = bpm0 * \
                 (1.0f + (float)ev->data.control.value / 127.0f);
          set_tempo();
        } 
        break;
      case SND_SEQ_EVENT_PITCHBEND:
        swing = (double)ev->data.control.value;        
        break;
    }
    snd_seq_free_event(ev);
  } while (snd_seq_event_input_pending(seq_arp_handle, 0) > 0);
}

void parse_sequence() {

  FILE *f;
  char c;
  
  if (!(f = fopen(seq_filename, "r"))) {
    fprintf(stderr, "Couldn't open sequence file %s\n", \
            seq_filename);
    exit(1);
  }
  
  int instr;
  int dur_integer;
  int num, den;
  int oct_ref;
  int oct;
  int cpt = 0;
  int dur_den, dur_num, len_den, len_num;
  if(fscanf(f,"%04d %02d %04d %02d ", &dur_integer, &num, &den, &oct_ref)<0)
  	printf("Problem with reading the file.\n");
  seq_len = 0;
  while((c = fgetc(f))!=EOF) {
  	oct = oct_ref;
  	while(c=='-'){
  		oct--;
  		c=fgetc(f);
  	} 
  	while(c=='+'){
  		oct++;
  		c=fgetc(f);
  	} 
    
    	switch (c) {
      		case 'c': 
        		sequence[2][seq_len] = 0; break; 
		case 'd': 
        		sequence[2][seq_len] = 2; break;
      		case 'e': 
        		sequence[2][seq_len] = 4; break;
      		case 'f': 
        		sequence[2][seq_len] = 5; break;
      		case 'g': 
        		sequence[2][seq_len] = 7; break;
      		case 'a': 
        		sequence[2][seq_len] = 9; break;
      		case 'b': 
        		sequence[2][seq_len] = 11; break;
    	}                    
    	c =  fgetc(f);
    	if (c == '#') {
      		sequence[2][seq_len]++;
      		c = fgetc(f);
   	}
    	sequence[2][seq_len] += 12 * oct;

   	dur_num = atoi(&c);
 	c = fgetc(f);
   	dur_num = 10*dur_num + atoi(&c);
   	c = fgetc(f);
   	dur_den = atoi(&c);
  	c = fgetc(f);
   	dur_den = 10*dur_den + atoi(&c);
  	c = fgetc(f);
   	dur_den = 10*dur_den + atoi(&c);
  	c = fgetc(f);
   	dur_den = 10*dur_den + atoi(&c);
  
   	if(dur_den!=0){
   	
   		 sequence[0][seq_len] = dur_num * TICKS_PER_QUARTER / dur_den; 
   		 cpt += dur_num * TICKS_PER_QUARTER / dur_den; 
   	}
   	else sequence[0][seq_len] = 0;

   	c = fgetc(f);
   	len_num = atoi(&c);
  	c = fgetc(f);
   	len_num = 10*len_num + atoi(&c);
   	c = fgetc(f);
   	len_den = atoi(&c);
  	c = fgetc(f);
   	len_den = 10*len_den + atoi(&c);
  	c = fgetc(f);
   	len_den = 10*len_den + atoi(&c);
  	c = fgetc(f);
   	len_den = 10*len_den + atoi(&c);
  	sequence[1][seq_len] = len_num * TICKS_PER_QUARTER / len_den;
   	
   	c = fgetc(f);
   	instr = atoi(&c);
  	c = fgetc(f);
   	instr = 10*instr + atoi(&c);
  	c = fgetc(f);
   	instr = 10*instr+ atoi(&c);
  	sequence[3][seq_len] = instr;
  	
   	seq_len++;
  	}
  fclose(f);
  float decimal = den == 0 ? 0.0f : (float) num / (float) den;
  bpm =  60 * (float) cpt / (((float) dur_integer + decimal) * TICKS_PER_QUARTER);
  printf("%d ticks, %f BPM\n",cpt,bpm);
  
  
}

bool isPlaying = 0;

bool isItPlaying() {return isPlaying;}

void sigterm_stop(int sig) {
	isPlaying = 0;
  clear_queue();
}

void sequencer(int sig){
	isPlaying = 1;
	FILE* temp_file = fopen("temp_file","r");
	if(!fgets(seq_filename,80,temp_file))
		printf("Error sequence.\n");
	fclose(temp_file);

  int npfd, l1;
  struct pollfd *pfd;
  
  parse_sequence();
  bpm0 = bpm;
  
  init_queue();
  set_tempo();
  arpeggio();
  snd_seq_start_queue(seq_arp_handle, queue_id, NULL);
  snd_seq_drain_output(seq_arp_handle);
  npfd = snd_seq_poll_descriptors_count(seq_arp_handle, POLLIN);
  pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
  snd_seq_poll_descriptors(seq_arp_handle, pfd, npfd, POLLIN);
  seq_transpose = 0;
  swing = 0;
  tick = 0;

  arpeggio();
  while (isItPlaying()) {
    if (poll(pfd, npfd, 100000) > 0) {
      for (l1 = 0; l1 < npfd; l1++) {
        if (pfd[l1].revents > 0) midi_action(); 
      }
    }  
  }
}

void arp(int client){
        midi_channel_out = client;
	seq_arp_handle = open_sequencer();
  	signal(SIGUSR1, sequencer);
	signal(SIGUSR2, sigterm_stop);
	while(1);
}
                             
