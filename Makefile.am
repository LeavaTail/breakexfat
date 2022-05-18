bin_PROGRAMS = breakexfat

breakexfat_SOURCES = src/main.c \
			src/super.c \
			src/cluster.c \
			src/cache.c \
			src/break.c

AM_CPPFLAGS = -I$(top_srcdir)/include

if DEBUG
AM_CPPFLAGS += -O0 -g3 -Wall -DEXFAT_DEBUG
else
AM_CPPFLAGS += -O2
endif
