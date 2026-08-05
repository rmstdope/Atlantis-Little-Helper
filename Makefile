
CXX      = g++
CXXFLAGS = -g -O2 -std=c++17
LDFLAGS  =
DEFS     = -DHAVE_CONFIG_H
LIBS     =  -L/opt/homebrew/lib   -framework IOKit -framework Carbon -framework Cocoa -framework QuartzCore -framework AudioToolbox -framework System -framework OpenGL -lwx_osx_cocoau_xrc-3.3 -lwx_osx_cocoau_html-3.3 -lwx_osx_cocoau_qa-3.3 -lwx_osx_cocoau_core-3.3 -lwx_baseu_xml-3.3 -lwx_baseu_net-3.3 -lwx_baseu-3.3
CPPFLAGS =  -I/opt/homebrew/lib/wx/include/osx_cocoa-unicode-3.3 -I/opt/homebrew/include/wx-3.3 -D_FILE_OFFSET_BITS=64 -DwxDEBUG_LEVEL=0 -DWXUSINGDLL -D__WXMAC__ -D__WXOSX__ -D__WXOSX_COCOA__


#CC          = c++
#CXXFLAGS    = -O2  -Wall
#CXXFLAGS_WX = $(shell wx-config --cflags)
#LIBS_WX     = $(shell wx-config --libs)
#CXXFLAGS_PYTHON =-I/usr/include/python2.2 -DALH_PYTHON_EXTEND
#LIBS_PYTHON =-lpython2.2


OBJECTS     = cfgfile.o string_utils.o files.o objs.o\
              data.o errs.o atlaparser.o \
              ahapp.o ahframe.o consts_ah.o editpane.o \
              editsframe.o extend.o extend_no.o \
			  flagsdlg.o hexfilterdlg.o listcoledit.o \
              listpane.o mapframe.o \
              mappane.o msgframe.o optionsdlg.o \
              unitfilterdlg.o unitframe.o unitframefltr.o unitpane.o \
              unitpanefltr.o unitsplitdlg.o utildlgs.o

TARGETS     = bin/ah
TEST_OBJECTS = obj/tests/parser_regression_tests.o \
               obj/tests/game_data_helper_stub.o \
               obj/string_utils.o obj/files.o obj/cfgfile.o obj/objs.o \
               obj/data.o obj/errs.o obj/consts_ah.o obj/atlaparser.o
TEST_TARGET  = bin/parser-tests

CXXFLAGS_TEST = $(CXXFLAGS) -DALH_TESTING
TEST_CPPFLAGS = $(CPPFLAGS) -I. -Itests


all: $(TARGETS)

dirs:
	mkdir -p bin obj obj/tests

bin/ah: $(patsubst %.o,obj/%.o,$(OBJECTS))
	$(CXX) -s $(LDFLAGS) -o $@ $(patsubst %.o,obj/%.o,$(OBJECTS)) $(LIBS)

bin/parser-tests: $(TEST_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(TEST_OBJECTS) $(LIBS)

test: dirs $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(patsubst %.o,obj/%.o,$(OBJECTS)) $(TARGETS)
	rm -f $(TEST_OBJECTS) $(TEST_TARGET)

$(patsubst %.o,obj/%.o,$(OBJECTS)): obj/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $(DEFS) -o $@ $<

obj/tests/%.o: tests/%.cpp | dirs
	$(CXX) -c $(CXXFLAGS_TEST) $(TEST_CPPFLAGS) $(DEFS) -o $@ $<
