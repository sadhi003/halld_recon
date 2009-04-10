// $Id$
//
//    File: DTrackFitterALT1.cc
// Created: Tue Sep  2 11:18:22 EDT 2008
// Creator: davidl
//
namespace{const char* GetMyID(){return "$Id$";}}

#include <math.h>

#include <TROOT.h>

#include <DVector3.h>
#include <DMatrix.h>

#include <JANA/JEventLoop.h>
using namespace jana;

#include "GlueX.h"
#include "DANA/DApplication.h"
#include "DMagneticFieldStepper.h"
#include "DTrackCandidate.h"
#include "DTrackFitterALT1.h"
#include "CDC/DCDCTrackHit.h"
#include "FDC/DFDCPseudo.h"
#include "DReferenceTrajectory.h"
#include "DMCThrown.h"

#define NaN std::numeric_limits<double>::quiet_NaN()

// The GNU implementation of STL includes definitions of "greater" and "less"
// but the SunOS implementation does not. Since it is a bit of a pain to
// define this only for SunOS, we just define "greaterthan" and use it for
// all platforms/compilers. Note that this is essentially the same as the
// GNU definition from stl_function.h, except it does not derive from the
// templated "binary_function" class.
template<typename T>
class greaterthan{
	public: bool operator()(const T &a, const T &b) const {return a>b;}
};


