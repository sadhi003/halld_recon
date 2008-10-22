#include <math.h>

#include <DVector3.h>
#include <DMatrix.h>
#include <TH2F.h>
#include <TROOT.h>


#include <JANA/JEventLoop.h>
using namespace jana;

#include "DANA/DApplication.h"
#include "DMagneticFieldStepper.h"
#include "DTrackCandidate.h"
#include "DTrack_factory_ALT3.h"
#include "CDC/DCDCTrackHit.h"
#include "FDC/DFDCPseudo.h"
#include "FDC/DFDCSegment.h"
#include "DKalmanFilter.h"
#include "DReferenceTrajectory.h"

#define CDC_OUTER_RADIUS 57.0

//------------------
// DTrack_factory_ALT3   (Constructor)
//------------------
DTrack_factory_ALT3::DTrack_factory_ALT3(){

  TOF_MASS = 0.13957018;
  MIN_FDC_HIT_PROB=0.1;
  MIN_HITS=6;
}

//------------------
// DTrack_factory_ALT3   (Destructor)
//------------------
DTrack_factory_ALT3::~DTrack_factory_ALT3()
{
}

//------------------
jerror_t DTrack_factory_ALT3::init(void){return NOERROR;}
jerror_t DTrack_factory_ALT3::erun(void){return NOERROR;}
jerror_t DTrack_factory_ALT3::fini(void){return NOERROR;}

