//******************************************************************************
// DFDCCathodeCluster_factory.cc
//
// Author:	Craig Bookwalter (craigb at jlab.org)
// Date:	April 2006
//
//******************************************************************************

#include "DFDCCathodeCluster_factory.h"

///
///	DFDCHit_gLayer_cmp(): 
/// a non-member function passed to std::sort() for sorting DFDCHit pointers by
/// their gLayer attributes.
///
bool DFDCHit_gLayer_cmp(const DFDCHit* a, const DFDCHit* b) {
	return a->gLayer < b->gLayer;
}

///
/// DFDCHit_element_cmp():
///	a non-member function passed to std::sort() for sorting DFDCHit pointers by
/// their element (wire or strip) numbers. Typically only used for a single layer
/// of hits.
///
bool DFDCHit_element_cmp(const DFDCHit* a, const DFDCHit* b) {
	return a->element < b->element;
}

///
/// DFDCHit_time_cmp()
///    a non-member function passed to std::stable_sort() for sorting DFDCHit 
/// pointers in order of increasing time, provided that the time difference is
/// significant.
///

bool DFDCHit_time_cmp(const DFDCHit* a, const DFDCHit* b) {
  if (fabs(a->t-b->t)>HIT_TIME_DIFF_MIN && (a->t < b->t))
    return true;
  return false;
}

///
/// DFDCCathodeCluster_gPlane_cmp():
/// a non-member function passed to std::sort() for sorting DFDCCathodeCluster pointers
/// by their gPlane (plane number over all modules, 1-74) attributes.
///
bool DFDCCathodeCluster_gPlane_cmp(	const DFDCCathodeCluster* a, 
									const DFDCCathodeCluster* b) {
	return a->gPlane < b->gPlane;
}

///
/// DFDCCathodeCluster_factory::DFDCCathodeCluster_factory():
///	default constructor--initializes log file
///
DFDCCathodeCluster_factory::DFDCCathodeCluster_factory() {
	_logFile = new ofstream("DFDCCathodeCluster_factory.log");
	_log = new JStreamLog(*_logFile, "CLUST");
}

///
/// DFDCCathodeCluster_factory::~DFDCCathodeCluster_factory():
/// default destructor--closes log file.
///
DFDCCathodeCluster_factory::~DFDCCathodeCluster_factory() {
	_logFile->close();
	delete _logFile;
	delete _log;
}

///
/// DFDCCathodeCluster_factory::evnt():
/// This (along with DFDCCathodeCluster_factory::pique()) 
/// is the place cathode hits are associated into cathode clusters.  
///
jerror_t DFDCCathodeCluster_factory::evnt(JEventLoop *eventLoop, int eventNo) {
	vector<const DFDCHit*> allHits;
	vector<const DFDCHit*> uHits;
	vector<const DFDCHit*> vHits;
	vector<const DFDCHit*> thisLayer;
	
	try {
		eventLoop->Get(allHits);
			
		// Sift through all hits and select out U and V hits.
		for (vector<const DFDCHit*>::iterator i = allHits.begin(); 
			 i != allHits.end(); ++i){
		  if ((*i)->type) {
		    if ((*i)->plane == 1)
		      vHits.push_back(*i);
		    else
		      uHits.push_back(*i);
		  }
		}
		
		// Ensure all cathode hits are in order of increasing Z position.
		std::sort(uHits.begin(), uHits.end(), DFDCHit_gLayer_cmp);
		std::sort(vHits.begin(), vHits.end(), DFDCHit_gLayer_cmp);
		thisLayer.clear();

		// Layer by layer, create clusters of U hits.
		if (uHits.size()>0){
		  vector<const DFDCHit*>::iterator i = uHits.begin();
		  for (int iLayer=1;iLayer<25;iLayer++){
		    while((i!=uHits.end()) && ((*i)->gLayer == iLayer)){
		      thisLayer.push_back(*i);
		      i++;
		    }
		    if (thisLayer.size()>0)
		      pique(thisLayer);
		    thisLayer.clear();
		  }
		}
			
		// Layer by layer, create clusters of V hits.	
		if (vHits.size()>0){
		  vector<const DFDCHit*>::iterator i = vHits.begin();
		  for (int iLayer=1;iLayer<25;iLayer++){
		    while((i!=vHits.end()) && ((*i)->gLayer == iLayer)){
		      thisLayer.push_back(*i);
		      i++;
		    }
		    if (thisLayer.size()>0)
		      pique(thisLayer);
		    thisLayer.clear();
		  }
		}
		
		// Ensure that the data are still in order of Z position.
		std::sort(_data.begin(), _data.end(), DFDCCathodeCluster_gPlane_cmp);
	}
	catch (JException d) {
		cout << d << endl;
	}	
	catch (...) {
		cerr << "exception caught in DFDCCathodeCluster_factory" << endl;
	}
	
	return NOERROR;	
}			

///
/// DFDCCathodeCluster_factory::pique():
/// takes a single layer's worth of cathode hits and attempts to create 
/// DFDCCathodeClusters by grouping together hits with consecutive strip 
/// numbers.
///
void DFDCCathodeCluster_factory::pique(vector<const DFDCHit*>& H) {
	int width(1);
	int beginStrip(0);
	int maxStrip(0);
	float q_tot(0.0);
	float q_max(0.0);
	
	// Ensure the hits are in ascending strip number order
	std::sort(H.begin(), H.end(), DFDCHit_element_cmp);
	// separate clusters in time
	std::stable_sort(H.begin(), H.end(), DFDCHit_time_cmp);
	
	beginStrip = (*(H.begin()))->element;

	// For all hits in this layer, associate consecutively-numbered strips 
        // into a DFDCCathodeCluster object. 
	for (vector<const DFDCHit*>::iterator i = H.begin(); i != H.end(); i++) {
	 
	  // If we're not at the end of the array, and the strip number of the 
	  // next hit is equal to the strip number + 1 of this hit, then we 
	  // continue our cluster.
	  if ((i+1 != H.end()) && ((*i)->element + 1 == (*(i+1))->element)
	      ) {
	    width++;
	    q_tot += (*i)->q;
	    if ((*i)->q > q_max) {
	      q_max = (*i)->q;
	      maxStrip = (*i)->element;
	    }
	  }
		
	  // If not, our cluster must have ended, so we record the information 
	  // into a new DFDCCathodeCluster object and reset for the next 
	  // cluster.
	  else {
	    DFDCCathodeCluster* newCluster = new DFDCCathodeCluster();
	    if (width > 1) {
	      newCluster->beginStrip  = beginStrip;
	      newCluster->maxStrip 	= maxStrip;
	      newCluster->q_tot 		= q_tot;
	    }
	    else {
	      newCluster->beginStrip  = (*i)->element;
	      newCluster->maxStrip	= (*i)->element;
	      newCluster->q_tot		= (*i)->q;
	    }
	    newCluster->width 		= width;
	    newCluster->endStrip	= (*i)->element;
	    newCluster->gLayer		= (*i)->gLayer;
	    newCluster->gPlane		= (*i)->gPlane;
	    newCluster->plane		= (*i)->plane;
	    for (vector<const DFDCHit*>::iterator j = i-width+1; 
		 j <=i ; ++j){
	      newCluster->members.push_back(*j);
	    }
	    _data.push_back(newCluster);
	    width 		= 1;
	    maxStrip 	= 0;
	    q_tot 		= 0.0;
	    q_max		= 0.0;
	    if (i+1 != H.end())
	      beginStrip  = (*(i+1))->element;
	  }
	}
}

