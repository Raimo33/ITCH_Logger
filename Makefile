TARGET := ITCH_Logger

SRCS := $(addprefix src/, main.cpp Client.cpp Logger.cpp utils.cpp error.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

CCXX := g++

CXXFLAGS += -std=c++23

CXXFLAGS += -Wall -Wextra -Werror -pedantic
CXXFLAGS += -O3 -march=znver2 -mtune=znver2

LDFLAGS = -static -static-libgcc -static-libstdc++

%.o: %.cpp %.d
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.d: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TARGET): $(OBJS) $(DEPS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(TARGET)

re: fclean $(TARGET)

.PHONY: fclean clean re
.IGNORE: fclean clean
.PRECIOUS: $(DEPS)
.SILENT: