
// The following are special comments used by RootSpy to know
// which histograms to fetch for the macro.
//
// hnamepath: /occupancy/bcal_num_events
// hnamepath: /occupancy/bcal_adc_occ
// hnamepath: /occupancy/bcal_tdc_occ
//
// e-mail: davidl@jlab.org
// e-mail: elton@jlab.org
// e-mail: dalton@jlab.org
// e-mail: zisis@uregina.ca
// e-mail: tbritton@jlab.org
//
// Guidance: --------------------------------------------
//
//  The ADC occupancies are influenced mostly by the distance between the signal
//  baseline (set during periodic pedestal scans) and the readout threshold 
//  (105 for all channels).  
//  The TDC occupancies are influenced mostly by the distance between the 
//  baseline and the discriminator threshold (set for each channel.)
//
//  In both cases, if there is a hot channel it is usually because the baseline has 
//  drifted upwards.  During office hours contact the Calorimeters/Scintillators on 
//  call phone.  After hours, this can usually wait until the next day so make a log 
//  entry and email to Halld-cal@jlab.org.  If the channel has a occupancy an order 
//  of magnitude higher than the neighbors then contact the on call phone.
//
//  If there is a dead channel make a log entry and email to Halld-cal@jlab.org, if 
//  there is more than 1 dead channel then call the expert.
//
//  Calorimeters/Scintillators on call phone:  354-9399
//
// End Guidance: ----------------------------------------


{
	// RootSpy saves the current directory and style before
	// calling the macro and restores it after so it is OK to
	// change them and not change them back.
	TDirectory *savedir = gDirectory;
	TDirectory *dir = (TDirectory*)gDirectory->FindObjectAny("occupancy");
	if(dir) dir->cd();

	TH2I *bcal_adc_occ = (TH2I*)gDirectory->FindObjectAny("bcal_adc_occ");
	TH2I *bcal_tdc_occ = (TH2I*)gDirectory->FindObjectAny("bcal_tdc_occ");

	double Nevents = 1.0;
	TH1I *bcal_num_events = (TH1I*)gDirectory->FindObjectAny("bcal_num_events");
	if(bcal_num_events) Nevents = (double)bcal_num_events->GetBinContent(1);

	// Just for testing
	if(gPad == NULL){
		TCanvas *c1 = new TCanvas("c1");
		c1->cd(0);
		c1->Draw();
		c1->Update();
	}
	if(!gPad) {savedir->cd(); return;}

	TCanvas *c1 = gPad->GetCanvas();
	c1->cd(0);
	
	c1->Divide(2,1);
	
	TVirtualPad *pad1 = c1->cd(1);
	pad1->SetTicks();
	pad1->SetLeftMargin(0.15);
	pad1->SetRightMargin(0.15);
	if(bcal_adc_occ){
		bcal_adc_occ->SetStats(0);
		bcal_adc_occ->Draw("colz");
	}

	TVirtualPad *pad2 = c1->cd(2);
	pad2->SetTicks();
	pad2->SetLeftMargin(0.15);
	pad2->SetRightMargin(0.15);
	if(bcal_tdc_occ){
		bcal_tdc_occ->SetStats(0);
		bcal_tdc_occ->Draw("colz");
	}

	char str[256];
	sprintf(str,"%0.0f events", Nevents);
	TLatex lat(50.0, 26.5, str);
	lat.SetTextAlign(22);
	lat.SetTextSize(0.035);
	lat.Draw();

#ifdef ROOTSPY_MACROS
	// ------ The following is used by RSAI --------
	if( rs_GetFlag("Is_RSAI")==1 ){
		auto min_events = rs_GetFlag("MIN_EVENTS_RSAI");
		if( min_events < 1 ) min_events = 1E4;
		if( Nevents >= min_events ) {
			cout << "BCAL Flagging AI check after " << Nevents << " events (>=" << min_events << ")" << endl;
			rs_SavePad("BCAL_occupancy", 0);
			rs_ResetAllMacroHistos("//BCAL_occupancy");
		}
	}
#endif
}


