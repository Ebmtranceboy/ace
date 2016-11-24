#include "pace.h"
#include <stdlib.h>
#include <stdio.h>

atom* parseNote(char* line){
	int instr; 
	float octave, pitch;
	float onset, length;
	sscanf(line, "i %d %f %f %f %f", &instr, &onset, &length, &octave, &pitch);
	return apply2(apply3(Note,intc(instr),floatc(onset),floatc(length)),
		intc((int)octave),intc((int)pitch));
}

atom* nextLines(FILE*file){
	char line[80];
	if(!fgets(line,80,file)) return nil;
	return apply2(cons,parseNote(line),nextLines(file));
}

atom* referencedDurations(int num_max, atom* pow2_max){
	atom* nums = apply(parse("\\ max -> range 1 (+) 1 max"),intc(num_max));
	atom* powersOf2 = apply(parse("\\ max -> range 0 (+) 1 max"),apply2(plus,intc(1),pow2_max));
	atom* powersOf3 = parse("[0,1]");
	atom* base = apply2(map,apply(uncurry,times),
		apply(cartesianProduct,
			apply2(cons,apply2(map,parse("2^"),powersOf2),
			apply2(cons,apply2(map,parse("3^"),powersOf3),nil))));
	atom* product = apply(cartesianProduct,	apply2(cons,nums,apply2(cons,base,nil)));
	atom* notDuplicate = parse("\\ xs -> (gcd (xs!!0) (xs!!1)) == 1");
	atom* candidates = apply2(filter,notDuplicate,product);
	atom* result = apply2(takeWhile, parse("\\ p -> (fst p) < 1"),
		apply2(sortBy,parse("\\ p1 p2 -> (fst p1) < (fst p2)"),
			apply2(zip, apply2(map,apply(uncurry,dividedBy),candidates),candidates)));
	return result;
}

char noteName(int c){
	switch(c){
		case 0:
		case 1: return 'c';break;
		case 2:
		case 3: return 'd';break;
		case 4: return 'e';break;
		case 5:
		case 6: return 'f';break;
		case 7:
		case 8: return 'g';break;
		case 9:
		case 10: return 'a';break;
		case 11: return 'b';break;
	}
	return 0;
}

bool isSharp(int c){return c==1||c==3||c==6||c==8||c==10;}

atom* read_fct(stack*list) {
	if(!list) return fromString(__func__);
	if(list->head->k != STRING) return NULL;
        char* seq_filename = toString(list->head);

	FILE *f;
	
	if (!(f = fopen(seq_filename, "r"))) {
		fprintf(stderr, "Couldn't open sequence file %s\n", \
			seq_filename);
	exit(1);
	} else{
		printf("%s opened successfully for reading.\n", seq_filename);
	}
	
	atom* result = apply(sortByOnset,nextLines(f));
	fclose(f);
	empty(list);
	return result;
}

atom* trim_fct(stack* list){
	if(!list) return fromString(__func__);
	atom*notes = list->head;
        atom*delay = apply(parse("select 5 2"),apply(head,notes));
        atom*result = apply2(map,apply(parse(
        	"\\ delay note -> Note (select 5 1 note)"
        	"                      ((select 5 2 note) - delay)"
        	"                      (select 5 3 note)"
        	"                      (select 5 4 note)"
        	"                      (select 5 5 note)"),delay),notes);
	empty(list);
	return result;
}