//------------------
// DTrackFitterALT1   (Constructor)
//------------------
DTrackFitterALT1::DTrackFitterALT1(JEventLoop *loop):DTrackFitter(loop)
{
	this->loop = loop;

	// Define target center
	target = new DCoordinateSystem();
	target->origin.SetXYZ(0.0, 0.0, 65.0);
	target->sdir.SetXYZ(1.0, 0.0, 0.0);
	target->tdir.SetXYZ(0.0, 1.0, 0.0);
	target->udir.SetXYZ(0.0, 0.0, 1.0);
	target->L = 30.0;

	MAX_HIT_DIST = 2.0; // cm
	DEBUG_HISTS = false;
	DEBUG_LEVEL = 0;
	USE_CDC = true;
	USE_FDC_ANODE = true;
	USE_FDC_CATHODE = true;
	MAX_CHISQ_DIFF = 1.0E-2;
	MAX_FIT_ITERATIONS = 50;
	SIGMA_CDC = 0.0150;
	SIGMA_FDC_ANODE = 0.0200;
	SIGMA_FDC_CATHODE = 0.0200;
	CHISQ_MAX_RESI_SIGMAS = 100.0;
	CHISQ_GOOD_LIMIT = 2.0;
	LEAST_SQUARES_DP = 0.0001;
	LEAST_SQUARES_DX = 0.010;
	LEAST_SQUARES_MIN_HITS = 3;
	LEAST_SQUARES_MAX_E2NORM = 1.0E8;
	DEFAULT_STEP_SIZE = 0.5;
	MIN_CDC_HIT_PROB = 0.2;
	MAX_CDC_DOUBLE_HIT_PROB = 0.1;
	MIN_FDC_HIT_PROB = 0.2;
	MAX_FDC_DOUBLE_HIT_PROB = 0.1;
	TOF_MASS = 0.13957018;
	TARGET_CONSTRAINT = false;
	
	gPARMS->SetDefaultParameter("TRKFIT:MAX_HIT_DIST",					MAX_HIT_DIST);
	gPARMS->SetDefaultParameter("TRKFIT:DEBUG_HISTS",					DEBUG_HISTS);
	gPARMS->SetDefaultParameter("TRKFIT:DEBUG_LEVEL",					DEBUG_LEVEL);
	gPARMS->SetDefaultParameter("TRKFIT:USE_CDC",						USE_CDC);
	gPARMS->SetDefaultParameter("TRKFIT:USE_FDC_ANODE",				USE_FDC_ANODE);
	gPARMS->SetDefaultParameter("TRKFIT:USE_FDC_CATHODE",				USE_FDC_CATHODE);
	gPARMS->SetDefaultParameter("TRKFIT:MAX_CHISQ_DIFF",				MAX_CHISQ_DIFF);
	gPARMS->SetDefaultParameter("TRKFIT:MAX_FIT_ITERATIONS",			MAX_FIT_ITERATIONS);
	gPARMS->SetDefaultParameter("TRKFIT:SIGMA_CDC",						SIGMA_CDC);
	gPARMS->SetDefaultParameter("TRKFIT:SIGMA_FDC_ANODE",				SIGMA_FDC_ANODE);
	gPARMS->SetDefaultParameter("TRKFIT:SIGMA_FDC_CATHODE",			SIGMA_FDC_CATHODE);
	gPARMS->SetDefaultParameter("TRKFIT:CHISQ_MAX_RESI_SIGMAS",		CHISQ_MAX_RESI_SIGMAS);
	gPARMS->SetDefaultParameter("TRKFIT:CHISQ_GOOD_LIMIT",			CHISQ_GOOD_LIMIT);
	gPARMS->SetDefaultParameter("TRKFIT:LEAST_SQUARES_DP",			LEAST_SQUARES_DP);
	gPARMS->SetDefaultParameter("TRKFIT:LEAST_SQUARES_DX",			LEAST_SQUARES_DX);
	gPARMS->SetDefaultParameter("TRKFIT:LEAST_SQUARES_MIN_HITS",	LEAST_SQUARES_MIN_HITS);
	gPARMS->SetDefaultParameter("TRKFIT:LEAST_SQUARES_MAX_E2NORM",	LEAST_SQUARES_MAX_E2NORM);		
	gPARMS->SetDefaultParameter("TRKFIT:DEFAULT_STEP_SIZE",			DEFAULT_STEP_SIZE);
	gPARMS->SetDefaultParameter("TRKFIT:MIN_CDC_HIT_PROB",			MIN_CDC_HIT_PROB);
	gPARMS->SetDefaultParameter("TRKFIT:MAX_CDC_DOUBLE_HIT_PROB",	MAX_CDC_DOUBLE_HIT_PROB);
	gPARMS->SetDefaultParameter("TRKFIT:MIN_FDC_HIT_PROB",			MIN_FDC_HIT_PROB);
	gPARMS->SetDefaultParameter("TRKFIT:MAX_FDC_DOUBLE_HIT_PROB",	MAX_FDC_DOUBLE_HIT_PROB);
	gPARMS->SetDefaultParameter("TRKFIT:TOF_MASS",						TOF_MASS);
	gPARMS->SetDefaultParameter("TRKFIT:TARGET_CONSTRAINT",			TARGET_CONSTRAINT);
	
	// DReferenceTrajectory objects
	rt = new DReferenceTrajectory(bfield);
	tmprt = new DReferenceTrajectory(bfield);
	
	// Set limits on CDC. (This should eventually come from HDDS)
	CDC_Z_MIN = 17.0;
	CDC_Z_MAX = CDC_Z_MIN + 150.0;
	hit_based = false;
	//cout<<__FILE__<<":"<<__LINE__<<"-------------- Helical Fitter TRACKING --------------"<<endl;
	cout<<__FILE__<<":"<<__LINE__<<"-------------- Least Squares TRACKING --------------"<<endl;

	DApplication* dapp = dynamic_cast<DApplication*>(loop->GetJApplication());
	if(!dapp){
		_DBG_<<"Cannot get DApplication from JEventLoop! (are you using a JApplication based program?)"<<endl;
		return;
	}
	
	if(DEBUG_HISTS){
		dapp->Lock();
		
		// Histograms may already exist. (Another thread may have created them)
		// Try and get pointers to the existing ones.
		cdcdoca_vs_dist = (TH2F*)gROOT->FindObject("cdcdoca_vs_dist");
		cdcdoca_vs_dist_vs_ring = (TH3F*)gROOT->FindObject("cdcdoca_vs_dist_vs_ring");
		fdcdoca_vs_dist = (TH2F*)gROOT->FindObject("fdcdoca_vs_dist");
		fdcu_vs_s = (TH2F*)gROOT->FindObject("fdcu_vs_s");
		dist_axial = (TH1F*)gROOT->FindObject("dist_axial");
		doca_axial = (TH1F*)gROOT->FindObject("doca_axial");
		dist_stereo = (TH1F*)gROOT->FindObject("dist_stereo");
		doca_stereo = (TH1F*)gROOT->FindObject("doca_stereo");
		chisq_final_vs_initial = (TH2F*)gROOT->FindObject("chisq_final_vs_initial");
		nhits_final_vs_initial = (TH2F*)gROOT->FindObject("nhits_final_vs_initial");
		Npasses = (TH1F*)gROOT->FindObject("Npasses");
		ptotal = (TH1F*)gROOT->FindObject("ptotal");
		residuals_cdc = (TH2F*)gROOT->FindObject("residuals_cdc");
		residuals_fdc_anode = (TH2F*)gROOT->FindObject("residuals_fdc_anode");
		residuals_fdc_cathode = (TH2F*)gROOT->FindObject("residuals_fdc_cathode");
		residuals_cdc_vs_s = (TH3F*)gROOT->FindObject("residuals_cdc_vs_s");
		residuals_fdc_anode_vs_s = (TH3F*)gROOT->FindObject("residuals_fdc_anode_vs_s");
		residuals_fdc_cathode_vs_s = (TH3F*)gROOT->FindObject("residuals_fdc_cathode_vs_s");
		initial_chisq_vs_Npasses = (TH2F*)gROOT->FindObject("initial_chisq_vs_Npasses");
		chisq_vs_pass = (TH2F*)gROOT->FindObject("chisq_vs_pass");
		dchisq_vs_pass = (TH2F*)gROOT->FindObject("dchisq_vs_pass");
		cdc_single_hit_prob = (TH1F*)gROOT->FindObject("cdc_single_hit_prob");
		cdc_double_hit_prob = (TH1F*)gROOT->FindObject("cdc_double_hit_prob");
		fdc_single_hit_prob = (TH1F*)gROOT->FindObject("fdc_single_hit_prob");
		fdc_double_hit_prob = (TH1F*)gROOT->FindObject("fdc_double_hit_prob");
		cdc_can_resi = (TH1F*)gROOT->FindObject("cdc_can_resi");
		fdc_can_resi = (TH1F*)gROOT->FindObject("fdc_can_resi");
		fdc_can_resi_cath = (TH1F*)gROOT->FindObject("fdc_can_resi_cath");
		chisq_vs_p_vs_theta = (TH2F*)gROOT->FindObject("chisq_vs_p_vs_theta");

		if(!cdcdoca_vs_dist)cdcdoca_vs_dist = new TH2F("cdcdoca_vs_dist","DOCA vs. DIST",300, 0.0, 1.2, 300, 0.0, 1.2);
		if(!cdcdoca_vs_dist_vs_ring)cdcdoca_vs_dist_vs_ring = new TH3F("cdcdoca_vs_dist_vs_ring","DOCA vs. DIST vs. ring",300, 0.0, 1.2, 300, 0.0, 1.2,23,0.5,23.5);
		if(!fdcdoca_vs_dist)fdcdoca_vs_dist = new TH2F("fdcdoca_vs_dist","DOCA vs. DIST",500, 0.0, 2.0, 500, 0.0, 2.0);
		if(!fdcu_vs_s)fdcu_vs_s = new TH2F("fdcu_vs_s","DOCA vs. DIST along wire",500, -60.0, 60.0, 500, -60.0, 60.0);
		if(!dist_axial)dist_axial = new TH1F("dist_axial","Distance from drift time for axial CDC wires",300,0.0,3.0);
		if(!doca_axial)doca_axial = new TH1F("doca_axial","DOCA of track for axial CDC wires",300,0.0,3.0);
		if(!dist_stereo)dist_stereo = new TH1F("dist_stereo","Distance from drift time for stereo CDC wires",300,0.0,3.0);
		if(!doca_stereo)doca_stereo = new TH1F("doca_stereo","DOCA of track for axial CDC wires",300,0.0,3.0);
		if(!chisq_final_vs_initial)chisq_final_vs_initial = new TH2F("chisq_final_vs_initial","Final vs. initial chi-sq.",200, 0.0, 10.0,50, 0.0, 2.5);
		if(!nhits_final_vs_initial)nhits_final_vs_initial = new TH2F("nhits_final_vs_initial","Final vs. initial nhits used in chi-sq.",30, -0.5, 29.5, 30, -0.5, 29.5);
		if(!Npasses)Npasses = new TH1F("Npasses","Npasses", 101, -0.5, 100.5);
		if(!ptotal)ptotal = new TH1F("ptotal","ptotal",1000, 0.1, 8.0);
		if(!residuals_cdc)residuals_cdc = new TH2F("residuals_cdc","Residuals in CDC",1000,-2.0,2.0,24,0.5,24.5);
		if(!residuals_fdc_anode)residuals_fdc_anode = new TH2F("residuals_fdc_anode","Residuals in FDC anodes",1000,-2.0,2.0,24,0.5,24.5);
		if(!residuals_fdc_cathode)residuals_fdc_cathode = new TH2F("residuals_fdc_cathode","Residuals in FDC cathode",1000,-2.0,2.0,24,0.5,24.5);
		if(!residuals_cdc_vs_s)residuals_cdc_vs_s = new TH3F("residuals_cdc_vs_s","Residuals in CDC vs. pathlength",1000,-2.0,2.0,24,0.5,24.5,100, 0.0, 800);
		if(!residuals_fdc_anode_vs_s)residuals_fdc_anode_vs_s = new TH3F("residuals_fdc_anode_vs_s","Residuals in FDC anode vs. pathlength",1000,-2.0,2.0,24,0.5,24.5,100, 0.0, 800);
		if(!residuals_fdc_cathode_vs_s)residuals_fdc_cathode_vs_s = new TH3F("residuals_fdc_cathode_vs_s","Residuals in FDC cathode vs. pathlength",1000,-2.0,2.0,24,0.5,24.5,100, 0.0, 800);
		if(!initial_chisq_vs_Npasses)initial_chisq_vs_Npasses = new TH2F("initial_chisq_vs_Npasses","Initial chi-sq vs. number of iterations", 25, -0.5, 24.5, 400, 0.0, 40.0);
		if(!chisq_vs_pass)chisq_vs_pass = new TH2F("chisq_vs_pass","Chi-sq vs. fit iteration", 25, -0.5, 24.5, 400, 0.0, 40.0);
		if(!dchisq_vs_pass)dchisq_vs_pass = new TH2F("dchisq_vs_pass","Change in Chi-sq vs. fit iteration", 25, -0.5, 24.5, 400, 0.0, 8.0);
		if(!cdc_single_hit_prob)cdc_single_hit_prob = new TH1F("cdc_single_hit_prob","Probability a CDC hit belongs to a track for all tracks",200,0.0,1.0);
		if(!cdc_double_hit_prob)cdc_double_hit_prob = new TH1F("cdc_double_hit_prob","Probability a CDC hit belongs to two tracks",200,0.0,1.0);
		if(!fdc_single_hit_prob)fdc_single_hit_prob = new TH1F("fdc_single_hit_prob","Probability a FDC hit belongs to a track for all tracks",200,0.0,1.0);
		if(!fdc_double_hit_prob)fdc_double_hit_prob = new TH1F("fdc_double_hit_prob","Probability a FDC hit belongs to two tracks",200,0.0,1.0);
		if(!cdc_can_resi)cdc_can_resi = new TH1F("cdc_can_resi","Residual of CDC hits with candidate tracks", 200, -1.0, 1.0);
		if(!fdc_can_resi)fdc_can_resi = new TH1F("fdc_can_resi","Residual of FDC hits with candidate tracks", 200, -1.0, 1.0);
		if(!fdc_can_resi_cath)fdc_can_resi_cath = new TH1F("fdc_can_resi_cath","Residual of FDC cathode hits with candidate tracks", 200, -1.0, 1.0);
		//if(!chisq_vs_p_vs_theta)chisq_vs_p_vs_theta = new TH2F("chisq_vs_p_vs_theta","#chi^{2}/dof map", 100, 0.9, 1.1, 100, 0.9, 1.1);

		dapp->Unlock();
	}
}

