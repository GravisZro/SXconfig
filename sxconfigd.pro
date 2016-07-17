TEMPLATE = app
CONFIG -= qt
#CONFIG += c++11

QMAKE_CXXFLAGS += -std=c++14
#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_CXXFLAGS += -nodefaultlibs
#-nostdlib -lc
#QMAKE_CXXFLAGS += -nostdinc -I/usr/include/x86_64-linux-musl
#QMAKE_CXXFLAGS += -specs "/usr/lib/x86_64-linux-musl/musl-gcc.specs"
#QMAKE_CXXFLAGS += -O2
#QMAKE_CXXFLAGS += -O0
#QMAKE_CXXFLAGS += -fno-exceptions -fno-rtti -fno-threadsafe-statics

QMAKE_CXXFLAGS += -Os
QMAKE_CXXFLAGS += -fno-exceptions
QMAKE_CXXFLAGS += -fno-rtti
QMAKE_CXXFLAGS += -fno-threadsafe-statics

#QMAKE_CXXFLAGS += -nostdinc
#QMAKE_CXXFLAGS += -isystem /usr/include/x86_64-linux-musl
#QMAKE_CXXFLAGS += -isystem /usr/include/c++/v1/

#QMAKE_CXXFLAGS += -nodefaultlibs
#LIBS += -nodefaultlibs
#LIBS += -L/usr/lib/x86_64-linux-musl
#LIBS += -stdlib=libc++
LIBS += -lpthread
#LIBS += -lc++
#LIBS += -lc
#LIBS += -lm
#LIBS += -lgcc
#LIBS += -lrt

INCLUDEPATH += ../pdtk

SOURCES = main.cpp \
    ../pdtk/cxxutils/configparser.cpp

HEADERS += \
    ../pdtk/cxxutils/configparser.h