atom* save_fct(stack*list){
	if(!list) return fromString(__func__);
        atom* notes = list->head;
        atom* order = list->rest->head;
        atom* prescribedLastLength = list->rest->rest->head;
        char* seq_filename = toString(list->rest->rest->rest->head);

        atom* getInstr = parse("select 5 1");
        atom* getOnset = parse("select 5 2");
        atom* getLength = parse("select 5 3");
        atom* getOctave = parse("select 5 4");
        atom* getPitch = parse("select 5 5");
        
        atom* sorted_octaves = apply(sort,apply2(map, getOctave, notes));
        int mostFrequentOctave = apply(head,apply(head,
        	apply2(sortBy,parse("\\ x y -> (length x) > (length y)"),
        		apply(group,sorted_octaves))))->p.n;
        
        atom*onsets = apply2(map,getOnset,apply(trim,notes));
        atom*totalDuration = apply(last,onsets);
        atom*lastLength;
        if(totalDuration->p.x == 0.0f){ // unique note
        	lastLength = prescribedLastLength;
        }
        else lastLength = apply2(dividedBy,
        	totalDuration,
        	intc((int)apply2(dividedBy,totalDuration,prescribedLastLength)->p.x));
        totalDuration = apply2(plus,totalDuration,lastLength);
        
        atom*durations = apply2(append,apply3(zipWith, minus, 
        	apply(tail, onsets), onsets),apply2(cons,lastLength,nil));
   
        atom*epsilon = apply(parse("\\ order -> 1/(3*(2^order))"),order);
        
        atom*find = apply(parse(
        	"\\ref x -> let diffs = map (x-) (map fst ref) "
        	"         in [zip (takeWhile (>=0) diffs) ref, "
        	"                  dropWhile ((>=0).fst) (zip diffs ref)]"),referencedDurations(6,order));
        atom*limitCases = apply(parse("\\ order lim -> if null (lim!!0) then [1,3*(2^order)] "
        	"                      else if null (lim!!1) then [1,1] else []"), order);
        atom*decision = parse("\\ lim -> if ((fst $ last $ lim!!0) + (fst $ head $ lim!!1)) < 0 "
        	"                              then (snd $ snd $ last $ lim!!0)"
        	"                              else (snd $ snd $ head $ lim!!1)");
        atom*nearest = apply2(B,apply3(predicate,apply2(B,apply(equal,nil),limitCases),decision,limitCases),find);

        atom*decimal = NULL;
        float integer = (float)(int)totalDuration->p.x;
        if(totalDuration->p.x - integer  > 0.0f){
   		decimal = apply(nearest,floatc(totalDuration->p.x - integer));
        	totalDuration = apply2(plus,floatc(integer),
        		apply2(dividedBy,apply(fst,decimal),apply(snd,decimal)));
        }
	atom*ratios = apply2(map,apply2(C,dividedBy,totalDuration),durations);
        
        FILE *f;
	
	if (!(f = fopen(seq_filename, "w"))) {
		fprintf(stderr, "Couldn't open sequence file %s\n", \
			seq_filename);
	exit(1);
	} else{
		printf("%s opened successfully for writing.\n", seq_filename);
	}
	
		
	int octave, pitch;
	atom*note,*duration,*ratio,*ratioLength,*noteLength,*index=notes;
	fprintf(f,"%04d ",(int)integer);
	if(decimal){
		fprintf(f,"%02d ",apply(fst,decimal)->p.n);
		fprintf(f,"%04d ",apply(snd,decimal)->p.n);
	} else fprintf(f, "000000 ");
	fprintf(f,"%02d ", mostFrequentOctave);
	int numNotes = apply(length,notes)->p.n; 
	float control = 0.0f;
	for(int i=0; i<numNotes; i++){
		note = apply(head,index);
	//println(note);	
		octave = apply(getOctave,note)->p.n;
		while(octave-- > mostFrequentOctave) fprintf(f,"+");
		octave = apply(getOctave,note)->p.n;
		while(octave++ < mostFrequentOctave) fprintf(f,"-");
		pitch = apply(getPitch,note)->p.n;
		fprintf(f,"%c",noteName(pitch));
		if(isSharp(pitch)) fprintf(f,"#");
		ratio = apply2(nth,ratios,intc(i));
	//println(ratio);	
		if(apply2(lessThan,ratio,epsilon)->p.b)
			fprintf(f,"000000");
		else{
		//println(apply(limitCases,apply(find,ratio)));
			duration = apply(nearest,ratio);
		//println(duration);	
			fprintf(f,"%02d",apply(fst,duration)->p.n);
			fprintf(f,"%04d",apply(snd,duration)->p.n);
			control += apply2(dividedBy,apply(fst,duration),apply(snd,duration))->p.x;
		}
	//println(apply(getLength,note));
	//println(totalDuration);
		ratioLength = apply2(dividedBy,apply(getLength,note),totalDuration);
		if(apply2(lessThan,ratioLength,epsilon)->p.b) ratioLength = epsilon;
	//print(epsilon); printf(" ");println(ratioLength);
		noteLength = apply(nearest,ratioLength);
		fprintf(f,"%02d",apply(fst,noteLength)->p.n);
		fprintf(f,"%04d",apply(snd,noteLength)->p.n);
		
		fprintf(f,"%03d",apply(getInstr,note)->p.n - 1);
	
		
		index=apply(tail,index);
	}

	printf("Last note length: ");
	print(lastLength);
	printf(" s.\nLoop duration: ");
	print(totalDuration);
	printf(" s.\n%f%% converted.\nNumber of notes:\n",control*100.0);
        atom*result = intc(numNotes);
        fclose(f);
        empty(list);
        return result;
}

atom*define;
extern stack*dictionary;

atom*_40_61_41_fct(stack*list){
	if(!list) return fromString(__func__);
        atom* item = list->head;
        atom* body = list->rest->head;
        stack* e = dictionary;
        atom* name = getName(item);
        if(name){
        	while(e && !areSameStrings(name, e->head->p.a.f)) e = e->rest;
        	e->head->p.a.x = body;
        }
	else stamp(body, item);
        empty(list);
        return body;
}

void initializeConverter(){
	Note = parse("\\ instr onset length octave pitch f -> f instr onset length octave pitch");
	stamp(Note, fromString("Note"));
	trim = new();                 declareFunction(&trim,               1,  trim_fct);
	read_ace = new();             declareFunction(&read_ace,           1,  read_fct);
	save_melody = new();          declareFunction(&save_melody,        4,  save_fct);
	define = new();               declareFunction(&define,             2,  _40_61_41_fct);
	sortByOnset = apply(sortBy,apply2(on,lessThan,parse("select 5 2"))); stamp(sortByOnset, fromString("sortByOnset"));
}



