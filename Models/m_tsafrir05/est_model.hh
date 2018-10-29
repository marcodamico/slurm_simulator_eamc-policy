//##############################################################################
//        file: est_model.hh
//      writer: Dan Tsafrir
// description: interface to generate users runtime-estimate distribution.
//              implementing the model described in JSSPP'05 in the paper
//              "Modeling User Runtime Estimates"
//##############################################################################
#ifndef EST_MODEL_HH__
#define EST_MODEL_HH__



//##############################################################################
// include files
//##############################################################################
#include <vector>
#include <iomanip>
#include <stdio.h>
#include "est_swf_job.hh"




//##############################################################################
// types
//##############################################################################


//------------------------------------------------------------------------------
// Representing one bin of the estimate distribution.
//------------------------------------------------------------------------------
struct EstBin_t {
    int time;			// actual estimate [in seconds]
    int njobs;			// num of jobs having 'time' as estimate
    EstBin_t(int t=-1, int n=-1) {time=t; njobs=n;}
};


//------------------------------------------------------------------------------
// Pramaters of the model.
// - Note that the first two parameters are mandatory.
// - Is is also strongly recommended to supply EstBin_t-s that reflect your
//   system (as many as possible, optimally 20)
//------------------------------------------------------------------------------
struct EstParams_t {
    
    // How many jobs are there? (how many estimates to produce) [MANDATORY]
    int njobs;


    // What is the maximal estimate (seconds) on the modeled system? [MANDATORY]
    int max_est;
    
    
    // Hints for the requested histogram. [STRONGLY RECOMMENDED]
    // - May contain the most popular estimate bins (up to 20).
    // - Each bin must have a unique time which is <= max_est.
    // - The sum of the jobs associated with each bin must be <= njobs.
    std::vector<EstBin_t> bins;
    
    
    // Determining number of estimate-bins according to linear or power models?
    // On by default.
    bool binnum_linear_model;

    
    // If using the power model (bins = a*njobs^b), the user may provide the
    // values of 'a' and 'b'. If left unchanged, the values used are:
    //   a = binnum_power_params[0] = 0.653566
    //   b = binnum_power_params[1] = 0.547761
    double binnum_power_params[2];

    
    // How many estimate bins? Set to -1 if undecided. If this parameter is
    // given, then 'binnum_linear_model' / 'binnum_power_params' will be
    // ignored.
    int binnum;

    
    // For determining the size (percent of jobs) associated with the most
    // popular estimates: those with popularity-rank of: 2,3,4...20
    //   [ that is, all top ranking but the first; if you also want to set
    //     the size of the top ranking estimate, so it through the 'bins'
    //     parameter ]
    // we use an exponential model:  size(x) = a * e^(b*x) + c
    //   [ where x is the popularity-rank (smaller x implies increased
    //     popularity) ]
    // The user may provide the values of a / b / c.
    // If left unchanged, the values used are:
    //   a = binsiz_head_params[0] = 14.0491
    //   b = binsiz_head_params[1] = -0.177531
    //   c = binsiz_head_params[2] =  0.462513 
    double binsiz_head_params[3];

    
    // Cumulative percentages of head bins (the percent of jobs using on of
    // the top 20 popular estimates). 89% by default.
    // This parameter along with binsiz_head_params[] is used to deduce the
    // size of the top ranking bin.
    // After the user supplied 'bins' are merged to the model, the non-user
    // supplied bins are scaled (if possible) such that the percentage sum
    // of the top20 is binsiz_head_sum.
    double binsiz_head_sum;

    
    // For determining the size (percent) of the rest of the estimate bins
    // (with popularity rank >= 21) we use a power model: size(x) = a * x^b
    // [ where x is the popularity rank (21 is the most popular estimate) ].
    // The user can provide the values of a / b.
    // If left unchanged, the values used are:
    //   a = binsiz_tail_params[0] =  795.6
    //   b = binsiz_tail_params[1] = -2.267
    double binsiz_tail_params[3];
    

    // For determining the actual times of estimates we use the model:
    //   time(x) = (a-1)*x / (a-x)
    // [ where x is the relative time-rank (time-rank / bin#)
    //   and time(x) is the relative time (estimate / max_estimate) ].
    // This field is the "a" parameter. If left unchanged, the value used is:
    //   a = bintim_param = 1 + p * K^q
    // where
    //   p = 12.1039
    //   q = -0.6026
    //   K = the 'binnum' of this structure
    double bintim_param;

    
    //--------------------------------------------------------------------------
    // Default ctor: supply default values for the above parameters.
    //--------------------------------------------------------------------------
    EstParams_t( int                   njobs,
		 int                   maxest/*seconds*/,
		 std::vector<EstBin_t> b = std::vector<EstBin_t>() )
	: bins(b)
    {
	this->njobs            =  njobs;
	this->max_est          =  maxest;
	
	binnum_linear_model    =   true;
	binnum_power_params[0] =   0.653566;
	binnum_power_params[1] =   0.547761;
	binnum                 =  -1;
	
	binsiz_head_params[0]  =  14.0491;
	binsiz_head_params[1]  =  -0.177531;
	binsiz_head_params[2]  =   0.462513;
	binsiz_head_sum        =  89.0;
	binsiz_tail_params[0]  = 795.6;
	binsiz_tail_params[1]  =  -2.267;

	bintim_param           =  -1;
	
#if 0
	if( njobs < 30000 )
	    bintim_param       =   1.9;
	else if( njobs < 100000 )
	    bintim_param       =   1.5;
	else
	    bintim_param       =   1.25;
#endif
    }
};



//##############################################################################
// The interface:
//##############################################################################


//------------------------------------------------------------------------------
// Generate an estimate histogram.
//      params [in ]: The parameters of the model as defined above.
//   est_dist  [out]: Non-null. Assigned with the generated distribution.
//------------------------------------------------------------------------------
void est_gen_dist(EstParams_t params, std::vector<EstBin_t> *est_dist);
	  

//------------------------------------------------------------------------------
// assign estimates to the jobs in 'jobs' according to the given estimate
// distribution as returned by est_gen_dist.
//------------------------------------------------------------------------------
void est_assign(const std::vector<EstBin_t>& est_dist,
		std::vector<Job_t>*          jobs);


#endif /*EST_MODEL_HH__*/
//##############################################################################
//                                   EOF
//##############################################################################