//------------------
// brun
//------------------
jerror_t DTrack_factory_ALT3::brun(JEventLoop *loop, int runnumber)
{
  // Get pointer to DMagneticFieldMap field object
  DApplication* dapp=dynamic_cast<DApplication*>(eventLoop->GetJApplication());
  bfield = dapp->GetBfield();
  dgeom  = dapp->GetDGeometry(runnumber);
  // Get the position of the exit of the CDC endplate from DGeometry
  dgeom->GetCDCEndplate(endplate_z,endplate_dz,endplate_rmin,endplate_rmax);
  // Get the half-length of the CDC 
  dgeom->Get("//posXYZ[@volume='centralDC_option-1']/@X_Y_Z",cdc_half_length);

  dapp->Lock();
  cdc_residuals=(TH2F*)gROOT->FindObject("cdc_residuals");
  if (!cdc_residuals){
    cdc_residuals=new TH2F("cdc_residuals","residuals vs R",
			60,0.,60.,100,-1,1.);
    cdc_residuals->SetXTitle("R (cm)");
    cdc_residuals->SetYTitle("#Deltad (cm)");
  }  
  cdc_pulls_histo=(TH2F*)gROOT->FindObject("cdc_pulls");
  if (!cdc_pulls_histo){
    cdc_pulls_histo=new TH2F("cdc_pulls","pulls vs R",
			60,0.,60.,100,-5,5.);
    cdc_pulls_histo->SetXTitle("R (cm)");
    cdc_pulls_histo->SetYTitle("#Deltad/#sigmad");
  } 
  fdc_xresiduals=(TH2F*)gROOT->FindObject("fdc_xresiduals");
  if (!fdc_xresiduals){
    fdc_xresiduals=new TH2F("fdc_xresiduals","x residuals vs z",
			200,170.,370.,100,-1,1.);
    fdc_xresiduals->SetXTitle("z (cm)");
    fdc_xresiduals->SetYTitle("#Deltax (cm)");
  }  
  fdc_yresiduals=(TH2F*)gROOT->FindObject("fdc_yresiduals");
  if (!fdc_yresiduals){
    fdc_yresiduals=new TH2F("fdc_yresiduals","y residuals vs z",
			200,170.,370.,100,-1,1.);
    fdc_yresiduals->SetXTitle("z (cm)");
    fdc_yresiduals->SetYTitle("#Deltay (cm)");
  }
  fdc_ypulls=(TH2F*)gROOT->FindObject("fdc_ypulls");
  if (!fdc_ypulls){
    fdc_ypulls=new TH2F("fdc_ypulls","y pulls vs z",
			    200,170.,370.,100,-5,5.);
    fdc_ypulls->SetXTitle("z (cm)");
    fdc_ypulls->SetYTitle("#Deltay/#sigmay");
  }  
  fdc_xpulls=(TH2F*)gROOT->FindObject("fdc_xpulls");
  if (!fdc_xpulls){
    fdc_xpulls=new TH2F("fdc_xpulls","x pulls vs z",
			    200,170.,370.,100,-5,5.);
    fdc_xpulls->SetXTitle("z (cm)");
    fdc_xpulls->SetYTitle("#Deltax/#sigmax");
  }

  dapp->Unlock();
  return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t DTrack_factory_ALT3::evnt(JEventLoop *loop, int eventnumber)
{
  // Store current event number
  this->eventnumber = eventnumber;
  
  // Get input data
  trackcandidates.clear();
  cdctrackhits.clear();
  fdctrackhits.clear();
  loop->Get(trackcandidates);
  loop->Get(cdctrackhits);
  loop->Get(fdctrackhits,"CORRECTED");	

  // Loop over track candidates
  for(unsigned int i=0; i<trackcandidates.size(); i++){ 
    const DTrackCandidate *tc = trackcandidates[i];    
    vector<const DFDCSegment *>segments;
    vector<const DCDCTrackHit *>cdchits;
    tc->GetT(segments);
    tc->GetT(cdchits);

     // Initialize energy loss sum
    double dEsum=0.;
    unsigned int num_matched_hits=0;
    DVector3 last_pos,last_mom; 
    double R=0;

    // Initialize Kalman filter with B-field
    DKalmanFilter fit(bfield,dgeom);

    for (unsigned int k=0;k<cdchits.size();k++){
      // Find the outer radius with a hit in the CDC
      double r=(cdchits[k]->wire->origin+cdc_half_length[2]*cdchits[k]->wire->udir).Perp();
      if (r>R) R=r;

      fit.AddCDCHit(cdchits[k]);
    }
    num_matched_hits+=cdchits.size();

    // Initialize the stepper 
    DMagneticFieldStepper stepper(bfield,tc->charge());
    
    // Find the track seed by swimming through the field
    DVector3 pos = tc->position();
    DVector3 mom = tc->momentum(); 	
    last_mom=mom;
    DVector3 norm(0,0,1);
    if (segments.size()==0){       
      if (cdchits.size()>0){
	stepper.SwimToRadius(pos,mom,CDC_OUTER_RADIUS,NULL);
      }
      last_pos=pos;
      last_mom=mom;
      
      // Add hits to be put through Kalman engine
      for(unsigned int j=0; j<fdctrackhits.size(); j++){
	const DFDCPseudo *hit = fdctrackhits[j];
	double variance=1.0; //guess for now
	
	// Swim to FDC hit plane
	stepper.SwimToPlane(pos,mom,hit->wire->origin,norm,NULL);

	// Find residual 
	double resi=sqrt((hit->x-pos.x())*(hit->x-pos.x())
			 +(hit->y-pos.y())*(hit->y-pos.y()));
	
	// Use an un-normalized gaussian so that for a residual
	// of zero, we get a probability of 1.0.
	double p = finite(resi) ? exp(-resi*resi/2./variance):0.0;
	if(p>=MIN_FDC_HIT_PROB){
	  fit.AddHit(hit->x,hit->y,hit->wire->origin(2),hit->covxx,
		     hit->covyy,hit->covxy,hit->dE);
	  last_pos=pos;
	  last_mom=mom;
	  num_matched_hits++;
	  dEsum+=hit->dE;
	}
      }      
    }
    else{	
      const DFDCSegment *segment=NULL;
      for (unsigned m=0;m<segments.size();m++){
	segment=segments[m];
	for (unsigned n=0;n<segment->hits.size();n++){
	  const DFDCPseudo *hit=segment->hits[n];
	  fit.AddHit(hit->x,hit->y,hit->wire->origin(2),hit->covxx,
		     hit->covyy,hit->covxy,hit->dE);
	  num_matched_hits++;
	  dEsum+=hit->dE;
	}

      }
      unsigned int last_segment_id=segments.size()-1;
      unsigned int last_hit_id=segments[last_segment_id]->hits.size()-1;

      // If we have a good last segment (with more than 3 hits) use this 
      // to provide the initial guess to the kalman routines, otherwise 
      // swim from the origin to the most downstream fdc plane with a hit.
      
      if (last_hit_id>2)
	GetPositionAndMomentum(segment,last_pos,last_mom);
      else
   
	{
	// Swim to FDC hit plane
	stepper.SwimToPlane(pos,mom,
	    segments[last_segment_id]->hits[last_hit_id]->wire->origin,
			    norm,NULL);
	last_pos=pos;
	last_mom=mom;
      }
     
    }

    if (num_matched_hits>=MIN_HITS){
    // Set the initial parameters from the track candidate
      fit.SetSeed(tc->charge(),last_pos,last_mom);

      // Kalman filter 
      jerror_t error=fit.KalmanLoop(TOF_MASS);

      if (error==NOERROR){
	// Create a new track object
	DTrack *track = new DTrack;
	//track->q=tc->charge();
	
	DVector3 mom,pos;
	fit.GetMomentum(mom);
	fit.GetPosition(pos);
	
	//track->x=pos(0);
	//track->y=pos(1);
	//track->z=pos(2);
	//track->p=mom.Mag();
	//track->phi=mom.Phi();
	//if(track->phi<0.0)track->phi+=2.0*M_PI;
	//track->theta=mom.Theta();
	track->chisq=fit.GetChiSq();
	track->Ndof=fit.GetNDF();
	track->candidateid=tc->id;
	//track->rt=rt;

	// Fill in DKinematicData part
	track->setMass(0.0);
	track->setMomentum(mom);
	track->setPosition(pos);
	track->setCharge(tc->charge());
	/*
	printf("p %f theta %f phi %f z %f doca %f\n",track->momentum().Mag(),track->momentum().Theta(),
	       (track->momentum().Phi()<0?track->momentum().Phi()+2.*M_PI:track->momentum().Phi()),
	       track->position().z(),pos.Perp());
	*/
	// dEdx
	//track->dE=dEsum;
	//track->ds=fit.GetActivePathLength();

	_data.push_back(track);
      }
    }
  }
  

  return NOERROR;
}


// Obtain position and momentum at the exit of a given package using the 
// helical track model.
//
jerror_t DTrack_factory_ALT3::GetPositionAndMomentum(const DFDCSegment *segment,
					      DVector3 &pos, DVector3 &mom){
  // Position of track segment at last hit plane of package
  double x=segment->xc+segment->rc*cos(segment->Phi1);
  double y=segment->yc+segment->rc*sin(segment->Phi1);
  double z=segment->hits[0]->wire->origin(2);
 
  // Track parameters
  double kappa=segment->S(0,0);
  double phi0=segment->S(1,0);
  double tanl=segment->S(3,0);
  double z0=segment->S(4,0);

  // Useful intermediate variables
  double cosp=cos(phi0);
  double sinp=sin(phi0);
  double sperp=(z-z0)/tanl;
  double sin2ks=sin(2.*kappa*sperp);
  double cos2ks=cos(2.*kappa*sperp); 
  kappa=fabs(kappa);  // magnitude of curvature

  // Get Bfield
  double Bx,By,Bz,B;
  bfield->GetField(x,y,z,Bx,By,Bz);
  B=sqrt(Bx*Bx+By*By+Bz*Bz);
  
  // Momentum
  double px=(cosp*cos2ks-sinp*sin2ks)*0.003*B/2./kappa;
  double py=(sinp*cos2ks+cosp*sin2ks)*0.003*B/2./kappa;
  double pz=0.003*B*tanl/2./kappa;

  //if (sqrt(px*px+py*py)>PT_MAX) return VALUE_OUT_OF_RANGE;

  pos.SetXYZ(x,y,z);
  mom.SetXYZ(px,py,pz);

  return NOERROR;
}
