// $Id$
//
//    File: DParticle_factory.cc
// Created: Thu Sep  4 14:02:44 EDT 2008
// Creator: davidl (on Darwin harriet.jlab.org 8.11.1 i386)
//


#include <iostream>
#include <iomanip>
using namespace std;

#include "DParticle_factory.h"
#include <TRACKING/DTrack.h>
#include <TRACKING/DReferenceTrajectory.h>
#include <TRACKING/DTrackFitter.h>
#include <TRACKING/DTrackHitSelector.h>
using namespace jana;

//------------------
// init
//------------------
jerror_t DParticle_factory::init(void)
{
	fitter = NULL;

	DEBUG_LEVEL = 0;

	gPARMS->SetDefaultParameter("TRKFIT:DEBUG_LEVEL",					DEBUG_LEVEL);

	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t DParticle_factory::brun(jana::JEventLoop *loop, int runnumber)
{
	// Get pointer to TrackFitter object that actually fits a track
	vector<const DTrackFitter *> fitters;
	loop->Get(fitters);
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
	
	// Reserve the first mass hypothesis slot for the one decided on by the wire-base fit
	mass_hypotheses.push_back(0.0);
	
	// Parse MASS_HYPOTHESES string to make list of masses to try
	if(MASS_HYPOTHESES.length()>0){
		string &str = MASS_HYPOTHESES;
		unsigned int cutAt;
		while( (cutAt = str.find(",")) != (unsigned int)str.npos ){
			if(cutAt > 0)mass_hypotheses.push_back(atof(str.substr(0,cutAt).c_str()));
			str = str.substr(cutAt+1);
		}
		if(str.length() > 0)mass_hypotheses.push_back(atof(str.c_str()));
	}

	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t DParticle_factory::evnt(JEventLoop *loop, int eventnumber)
{
	if(!fitter)return NOERROR;
	
	// Get candidates and hits
	vector<const DTrack*> tracks;
	loop->Get(tracks);
	
	// Loop over candidates
	for(unsigned int i=0; i<tracks.size(); i++){
		const DTrack *track = tracks[i];

		// Make sure there are enough DReferenceTrajectory objects
		while(rtv.size()<=_data.size())rtv.push_back(new DReferenceTrajectory(fitter->GetDMagneticFieldMap()));
		DReferenceTrajectory *rt = rtv[_data.size()];

		// Copy in the mass from the wire-based fit as the first one to try
		mass_hypotheses[0] = track->mass();

		// Loop over potential particle masses until one is found that gives a chisq/Ndof<3.0
		// If none does, then use the one with the smallest chisq
		DParticle *best_track = NULL;
		double best_fom = 0.0;
		for(unsigned int j=0; j<mass_hypotheses.size(); j++){
		
			// If best_track is set then check if the fom is large enough
			// that we can skip additional hypotheses
			//if(best_fom>1.0E-5)break;
		
			// Don't try the same mass twice!
			if(j!=0 && mass_hypotheses[j]==mass_hypotheses[0])continue;

			if(DEBUG_LEVEL>1){_DBG__;_DBG_<<"---- Starting time based fit with mass: "<<mass_hypotheses[j]<<endl;}

			// Do the fit
			fitter->SetFitType(DTrackFitter::kTimeBased);
			DTrackFitter::fit_status_t status = fitter->FindHitsAndFitTrack(*track, rt, loop, mass_hypotheses[j]);
			DParticle *dparticle = NULL;
			switch(status){
				case DTrackFitter::kFitNotDone:
					_DBG_<<"Fitter returned kFitNotDone. This should never happen!!"<<endl;
				case DTrackFitter::kFitFailed:
					continue;
					break;
				case DTrackFitter::kFitSuccess:
				case DTrackFitter::kFitNoImprovement:
					dparticle = MakeDParticle(track);
					break;
			}
			
			// Avoid division by zero below
			if(dparticle->Ndof < 1){
				if(DEBUG_LEVEL>1)_DBG_<<"-- new track with mass "<<mass_hypotheses[j]<<" has Ndof="<<dparticle->Ndof<<". Dropping ..."<<endl;
				delete dparticle;
				continue;
			}
			
			// If best_track hasn't been set, then this is the best track!
			if(!best_track){
				best_track = dparticle;
				best_fom = GetFOM(best_track);
				if(DEBUG_LEVEL>1)_DBG_<<"-- first successful fit this candidate with mass: "<<mass_hypotheses[j]<<" (chisq/Ndof="<<(best_track->chisq/best_track->Ndof)<<") fom="<<best_fom<<endl;
				continue;
			}
			
			// If the fit wasn't sucessful, try next mass
			if(!dparticle){
				if(DEBUG_LEVEL>1)_DBG_<<"-- no DTrack made for track with mass "<<mass_hypotheses[j]<<endl;
				continue;
			}
			
			// OK, now we have to make a choice as to which track to keep. The chisq/Ndof is a good,
			// but not sufficient indicator of which hypothesis is best.  For the most part, we 
			// are trying to distinguish between pions and protons, of which the protons may range
			// out if their momentum is low enough. We want to use the track range to help decide.
			// Form a figure of merit based on the chisq/Ndof and the probability of ranging out.
			double fom = GetFOM(dparticle);
			
			// There can be only one! (Highlander)
			if(fom > best_fom){
				if(DEBUG_LEVEL>1)_DBG_<<"-- new best track with mass "<<mass_hypotheses[j]<<" (old chisq/Ndof="<<(best_track->chisq/best_track->Ndof)<<" , new chisq/Ndof="<<(dparticle->chisq/dparticle->Ndof)<<") (old fom="<<best_fom<<" , new fom="<<fom<<")"<<endl;
				delete best_track;
				best_track = dparticle;
				best_fom = fom;
			}else{
				if(DEBUG_LEVEL>1)_DBG_<<"-- keeping best track with mass "<<best_track->mass()<<" (old chisq/Ndof="<<(best_track->chisq/best_track->Ndof)<<" , new chisq/Ndof="<<(dparticle->chisq/dparticle->Ndof)<<") (old fom="<<best_fom<<" , new fom="<<fom<<")"<<endl;
				delete dparticle;
			}
		}

		// If a track fit was successfull, then keep it
		if(best_track)_data.push_back(best_track);
	}

	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t DParticle_factory::erun(void)
{
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t DParticle_factory::fini(void)
{
	for(unsigned int i=0; i<rtv.size(); i++)delete rtv[i];
	rtv.clear();

	return NOERROR;
}

//------------------
// MakeDParticle
//------------------
DParticle* DParticle_factory::MakeDParticle(const DTrack *track)
{
	// Allocate a DReferenceTrajectory object if needed.
	// These each have a large enough memory footprint that
	// it causes noticable performance problems if we allocated
	// and deallocated them every event. Therefore, we allocate
	// when needed, but recycle them on the next event.
	// They are deleted in the fini method.
	while(rtv.size()<=_data.size())rtv.push_back(new DReferenceTrajectory(fitter->GetDMagneticFieldMap()));
	DReferenceTrajectory *rt = rtv[_data.size()];

	DParticle *particle = new DParticle;
	
	// Copy over DKinematicData part
	DKinematicData *track_kd = particle;
	*track_kd = fitter->GetFitParameters();
	rt->SetMass(track_kd->mass());
	rt->Swim(particle->position(), particle->momentum(), particle->charge());
	
	particle->rt = rt;
	particle->chisq = fitter->GetChisq();
	particle->Ndof = fitter->GetNdof();
	particle->trackid = track->id;

	// Add hits used as associated objects
	const vector<const DCDCTrackHit*> &cdchits = fitter->GetCDCFitHits();
	const vector<const DFDCPseudo*> &fdchits = fitter->GetFDCFitHits();
	for(unsigned int i=0; i<cdchits.size(); i++)particle->AddAssociatedObject(cdchits[i]);
	for(unsigned int i=0; i<fdchits.size(); i++)particle->AddAssociatedObject(fdchits[i]);
	
	// Add DTrack object as associate object
	particle->AddAssociatedObject(track);
	
	return particle;
}

//------------------
// GetFOM
//------------------
double DParticle_factory::GetFOM(DParticle *dparticle)
{
	//double range_out_fom = GetRangeOutFOM(dtrack);
	double chisq_per_dof = dparticle->chisq/(double)dparticle->Ndof;
	
	return chisq_per_dof;
	
	//double total_fom = exp(-pow(range_out_fom/0.5, 2.0))*exp(-pow(chisq_per_dof/2.0, 2.0));
	
	//return total_fom;
}

//------------------
// GetRangeOutFOM
//------------------
double DParticle_factory::GetRangeOutFOM(DParticle *dparticle)
{
	/// Calculate a figure of merit for the track ranging out within the
	/// detector. If the particle does range out (lose all of its energy)
	/// then the value returned would be essentially zero. If it does not,
	/// then the value will be greater than zero.
	///
	/// The FOM is ratio of the total pathlength of the track to the 
	/// pathlength to the outermost wire associated with the track.
	/// Therefore, FOM=1 corresponds to a track that goes just
	/// as far after it hit the last wire as before, before finally
	/// hitting the BCAL, FCAL, etc...

	// We want the pathlength to the last wire that is on this track
	// (as determined by the DParticleHitSelector??? class). Since the
	// tracks can curl back in toward the beamline, we have to check every
	// wire to see what the pathlength to it is
	
	// Need the reference trajectory to find pathlengths
	DReferenceTrajectory *rt = const_cast<DReferenceTrajectory*>(dparticle->rt);
	
	// Look first at FDC hits
	vector<const DFDCPseudo*> fdchits;
	dparticle->Get(fdchits);
	const DCoordinateSystem *outermost_wire = NULL;
	double s_to_outermost_wire=-1.0;
	for(unsigned int i=0; i<fdchits.size(); i++){
		DReferenceTrajectory::swim_step_t *step = rt->FindClosestSwimStep(fdchits[i]->wire);
		if(step->s > s_to_outermost_wire){
			s_to_outermost_wire = step->s;
			outermost_wire = fdchits[i]->wire;
		}
	}
	
	// Check CDC if no FDC wire was found
	if(!outermost_wire){
		vector<const DCDCTrackHit*> cdchits;
		dparticle->Get(cdchits);

		for(unsigned int i=0; i<cdchits.size(); i++){
			DReferenceTrajectory::swim_step_t *step = rt->FindClosestSwimStep(cdchits[i]->wire);
			if(step->s > s_to_outermost_wire){
				s_to_outermost_wire = step->s;
				outermost_wire = cdchits[i]->wire;
			}
		}
	}
	
	// Make sure *a* wire was found. (This is just a dummy check)
	if(!outermost_wire){
		_DBG_<<"ERROR: No outermost wire found for track!"<<endl;
		return 0.0;
	}
	
	// Get total pathlength from last swim step
	double total_s = dparticle->rt->swim_steps[dparticle->rt->Nswim_steps-1].s;
	
	// Calculate figure of merit
	double fom = (total_s - s_to_outermost_wire)/s_to_outermost_wire;

	return fom;
}