//------------------
// DTrackFitterALT1   (Destructor)
//------------------
DTrackFitterALT1::~DTrackFitterALT1()
{
	if(rt)delete rt;
	if(tmprt)delete tmprt;
}

//------------------
// FitTrack
//------------------
DTrackFitter::fit_status_t DTrackFitterALT1::FitTrack(void)
{
	/// Fit a track candidate
	
	// Debug message
	if(DEBUG_LEVEL>2){
		_DBG__;
		_DBG_<<"cdchits.size="<<cdchits.size()<<"  fdchits.size="<<fdchits.size()<<endl;
		if(fit_type==kTimeBased)_DBG_<<"============= Time-based ==============="<<endl;
		if(fit_type==kWireBased)_DBG_<<"============== Hit-based ==============="<<endl;
	}
	
	// Swim reference trajectory
	fit_status = kFitNotDone; // initialize to a safe default
	rt->Swim(input_params.position(), input_params.momentum(), input_params.charge());

	// Iterate over fitter until it converges or we reach the 
	// maximum number of iterations
	double last_chisq = 1.0E6;
	int Niterations;
	for(Niterations=0; Niterations<MAX_FIT_ITERATIONS; Niterations++){
	
		hitsInfo hinfo;	
		GetWiresShiftsErrs(fit_type, rt, hinfo);
	
		switch(fit_status = LeastSquaresB(hinfo, rt)){
			case kFitSuccess:
				if(DEBUG_LEVEL>2)_DBG_<<"Fit succeeded ----- "<<endl;
				break;
			case kFitNoImprovement:
				if(DEBUG_LEVEL>2)_DBG_<<"Unable to improve on input parameters (chisq="<<chisq<<")"<<endl;
				break;
			case kFitFailed:
				if(DEBUG_LEVEL>2)_DBG_<<"Fit failed on iteration "<<Niterations<<endl;
				if(Niterations>4){
					if(DEBUG_LEVEL>2)_DBG_<<"Number of iterations >4. Trying to keep fit from last iteration... "<<endl;
					break;
				}
				return fit_status;
				break;
			case kFitNotDone: // avoid compiler warnings
				break;
		}

		// Fill debug histos
		if(DEBUG_HISTS){
			chisq_vs_pass->Fill(Niterations+1, chisq);
			dchisq_vs_pass->Fill(Niterations+1, last_chisq-chisq);			
		}

		// If the chisq is too large, then consider it a hopeless cause
		if(chisq>1.0E4){
			if(DEBUG_LEVEL>3)_DBG_<<"Fit chisq too large on iteration "<<Niterations<<endl;
			return fit_status=kFitFailed;
		}
		
		// Check if we converged
		if(fabs(last_chisq-chisq) < MAX_CHISQ_DIFF)break;
		last_chisq = chisq;
	}

	if(DEBUG_LEVEL>1)_DBG_<<" Niterations="<<Niterations<<"  chisq="<<chisq<<" p="<<rt->swim_steps[0].mom.Mag()<<endl;
	
	// At this point we must decide whether the fit succeeded or not.
	// We'll consider the fit a success if any of the following is true:
	//
	// 1. We got through at least one iteration in the above loop
	// 2. The chi-sq is less than CHISQ_GOOD_LIMIT
	// 3. MAX_FIT_ITERATIONS is zero (for debugging)
	bool fit_succeeded = false;
	if(Niterations>0)fit_succeeded = true;
	if(chisq<=CHISQ_GOOD_LIMIT)fit_succeeded = true;
	if(MAX_FIT_ITERATIONS==0){fit_succeeded = true; fit_status=kFitSuccess;}
	if(!fit_succeeded)return fit_status=kFitFailed;
	
	if(DEBUG_LEVEL>10){
		_DBG_<<"-------- Final Chisq for track = "<<chisq<<endl;
		double chisq = ChiSq(fit_type, rt);
		_DBG_<<"-------- Check chisq = "<<chisq<<" Ndof="<<Ndof<<endl;
	}

	// Calculate final chi-squared for this track
	ChiSq(fit_type, rt, &this->chisq, &this->Ndof);

	// Find point of closest approach to target and use parameters
	// there for vertex position and momentum
	//DVector3 vertex_mom, vertex_pos;
	//double s;
	//rt->DistToRT(target, &s);
	//rt->GetLastDOCAPoint(vertex_pos, vertex_mom);
	DVector3 &vertex_mom = rt->swim_steps[0].mom;
	DVector3 &vertex_pos = rt->swim_steps[0].origin;

	// Copy final fit parameters into TrackFitter classes data members. Note that the chisq and Ndof
	// members are copied in during the ChiSq() method call in LeastSquaresB().
	fit_params.setPosition(vertex_pos);
	fit_params.setMomentum(vertex_mom);
	fit_params.setCharge(rt->q);
	fit_params.setMass(input_params.mass());
	cdchits_used_in_fit = cdchits;
	fdchits_used_in_fit = fdchits;

	// Fill debugging histos if requested
	if(DEBUG_HISTS){
		Npasses->Fill(Niterations);
		initial_chisq_vs_Npasses->Fill(Niterations, chisq);
		FillDebugHists(rt, vertex_pos, vertex_mom);
	}
	
	// Debugging messages
	if(DEBUG_LEVEL>2)_DBG_<<" -- Fit succeeded: q="<<rt->q<<" p="<<vertex_mom.Mag()<<" theta="<<vertex_mom.Theta()*57.3<<" phi="<<vertex_mom.Phi()*57.3<<endl;

	return fit_status;
}

