CC = gcc
ID = ace
SRC = src
APP = pace

LIB_DIR = lib
TESTS_DIR = tests
INCLUDE_DIR = include
GENERATOR_DIR = generator

FAUST = faust

POLYPHONY = 10

REPL = $(ID)
LIB = $(LIB_DIR)/lib$(ID).a

MELODYEDITOR = melodyEditor
MIDI = MIDI

APP_SRC = $(SRC)/$(APP)
APP_LIB = $(LIB_DIR)/lib$(APP).a
PLUGIN_DIR = $(APP_SRC)/$(MIDI)/plugin

CLIB_DIR = clib

TESTS = $(TESTS_DIR)/tests
BENCHMARK =  $(TESTS_DIR)/benchmark
CCOPT_SO = $(CCFLAGS) -fpic

CC_OPTIONS = -g -Wall -O1 -std=gnu11 -I$(INCLUDE_DIR)
LD_OPTIONS = -Llib -l$(ID) -lm

APP_CC_OPTIONS = $(CC_OPTIONS)
APP_LD_OPTIONS = -Llib -l$(APP) -ldl -lasound -pthread 


all: $(REPL) $(APP)

tests: $(TESTS) $(BENCHMARK)


$(REPL): $(LIB) repl.c
	$(CC) $(CC_OPTIONS) -o $@ $^ $(LD_OPTIONS)


$(PLUGIN_DIR)/$(GENERATOR_DIR)/%.ch: $(PLUGIN_DIR)/$(GENERATOR_DIR)/%.dsp
	$(FAUST) -double -lang c $< > $@

$(PLUGIN_DIR)/$(GENERATOR_DIR)/%.h :$(PLUGIN_DIR)/$(GENERATOR_DIR)/%.ch
	printf "#include <glue.h>\n" > tem.p; \
	cp $< $@; awk '/typedef/,/mydsp;/' $@ >> tem.p; \
	sed -ie "s/FAUSTFLOAT/double/g" tem.p; \
	printf "mydsp* newmydsp();\n" >> tem.p; \
	printf "void initmydsp(mydsp* dsp, int samplingFreq);\n" >> tem.p; \
	printf "void computemydsp(mydsp* dsp, int count, double** inputs, double** outputs);\n" >> tem.p; \
	printf "void deletemydsp(mydsp* dsp);\n" >> tem.p; \
	mv tem.p $@

$(PLUGIN_DIR)/$(GENERATOR_DIR)/%.c :$(PLUGIN_DIR)/$(GENERATOR_DIR)/%.ch
	printf "#include <$(basename $(notdir $@)).h>\n" > tem.p; \
	cp $< $@; perl -i -p0e 's/typedef struct.*?mydsp;//s' $@; \
	cat $@ >> tem.p; \
	mv tem.p $@; sed -ie "s/inline//g" $@; \
	sed -ie "s/FAUSTFLOAT float/FAUSTFLOAT double/g" $@ \
	

$(PLUGIN_DIR)/$(GENERATOR_DIR)/glue-%.c: $(PLUGIN_DIR)/$(GENERATOR_DIR)/%.h 
	printf "#include <$(basename $(notdir $<)).h>\n" > $@; \
	sed -e "s/tt/$(basename $(notdir $<))/g" $(APP_SRC)/$(MIDI)/glue.c >> $@;


$(PLUGIN_DIR)/$(GENERATOR_DIR)/%.so:$(PLUGIN_DIR)/$(GENERATOR_DIR)/%.c \
                                    $(PLUGIN_DIR)/$(GENERATOR_DIR)/glue-%.c
	$(CC) -o $@ $^ -I$(APP_SRC)/$(MIDI) -I. -I$(PLUGIN_DIR)/$(GENERATOR_DIR) -shared -fPIC -lm $(APP_LD_OPTIONS)
	mv $@ $(PLUGIN_DIR)
	
$(PLUGIN_DIR)/%.o: $(PLUGIN_DIR)/%.c
	$(CC) -c $< -o $@ $(CCOPT_SO) -I$(PLUGIN_DIR) -I.

	
	
