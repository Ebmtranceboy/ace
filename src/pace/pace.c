#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "pace.h"

int main(int argc, char**argv){
	initialize();
	initializeConverter();
	initializeTape();
	int keyboard     = atoi(argv[2]);// 24 (cf pmidi -l);
	
	do_claim_client();
	pid_synth = fork();
	if(pid_synth == 0) synth(argv[1],keyboard);// default (cf aplay -l and ~/.asoundrc)
	else{
		pid_arp = fork();
		if(pid_arp == 0){
			arp(client_id());
		}
		else repl();
	}
	
	kill(pid_arp,SIGKILL);
	kill(pid_synth,SIGKILL);
	unlink("temp_file");
	return 0;
}

atom* record_fct(stack*list){
	if(!list) return fromString(__func__);
	FILE* temp_file = fopen("temp_file","w");
	fprintf(temp_file,"%s",toString(list->head));
	fclose(temp_file);
	kill(pid_synth,SIGUSR1);
	empty(list);
	return fromString("recording ...");
}

atom* stop_fct(stack*list){
	if(!list) return fromString(__func__);
	atom *result = NULL;
	if(list->head == record){
		kill(pid_synth,SIGUSR2);
		result = fromString("... stopped.");
	} else if (list->head == play){
		kill(pid_arp,SIGUSR2);
		result = fromString("... stopped.");
	}
	empty(list);
	return result;
}

atom* play_fct(stack*list){
	if(!list) return fromString(__func__);
	FILE* temp_file = fopen("temp_file","w");
	fprintf(temp_file,"%s",toString(list->head));
	fclose(temp_file);
	kill(pid_arp,SIGUSR1);
	empty(list);
	return fromString("playing ...");
}

void initializeTape(){
	record = new();          declareFunction(&record,        1,  record_fct);
	stop = new();            declareFunction(&stop,          1,  stop_fct);
	play = new();            declareFunction(&play,          1,  play_fct);
}
