TARGET := ITCH_Logger

SRCS := $(addprefix src/, main.cpp Client.cpp Logger.cpp utils.cpp error.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

CXX := g++
CXXFLAGS += -std=c++23
CXXFLAGS += -Wall -Wextra -Werror -pedantic
CXXFLAGS += -march=znver2 -mtune=znver2 -flto
#TODO does the order matter?
CXXFLAGS += 
  -fauto-inc-dec
  -fbranch-count-reg
  -fcombine-stack-adjustments
  -fcompare-elim
  -fcprop-registers
  -fdce
  -fdefer-pop
  -fdelayed-branch
  -fdse
  -fforward-propagate
  -fguess-branch-probability
  -fif-conversion
  -fif-conversion2
  -finline-functions-called-once
  -fipa-modref
  -fipa-profile
  -fipa-pure-const
  -fipa-reference
  -fipa-reference-addressable
  -fmerge-constants
  -fmove-loop-invariants
  -fmove-loop-stores
  -fomit-frame-pointer
  -freorder-blocks
  -fshrink-wrap
  -fshrink-wrap-separate
  -fsplit-wide-types
  -fssa-backprop
  -fssa-phiopt
  -ftree-bit-ccp
  -ftree-ccp
  -ftree-ch
  -ftree-coalesce-vars
  -ftree-copy-prop
  -ftree-dce
  -ftree-dominator-opts
  -ftree-dse
  -ftree-forwprop
  -ftree-fre
  -ftree-phiprop
  -ftree-pta
  -ftree-scev-cprop
  -ftree-sink
  -ftree-slsr
  -ftree-sra
  -ftree-ter
  -funit-at-a-time
  -falign-functions
  -falign-jumps
  -falign-labels
  -falign-loops
  -fcaller-saves
  -fcode-hoisting
  -fcrossjumping
  -fcse-follow-jumps
  -fcse-skip-blocks
  -fdelete-null-pointer-checks
  -fdevirtualize
  -fdevirtualize-speculatively
  -fexpensive-optimizations
  -ffinite-loops
  -fgcse
  -fgcse-lm
  -fhoist-adjacent-loads
  -finline-functions
  -finline-small-functions
  -findirect-inlining
  -fipa-bit-cp
  -fipa-cp
  -fipa-icf
  -fipa-ra
  -fipa-sra
  -fipa-vrp
  -fisolate-erroneous-paths-dereference
  -flra-remat
  -foptimize-crc
  -foptimize-sibling-calls
  -foptimize-strlen
  -fpartial-inlining
  -fpeephole2
  -freorder-blocks-algorithm=stc
  -freorder-blocks-and-partition
  -freorder-functions
  -frerun-cse-after-loop
  -fschedule-insns
  -fschedule-insns2
  -fsched-interblock
  -fsched-spec
  -fstore-merging
  -fstrict-aliasing
  -fthread-jumps
  -ftree-builtin-call-dce
  -ftree-loop-vectorize
  -ftree-pre
  -ftree-slp-vectorize
  -ftree-switch-conversion
  -ftree-tail-merge
  -ftree-vrp
  -fvect-cost-model=very-cheap
  -fgcse-after-reload
  -fipa-cp-clone
  -floop-interchange
  -floop-unroll-and-jam
  -fpeel-loops
  -fpredictive-commoning
  -fsplit-loops
  -fsplit-paths
  -ftree-loop-distribution
  -ftree-partial-pre
  -funswitch-loops
  -fvect-cost-model=dynamic
  -fversion-loops-for-strides
  -favoid-store-forwarding
  -ffp-contract=fast
  -fearly-inlining
  -fmodulo-sched
  -fmodulo-sched-allow-regmoves
  -fsplit-wide-types-early
  -fgcse-sm
  -fgcse-las
  -faggressive-loop-optimizations
  -fdevirtualize-at-ltrans
  -flive-range-shrinkage
  -fira-hoist-pressure
  -fira-loop-pressure
  -freschedule-modulo-scheduled-loops
  -fselective-scheduling
  -fselective-scheduling2
  -fsel-sched-pipelining
  -fsel-sched-pipelining-outer-loops
  -fipa-pta
  -flate-combine-instructions
  -fisolate-erroneous-paths-attribute

  #TODO -ftree-sink ¶
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