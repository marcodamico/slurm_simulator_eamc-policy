//##############################################################################
//        file: est_parse.cc
//      writer: Dan Tsafrir
// description: implement est_parse.hh (do the actual parsing).
//##############################################################################



//##############################################################################
// include files
//##############################################################################
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>

#include "est_parse.hh"



//##############################################################################
// implementation 
//##############################################################################


//------------------------------------------------------------------------------
// error macro
//------------------------------------------------------------------------------
#define ER(fmt , ...) do {						\
    fprintf(stderr , fmt "\n" , ## __VA_ARGS__ );			\
    exit( EXIT_FAILURE );						\
} while( false )


//------------------------------------------------------------------------------
// print usage massage and exit.
//------------------------------------------------------------------------------
static int usage(char *exe)
{
    fprintf(stderr,
	    "usage: %s <maxest> <njobs|swf> [-b<userbins>] [-p<precision>]"
	    " [-s<seed>]\n"
	    "    maxest = maximal estimate in seconds\n"
	    "     njobs = number of jobs (= number of estimates to produce;\n"
	    "             if given, then prints only estimate histogram)\n"
	    "       swf = SWFfile (either this or njobs must be sepcified;\n"
	    "             if given, then reprints SWFfile with new estimates)\n"
	    "  userbins = user estimate bins (comma separated list of\n"
	    "             seconds=percent pairs; e.g. 3600=10.5,300=5.2 means\n"
	    "             10.5%% of the jobs will have a 1-hour estimate, and\n"
	    "             5.2%%  of the jobs will have a 5-min  estimate)\n"
	    " precision = if an SWFfile was given, set number of precision\n"
	    "             digits (right of floating point) for all SWF fields\n"
	    "             that may be expressed as floats (e.g. submit time,\n"
	    "             requested memory, etc). default=0.\n",
	    exe);
    exit(1);
}


//------------------------------------------------------------------------------
// read jobs from the given SWFfile into the jobs vector, while copying all
// non-job lines to the standard output (this is the header).
//
// only "sane" jobs are read, the rest are filtered out. a sane job is a jon
// with a positive and and a nonnegative runtime.
//------------------------------------------------------------------------------
static void parse_swf(vector<Job_t> *jobs, const char* swf_file)
{
    // 1- open the SWFfile
    ifstream swf(swf_file);
    if( ! swf ) 
	ER("failed opening file '%s': %s\n", swf_file, strerror(errno));

    // 2- ready...
    jobs->clear();
    jobs->reserve(100000);

    // 3- go!
    int insane=0;
    for(string line; ! getline(swf,line).eof(); ) {
	Job_t job;
	if( job.read( line.c_str() ) )
	    if( job.runtime >= 0 && job.size > 0 ) // sane job
		jobs->push_back( job );
	    else
		insane++;
	else
	    cout << line << "\n";
    }

    // 4- warn if filtered jobs
    if( insane > 0 )
	fprintf(stderr,
		"#\n"
		"# WARNING: %d jobs were filtered out (size<1 or runtime<0)\n",
		insane);
}


//------------------------------------------------------------------------------
// parse userbins_str, a comma separated list of user estimate bin pairs of
// the form: seconds=percent e.g. 3600=10.5,300=2.2 means 10.5% of the jobs
// will be set to use 1 hour as estimate (3600 sec) and 2.2% of the jobs will
// be set to use 5 minutes estimate (300 seconds).
//
// percents are converted to real numbers using 'njobs' (the number of jobs for
// which estimates are needed) and the result is placed in *userbins.
//------------------------------------------------------------------------------
static void
parse_userbins(vector<EstBin_t> *userbins, const char *userbins_str, int njobs)
{
    
    istringstream istr(userbins_str);	// wrap string with stream
    double        sump = 0;		// accumulate percents
    int           sumj = 0;		// accumulate number of jobs
    userbins->clear();

    for(int cnt=0;; cnt++) {
	
        int    seconds = -1;
        double percent = -1;
        char   comma   = -1;
        char   assign  = -1;
	
        // read separating comma
        if(cnt > 0) {
            istr >> comma;
            if( istr.eof() )
                break;
            if( comma != ',' )
                goto FAIL;
        }

        // read second=percent pair
        istr >> seconds >> assign >> percent;
        if( istr.fail() || assign != '=' || seconds <= 0 ||
            percent < 0 || percent > 100 )
            goto FAIL;
	
        // add pair
	int js = int( double(njobs) * percent / 100.0 );
	sumj  += js;
	sump  += percent;
        userbins->push_back( EstBin_t(seconds,js) );
    }

    // sanity
    if( sump > 100 )
	ER("sum of percents in '%s' must be <= 100", userbins_str);
    assert( sumj <= njobs );
    
    return;
    
 FAIL:
    ER("bad userbins='%s', should be comma separated list of sec=percent pairs",
       userbins_str);
}



//------------------------------------------------------------------------------
// parse command line and possibly an SWFfile.
//------------------------------------------------------------------------------
Args_t::Args_t(int argc, char *argv[])
    : precision(0)
{
    enum {OPT_USERBINS='b', OPT_PRECISION='p', OPT_SEED='s'};
    
    string userbins_str;
    int    c;
    
    // 1- first handle optional flags
    while( (c = getopt(argc,argv,"b:p:s:")) != -1 ) {
	switch(c) {
	case OPT_PRECISION:
	    if( (sscanf(optarg, "%d", &precision) != 1) || (precision < 0) ) 
		ER("precision=%s must be an integer >= 0 [digits]", optarg);
	    break;
	case OPT_USERBINS:
	    userbins_str= optarg; 
	    break;
	case OPT_SEED:
	    if( (sscanf(optarg, "%d", &seed) != 1) || (seed < 0) ) 
		ER("seed=%s must be an integer >= 0", optarg);
	    break;
	case -1:
	    break;
	default:
	    ER("unknown option or missing option argument");
	}
    }

    // 2- check that the two mandatory args are there
    if( argv[optind]==NULL || argv[optind+1]==NULL || argv[optind+2]!=NULL )
	usage(argv[0]);
  
    // 3- parse maxest
    if( (sscanf(argv[optind], "%d", &maxest) != 1) || (maxest <= 5700) ) 
	ER("maxest=%s must be an integer > 5700 [seconds]", argv[optind]);

    // 4- parse njobs|swf
    if( sscanf(argv[optind+1], "%d", &njobs) == 1 ) {
	if( njobs <= 226 )
	    ER("njobs=%s must be an integer > 226", argv[optind+1]);
    }
    else {
	parse_swf( &jobs, argv[optind+1] );
	njobs = jobs.size();
    }

    // 5- handle userbins
    if( userbins_str.length() > 0 )
	parse_userbins( &userbins, userbins_str.c_str(), njobs );

 
}


//##############################################################################
//                                   EOF
//##############################################################################
