
#ifndef PACE_H
#define PACE_H

#include "ace.h"
#include <unistd.h>

atom* Note;
atom* read_ace;
atom* save_melody;
atom* sortByOnset, *trim;

atom *record, *stop, *play;

char record_file_name[100];

void initializeConverter();
void initializeTape();

void do_claim_client();
int client_id();

pid_t pid_arp;
pid_t pid_synth;

void synth(char*,int);
void arp(int);

#endif