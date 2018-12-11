// $Id$
//
//    File: JEventProcessor_occupancy_online.h
// Created: Tue Apr 12 09:43:54 EDT 2016
// Creator: zihlmann (on Linux gluon47.jlab.org 2.6.32-358.23.2.el6.x86_64 x86_64)
//

#ifndef _JEventProcessor_occupancy_online_
#define _JEventProcessor_occupancy_online_

#include <JANA/JEventProcessor.h>

#include <GlueX.h>
#include <PAIR_SPECTROMETER/DPSGeometry.h>


#include <TDirectory.h>
#include <TH2.h>
#include <TH1.h>

using namespace jana;
using namespace std;

class JEventProcessor_occupancy_online:public jana::JEventProcessor{
	public:
		JEventProcessor_occupancy_online();
		~JEventProcessor_occupancy_online();
		const char* className(void){return "JEventProcessor_occupancy_online";}


		//------------------------ BCAL -----------------------
		TH1I *bcal_num_events;
		TH2I *bcal_adc_occ;
		TH2I *bcal_tdc_occ;	

		//------------------------ CCAL -----------------------
		TH1I *ccal_num_events;
		TH2F* ccal_occ;

		//------------------------ CDC ------------------------
		TH1I *cdc_num_events;
		TH2F *cdc_occ_ring[28];

		//------------------------ FCAL -----------------------
		TH1I *fcal_num_events;
		TH2F* fcal_occ;

		//------------------------ FDC ------------------------
        TH1I *fdc_num_events;
        TH2F *fdc_cathode_occ;
        TH2F *fdc_wire_occ;

		//------------------------ PS/PSC ---------------------
		TH1I *ps_num_events;
		TH1I *psc_adc_left_occ;
		TH1I *psc_adc_right_occ;
		TH1I *psc_tdc_left_occ;
		TH1I *psc_tdc_right_occ;
		TH1I *ps_left_occ;
		TH1I *ps_right_occ;

		//------------------------ RF -------------------------
		TH1I *rf_num_events;
		TH1D* rf_occ; //TH1D ON PURPOSE!
		map<DetectorSystem_t, double> dRFBinValueMap;
		
		//------------------------ Trigger -------------------------
		TH2F *L1GTPRate;  // n.b. plotted in highlevel_online/HistMacro_Trigger.C  !!
		TH1F *L1livetime; // n.b. plotted in highlevel_online/HistMacro_Trigger.C  !!

		//------------------------ ST -------------------------
		TH1I *st_num_events;
		TH1I *st_adc_occ;
		TH1I *st_tdc_occ;

		//------------------------ TAGH -----------------------
		TH1I *tag_num_events;
		TH1I *tagh_adc_occ;
		TH1I *tagh_tdc_occ;

		//------------------------ TAGM -----------------------
		TH1I *tagm_adc_occ;
		TH1I *tagm_tdc_occ;

		//------------------------ TPOL -----------------------
		TH1I *tpol_occ;

		//------------------------ TOF ------------------------
		TH1I *tof_num_events;
		TH1I *tof_tdc_S_occ;
		TH1I *tof_tdc_N_occ;
		TH1I *tof_tdc_U_occ;
		TH1I *tof_tdc_D_occ;

		TH1I *tof_adc_S_occ;
		TH1I *tof_adc_N_occ;
		TH1I *tof_adc_U_occ;
		TH1I *tof_adc_D_occ;

		//------------------------ DigiHits ------------------------
		map<string,double> digihitbinmap; // bin number
		map<string,double> digihitsclmap; // Scale number of hits by this (0 means don't scale)
		TH2I *digihits_trig1;
		TH2I *digihits_trig3;
		TH2I *digihits_trig4;
		TH1F *digihits_scale_factors;


	private:
		jerror_t init(void);
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber);
		jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber);
		jerror_t erun(void);
		jerror_t fini(void);
};

#endif // _JEventProcessor_occupancy_online_

