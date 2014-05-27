# autodetect c++11 compilation flags
override CXXFLAGS += --std=c++11 -Wno-multichar
ifeq ($(shell echo "int main(){}" | $(CXX) --stdlib=libc++ -x c - -o /dev/null >& /dev/null; echo $$?), 0)
	override CXXFLAGS += --stdlib=libc++
endif

all: lib/libtinytest.a

lib/libtinytest.a: src/tinytest.h src/utility.h src/tinytest.cpp
	@mkdir -p obj
	@mkdir -p include/tinytest
	@mkdir -p lib
	$(CXX) $(CXXFLAGS) -c src/tinytest.cpp -o obj/tinytest.o
	ar rcs $@ obj/tinytest.o
	cp src/*.h include/tinytest/

clean:
	rm -rf include
	rm -rf lib
	rm -rf obj