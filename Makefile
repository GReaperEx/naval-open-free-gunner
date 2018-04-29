SRCS := $(shell find ./ -name "*.cpp")
BINS := $(patsubst %.cpp,.obj/%.o,$(SRCS))

LIBS := -lGL -lSDL2 -lopenal -lmpeg2 -lmpeg2convert
FLAGS := -Wall -O2 -std=gnu++11

$(shell mkdir -p .obj)

naval-open : $(BINS)
	g++ -s -o $@ $^ $(LIBS)

.obj/%.o : %.cpp
	g++ -c $(FLAGS) -o $@ $<

.PHONY: clean
clean :
	rm -rv naval-open .obj/
