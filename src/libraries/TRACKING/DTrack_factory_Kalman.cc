// $Id$
//
//    File: DTrack_factory_Kalman.cc
// Created: Wed Sep  3 09:33:40 EDT 2008
// Creator: davidl (on Darwin harriet.jlab.org 8.11.1 i386)
//

// This is an exact copy of the DTrack_factory.cc file except
// it is hardwired to use the "Kalman" tagged track fitting
// algorithm. This is so one can get tracks fit by the Kalman
// and ALT1 methods simultaneously in the same program for the
// same event.


#include <iostream>
#include <iomanip>
using namespace std;

#include "DTrack_factory_Kalman.h"
#include <TRACKING/DTrackCandidate.h>
#include <TRACKING/DReferenceTrajectory.h>
#include <CDC/DCDCTrackHit.h>
#include <FDC/DFDCPseudo.h>

using namespace jana;

//------------------
// CDCSortByRincreasing
//------------------
bool static CDCSortByRincreasing(const DCDCTrackHit* const &hit1, const DCDCTrackHit* const &hit2) {
	// use the ring number to sort by R(decreasing) and then straw(increasing)
	if(hit1->wire->ring == hit2->wire->ring){
		return hit1->wire->straw < hit2->wire->straw;
	}
	return hit1->wire->ring < hit2->wire->ring;
}

//------------------
// FDCSortByZincreasing
//------------------
bool static FDCSortByZincreasing(const DFDCPseudo* const &hit1, const DFDCPseudo* const &hit2) {
	// use the layer number to sort by Z(decreasing) and then wire(increasing)
	if(hit1->wire->layer == hit2->wire->layer){
		return hit1->wire->wire < hit2->wire->wire;
	}
	return hit1->wire->layer < hit2->wire->layer;
}


