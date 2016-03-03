// $Id$
//
//    File: DBeamPhoton_factory.h
// Created: Mon Aug  5 14:29:24 EST 2014
// Creator: pmatt (on Linux ifarml6 2.6.18-128.el5 x86_64)
//

#ifndef _DBeamPhoton_factory_
#define _DBeamPhoton_factory_

#include <JANA/JFactory.h>
#include <PID/DBeamPhoton.h>
#include <TAGGER/DTAGMHit.h>
#include <TAGGER/DTAGHHit.h>
#include <DANA/DApplication.h>

class DBeamPhoton_factory:public jana::JFactory<DBeamPhoton>{
	public:
		DBeamPhoton_factory(){};
		~DBeamPhoton_factory(){};

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *locEventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *locEventLoop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.

		double dTargetCenterZ;
        int TAGM_PEAK_CUT, TAGH_PEAK_CUT, TAGM_USE_ADC, TAGH_USE_ADC;
};

#endif // _DBeamPhoton_factory_

