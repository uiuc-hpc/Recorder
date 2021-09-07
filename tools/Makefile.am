#bin_PROGRAMS = recorder2text metaops_checker conflict_detector
bin_PROGRAMS = recorder2text
lib_LTLIBRARIES = librreader.la

AM_CFLAGS = -std=c99
AM_CPPFLAGS = -I$(top_srcdir)/include -Wno-unused-result
AM_CXXFLAGS = -std=c++11

librreader_la_SOURCES = reader.c

recorder2text_SOURCES = recorder2text.c
recorder2text_LDADD = librreader.la

metaops_checker_SOURCES = meta_ops_checker.c
metaops_checker_LDADD = librreader.la

conflict_detector_SOURCES = overlap_conflict.c build_offset_intervals.cpp
conflict_detector_LDADD = librreader.la
