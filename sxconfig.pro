TEMPLATE = app
CONFIG -= qt
#CONFIG += c++14

# optimizations
QMAKE_CXXFLAGS += -Os
QMAKE_CXXFLAGS += -fno-exceptions
QMAKE_CXXFLAGS += -fno-rtti
QMAKE_CXXFLAGS += -fno-threadsafe-statics


# for using musl/libc++
libcxx {
QMAKE_CXXFLAGS += -specs /usr/lib/x86_64-linux-musl/musl-gcc.specs
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

system( test -z $CONTINUOUS_INTEGRATION ) {
system( test "$CC" = "clang" ):QMAKE_CXXFLAGS+=-std=c++11
else:QMAKE_CXXFLAGS+=-std=c++1y
}

PDTK = ../pdtk
INCLUDEPATH += $$PDTK

SOURCES = main.cpp \
    configserver.cpp \
    executorconfigserver.cpp \
    $$PDTK/application.cpp \
    $$PDTK/socket.cpp \
    $$PDTK/cxxutils/configmanip.cpp\
    $$PDTK/specialized/eventbackend.cpp \
    $$PDTK/specialized/peercred.cpp \
    $$PDTK/specialized/procstat.cpp

HEADERS = \
    configserver.h \
    executorconfigserver.h \
    $$PDTK/application.h \
    $$PDTK/object.h \
    $$PDTK/socket.h \
    $$PDTK/cxxutils/posix_helpers.h \
    $$PDTK/cxxutils/socket_helpers.h \
    $$PDTK/cxxutils/error_helpers.h \
    $$PDTK/cxxutils/configmanip.h \
    $$PDTK/cxxutils/vfifo.h \
    $$PDTK/specialized/eventbackend.h \
    $$PDTK/specialized/peercred.h \
    $$PDTK/cxxutils/syslogstream.h \
    $$PDTK/specialized/procstat.h
