#
# Makefile for simdate

AUTOMAKE_OPTIONS = foreign

LIBS = @LIBS@ -lrt

AM_CPPFLAGS = -I$(top_srcdir)

bin_PROGRAMS = sim_mgr simdate edit_trace simqsnap  list_trace trace_builder update_trace

sim_mgr_LDADD = 	$(top_builddir)/src/api/libslurm.o $(DL_LIBS) $(CMD_LDFLAGS)
simdate_LDADD = 	$(top_builddir)/src/api/libslurm.o $(DL_LIBS) $(CMD_LDFLAGS)
list_trace_LDADD = 	$(top_builddir)/src/api/libslurm.o $(DL_LIBS) $(CMD_LDFLAGS)
trace_builder_LDADD = 	$(top_builddir)/src/api/libslurm.o $(DL_LIBS) $(CMD_LDFLAGS)
update_trace_LDADD = 	$(top_builddir)/src/api/libslurm.o $(DL_LIBS) $(CMD_LDFLAGS)
#mysql_trace_builder_LDADD = 	$(top_builddir)/src/api/libslurm.o $(DL_LIBS) $(CMD_LDFLAGS)
simqsnap_LDADD = 	$(top_builddir)/src/api/libslurm.o $(DL_LIBS) $(CMD_LDFLAGS)

#noinst_HEADERS =
sim_mgr_SOURCES = sim_mgr.c sim_trace.h sim_trace.c
simdate_SOURCES = simdate.c
edit_trace_SOURCES = edit_trace.c
simqsnap_SOURCES = simqsnap.c
list_trace_SOURCES = list_trace.c sim_trace.c
trace_builder_SOURCES = trace_builder.c
update_trace_SOURCES = update_trace.c
#mysql_trace_builder_SOURCES = mysql_trace_builder.c

force:
$(simdate_LDADD) : force
	@cd `dirname $@` && $(MAKE) `basename $@`

sim_mgr_LDFLAGS = -export-dynamic $(CMD_LDFLAGS) \
	$(HWLOC_LDFLAGS) $(HWLOC_LIBS)

simdate_LDFLAGS = -export-dynamic $(CMD_LDFLAGS) \
	$(HWLOC_LDFLAGS) $(HWLOC_LIBS)

edit_trace_LDFLAGS = -export-dynamic $(CMD_LDFLAGS) \
	$(HWLOC_LDFLAGS) $(HWLOC_LIBS)

simqsnap_LDFLAGS = -export-dynamic $(CMD_LDFLAGS) \
	$(HWLOC_LDFLAGS) $(HWLOC_LIBS)

list_trace_LDFLAGS = -export-dynamic $(CMD_LDFLAGS) \
	$(HWLOC_LDFLAGS) $(HWLOC_LIBS)

trace_builder_LDFLAGS = -export-dynamic $(CMD_LDFLAGS) \
	$(HWLOC_LDFLAGS) $(HWLOC_LIBS)

update_trace_LDFLAGS = -export-dynamic $(CMD_LDFLAGS) \
	$(HWLOC_LDFLAGS) $(HWLOC_LIBS)

#mysql_trace_builder_LDFLAGS = -export-dynamic $(CMD_LDFLAGS) $(MYSQL_CFLAGS) \
#	$(HWLOC_LDFLAGS) $(HWLOC_LIBS) $(MYSQL_LIBS)
#mysql_trace_builder_CFLAGS = $(MYSQL_CFLAGS)