$(APP): $(LIB) $(APP_LIB) $(APP_SRC)/pace.c
	$(CC) $(CC_OPTIONS) -o $@ $^ $(LD_OPTIONS) $(APP_LD_OPTIONS); \
	mkdir -p $(PLUGIN_DIR)/$(GENERATOR_DIR)/bak; \
	mv $(wildcard $(PLUGIN_DIR)/$(GENERATOR_DIR)/*.dsp) $(PLUGIN_DIR)/$(GENERATOR_DIR)/bak; \
	cd $(PLUGIN_DIR)/$(GENERATOR_DIR)/bak; \
	for n in $(shell seq 1 $(POLYPHONY)); do \
		for i in *; do \
			cp $$i ../$$(basename $$i .dsp)$$n.dsp; \
		done \
	done; \
	cd ../../../../../..; \
	make plugins; \
	cd $(PLUGIN_DIR)/$(GENERATOR_DIR)/; \
	rm *.dsp; \
	mv bak/* .; \
	rmdir bak; \
	cd ../../../../..

$(APP_LIB): $(MELODYEDITOR).o synth.o arp.o dstring.o mem.o die.o db.o \
                    plugin_discovery.o plugin_manager.o
	ar rc $(APP_LIB) $^
	ranlib $(APP_LIB)


$(MELODYEDITOR).o: $(APP_SRC)/$(MELODYEDITOR)/$(MELODYEDITOR).c
	$(CC) -c $(APP_CC_OPTIONS) $<
dstring.o: $(APP_SRC)/$(MIDI)/$(CLIB_DIR)/dstring.c
	$(CC) -c $(APP_CC_OPTIONS) $< -fPIC
die.o: $(APP_SRC)/$(MIDI)/$(CLIB_DIR)/die.c $(APP_SRC)/$(MIDI)/$(CLIB_DIR)/die.h
	$(CC) -c $(APP_CC_OPTIONS) $< -fPIC
mem.o: $(APP_SRC)/$(MIDI)/$(CLIB_DIR)/mem.c $(APP_SRC)/$(MIDI)/$(CLIB_DIR)/mem.h
	$(CC) -c $(APP_CC_OPTIONS) $< -fPIC
plugin_discovery.o: $(APP_SRC)/$(MIDI)/plugin_discovery.c
	$(CC) -c $(APP_CC_OPTIONS) $<
plugin_manager.o: $(APP_SRC)/$(MIDI)/plugin_manager.c
	$(CC) -c $(APP_CC_OPTIONS) $< -fPIC
db.o: $(APP_SRC)/$(MIDI)/db.c $(APP_SRC)/$(MIDI)/db.h
	$(CC) -c $(APP_CC_OPTIONS) $< -fPIC
	
arp.o: $(APP_SRC)/$(MIDI)/arp.c
	$(CC) -c $(APP_CC_OPTIONS) $<
	
synth.o: $(APP_SRC)/$(MIDI)/synth.c
	$(CC) -c $(APP_CC_OPTIONS) $<

$(LIB): $(SRC)/$(ID).c
	$(CC) -c $(CC_OPTIONS) -o $(ID).o $^
	ar rc $(LIB) $(ID).o
	ranlib $(LIB)

$(TESTS): $(LIB) $(TESTS_DIR)/tests.c
	$(CC) -o $(TESTS) $(TESTS_DIR)/tests.c $(LD_OPTIONS); \
	$(TESTS)
	diff $(TESTS_DIR)/expected_output $(TESTS_DIR)/output
	
$(BENCHMARK): $(LIB)  $(TESTS_DIR)/benchmark.c
	$(CC) -o $(BENCHMARK) $(TESTS_DIR)/benchmark.c $(LD_OPTIONS)

plugins: $(patsubst %.dsp,%.so,$(wildcard $(PLUGIN_DIR)/$(GENERATOR_DIR)/*.dsp))
	make room

room:
	rm -rf core *.o	tem.p* $(PLUGIN_DIR)/*.o $(PLUGIN_DIR)/$(GENERATOR_DIR)/*.c \
	$(PLUGIN_DIR)/$(GENERATOR_DIR)/*.h $(PLUGIN_DIR)/$(GENERATOR_DIR)/*.o \
	$(PLUGIN_DIR)/$(GENERATOR_DIR)/*.ch $(PLUGIN_DIR)/$(GENERATOR_DIR)/glue-*.c \
	$(PLUGIN_DIR)/$(GENERATOR_DIR)/*.ce $(PLUGIN_DIR)/$(GENERATOR_DIR)/*.so

clean:
	rm -rf *.o

veryclean:
	cd $(APP_SRC)/$(MIDI)/; \
	make clean; \
	cd ../..
	make clean
	rm -f $(ID) $(APP) $(LIB) $(APP_LIB) $(BENCHMARK) $(TESTS) $(TESTS_DIR)/output
	make room; \
	rm -rf *.a main $(PLUGIN_DIR)/*.so $(PLUGIN_DIR)/$(GENERATOR_DIR)/*.so \

	