//##############################################################################
//        File: est_model.cc
//      Writer: Dan Tsafrir
// Description: Implementing the estimates model.
//##############################################################################



//##############################################################################
// Include
//##############################################################################
#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <iterator>
#include <functional>
#include <math.h>
#include <stdlib.h>

// debug:
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdarg.h>

#include <assert.h>
#include "est_model.hh"

using namespace std;



//##############################################################################
// Macros
//##############################################################################

//------------------------------------------------------------------------------
// determine the number of entries in an array
//------------------------------------------------------------------------------
#define ARRLEN(arr) (sizeof(arr)/sizeof(arr[0]))

//------------------------------------------------------------------------------
// print error message and exit(1) [ switched to function for WIN32 ]
//------------------------------------------------------------------------------
void ERR(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

//------------------------------------------------------------------------------
// DBG messages
//------------------------------------------------------------------------------
#ifdef WIN32
#  if defined(DBG)
#    undef DBG
#  endif
#  define DBG __noop
#else
#  define DBG(fmt, ...)
   //#define DBG(fmt, ...)fprintf(stderr,"%s: " fmt, __func__, ## __VA_ARGS__)
#endif


//------------------------------------------------------------------------------
// missing functions from win32
//------------------------------------------------------------------------------
#ifdef WIN32
inline static int round(double x) { return x > 0 ? int(x+0.5) : int(x-0.5); }
inline void       srand48(long int seedval) { srand( int(seedval) ); }
inline double     drand48(void) { return double(rand()) / double(RAND_MAX); }
inline long int   lrand48(void) { return rand(); }
#endif



//##############################################################################
// Config
//##############################################################################

// seconds in minute
static const int MIN = 60;

// seconds in hour
static const int HOUR = MIN*60;

// how many ests constitute the head of the estimates distribution (these
// are used by 85-90% of the jobs).
static const int TOP = 20;

// top20-time-rank table (a total of TOP lines):
//     entry: 0 is max, 1-19 as usual
//   content: the popularity rank associated with that of the top20-time-rank
//            in the specified trace (column)
static const int TRACE_NUM = 4;
static const int TIMRNK_TO_POPRNK[][TRACE_NUM] = {
    //sdsc-106	ctc	kth4h	blue
    { 3,	1,	1,	1,	}, // 0
    { 1,	3,	4,	6,	}, // 1
    { 4,	4,	10,	5,	}, // 2
    { 17,	2,	14,	3,	}, // 3
    { 13,	12,	20,	7,	}, // 4
    { 7,	9,	2,	2,	}, // 5
    { 8,	8,	3,	18,	}, // 6
    { 18,	18,	7,	19,	}, // 7
    { 2,	6,	12,	4,	}, // 8
    { 6,	7,	6,	11,	}, // 9
    { 16,	11,	19,	20,	}, // 10
    { 10,	20,	5,	9,	}, // 11
    { 5,	16,	18,	10,	}, // 12
    { 15,	5,	16,	14,	}, // 13
    { 14,	14,	9,	13,	}, // 14
    { 19,	13,	17,	16,	}, // 15
    { 11,	10,	15,	15,	}, // 16
    { 12,	15,	13,	17,	}, // 17
    { 9,	17,	8,	8,	}, // 18
    { 20,	19,	11,	12,	}, // 19
};

// the 15 estimates out of the top-20, which appear in ALL four examined traces,
// [ NOTE that this array doesn't embody the popularity of the maximal estimate
//   which is set by default to be the most popular estimate (e.g. 18:00 will
//   be treated the most popular if it is the maximum ]
static const int EST_JOINT[] = {
      0*HOUR +  5*MIN, 		//  1
      0*HOUR + 15*MIN,		//  2
      0*HOUR + 10*MIN,		//  3
      0*HOUR + 20*MIN,		//  4
      0*HOUR + 30*MIN,		//  5
      1*HOUR +  0*MIN,		//  6
      2*HOUR +  0*MIN,		//  7
      3*HOUR +  0*MIN,		//  8
      4*HOUR +  0*MIN,		//  9
      5*HOUR +  0*MIN,		// 10
      6*HOUR +  0*MIN,		// 11
      8*HOUR +  0*MIN,		// 12
     10*HOUR +  0*MIN,		// 13
     12*HOUR +  0*MIN,		// 14
     18*HOUR +  0*MIN,		// 15
};
static const int EST_JOINT_SIZ = ARRLEN(EST_JOINT);

// to complement the above to TOP estimates, we add multiples of
// the following values (first of 100, then 50, etc) while conservatively
// moving from the maximal estimate downwards (e.g. if max_est=160h then
// the chosen bins in order would be 100,50,90,80,70,60,40, ...).
static const int EST_ROUND[] = {
    200*HOUR,
    100*HOUR,
     50*HOUR,
     10*HOUR,
      5*HOUR,
      2*HOUR,
      1*HOUR,
     20*MIN,
     10*MIN,
      5*MIN,
};
static const int EST_ROUND_SIZ = ARRLEN(EST_ROUND);

// points defining the binnum linear model (nests as a function of njobs)
struct binum_linear_t { int njobs, nests; } ;
binum_linear_t BINUM_LINEAR[] = {
    //	x	y	
    {	0,	0	},
    {	20,	10	},
    {	200,	20	},
    {	1000,	35	},
    {	10000,	90	},
    {	70000,	340	},
    {	250000,	565	},
};
static const int BINUM_LINEAR_SIZ = ARRLEN(BINUM_LINEAR);




//##############################################################################
// Utils:
//##############################################################################


//------------------------------------------------------------------------------
// Concat vector to a string.
//------------------------------------------------------------------------------
#if 0
static string vec2str(const vector<int> v, bool do_sort)
{
    vector<int>   u = v;
    ostringstream out;

    if( do_sort )
	sort(u.begin(), u.end());
    
    for(vector<int>::const_iterator i=u.begin(); i!=u.end(); ++i)
	out << *i << " "; 

    return out.str();
}
#endif


//------------------------------------------------------------------------------
// Check if the given key exists in the given container.
//------------------------------------------------------------------------------
template<typename Container_t>
bool exists(Container_t cont, typename Container_t::key_type key) {
    return cont.find(key) != cont.end();
}





//##############################################################################
// head:
//##############################################################################

//------------------------------------------------------------------------------
// Return a vector v containing the default times (seconds) of the TOP most
// popular estimates (or less than TOP if impossible).
//
// [ - Other than maxest located at v[0], the rest of the estimates
//     are ordered chronologically.
//   - All resulting estimates are <= maxest [seconds].
//   - This is the default suggestion, without considering the parameters
//     given by the users. ]
//------------------------------------------------------------------------------
static vector<int> gen_head_bintim(const EstParams_t& params)
{
    // what we're using from params
    int maxest = params.max_est;

    
    // return var
    vector<int> v;
    v.reserve(TOP);

    
    // 1- this is probably a realistic assumption (though arbitrary, not sure
    //    if it's needed, haven't tested).
    if( maxest < HOUR )
	ERR("maximal estimate=%d must be at least one hour", maxest);

    
    // 2- by definition of top20-time-rank, maxest comes first
    v.push_back( maxest );

    
    // 3- choose from EST_JOINT: estimates that are in range (and are different
    //    than the max, which was already chosen)
    for(int i=0; i < min( TOP, EST_JOINT_SIZ ); i++)
	if( EST_JOINT[i] < maxest )
	    v.push_back( EST_JOINT[i] );

    
    // 4- add the "round" estimates (complement v to hold TOP estimates)
    int missing = max( 0, int(TOP - v.size()) );
    for(int i=0; i<EST_ROUND_SIZ && missing>0; i++) {

	int mul   = EST_ROUND[i];
	int start = ( maxest / mul ) * mul;
	if( start == 0 )
	    continue; 	    // mul is bigger than maxest

	for(int j=0; (start-j*mul) > 0; j++) {
	    int est = start - j*mul;
	    if( ( missing                        > 0       ) &&
		( find(v.begin(), v.end(), est) == v.end() ) ) {
		v.push_back( est );
		missing--;
	    }
	    if( missing==0 )
		break;
	}
    }

    
    // 5- maxest is in entry[0], the rest is ascendingly sorted.
    sort( v.begin()+1, v.end() );

    
#ifndef NDEBUG
    // 6- sanity: should have
    //    a) length <= TOP
    //    b) only unique entries
    //    c) all <= maxest
    assert( v.size() <= unsigned(TOP) );
    vector<int>::const_iterator head=v.begin();
    for(vector<int>::const_iterator i=head+1; i!=v.end(); ++i) {
	assert( *i     < *head );
	assert( *(i-1) < *i    || (i-1==head) );
    }
#endif

    return v;
}


//------------------------------------------------------------------------------
// Needed for the implementation of gen_head_bintim_merge().
// Comparing a [user-estimate, default-estimate] pairs based on the absolute
// value of the time distance between the two estimates.
//------------------------------------------------------------------------------
struct Replace_t {
    int user_est, default_est;
    Replace_t (int ue, int de) : user_est(ue), default_est(de) {}
    bool operator<(const Replace_t& r) const {
	int me  = abs(user_est   - default_est  );
	int him = abs(r.user_est - r.default_est);
	if( me != him )
	    return me < him;
	else if( user_est != r.user_est )
	    return user_est < r.user_est;
	else {
	    assert( default_est != r.default_est );
	    return default_est < r.default_est;
	}
    }
};


//------------------------------------------------------------------------------
// Merge default estimates (generated by gen_head_bintim()) with the estimates
// supplied by the user (if any).
//
// [ Giving precedence (in order) to:
//   1) user estimates,
//   2) joint estimates ]
//------------------------------------------------------------------------------
static vector<int>
gen_head_bintim_merge(const EstParams_t & params,
		      const vector<int> & default_ests)
{
    // what we're using from params
    int                     maxest    = params.max_est;
    const vector<EstBin_t>& user_ests = params.bins;

    
    // 1- no user estimates => nothing to do
    if( user_ests.size() == 0 )
	return default_ests;

    
    // 2- user estimates exist.
    //    default[ROUND] contains "round" estimates,
    //    default[JOINT] contains "joint" estimates,
    //    these to partition 'default_ests'
    enum {ROUND=0, JOINT, PARTITION_NUM};
    set<int>  defaultp[PARTITION_NUM];
    const int NDEFAULT = default_ests.size();

    
    // 3- make the partition
    for(int i=0; i<NDEFAULT; i++) {
	
	int  est      = default_ests[i];
	bool is_joint = false;
	
	for(int k=0; k<EST_JOINT_SIZ; k++)
	    if( est == EST_JOINT[k] ) {
		defaultp[JOINT].insert( est );
		is_joint = true;
		break;
	    }

	if( ! is_joint )
	    defaultp[ROUND].insert( est );
    }

    
    // 4- filter the user estimates that aren't already found in 'default_ests'
    //    placing them in 'new_user_ests'
    const int NUSER = user_ests.size();
    set<int>  new_user_ests;
    for(int i=0; i<NUSER; i++) {
	
	int  uest  = user_ests[i].time;
	bool found = false;
	
	for(int k=0; k<NDEFAULT; k++)
	    if( uest == default_ests[k] ) {
		found = true;
		break;
	    }

	if( ! found )
	    new_user_ests.insert( uest );
    }

    
    // 5- how many default estimates must we replace?
    //    plus some helper vars/types to help us perform the replacement
    const int      NNEW          = new_user_ests.size();
    const int      NEED_REPLACE  = max( 0, NNEW - (TOP-NDEFAULT) );
    int            replace_count = 0;
    map<int,int>   replace_map_u2d; 	// user-est    => default-est
    map<int,int>   replace_map_d2u; 	// default-est => user-est
    set<Replace_t> replace_pool;
    typedef set<int>::const_iterator sci_t;

    
    // 6- decide which default estimates to replace by inserting:
    //       <user-estimate> => <default estimate it replaces>
    //    to the 'replace_map_u2d' mapping and the opposite pair to
    //    'replace_map_d2u'.
    for(int p=0; p<PARTITION_NUM; p++) {
	
	if( replace_count == NEED_REPLACE )
	    break;
	
	for(sci_t ue=new_user_ests.begin(); ue!=new_user_ests.end(); ++ue)
	    for(sci_t de=defaultp[p].begin(); de!=defaultp[p].end(); de++) {
		assert( *ue != * de );
		replace_pool.insert( Replace_t(*ue,*de) );
	    }

	while( ! replace_pool.empty() && replace_count < NEED_REPLACE ) {

	    set<Replace_t>::iterator repi = replace_pool.begin();//smallest dist
	    int                      uest = repi->user_est;
	    int                      dest = repi->default_est;
	    replace_pool.erase( repi );

	    // a) already assigned a replacement to uest, or
	    // b) dest was already assigned as a replacement, or
	    // c) not allowed to replace the max
	    if( exists(replace_map_u2d,uest) ||
		exists(replace_map_d2u,dest) ||
		dest == maxest )
		continue;
	    
	    replace_map_u2d[uest] = dest;
	    replace_map_d2u[dest] = uest;
	    replace_count++;
	}
    }


    // 7- finally, replace:
    vector<int> res = default_ests;
    for(int i=0; i<NDEFAULT; i++)
	if( exists(replace_map_d2u, res[i]) )
	    res[i] = replace_map_d2u[ res[i] ];

    
    // 8- also, append the NUSER-NEED_REPLACE that didn't need a replacement
    for(sci_t e=new_user_ests.begin(); e!=new_user_ests.end(); ++e)
	if( ! exists(replace_map_u2d, *e) )
	    res.push_back(*e);


    // 9- sort chronologically (recall the exception: [0] is maxest)
    assert( res[0] == maxest );
    sort( res.begin()+1, res.end() );
    
    return res;
}
    

//------------------------------------------------------------------------------
// Return percentage of jobs for the top20 popularity ranking estimates
// according to the model:  size(x) = a * e^(b*x) + c
// where x is the popularity rank (1 is the most popular estimate) and size
// is given in percentage of jobs.
// 
// NTOP:
//   How many bins (sizes) to produce (might be smaller than TOP).
//
// [ Note
//   - The exponential model is actually applied for x=2..20. The percentage
//     of x=1 is: params.binsiz_head_sum - sigma[size(x)] (x=2..20).
//   - x=1 is associated by default to the maximal estimate.
//   - Therefore, if one of 'ubins' is the MAXEST, then insread of doing the
//     above, we choose take the size associated with the MAXEST from 'ubins'
//     as the "missing" (so that we'd have a total of 20 values), and
//     proportionally scale the other sizes such that the total sum would
//     be binsiz_head_sum ]
//------------------------------------------------------------------------------
static vector<double>
gen_head_binsiz(const EstParams_t & params, const int NTOP)
{
    // what we're using from params
    const int                NJOBS           = params.njobs;
    const int                MAXEST          = params.max_est;
    const vector<EstBin_t> & ubins           = params.bins;
    const double           * hparam          = params.binsiz_head_params;
    const double             BINSIZ_HEAD_SUM = params.binsiz_head_sum;
    
    // return var
    vector<double> ret;
    ret.reserve(NTOP);

    
    // 1- the exponential model parameters: size(x) = a * e^(b*x) + c
    double a = hparam[0];
    double b = hparam[1];
    double c = hparam[2];

    // 2- find out whether one of the bins is associated with maxest, and if
    //    place its job number in 'maxest_njobs'
    int maxest_njobs = -1; 
    const int NUSER=ubins.size();
    for(int bin=0; bin<NUSER; bin++)
	if( ubins[bin].time == MAXEST ) {
	    maxest_njobs = ubins[bin].njobs;
	    break;
	}
	    
    // 3- reserve room to the first size and manufacture sizes for ranks 2..20
    ret.push_back(0);
    double sum = 0;
    for(int i=1; i<NTOP; i++) {
	double t = a * exp(b*double(i+1/*the rank*/)) + c;
	ret.push_back( t );
	sum += t;
    }

    // 4- now, if user supplied a size for maxest, well she's the boss, and
    //    the above manufactured values are scaled such that their sum (along
    //    with maxest's size) is BINSIZ_HEAD_SUM.
    if( maxest_njobs == -1 ) {
	ret[0] = BINSIZ_HEAD_SUM - sum;
	assert( ret[0] > 0 );
    }
    else {
	ret[0] = 100.0 * double(maxest_njobs) / double(NJOBS);
	const double F = ( double(BINSIZ_HEAD_SUM) - ret[0] ) / sum;
	for(int i=1; i<NTOP; i++)
	    ret[i] *= F;
    }
    

    sort(ret.begin(), ret.end(), greater<double>());// for the sake of the first
    return ret;
}


//------------------------------------------------------------------------------
// Needed for the implementation of gen_head_binsiz_merge().
// Descendingly compare indexes to user's 'bins' by their size (njobs).
//------------------------------------------------------------------------------
struct sizcmp_t {
    const vector<EstBin_t>& ubins;
    sizcmp_t(const vector<EstBin_t>& ub) : ubins(ub) {}
    bool operator()(int i,int j) const {return ubins[i].njobs > ubins[j].njobs;}
};


//------------------------------------------------------------------------------
// Merge default estimate sizes (generated by gen_head_binsiz) with the
// number-of-jobs supplied by the user through 'bins' (if any).
//
// default_siz
//   The default sizes (vector returned from gen_head_binsiz())
// uidx2sidx
//   Map bin-indexes (that is, indexes to 'params.bins') to indexes to the
//   returned vector. These associate the user bins to the position of their
//   size within the returned vector.
// Return
//   default_siz after merged with user bin sizes, sorted in descending order
//   (with the possible exception of [0], see note below).
// 
// [ Notes:
//   -The rules are to replace the default bins that are the closest to that of
//     the user. However, the size associated with maxest (entry 0 in
//     default_siz) is replaced only if maxest was explicitly given by the user.
//   - On the other hand, mexest's size [0] is scaled along with the others
///    non-user-given sizes.
//   - The result is that [0] of the returned vector is guaranteed to be the
//     biggest size among non-user sizes (unless it is specified by the user)
//   - All non-user sizes are set to 0 if the sum of the user sizes
//     exceed params.binsiz_head_sum ]
//------------------------------------------------------------------------------
static vector<double>
gen_head_binsiz_merge(const EstParams_t    & params,
		      const vector<double> & default_siz,
		      map<int,int>         * uidx2sidx)
{
    // what we're using from params
    const int     NJOBS           = params.njobs;
    const int     MAXEST          = params.max_est;
    const double  BINSIZ_HEAD_SUM = params.binsiz_head_sum;
    const vector<EstBin_t>& ubins = params.bins;
    
    // some more vars:
    const int NUSER    = ubins.size();
    const int NDEFAULT = default_siz.size(); 
    uidx2sidx->clear();

    
    // 1- no user estimates => nothing to do
    if( ubins.size() == 0 )
	return default_siz;

    
    // 2- order ubins by size (njobs), that is, when traversing ubins_by_siz
    //    the data is indexes to ubins, ordered from largest to smallest
    vector<int> ubins_by_siz;
    for(int i=0; i<NUSER; i++)
	ubins_by_siz.push_back(i);
    sort(ubins_by_siz.begin(), ubins_by_siz.end(), sizcmp_t(ubins));

    
    // 3- choose replacements
    map<int,int> sz2ubn;	// size-index to ubin-index
    map<int,int> ubn2sz;	// and vice versa
    
    for(int i=0; i<NUSER; i++) {
	
	int    ubidx = ubins_by_siz[i];
	double ubsiz = 100.0 * double(ubins[ubidx].njobs) / double(NJOBS);
	double ubest = ubins[ubidx].time;

	// find the (index of the) closest default-size to that of ubsiz
	int    closest      = -1;
	double closest_dist = -1;
	for(int k=0; k<NDEFAULT; k++) {

	    // size is already replaced, or maxest (which can only be replaced
	    // by explicit user request)
	    if( exists(sz2ubn,k) || (k==0 && ubest!=MAXEST) )
		continue;

	    double dsiz = default_siz[k];
	    double dist = fabs( ubsiz - dsiz );
		
	    if( (closest == -1) || (closest_dist > dist) ) {
		assert( k!=0 || ubest==MAXEST );
		closest      = k;
		closest_dist = dist;
	    }
	}

	// the ubsiz should replace the 'closest' size
	assert( closest      >= 0 );
	assert( closest_dist >= 0 );
	sz2ubn[closest] = ubidx;
	ubn2sz[ubidx  ] = closest;
    }

    
    // 4- replace
    vector<double> res = default_siz;
    for(int ubidx=0; ubidx<NUSER; ubidx++) {
	int sidx  = ubn2sz[ubidx];
	int ubsiz = ubins[ubidx].njobs;
	res[sidx] = 100 * double(ubsiz) / double(NJOBS);
    }


    // 5- sum percentage belonging to user and non-users bins
    enum {USER, NONUSER, ALL};
    double sum[] = {0,0,0};
    for(int sidx=0; sidx<NDEFAULT; sidx++) {
	int k   = ( ! exists(sz2ubn,sidx) ) ? NONUSER : USER;
	sum[k] += res[sidx];
    }
    sum[ALL] = sum[USER] + sum[NONUSER];

    
    // 6- normalize non-user sizes such that sum would be BINSIZ_HEAD_SUM, or
    //    zero non-user sizes if impossible.
    bool   do_norm = sum[USER] < BINSIZ_HEAD_SUM;
    const double F = ( double(BINSIZ_HEAD_SUM) - sum[USER] ) / sum[NONUSER];
    for(int sidx=0; sidx<NDEFAULT; sidx++)
	if( ! exists(sz2ubn,sidx) )
	    res[sidx] = do_norm ? F*res[sidx] : 0;

    
    // 7- almost done: indeed, finished replacing/normalizing, but 'res' might
    //    not be sorted. sorting is very tricky because ubn2sz/sz2ubn must be
    //    updated accordingly. this mandates sorting "by hand" :(
    //    recall that the size associated with maxest must stay at [0] so we're
    //    actually sorting the array from [1].
    for(int sz_idx=1; sz_idx<NDEFAULT; sz_idx++) {
	
	int sz_idx_max = sz_idx;
	
	for(int sz_j=sz_idx; sz_j<NDEFAULT; sz_j++) 
	    if( res[sz_idx_max] < res[sz_j] )
		sz_idx_max = sz_j;
	
	if( sz_idx_max != sz_idx ) { // swap...
	    //fprintf(stderr, "swapping: res[sz_idx=%d]=%.2f with res[sz_idx_max=%d]=%.2f\n",
	    //	sz_idx, res[sz_idx], sz_idx_max, res[sz_idx_max]);
	    
	    // swap values:
	    double tmp      = res[sz_idx];
	    res[sz_idx]     = res[sz_idx_max];
	    res[sz_idx_max] = tmp;
	    
	    // find indexes to ubins (if associated with the sizes):
	    int ub_idx      = -1;
	    int ub_idx_max  = -1;
	    if( exists(sz2ubn,sz_idx    ) ) 
		ub_idx      = sz2ubn[sz_idx    ];
	    if( exists(sz2ubn,sz_idx_max) )
		ub_idx_max  = sz2ubn[sz_idx_max];

	    // sanity
	    if( ub_idx != -1 ) {
		assert( ubn2sz[ub_idx] == sz_idx );
		assert( sz2ubn[sz_idx] == ub_idx );
	    }
	    if( ub_idx_max != -1 ) {
		//if( ubn2sz[ub_idx_max] != sz_idx_max ) 
		//fprintf(stderr,"ubn2sz[ub_idx_max=%d]=%d <> sz_idx_max=%d, sz_idx=%d\n",
		//ub_idx_max, ubn2sz[ub_idx_max], sz_idx_max, sz_idx);
		assert( ubn2sz[ub_idx_max] == sz_idx_max );
		assert( sz2ubn[sz_idx_max] == ub_idx_max );
	    }

	    // swap indexes
	    if( ub_idx != -1 ) {
		ubn2sz[ub_idx    ] = sz_idx_max;
		sz2ubn[sz_idx_max] = ub_idx;
		sz2ubn.erase(sz_idx);
	    }
	    if( ub_idx_max != -1 ) {
		ubn2sz[ub_idx_max] = sz_idx;
		sz2ubn[sz_idx    ] = ub_idx_max;
		if( ub_idx == -1 )
		    sz2ubn.erase(sz_idx_max);
	    }
	}
    }


    // done.
    *uidx2sidx = ubn2sz;
    return res;
}

//------------------------------------------------------------------------------
// bintim_m:
//   As returned by gen_head_bintim_merge(), that is, the top20 estimate times
//   after user supplied estimates were incorporated.
// bidx2sidx:
//   As returned by gen_head_binsiz_merge(), that is, mapping between user bin
//   indexes to indexes of the 'binsiz_m' vector (that was returned by the same
//   function) which directly correspond to popularity ranks.
// Return:
//   Mapping between top20-time-rank to popularity-rank, related to the bins
//   supplied by the user. This value later serves as the constraint to the
//   gen_head_timrnk_poprnk_map() function.
//------------------------------------------------------------------------------
static map<int,int>
gen_head_user_constraint(const EstParams_t & params,
			 const vector<int> & bintim_m,
			 map<int,int>        bidx2sidx)
{
    // what we're using from params
    const vector<EstBin_t>& ubins = params.bins;
    
    // some more vars:
    const int NTIMES = bintim_m.size();
    const int NBINS  = ubins.size();
    
    // generate a mapping between indexes of 'bintim_m' to associated bins
    // indexes (indexes to 'ubins'), if exist
    map<int,int> tidx2bidx;
    for(int tidx=0; tidx<NTIMES; tidx++)
	for(int bidx=0; bidx<NBINS; bidx++)
	    if( bintim_m[tidx] == ubins[bidx].time ) {
		tidx2bidx[tidx] = bidx;
		break;
	    }

    // generate the constraint
    map<int,int> tidx2sidx;
    for(int tidx=0; tidx<NTIMES; tidx++)
	if( exists(tidx2bidx,tidx) ) {
	    int bidx        = tidx2bidx[tidx];
	    int sidx        = bidx2sidx[bidx];
	    tidx2sidx[tidx] = sidx+1;
	    // +1 to shift rank from [0..19] (indexes) to [1..20] ranks
	}

    return tidx2sidx;
}

    
//------------------------------------------------------------------------------
// generate a mapping from top20-time-rank (recall 0 is maxest) to
// popularity rank according to TIMRNK_TO_POPRNK.
//
// user_constraint
//   A constraint fot mapping timrnk (tidx) to poprnk (sidx+1) according to the
//   user supplied bins, after those were merged to the default bintim/binsiz.
//   The mapping takes precedence on all other mapping considerations.
//------------------------------------------------------------------------------
static vector<int>
gen_head_timrnk_poprnk_map(map<int,int> user_constraint)
{
    vector<int>  mapping;	      // top20timrnk => popularity-rank
    vector<int>  pool;		      // randomly choose from this pool
    set<int>     chosen; 	      // which pop-ranks were already chosen
    set<int>     reserved;	      // data (not keys) of user_constraint
    map<int,int> bound;		      // poprnk => max-top20timrnk
    const bool   ENFORCE_BOUND=true;  // activate bound
    const int    FAVOR_SMALL=2;	      // favor smaller poprnks >= 1
    const bool   ENFORCE_MAX=true;    // activate max

    // fill 'bound': the maximal top20timrnk allowed for each poprnk
    // ttr = Top20-Time-Rank
    for(int ttr=TOP-1; ttr>=0; ttr--) { 
	for(int trace=0; trace<TRACE_NUM; trace++) {
	    int poprnk = TIMRNK_TO_POPRNK[ttr][trace];
	    if( ! exists(bound,poprnk) )
		bound[poprnk] = ttr;
	}
    }
 
    // init reserved. this is needed so that no non-user estimate will be
    // assigned a size that is associated with a user supplied estimate.
    typedef map<int,int>::const_iterator mci_t;
    for(mci_t p=user_constraint.begin(); p!=user_constraint.end(); ++p) {
	reserved.insert( p->second );
    }
     
    // create mapping for the rest
    mapping.reserve(TOP);
    pool.reserve( TOP*TRACE_NUM );
    for(int ttr=0; ttr<TOP; ttr++) { // ttr = Top20-Time-Rank

	DBG("-------------- mapping top20timrnk=%d ----------- \n",ttr);
	
	// 1- insert current row to the pool
	for(int trace=0; trace<TRACE_NUM; trace++) {
	    int poprnk = TIMRNK_TO_POPRNK[ttr][trace];
	    if( exists(chosen,poprnk) || exists(reserved,poprnk) )
		continue;
	    pool.push_back( poprnk );
	}
	
	// 2- random shuffle the pool
	DBG("pool before: %s\n", vec2str(pool,true /*sort */).c_str());
	DBG("pool before: %s\n", vec2str(pool,false/*don't*/).c_str());
	random_shuffle( pool.begin(), pool.end() );
	int N = pool.size();
	assert( N > 0 || exists(user_constraint,ttr) );
	DBG("pool after : %s\n", vec2str(pool,false).c_str());

	// 2- enforce user constraint if necessary
	int poprnk = -1;
	if( exists(user_constraint, ttr) ) {
	    poprnk = user_constraint[ttr];
	    DBG("CHOOSE poprnk=%2d [USER at ttr=%d]\n", poprnk, ttr);
	}
	
	// 3- enforce ttr=0 (max) to be the most popular
	if( poprnk==-1 && ttr==0 && ENFORCE_MAX ) {
	    poprnk=1;
	    DBG("CHOOSE poprnk=%2d [MAX at ttr=%d]\n", poprnk, ttr);
	}

	// 4- enforce bound if necessary:
	//    if there exists due poprnk-s, choose the most popular (smallest)
	if( poprnk==-1 && ENFORCE_BOUND )
	    for(int pr=1; pr<=TOP; pr++)
		if( bound[pr] <= ttr      &&   // is due
		    ! exists(chosen  ,pr) &&   // not chosen
		    ! exists(reserved,pr) ) {  // not reserved
		    poprnk = pr;
		    DBG("CHOOSE poprnk=%2d [BOUND at ttr=%d]\n", poprnk, ttr);
		    break;
		}

	// 5- otherwise, choose a poprnk at random from the pool
	if( poprnk == -1 ) {
	    for(int k=0; (k<FAVOR_SMALL)/* && (poprnk==-1)*/; k++) {
		int j = lrand48() % N;
		DBG("rand j=%d\n", j);
		if( (poprnk == -1) || (pool[j] < poprnk) ) {
		    poprnk = pool[j];
		    DBG("CHOOSE poprnk=%2d [RAND=%d at ttr=%d]\n",
			poprnk, j, ttr);
		}
	    }
	}

	// 6- set ttr to be mapped to poprnk
	assert( ! exists(chosen,poprnk) );
	mapping.push_back( poprnk );
	chosen.insert( poprnk );

	// 7- remove all occurrences of poprnk from pool
	vector<int> tmp = pool;
	pool.clear();
	for(int i=0; i<N; i++)
	    if( tmp[i] != poprnk )
		pool.push_back( tmp[i] );
    }

    return mapping;
}


//------------------------------------------------------------------------------
// Return a mapping between time (seconds) to size (percent) of the bin.
//------------------------------------------------------------------------------
static map<int,double> gen_head_map(const EstParams_t& params)
{
    // 1- generate the default head times (seconds) and merge to this default
    //    the time of the bins supplied by the user (if any).
    vector<int> head_bintim   = gen_head_bintim(params);
    vector<int> head_bintim_m = gen_head_bintim_merge(params, head_bintim);

    // 2- do the same for bin sizes. here we must also generate 'bidx2sidx'
    //    (a mapping between indexes of params.bins[] to the resulting
    //    vector 'head_binsiz_mrg') such that we can pinpoint which are the
    //    user sizes within the generated 'head_binsiz_mrg'. this is the
    //    "user constraint" later supplied to gen_head_timrnk_poprnk_map().
    map<int,int>   bidx2sidx;
    vector<double> head_binsiz  = gen_head_binsiz(params, head_bintim_m.size());
    vector<double> head_binsiz_m= gen_head_binsiz_merge(params, head_binsiz,
							&bidx2sidx);

    // 3- ontain the constraint for when mapping head_bintim_m to head_binsiz_m,
    //    and do the mapping: from=timrnk(0 is max), to=poprnk
    map<int,int> trnk2prnk_u   = gen_head_user_constraint(params, head_bintim_m,
							 bidx2sidx);
    vector<int>  timrnk2poprnk = gen_head_timrnk_poprnk_map(trnk2prnk_u);

    // 4- sanity
    assert( head_binsiz_m.size() == head_bintim_m.size() );	    
    assert( head_binsiz_m.size() == timrnk2poprnk.size() );

    // 5- this is the returned value
    map<int,double> ret;
    const int       N=head_bintim_m.size();

    // 6- do the mapping
    for(int i=0; i<N; i++) {
	int    mapto = timrnk2poprnk[i]-1; // -1 turns popularity-rank to index
	int    tim   = head_bintim_m[i];
	double siz   = head_binsiz_m[mapto];
	assert( ! exists(ret,tim) );
	ret[tim]     = siz;
    }

    return ret;
}




//##############################################################################
// tail
//##############################################################################


//------------------------------------------------------------------------------
// Return a vector of 'tail_nbins' tail estimate times (seconds), sorted from
// biggest to smalles. These are generated according to this model:
//     y = (a-1)*x / (a-x)
// where:
//     a = 'params.bintim_param'
//     x = normalized-time-rank (time rank divided by 'tail_nbins')
//     y = normalized-time      (time divided by 'params.max_est')
// generated values are aligned on a minute boundary, but may be offset-ed by
// a few seconds round number of seconds to avoid colliding with the the times
// om 'head_map' (which muchs time in seconds to size in percent).
//
// [ REMARK:
//   The actual definition of normalized-time-rank is time rank divided by
//   the total number of bins (nbins) not just tail_binsiz. But since there
//   are only TOP head bins and (usually) much more tail bins, it hopefully
//   doesn't matter. The alternative for this simplification is much more
//   complex :( ]
//------------------------------------------------------------------------------
static vector<int> gen_tail_bintim(const EstParams_t     & params,
				   const map<int,double> & head_map,
				   int                     tail_nbins)
{
    // what we're using from params
    int                maxest       = params.max_est;
    double             bintim_param = params.bintim_param;
    
    // vars
    vector<int> tail_v;		// the returned vector
    set<int>    tail_s;		// a set with same content as tail_v
    double      a=bintim_param;	// for short

    // offsets used in-order in case generated estimates collide with previously
    // generated offsets (whether head or tail)
    const int   OFFSET[]   = {0,+30,-30,+20,-20,+10,-10};
    const int   OFFSET_SIZ = ARRLEN(OFFSET);

    // generate the tail estimate times
    tail_v.reserve(tail_nbins);
    for(int i=1; i<=tail_nbins; i++) {

	// 1- obtain time in seconds
	double x   = double(i) / double(tail_nbins); // normalized time rank
	double y   = ((a-1)*x) / (a-x);         // normalized time
	double tim = y * double(maxest);        // time
	int    sec = int( round(tim/60.0)*60 ); // seconds (aligned to minutes)
	bool   ok  = false;

	// 2- add offset to avoid collision
	for(int off=0; off<OFFSET_SIZ; off++) {
	    int s = sec + OFFSET[off];
	    if( ! exists(tail_s  ,s) &&
		! exists(head_map,s) &&
		s>0 && s<maxest ) {
		ok  = true;
		sec = s;
		break;
	    }
	}

	// 3- add if succeeded avoiding collision
	if( ok ) {
	    tail_v.push_back(sec);
	    tail_s.insert(sec);
	}
    }

    sort(tail_v.begin(), tail_v.end());
    return tail_v;
}


//------------------------------------------------------------------------------
// Return a vector of 'tail_nbins' (tail) estimate sizes (percent), sorted from
// shortest to largest. These are generated according to this model:
//     y = a * x^b
// where:
//     a = params.binsiz_tail_params[0]
//     b = params.binsiz_tail_params[1]
//     x = popularity rank (21 ...)
//     y = percent of jobs (in the range (0,100))
// after the generation process is over, the sum of the percentages is
// normalized to be: 100-params.binsiz_head_sum
//------------------------------------------------------------------------------
static vector<double> gen_tail_binsiz(const EstParams_t& params, int tail_nbins)
{
    // what we're using from params
    const double *binsiz_tail_params = params.binsiz_tail_params;
    double        binsiz_head_sum    = params.binsiz_head_sum;
    
    // vars:
    vector<double> ret;
    const int      BASE = TOP+1;
    const double   A    = binsiz_tail_params[0];
    const double   B    = binsiz_tail_params[1];    
    ret.reserve(tail_nbins);

    // generate percentages
    for(int i=0; i<tail_nbins; i++) {
	int    poprnk  = BASE + i;
	double percent = A * pow( double(poprnk), B );
	ret.push_back( percent );
    }
    assert( unsigned(tail_nbins) == ret.size() );

    // normalize
    const int N   = ret.size();
    double    sum = 0;
    for(int i=0; i<N; i++)
	sum += ret[i];
    
    const double F = (100.0 - binsiz_head_sum) / sum;
    for(int i=0; i<N; i++)
	ret[i] *= F;

    // sort
    sort(ret.begin(), ret.end(), greater<double>());
    return ret;
}


//------------------------------------------------------------------------------
// Randomly map from times (second) to sizes (percent).
// The two vector (with equal lengths) are those returned from gen_tail_bintim()
// and gen_tail_binsiz().
//------------------------------------------------------------------------------
static map<int,double> gen_tail_map(const EstParams_t     & params,
				    const map<int,double> & head_map)
{
    // what we're using from params
    const int BINNUM = params.binnum;
    typedef map<int,double>::const_iterator mci_t;

    // 1- obtain tail_bintim (make sure not to collide with head_map, that
    //    is that no tail estimate would be a head estimate)
    int         tail_nbins  = BINNUM - (int)head_map.size();
    vector<int> tail_bintim = gen_tail_bintim(params, head_map, tail_nbins);
    
    // 2- obtain tail_binsiz. the size of tail_bintim theoretically might be
    //    less than requested if failed to avoid some collisions.
    tail_nbins = tail_bintim.size(); 
    vector<double> tail_binsiz = gen_tail_binsiz(params, tail_nbins);
    assert( tail_bintim.size() == tail_binsiz.size() );
    
    // 3- matching is randomized
    random_shuffle( tail_bintim.begin(), tail_bintim.end() );
    random_shuffle( tail_binsiz.begin(), tail_binsiz.end() );

    // 4- do the match
    const int       N=tail_bintim.size();
    map<int,double> ret;
    
    for(int i=0; i<N; i++) {
	assert( ! exists(ret, tail_bintim[i]) );
	ret[ tail_bintim[i] ] = tail_binsiz[i];
    }

    return ret;
}




//##############################################################################
// General:
//##############################################################################


//------------------------------------------------------------------------------
// Decide what will be the bin number in the estimates histogram.
//------------------------------------------------------------------------------
int get_binnum(const EstParams_t & params)
{
    const int               BINNUM = params.binnum;
    const int               NJOBS  = params.njobs;
    const vector<EstBin_t>& bins   = params.bins;
    const int               UBINS  = bins.size();

    
    // number of jobs by the user
    int user_njobs = 0;
    for(vector<EstBin_t>::const_iterator i=bins.begin(); i!=bins.end(); ++i)
	user_njobs += i->njobs;

    
    // binnum supplied by the user
    if( BINNUM > 0 ) {
	if( BINNUM < UBINS )
	    ERR("binnum=%d < bins.size()=%d", BINNUM, UBINS);
	if( BINNUM < user_njobs )
	    ERR("binnum=%d < user_njobs=%d", BINNUM, user_njobs);
	return BINNUM;
    }

    
    // use power model
    if( params.binnum_linear_model == false ) {
	double a   = params.binnum_power_params[0];
	double b   = params.binnum_power_params[1];
	int    ret = max( UBINS, (int)round(a*pow(double(NJOBS),b)) );
	return ret;
    }

    //
    // use linear model (the default)
    //
    if( NJOBS > BINUM_LINEAR[ BINUM_LINEAR_SIZ-1 ].njobs )
	return  BINUM_LINEAR[ BINUM_LINEAR_SIZ-1 ].nests;

    // find first k for which: NJOBS <= BINUM_LINEAR[k].njobs
    int ret=0, k;
    for(k=0; k<BINUM_LINEAR_SIZ; k++)
	if( NJOBS > BINUM_LINEAR[k].njobs )
	    ret = BINUM_LINEAR[k].nests;
	else
	    break;

    // sanity
    assert( k < BINUM_LINEAR_SIZ );
    assert( k > 0                );

    // linearly add leftover
    int    dx    = BINUM_LINEAR[k].njobs - BINUM_LINEAR[k-1].njobs;
    int    dy    = BINUM_LINEAR[k].nests - BINUM_LINEAR[k-1].nests;
    double slope = double(dy)/double(dx);
    double jleft = NJOBS - BINUM_LINEAR[k-1].njobs;
    ret         += int( round(jleft*slope) );
    
    return ret;
}



//------------------------------------------------------------------------------
// Comparing estimate times (seconds) according to the associated number
// of jobs. Needed by .
//------------------------------------------------------------------------------
struct sizcmp2_t {
    const map<int,int>& time2j;
    sizcmp2_t(const map<int,int>& t2j) : time2j(t2j) {}
    bool operator() (int time1, int time2) const {
	assert( exists( time2j, time1) );
	assert( exists( time2j, time2) );
	return time2j.find(time1)->second > time2j.find(time2)->second;
    }
};


//------------------------------------------------------------------------------
// Since the model works with percentages rather than actual job-numbers,
// when turning percentages into jobs, there might be a difference between
// the number of jobs the user requested and the one generated by the model.
// 'time2j' is this "dirty" mapping between estimate times (seconds) and job
// numbers.
// This function performs the equalization between the sum of jobs in 'time2j'
// to params.njobs.
//------------------------------------------------------------------------------
static map<int,int> adjust_njobs(const EstParams_t & params,
				 map<int,int>        time2j)
{
    const vector<EstBin_t> & bins  = params.bins;
    const int                NJOBS = params.njobs;
    typedef map<int,int>::const_iterator mapii_ci_t;

    
    // 1- make map of user supplied bins: from time to job number
    map<int,int> utime2j;
    for(vector<EstBin_t>::const_iterator i=bins.begin(); i!=bins.end(); ++i)
	utime2j[ i->time ] = i->njobs;

    
    // 2- current distribution of jobs between user/nonuser bins
    enum {USER=0, NONUSER, ALL, N};
    int jsum[] = {0,0,0};
    for(mapii_ci_t i=time2j.begin(); i!=time2j.end(); ++i) {
	int    time  = i->first;
	int    njobs = i->second;
	int    who   = exists(utime2j,time) ? USER : NONUSER;
	jsum[who]   += njobs;
	jsum[ALL]   += njobs;
    }

    
    // 3- do we have a problem?
    int diff = abs( NJOBS - jsum[ALL] );
    int sign = NJOBS > jsum[ALL] ? 1 : -1;
    if( diff == 0 )
	return time2j;
    
    
    // 4- heuristics of equalization
    enum {
	ATTEMPT_PROPORTIONAL=0,	// account for the difference proportionally
	ATTEMPT_INCREMENTAL,	// +/-1 change in each bin size
	ATTEMPT_TO_ONE,		// bin size is set to be 1
	ATTEMPT_TO_ZERO,	// bin size is set to be 0
    };


    // 5- equalize
    for(int attempt=0; diff!=0; attempt++) {

	// generate vector of non-user times sorted by size, descendingly
	vector<int> times;
	for(mapii_ci_t i=time2j.begin(); i!=time2j.end(); ++i)
	    if( ! exists(utime2j, i->first) )
		times.push_back( i->first );
	sort( times.begin(), times.end(), sizcmp2_t(time2j) );

	// vars
	int    cur_diff = diff;
	double f        = double(diff)/double(jsum[NONUSER]);

	// from biggest to smallest bin (in terms of size)
	for(vector<int>::const_iterator t=times.begin(); t!=times.end(); ++t) {
	    
	    int time   = *t;
	    int njobs  = time2j[*t];
	    int adjust = 0;

	    if( njobs == 0 )
		continue;

	    // adjustment according to the heuristic
	    switch (attempt) {
	    case ATTEMPT_PROPORTIONAL: adjust =(int)ceil(f*double(njobs));break;
	    case ATTEMPT_INCREMENTAL : adjust = 1                        ;break;
	    case ATTEMPT_TO_ONE      : adjust = njobs-1                  ;break;
	    case ATTEMPT_TO_ZERO     : adjust = njobs                    ;break;
	    default                  : assert(0);
	    }

	    // if we're about to subtract
	    if( sign < 0 ){
		if( adjust >  njobs )
		    adjust = njobs;
		if( adjust == njobs && attempt != ATTEMPT_TO_ZERO )
		    adjust--;
		assert( adjust <= njobs );
	    }

	    // in any case, shouldn't adjust more than difference:
	    if( adjust > cur_diff )
		adjust    = cur_diff;

	    // adjust
	    time2j[time] += ( sign*adjust );
	    cur_diff     -= adjust;
	    
	    if( cur_diff == 0 )
		break;
	}

	diff = cur_diff;
    }

    return time2j;
}


//------------------------------------------------------------------------------
// Some sanity checks to 'params'.
//------------------------------------------------------------------------------
void params_sanity_check(const EstParams_t& params)
{
        
    // make sure 'bins' contains not more than 20 (or 19 if not containing
    // maxest) _unique_ bins, and that their sum is <= 100%

    // remember: generate binsiz _after_ bintim was merged and the new size
    // is known.

    // make sure there are at least 500 (?) jobs 
}

//------------------------------------------------------------------------------
// Generate an estimate histogram.
//   params [in ]: The parameters of the model as defined above.
//   v      [out]: A non-null pointer. Assigned with the generated distribution.
//------------------------------------------------------------------------------
void est_gen_dist(EstParams_t params, vector<EstBin_t> *v) {

    const vector<EstBin_t> & bins  = params.bins;
    const int                NJOBS = params.njobs;
    typedef map<int,double>::const_iterator mapid_ci_t;
    typedef map<int,int   >::const_iterator mapii_ci_t;
    
    // 1- some validity checks on params & computing binnum 
    params_sanity_check(params);
    params.binnum = get_binnum(params);

    // 2- compute bintim_param, unless given by the user
    if( params.bintim_param <= 1.0 )
	params.bintim_param = 1.0 + 12.1039 * pow( params.binnum, -0.6026 );
    
    // 3- construct time2p: the actual distribution from time (sec) to
    //    size (% of jobs)
    map<int,double> head_time2p = gen_head_map(params             );
    map<int,double> tail_time2p = gen_tail_map(params, head_time2p);
    map<int,double> time2p      = tail_time2p;

    for(mapid_ci_t i=head_time2p.begin(); i!=head_time2p.end(); ++i) {
	int    time    = i->first;
	double percent = i->second;
	assert( ! exists(time2p,time) );
	time2p[time]   = percent;
    }

    // 4- sanity: make sure sum of percentages is 100%.
#ifndef NDEBUG
    double psum = 0;
    for(mapid_ci_t i=time2p.begin(); i!=time2p.end(); ++i)
	psum += i->second;
    assert( fabs(psum-100.0) < 1e-4 );
#endif

    // 5- construct utime2j: a map of user supplied bins from time to job number
    map<int,int> utime2j;
    for(vector<EstBin_t>::const_iterator i=bins.begin(); i!=bins.end(); ++i)
	utime2j[ i->time ] = i->njobs;
 
    // 6- construct time2j: turning percents (time2p) to actual job numbers,
    //    while respecting user bins specification (utime2j)
    map<int,int> time2j;
    for(mapid_ci_t i=time2p.begin(); i!=time2p.end(); ++i) {
	
	int    time    = i->first;
	double percent = i->second;
	bool   is_user = exists(utime2j,time);
	int    njobs   = is_user
	    ? utime2j[time]
	    : max( 1, (int)round( percent * (double(NJOBS)/100.0) ) );
	time2j[time]   = njobs;
    }

    // 7- obviously, due to rounding, it is very likely njobs_sum <> NJOBS.
    //    equalize the former to the latter.
    time2j = adjust_njobs(params,time2j);

    // 8- finally, assign result to the user vector
    v->clear();
    v->reserve( time2j.size() );
    for(mapii_ci_t i=time2j.begin(); i!=time2j.end(); ++i)
	v->push_back( EstBin_t( i->first/*time*/, i->second/*njobs*/ ) );
}



//##############################################################################
// tester
//##############################################################################

//------------------------------------------------------------------------------
// sort EstBin_t descendingly according to njobs
//------------------------------------------------------------------------------
struct sizcmp3_t {
    bool operator() (const EstBin_t& x, const EstBin_t& y) const {
	return x.njobs != y.njobs
	    ? x.njobs > y.njobs
	    : drand48() < 0.5;
    }
};



//##############################################################################
// assigning estimates to jobs.
//##############################################################################
#include <functional>


//------------------------------------------------------------------------------
// comparator of jobs: is runtime of left bigger than of right?
//------------------------------------------------------------------------------
struct job_runtime_greater_t {
    bool operator() (const Job_t *l, const Job_t *r) const {
	return l->runtime > r->runtime;
    }
};


//------------------------------------------------------------------------------
// shuffle the given estimate vector among the given jobs (instead of
// existing estimates). obviously the two vectors must have the same length.
//
// ASSUME: 'sests' is sorted from biggest to smallest (sests = sorted estimates)
//------------------------------------------------------------------------------
static void shuffle(vector<Job_t> *jobs, vector<double>& sests)
{
    // 1- descendingly sort jobs by runtime
    vector<Job_t*> sjobs;	
    sjobs.reserve( jobs->size() );
    for(vector<Job_t>::iterator ji=jobs->begin(); ji!=jobs->end(); ++ji) {
	Job_t& j = *ji;
	sjobs.push_back( &j );
    }
    sort(sjobs.begin(), sjobs.end(), job_runtime_greater_t());


    // 2- check if an assignment is possible: it is not possible if e.g.
    //    for some reason 80% of the jobs e.g. use the maximal allowed
    //    estimate, and the user didn't supply this hint.
    //
    //    note that a situation in which there are very many jobs using the
    //    maximal estimate can arise when using the estimate model in an
    //    autonomous mode with a maximal estimate that is much smaller than
    //    what was in reality.
    const int SIZE = (int)sjobs.size();
    for(int k=0; k<SIZE; k++) {
	if( sjobs[k]->runtime <= sests[k] )
	    continue;
	fprintf(stderr,
	  "the model FAILED to generate estimates to the input SWFfile\n"
	  "because many runtimes are suspiciously big (maybe the maximal\n"
	  "estimate you've chosen is too small?). two ways to OVERCOME this:\n"
	  "1) enlarge the maxest to be a more suitable value for the SWFfile.\n"
	  "2) explicitly suppling the number of jobs associated with the\n"
	  "   maximal estimate through the 'user-supplied bins' parameter of\n"
	  "   the model (see: est_model.hh; or the -b option if you use the\n"
	  "   driver of the model) and making this number suitablefor the\n"
	  "   input SWFfile.\n");
	exit(1);
    }

    
    // 3- shuffle ...
    for(int j=0, hi=0; j<SIZE; j++) {

	// find the upper bound (hi) of the estimates from which
	// it is possible to choose an estimate to j:
	while( (hi < SIZE-1) && (sjobs[j]->runtime <= sests[hi+1]) )
	    hi++;
	
	// randomly choose an index from the range [j...hi].
	// the estimate will be drawn from this range.
	int len            = hi - j + 1;
	int k              = (lrand48() % len) + j;
	sjobs[j]->estimate = sests[k];	
	assert( sjobs[j]->runtime <= sjobs[j]->estimate );

	// overwrite the k-estimation with the j-estimation, as it has already
	// been used.
	// note that there's no problem in placing the j-estimation wherever we
	// want (as long as it's forward) because this is an esstime that
	// is currently bigger than all the runtimes of jobs that are placed
	// left of j in sjobs.
	double tmp = sests[k];
	sests[k]   = sests[j];
	sests[j]   = tmp;
    }

    
    // 4- sanity: make sure all estimates are equal to or bigger than the
    // associated runtimes.
    for(vector<Job_t>::iterator j=jobs->begin(); j!=jobs->end(); ++j)
	assert( j->runtime <= j->estimate );
}


//------------------------------------------------------------------------------
// assign estimates to the jobs in 'jobs' according to the given estimate
// distribution as returned by est_gen_dist.
//------------------------------------------------------------------------------
void est_assign(const vector<EstBin_t>& est_dist, vector<Job_t>* jobs)
{
    int            maxest=0;
    vector<double> ests;
    ests.reserve( jobs->size() );
    
    // "flatten" distribution into a vector: one entry for each estimate.
    // also, find maxest while you're at it.
    for(vector<EstBin_t>::const_iterator b=est_dist.begin();
	b!=est_dist.end();
	++b) {
	for(int j=0; j < b->njobs; j++)
	    ests.push_back( b->time );
	if( maxest < b->time )
	    maxest = b->time;
    }

    // descendingly sort estimates (needed by shuffle)
    sort(ests.begin(), ests.end(), greater<double>());

    // if we don't have a bug, the number of generated estimates should be
    // equal to the number of jobs.
    assert( ests.size() == jobs->size() );

    // all runtimes must be equal to or smaller than maxest.
    // truncate those that aren't.
    int ntrunc=0;
    for(vector<Job_t>::iterator j=jobs->begin(); j != jobs->end(); ++j)
	if( j->runtime > maxest ) {
	    j->runtime = maxest;
	    ntrunc++;
	}
    if( ntrunc > 0 )
	fprintf(stderr,
	  "#\n"
	  "# WARNING: %d jobs have runtime > maxest=%d.\n"
	  "# WARNING: the runtime of these jobs was truncated to be maxest.\n"
	  "# WARNING: if this is done to too many jobs the model might fail.\n"
	  "#\n",
	  ntrunc, maxest);
    
    // all that's left is shuffle the artificial estimates within the jobs
    // such that all estimates are equal to or bigger than the runtime of
    // the corresponding jobs.
    shuffle(jobs, ests);
}


//##############################################################################
//                                   EOF
//##############################################################################
