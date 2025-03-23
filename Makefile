TARGET := ITCH_Logger

SRCS := $(addprefix src/, main.cpp Client.cpp Logger.cpp utils.cpp error.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

CXX := g++
CXXFLAGS += -std=c++23
CXXFLAGS += -Wall -Wextra -Werror -pedantic
CXXFLAGS += -O3 -march=znver2 -mtune=znver2 -flto
CXXFLAGS += -falign-functions=32 -falign-loops=32 -falign-jumps=32 -falign-labels=32
LDFLAGS = -static -static-libgcc -static-libstdc++
LDFLAGS += -luring

%.o: %.cpp %.d
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.d: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TARGET): $(OBJS) $(DEPS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(DEPS)

re: clean $(TARGET)

.PHONY: clean re
.IGNORE: clean
.PRECIOUS: $(DEPS)
.SILENT: