// $Id$

#include "JANA/JEventLoop.h"
#include "DTrack_factory.h"
#include "DTrackCandidate_factory.h"
#include "DTrackCandidate_factory_THROWN.h"
#include "DTrackCandidate_factory_CDC.h"
#include "DTrackCandidate_factory_FDC.h"
#include "DTrackCandidate_factory_FDCCathodes.h"
#include "DTrackCandidate_factory_FDCpseudo.h"
#include "DTrackCandidate_factory_CDC_or_FDCpseudo.h"
#include "DTrack_factory_THROWN.h"
#include "DTrack_factory_ALT2.h"
#include "DTrack_factory_ALT3.h"
#include "DTrack_factory_Kalman.h"
#include "DTrackFitter_factory.h"
#include "DTrackFitter_factory_ALT1.h"
#include "DTrackFitter_factory_Kalman.h"
#include "DTrackHitSelector_factory.h"
#include "DTrackHitSelector_factory_ALT1.h"
#include "DTrackHitSelector_factory_THROWN.h"

#include "DMCThrown.h"
#include "DMCTrackHit.h"
#include "DMCTrajectoryPoint.h"
typedef JFactory<DMCThrown> DMCThrown_factory;
typedef JFactory<DMCTrackHit> DMCTrackHit_factory;
typedef JFactory<DMCTrajectoryPoint> DMCTrajectoryPoint_factory;

jerror_t TRACKING_init(JEventLoop *loop)
{
	/// Create and register TRACKING data factories
	loop->AddFactory(new DTrack_factory_ALT2());
	loop->AddFactory(new DTrack_factory_ALT3());
	loop->AddFactory(new DTrack_factory());
	loop->AddFactory(new DTrack_factory_Kalman());
	loop->AddFactory(new DTrackCandidate_factory());
	loop->AddFactory(new DTrackCandidate_factory_CDC());
	loop->AddFactory(new DTrackCandidate_factory_FDC());
	loop->AddFactory(new DTrackCandidate_factory_FDCCathodes());
	loop->AddFactory(new DTrackCandidate_factory_FDCpseudo());
	loop->AddFactory(new DTrackCandidate_factory_CDC_or_FDCpseudo());
	loop->AddFactory(new DTrackCandidate_factory_THROWN());
	loop->AddFactory(new DMCTrackHit_factory());
	loop->AddFactory(new DMCThrown_factory());
	loop->AddFactory(new DMCTrajectoryPoint_factory());
	loop->AddFactory(new DTrack_factory_THROWN());
	loop->AddFactory(new DTrackFitter_factory());
	loop->AddFactory(new DTrackFitter_factory_ALT1());
	loop->AddFactory(new DTrackFitter_factory_Kalman());
	loop->AddFactory(new DTrackHitSelector_factory());
	loop->AddFactory(new DTrackHitSelector_factory_ALT1());
	loop->AddFactory(new DTrackHitSelector_factory_THROWN());

	return NOERROR;
}
