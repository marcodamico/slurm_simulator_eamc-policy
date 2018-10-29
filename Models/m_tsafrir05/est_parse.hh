//##############################################################################
//        file: est_parse.hh
//      writer: Dan Tsafrir
// description: used by est_driver.cc to parse command line arguments and
//              possibly an SWFfile.
//##############################################################################
#ifndef EST_PARSE_HH__
#define EST_PARSE_HH__


//##############################################################################
// include files
//##############################################################################
#include <vector>
#include "est_model.hh"
using namespace std;



//##############################################################################
// the argv parser 
//##############################################################################
struct Args_t {
    
    // maximal estimate value, in seconds. mandatory parameter.
    int maxest;
    
    // if user gives a name of an SWFfile name as an argument, this file
    // is parsed and all "sane" jobs are placed in the following vector
    // (sane means:  runtime>=0). the driver will then generate estimates,
    // assign them to the jobs and reprint the SWFfile.
    // if user does not specifies an SWFfile name, this vector stays empty.
    vector<Job_t> jobs;
    
    // the number of jobs (= estimate values) to generate.
    int njobs;

    // optional param [default=empty]
    // user-specified estimate bins (defined in est.hh; an optinal argument).
    // o this is a comma separated list of user estimate bin pairs of the form:
    //   seconds=percent e.g. 3600=10.5,300=2.2 means 10.5% of the jobs will be
    //   set to use 1 hour as estimate (3600 sec) and 2.2% of the jobs will be
    //   set to use 5 minutes estimate (300 seconds).
    // o this list is translated to seconds=njobs pairs as required by est.hh
    //   and thi is the what is held in the following field.
    vector<EstBin_t> userbins;

    // optional param [default=0]
    // if an SWFfile is given, this field sets the number of digits appearing
    // to the right of the floating point, for all the SWF fields of the type
    // double. default is 0.
    int precision;

    // optional param [default=0]
    // seed of random numbers used by the model.
    int seed;
    
    // parse argv and initialize the above fields.
    Args_t (int argc, char *argv[]);
};


#endif /*EST_PARSE_HH__*/
//##############################################################################
//                                   EOF
//##############################################################################