//------------------
// init
//------------------
jerror_t DTrack_factory_Kalman::init(void)
{
	fitter = NULL;

        DEBUG_LEVEL = 0;

        gPARMS->SetDefaultParameter("TRKFIT:DEBUG_LEVEL",
                DEBUG_LEVEL);

	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t DTrack_factory_Kalman::brun(jana::JEventLoop *loop, int runnumber)
{
	// Get pointer to DTrackFitter object that actually fits a track
	vector<const DTrackFitter *> fitters;
	loop->Get(fitters, "Kalman");
	if(fitters.size()<1){
		_DBG_<<"Unable to get a DTrackFitter object! NO Charged track fitting will be done!"<<endl;
		return RESOURCE_UNAVAILABLE;
	}
	
	// Drop the const qualifier from the DTrackFitter pointer (I'm surely going to hell for this!)
	fitter = const_cast<DTrackFitter*>(fitters[0]);

	// Warn user if something happened that caused us NOT to get a fitter object pointer
	if(!fitter){
		_DBG_<<"Unable to get a DTrackFitter object! NO Charged track fitting will be done!"<<endl;
		return RESOURCE_UNAVAILABLE;
	}
	
	string MASS_HYPOTHESES = "0.13957, 0.938";
	gPARMS->SetDefaultParameter("TRKFIT:MASS_HYPOTHESES", MASS_HYPOTHESES);

	// Parse MASS_HYPOTHESES string to make list of masses to try
	if(MASS_HYPOTHESES.length()>0){
		string &str = MASS_HYPOTHESES;
		unsigned int cutAt;
		while( (cutAt = str.find(",")) != (unsigned int)str.npos ){
			if(cutAt > 0)mass_hypotheses.push_back(atof(str.substr(0,cutAt).c_str()));
			str = str.substr(cutAt+1);
		}
		if(str.length() > 0)mass_hypotheses.push_back(atof(str.c_str()));
	}else{
		mass_hypotheses.push_back(0.0); // If empty string is specified, assume they want massless particle
	}

	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t DTrack_factory_Kalman::evnt(JEventLoop *loop, int eventnumber)
{
	if(!fitter)return NOERROR;
	
	// Get candidates and hits
	vector<const DTrackCandidate*> candidates;
	loop->Get(candidates);
	
	// Loop over candidates
	for(unsigned int i=0; i<candidates.size(); i++){
		const DTrackCandidate *candidate = candidates[i];
	// Make sure there are enough DReferenceTrajectory objects
		while(rtv.size()<=_data.size())rtv.push_back(new DReferenceTrajectory(fitter->GetDMagneticFieldMap()));
		DReferenceTrajectory *rt = rtv[_data.size()];

		// Loop over potential particle masses until one is found that gives a chisq/Ndof<3.0
		// If none does, then use the one with the smallest chisq
		DTrack *best_track = NULL;
		double best_fom = 0.0;
		for(unsigned int j=0; j<mass_hypotheses.size(); j++){

			if(DEBUG_LEVEL>1){_DBG__;_DBG_<<"---- Starting wire based fit for candidate "<<i<<" with mass: "<<mass_hypotheses[j]<<endl;}
			
			// Do the fit
			fitter->SetFitType(DTrackFitter::kWireBased);
			DTrackFitter::fit_status_t status = fitter->FindHitsAndFitTrack(*candidate, rt, loop, mass_hypotheses[j]);
			DTrack *dtrack = NULL;
			switch(status){
				case DTrackFitter::kFitNotDone:
					_DBG_<<"Fitter returned kFitNotDone. This should never happen!!"<<endl;
				case DTrackFitter::kFitFailed:
					continue;
					break;
				case DTrackFitter::kFitSuccess:
				case DTrackFitter::kFitNoImprovement:
					dtrack = MakeDTrack(candidate);
					break;
			}
			
			// Avoid division by zero below
			if(dtrack->Ndof < 1){
				if(DEBUG_LEVEL>1)_DBG_<<"-- new track with mass "<<mass_hypotheses[j]<<" has Ndof="<<dtrack->Ndof<<". Dropping ..."<<endl;
				delete dtrack;
				continue;
			}
			
			// If best_track hasn't been set, then this is the best track!
			if(!best_track){
				best_track = dtrack;
				best_fom = GetFOM(best_track);
				if(DEBUG_LEVEL>1)_DBG_<<"-- first successful fit this candidate with mass: "<<mass_hypotheses[j]<<" (chisq/Ndof="<<(best_track->chisq/best_track->Ndof)<<") fom="<<best_fom<<endl;
				continue;
			}
			
			// If the fit wasn't sucessful, try next mass
			if(!dtrack){
				if(DEBUG_LEVEL>1)_DBG_<<"-- no DTrack made for track with mass "<<mass_hypotheses[j]<<endl;
				continue;
			}
			
			// OK, now we have to make a choice as to which track to keep. The chisq/Ndof is a good,
			// but not sufficient indicator of which hypothesis is best.  For the most part, we 
			// are trying to distinguish between pions and protons, of which the protons may range
			// out if their momentum is low enough. We want to use the track range to help decide.
			// Form a figure of merit based on the chisq/Ndof and the probability of ranging out.
			double fom = GetFOM(dtrack);
			
			// There can be only one! (Highlander)
			if(fom > best_fom){
				if(DEBUG_LEVEL>1)_DBG_<<"-- new best track with mass "<<mass_hypotheses[j]<<" (old chisq/Ndof="<<(best_track->chisq/best_track->Ndof)<<" , new chisq/Ndof="<<(dtrack->chisq/dtrack->Ndof)<<") (old fom="<<best_fom<<" , new fom="<<fom<<")"<<endl;
				delete best_track;
				best_track = dtrack;
				best_fom = fom;
			}else{
				if(DEBUG_LEVEL>1)_DBG_<<"-- keeping best track with mass "<<best_track->mass()<<" (old chisq/Ndof="<<(best_track->chisq/best_track->Ndof)<<" , new chisq/Ndof="<<(dtrack->chisq/dtrack->Ndof)<<") (old fom="<<best_fom<<" , new fom="<<fom<<")"<<endl;
				delete dtrack;
			}
		}

		// If a track fit was successful, then keep it
		if(best_track){
			_data.push_back(best_track);
			if(DEBUG_LEVEL>2)_DBG_<<"adding wire-based track for candidate "<<i<<" (p="<<best_track->momentum().Mag()<<", "<<_data.size()<<" tracks total now)"<<endl;
		}
	}
	

	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t DTrack_factory_Kalman::erun(void)
{
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t DTrack_factory_Kalman::fini(void)
{
	for(unsigned int i=0; i<rtv.size(); i++)delete rtv[i];
	rtv.clear();

	return NOERROR;
}

//------------------
// MakeDTrack
//------------------
DTrack* DTrack_factory_Kalman::MakeDTrack(const DTrackCandidate *candidate)
{
	// Allocate a DReferenceTrajectory object if needed.
	// These each have a large enough memory footprint that
	// it causes noticable performance problems if we allocated
	// and deallocated them every event. Therefore, we allocate
	// when needed, but recycle them on the next event.
	// They are deleted in the fini method.
	while(rtv.size()<=_data.size())rtv.push_back(new DReferenceTrajectory(fitter->GetDMagneticFieldMap()));
	DReferenceTrajectory *rt = rtv[_data.size()];

	DTrack *track = new DTrack;
	
	// Copy over DKinematicData part
	DKinematicData *track_kd = track;
	*track_kd = fitter->GetFitParameters();
	rt->SetMass(track_kd->mass());
	rt->Swim(track->position(), track->momentum(), track->charge());
	
	track->rt = rt;
	track->chisq = fitter->GetChisq();
	track->Ndof = fitter->GetNdof();
	track->candidateid = candidate->id;
	
	// Add hits used as associated objects
	vector<const DCDCTrackHit*> cdchits = fitter->GetCDCFitHits();
	vector<const DFDCPseudo*> fdchits = fitter->GetFDCFitHits();
	sort(cdchits.begin(), cdchits.end(), CDCSortByRincreasing);
	sort(fdchits.begin(), fdchits.end(), FDCSortByZincreasing);
	for(unsigned int i=0; i<cdchits.size(); i++)track->AddAssociatedObject(cdchits[i]);
	for(unsigned int i=0; i<fdchits.size(); i++)track->AddAssociatedObject(fdchits[i]);

	// Add DTrackCandidate as associated object
	track->AddAssociatedObject(candidate);
	
	return track;
}



//------------------
// GetFOM
//------------------
double DTrack_factory_Kalman::GetFOM(DTrack *dtrack)
{
	//double range_out_fom = GetRangeOutFOM(dtrack);
	double chisq_per_dof = dtrack->chisq/(double)dtrack->Ndof;
	
	return chisq_per_dof;
	
	//double total_fom = exp(-pow(range_out_fom/0.5, 2.0))*exp(-pow(chisq_per_dof/2.0, 2.0));
	
	//return total_fom;
}
