bin_PROGRAMS = recorder2text metaops_checker conflict_detector recorder2parquet
lib_LTLIBRARIES = librreader.la
LIBS = -lparquet -larrow

AM_CPPFLAGS = $(CXXFLAGS) -I$(top_srcdir)/include -Wno-unused-result
AM_CXXFLAGS = $(CXXFLAGS) -std=c++11

librreader_la_SOURCES = reader.c

recorder2text_SOURCES = recorder2text.c
recorder2text_LDADD = librreader.la

recorder2parquet_SOURCES = recorder2parquet.cpp
recorder2parquet_LDFLAGS = -lparquet -larrow $(PWD)/.libs/librreader.so
recorder2parquet_DEPENDENCIES = librreader.la

metaops_checker_SOURCES = meta_ops_checker.c
metaops_checker_LDADD = librreader.la

conflict_detector_SOURCES = overlap_conflict.c build_offset_intervals.cpp
conflict_detector_LDADD = librreader.la