//------------------
// ChiSq
//------------------
double DTrackFitterALT1::ChiSq(fit_type_t fit_type, DReferenceTrajectory *rt, double *chisq_ptr, int *dof_ptr)
{
	hitsInfo hinfo;
	
	GetWiresShiftsErrs(fit_type, rt, hinfo);
	
	return ChiSq(rt, hinfo, chisqv, chisq_ptr, dof_ptr);
}

//------------------
// ChiSq
//------------------
double DTrackFitterALT1::ChiSq(DMatrix &state, const swim_step_t *start_step, DReferenceTrajectory *rt, hitsInfo &hinfo, vector<double> &chisqv, double *chisq_ptr, int *dof_ptr)
{
	/// Calculate the chi-squared for a track specified by state relative to the
	/// given reference trajectory. This is just a wrapper for 
	/// ChiSq(DReferenceTrajectory *rt, vector<const DCoordinateSystem*> &wires, vector<DVector3> &shifts, vector<double> &errs, vector<double> &chisqv)
	/// that accepts the state vector and re-swims the trajectory.
	
	// "v" direction is perpendicular to both the rt direction and the
	// x-direction. See LeastSquares() for more.
	DVector3 vdir = start_step->sdir.Cross(start_step->mom);
	vdir.SetMag(1.0);

	DVector3 pos =   start_step->origin
						+ state[state_x ][0]*start_step->sdir
						+ state[state_v ][0]*vdir;
	DVector3 mom =   state[state_px][0]*start_step->sdir
						+ state[state_py][0]*start_step->tdir
						+ state[state_pz][0]*start_step->udir;

	if(!rt){
		_DBG_<<"NULL pointer passed for DReferenceTrajectory object!"<<endl;
		chisqv.clear();
		for(unsigned int i=0; i<hinfo.size(); i++){
			chisqv.push_back(1.0E6); // CDC and FDC wires
			if(hinfo[i].u_err!=0.0)chisqv.push_back(1.0E6); // FDC cathodes
		}
		return 1.0E6;
	}
	
	if(pos.Mag()>200.0 || fabs(state[state_x ][0])>100.0 || fabs(state[state_v ][0])>100.0){
		if(DEBUG_LEVEL>3)_DBG_<<"state values out of range"<<endl;
		if(DEBUG_LEVEL>6){
			pos.Print();
			mom.Print();
			_DBG_<<"state[state_x ][0]="<<state[state_x ][0]<<"  state[state_v ][0]="<<state[state_x ][0]<<endl;
		}
		chisqv.clear();
		for(unsigned int i=0; i<hinfo.size(); i++){
			chisqv.push_back(1.0E6); // CDC and FDC wires
			if(hinfo[i].u_err!=0.0)chisqv.push_back(1.0E6); // FDC cathodes
		}

		return 1.0E6;
	}
	
	rt->Swim(pos,mom);

	return ChiSq(rt, hinfo, chisqv, chisq_ptr, dof_ptr);
}

//------------------
// ChiSq
//------------------
double DTrackFitterALT1::ChiSq(DReferenceTrajectory *rt, hitsInfo &hinfo, vector<double> &chisqv, double *chisq_ptr, int *dof_ptr)
{
	/// Calculate the chisq for a track represented by the given reference
	/// trajectory with the given list of wires. The values in the "shifts"
	/// vector are used to shift the wire positions and can therefore be used
	/// to implement the drift time information. If no shifts are required, then
	/// the shifts vector should be empty. The values in the errs vector
	/// should be the errs for each of the wire measurements.
	///
	/// The return value is the chi-squared per dof. Upon return, the chisqv
	/// vector will be filled with the individual chisq contributions from
	/// each wire hit.
	///
	/// Upon entry, the lengths of the vectors "wires", "errs", and "shifts"
	/// (if non-zero) are checked to make sure they all match. If not, an error
	/// message is printed and a value of 1.0E6 is returned and chisqv
	/// will be empty

	// Make sure input vectors match
	chisqv.clear();
	
	// Sometimes, we end up with a zero step rt due to bad parameters.
	// Avoid all the warning messages by detecting this now and bailing early.
	if(rt->Nswim_steps<1){
		for(unsigned int i=0; i<hinfo.size(); i++){
			chisqv.push_back(1.0E6); // CDC and FDC wires
			if(hinfo[i].u_err!=0.0)chisqv.push_back(1.0E6); // FDC cathodes
		}

		return 1.0E6;
	}
	
	// These are for debugging
	const DFDCWire *fdcwire=NULL;
	const DCDCWire *cdcwire=NULL;
	
	// Loop over wires
	double chisq = 0.0;
	int dof=0;
	for(unsigned int i=0; i<hinfo.size(); i++){
		hitInfo &hi = hinfo[i];
		const DCoordinateSystem *wire = hi.wire;
		
		if(DEBUG_LEVEL>10){
			fdcwire = dynamic_cast<const DFDCWire*>(wire);
			cdcwire = dynamic_cast<const DCDCWire*>(wire);
		}
	
		// Get distance of the (shifted) wire from the reference trajectory
		double s;
		double d = rt->DistToRT(wire, &s);
		
		// It can happen that a track looks enough like a helix that
		// a momentum pointing in the opposite direction of the real track
		// can have a small value of d, even when using a swim step close
		// to the target. In these cases, s will be negative and large.
		// When this happens, set d to R to force a bad chisq.
		if(s<-5.0)d = wire->origin.Perp();
		
		// Residual. If we're on the correct side of the wire, then this is
		// dist-doca. If we're on the incorrect side of the wire, then this
		// is -dist-doca. Prior to calling us, the value of hi.dist will have
		// a sign that already assigned to indicate the side of the wire
		// the track is believed to be on.
		double resi = hi.dist - d;

		// Add this to the chisq. It may turn out that d is NAN in which case we
		// don't want to include it in the total tally. We still must add it
		// however to the chisqv since that must have an entry for every entry
		// in the wires vector.
		//
		// The value going into chisqv needs to be a signed quantity with positive
		// values meaning the doca is larger than the dist and negative values
		// meaning the doca is smaller than the dist (doca from track, dist
		// from drift time). 
		double c = resi/hi.err;
		chisqv.push_back(c);

		if(finite(c)){
			chisq += c*c;
			dof++;
			if(DEBUG_LEVEL>10){
				_DBG_<<"  chisqv["<<dof<<"] = "<<c<<"  d="<<d<<"  Rwire="<<wire->origin.Perp()<<"  Rshifted="<<wire->origin.Perp()<<" s="<<s;
				if(fdcwire)cerr<<" FDC: layer="<<fdcwire->layer<<" wire="<<fdcwire->wire<<" angle="<<fdcwire->angle;
				if(cdcwire)cerr<<" CDC: ring="<<cdcwire->ring<<" straw="<<cdcwire->straw<<" stereo="<<cdcwire->stereo;
				cerr<<endl;
			}
		}else{
			if(DEBUG_LEVEL>10)_DBG_<<"  chisqv[unused] = "<<c<<"  d="<<d<<"  Rwire="<<wire->origin.Perp()<<"  Rshifted="<<wire->origin.Perp()<<" s="<<s<<endl;
		}
		
		// Add FDC cathode (if appropriate)
		if(hi.u_err!=0.0){
			double u = rt->GetLastDistAlongWire();
			c = (u - hi.u_dist)/hi.u_err;
			chisqv.push_back(c);
			if(finite(c)){
				chisq += c*c;
				dof++;
				if(DEBUG_LEVEL>10){
					_DBG_<<"  chisqv["<<dof<<"] = "<<c<<"  d="<<d<<"  Rwire="<<wire->origin.Perp()<<"  Rshifted="<<wire->origin.Perp()<<" s="<<s<<" FDC: layer="<<fdcwire->layer<<" wire="<<fdcwire->wire<<" angle="<<fdcwire->angle;
					cerr<<endl;
				}
			}else{
				if(DEBUG_LEVEL>10)_DBG_<<"  chisqv[unused] = "<<c<<"  d="<<d<<"  Rwire="<<wire->origin.Perp()<<"  Rshifted="<<wire->origin.Perp()<<" s="<<s<<endl;
			}
		}
	}

	// If the caller supplied pointers to chisq and dof variables, copy the values into them
	if(chisq_ptr)*chisq_ptr = chisq;
	if(dof_ptr)*dof_ptr = dof - 5; // assume 5 fit parameters

	// If it turns out the dof is zero, return 1.0E6. Otherwise, return
	// the chisq/dof
	return dof<2 ? 1.0E6:chisq/(double)dof;
}

