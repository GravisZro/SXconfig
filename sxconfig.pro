TEMPLATE = app
CONFIG -= qt
CONFIG += c++14
CONFIG += strict_c++
CONFIG += exceptions_off
CONFIG += rtti_off

# FOR CLANG
#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_LFLAGS += -stdlib=libc++
QMAKE_CXXFLAGS += -fconstexpr-depth=256
QMAKE_CXXFLAGS += -fconstexpr-steps=900000000

# universal arguments
QMAKE_CXXFLAGS += -fno-rtti

QMAKE_CXXFLAGS_DEBUG += -O0 -g3
QMAKE_CXXFLAGS_RELEASE += -Os


#QMAKE_CXXFLAGS_RELEASE += -fno-threadsafe-statics
QMAKE_CXXFLAGS_RELEASE += -fno-asynchronous-unwind-tables
#QMAKE_CXXFLAGS_RELEASE += -fstack-protector-all
QMAKE_CXXFLAGS_RELEASE += -fstack-protector-strong

# optimizations
QMAKE_CXXFLAGS_RELEASE += -fdata-sections
QMAKE_CXXFLAGS_RELEASE += -ffunction-sections
QMAKE_LFLAGS_RELEASE += -Wl,--gc-sections

# libraries
LIBS += -lrt

#DEFINES += DISABLE_INTERRUPTED_WRAPPER

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
    $$PUT/specialized/mutex.cpp \
    $$PUT/specialized/peercred.cpp \
    $$PUT/specialized/procstat.cpp \
    $$PUT/specialized/fstable.cpp \
    $$PUT/specialized/mountpoints.cpp \
    $$PUT/specialized/pollevent.cpp \
    $$PUT/specialized/fileevent.cpp

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
    $$PUT/specialized/osdetect.h \
    $$PUT/specialized/eventbackend.h \
    $$PUT/specialized/mutex.h \
    $$PUT/specialized/peercred.h \
    $$PUT/specialized/procstat.h \
    $$PUT/specialized/fstable.h \
    $$PUT/specialized/mountpoints.h \
    $$PUT/specialized/pollevent.h \
    $$PUT/specialized/fileevent.h
