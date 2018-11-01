TEMPLATE = app
CONFIG -= qt
CONFIG += c++14

# optimizations
QMAKE_CXXFLAGS += -std=c++14
QMAKE_CXXFLAGS += -Os
QMAKE_CXXFLAGS += -fno-exceptions
QMAKE_CXXFLAGS += -fno-rtti
QMAKE_CXXFLAGS += -fno-threadsafe-statics

DEFINES += INTERRUPTED_WRAPPER

# for using musl/libc++
libcxx {
QMAKE_CXXFLAGS += -nostdinc -isystem /usr/include/x86_64-linux-musl
QMAKE_LFLAGS += -L/usr/lib/x86_64-linux-musl

#QMAKE_CXXFLAGS += -specs /usr/lib/x86_64-linux-musl/musl-gcc.specs
QMAKE_CXXFLAGS += -fpermissive
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

PUT = ../put
INCLUDEPATH += $$PUT

SOURCES = main.cpp \
    configcore.cpp \
    configserver.cpp \
    directorconfigserver.cpp \
    $$PUT/application.cpp \
    $$PUT/socket.cpp \
    $$PUT/asyncfd.cpp \
    $$PUT/cxxutils/vfifo.cpp \
    $$PUT/cxxutils/configmanip.cpp \
    $$PUT/cxxutils/syslogstream.cpp \
    $$PUT/specialized/eventbackend.cpp \
    $$PUT/specialized/peercred.cpp \
    $$PUT/specialized/procstat.cpp \
    $$PUT/specialized/PollEvent.cpp \
    $$PUT/specialized/FileEvent.cpp

HEADERS = \
    configcore.h \
    configserver.h \
    directorconfigserver.h \
    $$PUT/application.h \
    $$PUT/object.h \
    $$PUT/socket.h \
    $$PUT/asyncfd.h \
    $$PUT/cxxutils/vfifo.h \
    $$PUT/cxxutils/configmanip.h \
    $$PUT/cxxutils/syslogstream.h \
    $$PUT/cxxutils/posix_helpers.h \
    $$PUT/cxxutils/socket_helpers.h \
    $$PUT/cxxutils/error_helpers.h \
    $$PUT/specialized/eventbackend.h \
    $$PUT/specialized/peercred.h \
    $$PUT/specialized/procstat.h \
    $$PUT/specialized/PollEvent.h \
    $$PUT/specialized/FileEvent.h