//------------------
// GetWiresShiftsErrs
//------------------
void DTrackFitterALT1::GetWiresShiftsErrs(fit_type_t fit_type, DReferenceTrajectory *rt, hitsInfo &hinfo)
{

	// -- Target --
	if(TARGET_CONSTRAINT){
		hitInfo hi;
		hi.wire = target;
		hi.dist = 0.0;
		hi.err = 0.1; // 1mm beam width
		hi.u_dist = 0.0;
		hi.u_err = 0.0;
		hinfo.push_back(hi);
	}

	// --- CDC ---
	for(unsigned int i=0; i<cdchits.size(); i++){
		const DCDCTrackHit *hit = cdchits[i];
		const DCoordinateSystem *wire = hit->wire;
		hitInfo hi;

		hi.wire = wire;
		hi.u_dist = 0.0;
		hi.u_err = 0.0;

		// Fill in shifts and errs vectors based on whether we're doing
		// hit-based or time-based tracking
		if(fit_type==kWireBased){
			// If we're doing hit-based tracking then only the wire positions
			// are used and the drift time info is ignored. 
			hi.dist = 0.0;
			hi.err = 0.8/sqrt(12.0); // variance for evenly illuminated straw
		}else{
			// Find the DOCA point for the RT and use the momentum direction
			// there and the wire direction to determine the "shift".
			// Note that whether the shift is to the left or to the right
			// is not decided here. That ambiguity is left to be resolved later
			// by applying a minus sign (or not) to the shift.
			DVector3 pos_doca, mom_doca;
			double s;
			rt->DistToRT(wire, &s);
			rt->GetLastDOCAPoint(pos_doca, mom_doca);
			
			// The magnitude of the shift is based on the drift time. The
			// value of the dist member of the DCDCTrackHit object does not
			// subtract out the TOF. This can add 50-100 microns to the
			// resolution in the CDC. Here, we actually can calculate the TOF
			// (for a given mass hypothesis).
			double mass = 0.13957;
			double beta = 1.0/sqrt(1.0 + pow(mass/mom_doca.Mag(), 2.0))*2.998E10;
			double tof = s/beta/1.0E-9; // in ns
			hi.dist = hit->dist*((hit->tdrift-tof)/hit->tdrift);
			hi.err = SIGMA_CDC;
		}
		hinfo.push_back(hi);
	}

	// --- FDC ---
	for(unsigned int i=0; i<fdchits.size(); i++){
		const DFDCPseudo *hit = fdchits[i];
		const DCoordinateSystem *wire = hit->wire;
		hitInfo hi;

		hi.wire = wire;

		// Fill in shifts and errs vectors based on whether we're doing
		// hit-based or time-based tracking
		if(fit_type==kWireBased){
			// If we're doing hit-based tracking then only the wire positions
			// are used and the drift time info is ignored. Thus, we don't
			// have to calculate the shift vectors
			//errs.push_back(0.261694); // empirically from single pi+ events
			hi.err = 0.5/sqrt(12.0); // variance for evenly illuminated cell - anode
			hi.u_err = 0.0; // variance for evenly illuminated cell - cathode
			hi.u_dist = 0.0;
		}else{
			// Find the DOCA point for the RT and use the momentum direction
			// there and the wire direction to determine the "shift".
			// Note that whether the shift is to the left or to the right
			// is not decided here. That ambiguity is left to be resolved later
			// by applying a minus sign (or not) to the shift.
			DVector3 pos_doca, mom_doca;
			double s;
			rt->DistToRT(wire, &s);
			rt->GetLastDOCAPoint(pos_doca, mom_doca);
			
			// The magnitude of the shift is based on the drift time. The
			// value of the dist member of the DCDCTrackHit object does not
			// subtract out the TOF. This can add 50-100 microns to the
			// resolution in the CDC. Here, we actually can calculate the TOF
			// (for a given mass hypothesis).
			double mass = 0.13957;
			double beta = 1.0/sqrt(1.0 + pow(mass/mom_doca.Mag(), 2.0))*2.998E10;
			double tof = s/beta/1.0E-9; // in ns
			hi.dist = hit->dist*((hit->time-tof)/hit->time);
			hi.err = SIGMA_FDC_ANODE;
			
			// Find whether the track is on the "left" or "right" of the wire
			DVector3 shift = wire->udir.Cross(mom_doca);
			shift.SetMag(1.0);
			double u = rt->GetLastDistAlongWire();
			DVector3 pos_wire = wire->origin + u*wire->udir;
			double LRsign = shift.Dot(pos_doca-pos_wire)<0.0 ? +1.0:-1.0;

			// Lorentz corrected poisition along the wire is contained in x,y
			// values, BUT, it is based on a left-right decision of the track
			// segment. This may or may not be the same as the global track. 
			// As such, we have to determine the correction for our track.
			//DVector3 wpos(hit->x, hit->y, wire->origin.Z());
			//DVector3 wdiff = wpos - wire->origin;
			//double u_corr = wire->udir.Dot(wdiff);
			double alpha = mom_doca.Angle(DVector3(0,0,1));
			double u_lorentz = LRsign*lorentz_def->GetLorentzCorrection(pos_doca.X(), pos_doca.Y(), pos_doca.Z(), alpha, hi.dist);
//_DBG_<<"u_corr-hit->s="<<u_corr-hit->s<<"  u_lorentz="<<u_lorentz<<" \tratio="<<u_lorentz/(u_corr-hit->s)<<endl;
			double u_corr = hit->s + u_lorentz;
			hi.u_dist = u_corr;
			hi.u_err = SIGMA_FDC_CATHODE;
		}
		hinfo.push_back(hi);
	}
	
}

