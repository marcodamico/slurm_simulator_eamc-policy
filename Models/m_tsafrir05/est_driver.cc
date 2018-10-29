//##############################################################################
//        file: est_driver.cc
//      writer: Dan Tsafrir
// description: driver for the estimate model as defined in
//              "Modeling User Runtime Estimates", JSSPP'05
//##############################################################################


//##############################################################################
// include files
//##############################################################################
#include <vector>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include "est_model.hh"
#include "est_parse.hh"

using namespace std;


//##############################################################################
// the driver
//##############################################################################
int main(int argc, char *argv[])
{
    // read and parse command line args. if an SWFfile name was given, parse
    // and load it. args contains:
    // 1 maxest:    the maximal allowed estimate
    // 2 jobs:      if an SWFfile this vector hold all the jobs.
    // 3 njobs:     the number of jobs = the number of estimates to produce.
    // 4 userbins:  vector containing user give <estimate,job#> pairs (optional)
    // 5 precision: number of precision digits for flaoting-point SWF fields
    // 6 seed:      to the randome number generator.
    Args_t args(argc,argv);

    
    // this structure encapsulates all the parameters of the model.
    // - its constructor gets only the two mandatory parameters (njobs and
    //   maxest) along with the optional (and possibly empty) user bins that
    //   allow the user to specify site specific/unique information.
    // - howerver, all the other parameters can also be set explicitly e.g.
    //   model_params.binnum = 200;         // num of different estimate values
    //   model_params.binsiz_head_sum = 85; // % of "head" estimates
    EstParams_t model_params(args.njobs, args.maxest, args.userbins);

    
    // generate an estimate distribution according to the above parameters.
    // the model uses the drand48/lrand48 standard random generators.
    vector<EstBin_t> estimate_distribution;
    srand48(args.seed);
    est_gen_dist(model_params, &estimate_distribution);

    
    // print result.
    if( args.jobs.size() > 0 ) {
	//
	// SWFfile was given, jobs are held by the vector args.jobs; need to
	// assign the estimates to jobs (such that runtimes are never bigger
	// than estimates) and print resulting SWFfile.
	//
	vector<Job_t>& jobs = args.jobs;
	est_assign(estimate_distribution, &jobs);
	for(vector<Job_t>::iterator j = jobs.begin() ; j != jobs.end() ; ++j)
	    j->write(cout, args.precision);
    }
    else {
	//
	// no SWFfile: just print the estimate distribution (both PDF and CDF)
	//
	double            cdf  = 0;
	vector<EstBin_t>& dist = estimate_distribution;

	printf("#%7s %7s %10s %10s\n", "seconds", "njobs", "PDF", "CDF");
	
	for(vector<EstBin_t>::iterator i=dist.begin() ; i != dist.end() ; ++i) {
	    
	    double pdf = 100.0 * i->njobs / args.njobs;
	    cdf       += pdf;
		
	    printf("%8d %7d %10.6f %10.6f\n",
		   i->time,	// the estimate in seconds
		   i->njobs,	// number of jobs using this estimate
		   pdf,		// % of jobs using estimate
		   cdf);	// % of jobs using <= estimate
	}
    }
    
    return 0;
}


//##############################################################################
//                                   EOF
//##############################################################################
