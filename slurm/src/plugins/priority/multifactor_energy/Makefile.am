# Makefile for priority/multifactor_energy plugin

AUTOMAKE_OPTIONS = foreign

PLUGIN_FLAGS = -module -avoid-version --export-dynamic

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src/common -I$(LRZ_MODEL)/include

pkglib_LTLIBRARIES = priority_multifactor_energy.la

# Null priority logging plugin.
priority_multifactor_energy_la_SOURCES = priority_multifactor_energy.c fair_tree.c fair_tree.h priority_multifactor_energy.h
priority_multifactor_energy_la_LDFLAGS = $(SO_LDFLAGS) $(PLUGIN_FLAGS)
priority_multifactor_energy_la_LIBADD  = -lm $(PROTOBUF_LIB) -L$(LRZ_MODEL)/lib -lmodel