//------------------
// LeastSquaresB
//------------------
DTrackFitterALT1::fit_status_t DTrackFitterALT1::LeastSquaresB(hitsInfo &hinfo, DReferenceTrajectory *rt)
{
	/// Fit the track with starting parameters given in the first step
	/// of the reference trajectory rt. On return, the reference
	/// trajectory rt will represent the final fit parameters and
	/// chisq will contain the chisq/dof of the track.
	///
	/// This determines the best fit of the track using the least squares method
	/// described by R. Mankel Rep. Prog. Phys. 67 (2004) 553-622 pg 565.
	/// Since it uses a linear approximation for the chisq dependance on
	/// the fit parameters, several calls may be required for convergence.

	// Initialize the chisq and Ndof data members in case we need to bail early
	this->chisq = 1.0E6;
	this->Ndof = 0;

	// Make sure RT is not empty
	if(rt->Nswim_steps<1)return kFitFailed;

	// For fitting, we want to define a coordinate system very similar to the
	// Reference Trajectory coordinate system. The difference is that we want
	// the position to be defined in a plane perpendicular to the momentum.
	// The RT x-direction is in this plane, but the momentum itself lies
	// somewhere in the Y/Z plane so that neither Y nor Z makes a good choice
	// for the second postion dimension. We will call the second coordinate in 
	// the perpendicular plane "v" which is the coordinate along the axis that
	// is perpendicular to both the x-direction and the momentum direction.
	swim_step_t &start_step = rt->swim_steps[0];
	DVector3 pos = start_step.origin;
	DVector3 mom = start_step.mom;

	// Copy the errors in errs and u_errs into all_errs. This is needed so we have an easy-to-access
	// list of the errors corresponding to each element in the chisqv vector.
	vector<double> all_errs;
	for(unsigned int i=0; i<hinfo.size(); i++){
		all_errs.push_back(hinfo[i].err);
		if(hinfo[i].u_err!=0.0)all_errs.push_back(hinfo[i].u_err);
	}

	// Get the chi-squared vector for the initial reference trajectory
	vector<double> chisqv_initial;
	double initial_chisq;
	int initial_Ndof;
	double initial_chisq_per_dof = ChiSq(rt, hinfo, chisqv_initial, &initial_chisq, &initial_Ndof);

	// Because we have a non-linear system, we must take the derivatives
	// numerically.
	//
	// Note that in the calculations of the deltas below,
	// the change in state should be set first and the value
	// of deltas[...] calculated from that. See Numerical
	// Recipes in C 2nd ed. section 5.7 ppg. 186-189.
	const int Nparameters = 5;
	double deltas[Nparameters];
	DMatrix state(5,1);
	switch(Nparameters){
		case 5: state[state_v	][0] = 0.0;
		case 4: state[state_x	][0] = 0.0;
		case 3: state[state_pz	][0] = mom.Dot(start_step.udir);
		case 2: state[state_py	][0] = mom.Dot(start_step.tdir);
		case 1: state[state_px	][0] = mom.Dot(start_step.sdir);
	}
	
	// For the swimming below, we use a second reference trajectory so as
	// to preserve the original. Set the charge here. The reset of the
	// parameters (starting position and momentum) will be set using
	// values from the state vector.
	tmprt->q = rt->q;
	
	// dpx : tweak by +/- 0.01
	DMatrix state_dpx = state;
	state_dpx[state_px][0] += LEAST_SQUARES_DP;
	deltas[state_px] = state_dpx[state_px][0] - state[state_px][0];
	vector<double> &chisqv_dpx_lo = chisqv_initial;
	vector<double> chisqv_dpx_hi;
	double chisq_dpx = ChiSq(state_dpx, &start_step, tmprt, hinfo, chisqv_dpx_hi);

	// dpy : tweak by +/- 0.01
	DMatrix state_dpy = state;
	state_dpy[state_py][0] += LEAST_SQUARES_DP;
	deltas[state_py] = state_dpy[state_py][0] - state[state_py][0];
	vector<double> &chisqv_dpy_lo = chisqv_initial;
	vector<double> chisqv_dpy_hi;
	double chisq_dpy = ChiSq(state_dpy, &start_step, tmprt, hinfo, chisqv_dpy_hi);

	// dpz : tweak by +/- 0.01
	DMatrix state_dpz = state;
	state_dpz[state_pz][0] += LEAST_SQUARES_DP;
	deltas[state_pz] = state_dpz[state_pz][0] - state[state_pz][0];
	vector<double> &chisqv_dpz_lo = chisqv_initial;
	vector<double> chisqv_dpz_hi;
	double chisq_dpz = ChiSq(state_dpz, &start_step, tmprt, hinfo, chisqv_dpz_hi);

	// dv : tweak by +/- 0.01
	DMatrix state_dv = state;
	state_dv[state_v][0] += LEAST_SQUARES_DX;
	deltas[state_v] = state_dv[state_v][0] - state[state_v][0];
	vector<double> &chisqv_dv_lo = chisqv_initial;
	vector<double> chisqv_dv_hi;
	double chisq_dv = ChiSq(state_dv, &start_step, tmprt, hinfo, chisqv_dv_hi);

	// dx : tweak by +/- 0.01
	DMatrix state_dx = state;
	state_dx[state_x][0] += LEAST_SQUARES_DX;
	deltas[state_x] = state_dx[state_x][0] - state[state_x][0];
	vector<double> &chisqv_dx_lo = chisqv_initial;
	vector<double> chisqv_dx_hi;
	double chisq_dx = ChiSq(state_dx, &start_step, tmprt, hinfo, chisqv_dx_hi);
	
	// This line just avoids a compiler warning
	if(DEBUG_LEVEL>4){
		_DBG_<<"initial_chisq_per_dof="<<initial_chisq_per_dof<<endl;
		_DBG_<<"chisq_dpx="<<chisq_dpx<<endl;
		_DBG_<<"chisq_dpy="<<chisq_dpy<<endl;
		_DBG_<<"chisq_dpz="<<chisq_dpz<<endl;
		_DBG_<<"chisq_dv="<<chisq_dv<<endl;
		_DBG_<<"chisq_dx="<<chisq_dx<<endl;
	}
	if(DEBUG_LEVEL>10){
		_DBG_<<"hit\tinitial\tpx   \tpy   \tpz   \tx   \tv"<<endl;
		for(unsigned int j=0; j<chisqv_initial.size(); j++){
			cout<<j<<"\t";
			cout<<chisqv_initial[j]<<"\t";
			cout<<chisqv_dpx_hi[j]<<"\t";
			cout<<chisqv_dpy_hi[j]<<"\t";
			cout<<chisqv_dpz_hi[j]<<"\t";
			cout<<chisqv_dv_hi[j]<<"\t";
			cout<<chisqv_dx_hi[j]<<endl;
		}
	}

	// Make a list of "clean" hits. Ones that are less than 4 sigma and
	// are not "nan" for the initial as well as the tweaked  cases.
	vector<bool> good;
	unsigned int Ngood=0;
	unsigned int Nhits = chisqv_initial.size();
	for(unsigned int i=0; i<Nhits; i++){
		// Here, we cut hits based on the difference between the track and the 
		// drift time in units of sigmas. However, on the first pass, the fit won't
		// be good and the sigmas which represent the best-fit track to drift
		// time conditions underestimate the quantity. Subsequent passes should
		// have better and better parameters and so the sigmas should be more
		// and more appropriate.
		//
		// To accomodate this, we scale the CHISQ_MAX_RESI_SIGMAS value based
		// on the initial_chisq_per_dof value since that gives us a handle on how
		// go these hits match to the track overall.
		double max=CHISQ_MAX_RESI_SIGMAS*(initial_chisq_per_dof>1.0 ? initial_chisq_per_dof:1.0);
		double sigma;
		sigma = chisqv_initial[i]; if(!finite(sigma) || fabs(sigma)>max){good.push_back(false); continue;}
		sigma = chisqv_dpx_hi[i];  if(!finite(sigma) || fabs(sigma)>max){good.push_back(false); continue;}
		sigma = chisqv_dpy_hi[i];  if(!finite(sigma) || fabs(sigma)>max){good.push_back(false); continue;}
		sigma = chisqv_dpz_hi[i];  if(!finite(sigma) || fabs(sigma)>max){good.push_back(false); continue;}
		sigma = chisqv_dx_hi[i];   if(!finite(sigma) || fabs(sigma)>max){good.push_back(false); continue;}
		sigma = chisqv_dv_hi[i];   if(!finite(sigma) || fabs(sigma)>max){good.push_back(false); continue;}

		good.push_back(true);
		Ngood++;
	}
	if(Ngood<LEAST_SQUARES_MIN_HITS){
		if(DEBUG_LEVEL>2)_DBG_<<" Bad number of good distance calculations!(Ngood="<<Ngood<<" LEAST_SQUARES_MIN_HITS="<<LEAST_SQUARES_MIN_HITS<<")"<<endl;
		return kFitFailed;
	}
	
	// Verify that the sizes of the all_errs and chisqv vectors match
	if(Nhits != all_errs.size()){
		_DBG_<<"Nhits != all_errs.size() !!! This indicates a bug in the program!"<<endl;
		return kFitFailed;
	}

	// Build "F" matrix of derivatives
	DMatrix F(Ngood,Nparameters);
	for(unsigned int i=0,j=0; j<Nhits; j++){
		if(!good[j])continue; // skip bad hits
		switch(Nparameters){
			// Note: This is a funny way to use a switch!
			case 5: F[i][state_v ] = pow(all_errs[j],1.0)*(chisqv_dv_hi[j]-chisqv_dv_lo[j])/deltas[state_v];
			case 4: F[i][state_x ] = pow(all_errs[j],1.0)*(chisqv_dx_hi[j]-chisqv_dx_lo[j])/deltas[state_x];
			case 3: F[i][state_pz] = pow(all_errs[j],1.0)*(chisqv_dpz_hi[j]-chisqv_dpz_lo[j])/deltas[state_pz];
			case 2: F[i][state_py] = pow(all_errs[j],1.0)*(chisqv_dpy_hi[j]-chisqv_dpy_lo[j])/deltas[state_py];
			case 1: F[i][state_px] = pow(all_errs[j],1.0)*(chisqv_dpx_hi[j]-chisqv_dpx_lo[j])/deltas[state_px];
		}
		i++;
	}
	
	// Sometimes, "F" has lots of values like 1.44E+09 indicating a problem (I think comming
	// from some nan values floating around.) Anyway, in these cases, the E2Norm value is
	// quite large (>1E+18) which we  can use to punt now. In reality, we do this to avoid
	// ROOT error messages about a matrix being singular when Ft*Vinv*F is inverted below.
	if(F.E2Norm()>1.0E18){
		if(DEBUG_LEVEL>1){
			_DBG_<<" -- F matrix E2Norm out of range(E2Norm="<<F.E2Norm()<<" max="<<1.0E18<<")"<<endl;
		}
		return kFitFailed;
	} 
	
	DMatrix Ft(DMatrix::kTransposed, F);

	// V is a diagonal matrix of the measurement errors. In
	// principle, we could fold in the multiple scattering
	// here, but for now, we don't. This is filled in the
	// loop below.
	DMatrix V(Ngood,Ngood);
	V.Zero();
	
	// Measurement vector. This contains the residuals.
	DMatrix m(Ngood,1);
	for(unsigned int i=0,j=0; j<Nhits; j++){
		if(!good[j])continue; // skip bad hits
		// The "measurement" here is the distance of the track from the 
		// shifted wire position where the shift is based on the drift time
		// and a choice of the left-right ambiguity. These are in the 
		// chisqv_XXX vectors in units of sigmas and and are all
		// positive definite. We multiply back in the errors to convert
		// the units back to cm since the errors are handled in the 
		// V matrix. I *think* since we're using a diagonal error matrix
		// we could actually just leave the measurments in units of
		// sigmas and make the V matrix the identity matrix, but we leave
		// it this way for now to make it easier to include covariance
		// later.
		m[i][0] = -chisqv_initial[j]*all_errs[j];
		V[i][i] = pow(all_errs[j], 2.0);
		i++;
	}
	DMatrix Vinv(DMatrix::kInverted, V);
	DMatrix B(DMatrix::kInverted, Ft*Vinv*F);
	
	// If the inversion failed altogether then the invalid flag
	// will be set on the matrix. In these cases, we're dead.
	if(!B.IsValid()){
		if(DEBUG_LEVEL>1)_DBG_<<" -- B matrix invalid"<<endl;
		return kFitFailed;
	}

	// The "B" matrix happens to be the covariance matrix of the
	// state parameters. A problem sometimes occurs where one or
	// more elements of B are very large. This tends to happen
	// when a column of F is essentially all zeros making the
	// matrix un-invertable. What we should really do in these
	// cases is check beforehand and drop the bad column(s)
	// before trying to invert. That will add complication that
	// I don't want to introduce quite yet. What we do now
	// is check for it and punt rather than return a nonsensical
	// value.
	if(B.E2Norm() > LEAST_SQUARES_MAX_E2NORM){
		if(DEBUG_LEVEL>1){
			_DBG_<<" -- B matrix E2Norm out of range(E2Norm="<<B.E2Norm()<<" max="<<LEAST_SQUARES_MAX_E2NORM<<")"<<endl;
			F.Print();
			B.Print();
		}
		return kFitFailed;
	}
	
	// Copy the B matrix into last_covariance to later copy into DTrack
	last_covariance.ResizeTo(B);
	last_covariance = B;

	// Calculate step direction and magnitude	
	DMatrix delta_state = B*Ft*Vinv*m;

	// The following is based on Numerical Recipes in C 2nd Ed.
	// ppg. 384-385.
	//
	// We now have the "direction" by which to step in the-d
	// parameter space as well as an amplitude by which to
	// step in "delta_state". A potential problem is that
	// we can over-step such that we end up at a worse
	// chi-squared value. To address this, we try taking 
	// the full step, but if we end up at a worse chi-sq
	// then we backtrack and take a smaller step until
	// we see the chi-sq improve.
	double min_chisq_per_dof = initial_chisq_per_dof;
	double min_lambda = 0.0;
	double lambda = 1.0;
	int Ntrys = 0;
	int max_trys = 8;
	DMatrix new_state(5,1);
	for(; Ntrys<max_trys; Ntrys++){

		for(int i=0; i<Nparameters; i++)new_state[i][0] = state[i][0] + delta_state[i][0]*lambda;

		vector<double> new_chisqv;
		double chisq_per_dof = ChiSq(new_state, &start_step, tmprt, hinfo, new_chisqv);
		
		if(chisq_per_dof<min_chisq_per_dof){
			min_chisq_per_dof = chisq_per_dof;
			min_lambda = lambda;
		}
		
		// If we're at a lower chi-sq then we're done
		if(DEBUG_LEVEL>4)_DBG_<<" -- initial_chisq_per_dof="<<initial_chisq_per_dof<<"  new chisq_per_dof="<<chisq_per_dof<<" nhits="<<new_chisqv.size()<<" p="<<tmprt->swim_steps[0].mom.Mag()<<"  lambda="<<lambda<<endl;
		if(chisq_per_dof-initial_chisq_per_dof < 0.1 && chisq_per_dof<2.0)break;
		
		// Chi-sq was increased, try a smaller step on the next iteration
		lambda/=2.0;
	}
	
	// If we failed to find a better Chi-Sq above, maybe we were looking 
	// in the wrong direction(??) Try looking in the opposite direction.
	if(Ntrys>=max_trys){
		lambda = -1.0;
		for(; Ntrys<2*max_trys; Ntrys++){

			for(int i=0; i<Nparameters; i++)new_state[i][0] = state[i][0] + delta_state[i][0]*lambda;

			vector<double> new_chisqv;
			double chisq_per_dof = ChiSq(new_state, &start_step, tmprt, hinfo, new_chisqv);
		
			if(chisq_per_dof<min_chisq_per_dof){
				min_chisq_per_dof = chisq_per_dof;
				min_lambda = lambda;
			}
			
			// If we're at a lower chi-sq then we're done
			if(DEBUG_LEVEL>4)_DBG_<<" -- initial_chisq_per_dof="<<initial_chisq_per_dof<<"  new chisq="<<chisq<<" nhits="<<new_chisqv.size()<<"  lambda="<<lambda<<endl;
			if(chisq_per_dof-initial_chisq_per_dof < 0.1 && chisq_per_dof<2.0)break;
			
			// Chi-sq was increased, try a smaller step on the next iteration
			lambda/=2.0;
		}
	}

	// If we failed to make a step to a smaller chi-sq then signal
	// that we were unable to make any improvement.
	if(min_lambda==0.0){
		if(DEBUG_LEVEL>1)_DBG_<<"Chisq only increased (both directions searched!)"<<endl;
		this->chisq = initial_chisq;
		this->Ndof = initial_Ndof;
		return kFitNoImprovement;
	}
	
	// Re-create new_state using min_lambda
	for(int i=0; i<Nparameters; i++)new_state[i][0] = state[i][0] + delta_state[i][0]*min_lambda;

	// Re-swim reference trajectory using these parameters and re-calc chisq.
	// Note that here we have the chisq and Ndof members set.
	ChiSq(new_state, &start_step, rt, hinfo, chisqv, &this->chisq, &this->Ndof);
	
	if(DEBUG_LEVEL>3){
		DVector3 pos = start_step.origin;
		DVector3 mom = start_step.mom;
		double phi = mom.Phi();
		if(phi<0.0)phi+=2.0*M_PI;
		_DBG_<<"LeastSquaresB succeeded: p="<<mom.Mag()<<" theta="<<mom.Theta()<<" phi="<<phi<<" z="<<pos.Z()<<endl;
	}

	return kFitSuccess;
}

