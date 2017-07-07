TEMPLATE = app
CONFIG -= qt
#CONFIG += c++14

QMAKE_CXXFLAGS += -std=c++14

# optimizations
QMAKE_CXXFLAGS += -Os
QMAKE_CXXFLAGS += -fno-exceptions
QMAKE_CXXFLAGS += -fno-rtti
QMAKE_CXXFLAGS += -fno-threadsafe-statics


# for using libc++
libcxx {
QMAKE_CXXFLAGS += -stdlib=libc++
LIBS += -lc++
QMAKE_CXXFLAGS += -isystem /usr/include/c++/v1/
}


#QMAKE_CXXFLAGS += -nodefaultlibs
#QMAKE_CXXFLAGS += -nostdlib
#QMAKE_CXXFLAGS += -nostdinc -I/usr/include/x86_64-linux-musl
#QMAKE_CXXFLAGS += -specs "/usr/lib/x86_64-linux-musl/musl-gcc.specs"
#QMAKE_CXXFLAGS += -O2
#QMAKE_CXXFLAGS += -O0


#QMAKE_CXXFLAGS += -nostdinc
#QMAKE_CXXFLAGS += -isystem /usr/include/x86_64-linux-musl

#QMAKE_CXXFLAGS += -nodefaultlibs
#LIBS += -nodefaultlibs
#LIBS += -L/usr/lib/x86_64-linux-musl
#LIBS += -lpthread
#LIBS += -lc
#LIBS += -lm
#LIBS += -lgcc_s
#LIBS += -lclang-3.9
#LIBS += -lrt

INCLUDEPATH += ../pdtk

SOURCES = main.cpp \
    ../pdtk/application.cpp \
    ../pdtk/socket.cpp \
    ../pdtk/cxxutils/configmanip.cpp\
    ../pdtk/specialized/eventbackend.cpp \
    ../pdtk/specialized/peercred.cpp \
    configserver.cpp

HEADERS = \
    ../pdtk/application.h \
    ../pdtk/object.h \
    ../pdtk/socket.h \
    ../pdtk/cxxutils/posix_helpers.h \
    ../pdtk/cxxutils/socket_helpers.h \
    ../pdtk/cxxutils/error_helpers.h \
    ../pdtk/cxxutils/configmanip.h \
    ../pdtk/cxxutils/vfifo.h \
    ../pdtk/specialized/eventbackend.h \
    ../pdtk/specialized/peercred.h \
    ../pdtk/cxxutils/syslogstream.h \
    configserver.h
