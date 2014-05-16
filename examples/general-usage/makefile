EXENAME = unittests

SRCDIR = .
OBJDIR = obj

override CXXFLAGS += "-I../../include"
override LDFLAGS += "../../lib/libtinytest.a"

# autodetect c++11 compilation flags
override CXXFLAGS += --std=c++11 -Wno-multichar
ifeq ($(shell echo "int main(){}" | $(CXX) --stdlib=libc++ -x c - -o /dev/null >& /dev/null; echo $$?), 0)
	override CXXFLAGS += --stdlib=libc++
endif

SRCS := $(shell find $(SRCDIR) -name '*.cpp')
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS)))

.PHONY: clean tinytest

all: tinytest $(EXENAME)

tinytest:
	make -C ../..

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MT '$@' -MF '$(patsubst $(OBJDIR)/%.o,$(OBJDIR)/%.d,$@)'

$(EXENAME): $(OBJS) tinytest
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -f $(EXENAME)
	rm -rf $(OBJDIR)