//------------------
// FillDebugHists
//------------------
void DTrackFitterALT1::FillDebugHists(DReferenceTrajectory *rt, DVector3 &vertex_pos, DVector3 &vertex_mom)
{
	//vertex_mom.SetMagThetaPhi(6.0, 17.2*M_PI/180.0, 90.0*M_PI/180.0);
	//vertex_pos.SetXYZ(0.0,0.0,65.0);
	//rt->Swim(vertex_pos, vertex_mom);
	ptotal->Fill(vertex_mom.Mag());

	// Calculate particle beta
	double beta = 1.0/sqrt(1.0+TOF_MASS*TOF_MASS/vertex_mom.Mag2()); // assume this is a pion for now. This should eventually come from outer detectors

	for(unsigned int j=0; j<cdchits.size(); j++){
		const DCDCTrackHit *hit = cdchits[j];
		const DCDCWire *wire = hit->wire;
		
		// Distance of closest approach for track to wire
		double s;
		double doca = rt->DistToRT(wire, &s);
			
		// Distance from drift time. Hardwired for simulation for now
		double tof = s/(beta*3E10*1E-9);
		double dist = (hit->tdrift-tof)*55E-4;

		// NOTE: Sometimes this could be nan
		double resi = dist - doca;
		if(!finite(resi))continue;
		
		// Fill histos
		residuals_cdc->Fill(resi, wire->ring);
		residuals_cdc_vs_s->Fill(resi, wire->ring, s);

		cdcdoca_vs_dist->Fill(dist, doca);
		cdcdoca_vs_dist_vs_ring->Fill(dist, doca, wire->ring);
		if(wire->stereo==0.0){
			dist_axial->Fill(dist);
			doca_axial->Fill(doca);
		}else{
			dist_stereo->Fill(dist);
			doca_stereo->Fill(doca);
		}

	}
	
	for(unsigned int j=0; j<fdchits.size(); j++){
		const DFDCPseudo *hit = fdchits[j];
		const DFDCWire *wire = hit->wire;

		// Distance of closest approach for track to wire
		double s;
		double doca = rt->DistToRT(wire,&s);

		double tof = s/(beta*3E10*1E-9);
		double dist = (hit->time-tof)*55E-4;

		// NOTE: Sometimes this is nan
		double resi = dist - doca;
		if(finite(resi)){
			fdcdoca_vs_dist->Fill(dist, doca);
			residuals_fdc_anode->Fill(resi, wire->layer);
			residuals_fdc_anode_vs_s->Fill(resi, wire->layer,s);
		}
		
		double u = rt->GetLastDistAlongWire();
		resi = u - hit->s;
		if(finite(resi)){
			fdcu_vs_s->Fill(u, hit->s);
			residuals_fdc_cathode->Fill(resi, wire->layer);
			residuals_fdc_cathode_vs_s->Fill(resi, wire->layer,s);
		}
	}
}


