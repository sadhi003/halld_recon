

void CDC_gains(int EXIT_EARLY=0) {

  // set EXIT_EARLY to 1 to fit the sum histograms only, 0 for the individual straw gains


  // Fits histograms from CDC_amp plugin to estimate CDC gains
  // writes digi_scales/ascale to cdc_new_ascale.txt
  // writes wire_gains to cdc_new_wiregains.txt
  // writes fitted histograms to cdc_amphistos.root
  //
  // wire_gains are gain correction constants for individual straws, with respect to sum histogram
  // fit landau to histogram for sum of all straws and histograms for each individual straw
  // set wire_gains to scale individual fit mpvs to the mpv for the sum of all straws histogram.

  // digi_scales/ascale is the overall gain correction factor for the chamber
  // Reference values of IDEALMPV and ASCALE are chosen from a high gain (low pressure) run 11621  
  // The new ascale is ASCALE*IDEALMPV/sum fit mpv
  //
  // (For the first round of gain calibrations, ASCALE was chosen to match the simulation charge histogram.
  // For the next round, ASCALE was chosen to match the dedx histograms from the first round).

  // CDC_amp provides 2D histograms of pulse amplitude vs straw
  // The histograms are projected here into 1D histograms, one per straw
  // For Landau-like fits, need tracked hits with a narrow range of theta

  // NSJ 18 Nov 2016

  // Need 80+ run files to get good fits for individual straws.  2 run files are enough for sum.


  // NSJ 24 Apr 2018
  // Updated in 2018 to use hits within the first 100ns only.  This avoids the gain loss at later drift times which is probably caused by oxygen in the chamber.



  // reference values  

  //  const float IDEALMPV=32.385; //mpv for tracked hits with restricted z,theta from low pressure run 11621

  //  const float IDEALMPV=37.9649; //mpv for tracked hits ***at 0-100ns*** with restricted z,theta from low pressure run 11621

  // NSJ 24 Aug 2018 modified to use 30 degree tracks instead of 90 degree tracks.
  // Still use the restricted time range 0-100ns
  // Both of these give a higher amplitude peak which is easier to fit for high pressure runs

  const float IDEALMPV=88.5315*2.02/2.59; //mpv for hits in 0-100ns with theta 28-32 degrees, z 52-78cm, from low pressure run 11621, adjusted to put dE/dx (1.5GeV) at 2.02 keV/cm to match geant

  const float ASCALE=0.176;   //ascale for tracked ztheta hits from 011621

  const bool REBIN_HISTOS=kFALSE;


  // which theta histos to use for fits 

   int theta = 30;
   int theta_low = 28;
   int theta_high = 32;


  // get histograms
   
  TDirectory *fmain = (TDirectory*)gDirectory->FindObjectAny("CDC_amp");
  if (!fmain) printf("Cannot find directory CDC_amp\n"); 
  if (!fmain) return;
  fmain->cd();


  TH1I *attsum = (TH1I*)fmain->Get(Form("a%i_100ns",theta));
  if (!attsum) printf("Cannot find histogram a%i_100ns\n",theta);
  if (!attsum) return;

  // get sum amplitude histo to find readout range

  TH1I *asum_all= (TH1I*)fmain->Get("a");
  if (!asum_all) printf("Cannot find amplitude (sum over all straws) histogram a\n");
  if (!asum_all) return;

  // find amplitude range 
  int SCALE_UP = 1;
  double highcounts = asum_all->FindLastBinAbove(0);
  if (highcounts > 512) SCALE_UP = 8;  // this is full range 0-4095

  if (SCALE_UP==1) cout << "Amp range 0 to 511" << endl;
  if (SCALE_UP==8) cout << "Amp range 0 to 4095" << endl;

  // fit range constants

  int AL;
  int ANL;
  int AH;

  int INCREMENT_FITSTART;


  if (SCALE_UP==1) { 

    AL = 20; // lower limit attsum 

    ANL=14; // lower limit attsum indiv straw fits

    AH=400; //amp fit upper limit landau

    INCREMENT_FITSTART=1;

  } else {

    AL = 130; // lower limit attsum 

    ANL=110; // lower limit attsum indiv straw fits

    AH=3000; //amp fit upper limit landau

    INCREMENT_FITSTART=8;
 
 }

  const int MINCOUNTS=1000; //counts required to fit histogram
  

  gStyle->SetOptFit(1);
  gStyle->SetFuncWidth(1);

  TF1 *f = new TF1("f","landau");
  f->SetLineColor(6);

  int a_fitstat,q_fitstat;

  float newascale = 0;

  float thismpv = 0;


  new TCanvas;
  printf("\nattsum fit, tracked hits at 0-100ns, restricted z and theta:\n");
  f->SetRange(AL,AH);
  a_fitstat = attsum->Fit(f,"RW");

  if (!a_fitstat) {
    thismpv = f->GetParameter(1);
    printf("\n This mpv: %.3f  ideal mpv: %.3f\n",thismpv,IDEALMPV*SCALE_UP);
    newascale = ASCALE*IDEALMPV*SCALE_UP/thismpv;
    printf("\nnew digi_scales/ascale should be %.3f\n\n",newascale);
  }

  if (a_fitstat) printf("\nSum histogram fit error, exiting script\n");
  if (a_fitstat) return;


  FILE *outfile = fopen("cdc_new_ascale.txt","w");
  fprintf(outfile,"%.3f 0.8\n",newascale); //0.8 is the time scale, for in the 2nd column of ccdb's CDC/digi_scales table
  fclose(outfile);


  TFile *hfile = new TFile("cdc_amphistos.root","RECREATE");

  hfile->cd();
  attsum->Write();

  if (EXIT_EARLY==1) hfile->Write();
  if (EXIT_EARLY==1) hfile->Close();
  if (EXIT_EARLY==1) return;



  TTree *attstats = new TTree("attstats","fit stats for tracked hits with restricted z and theta");

  int n,ring;  //straw number (1-3522), ring number (1-28)

  int a_n;  //count
  double a_mean;
  double a_c;  //const
  double a_mpv;
  double a_mpverr;
  double a_sig;
  double a_chisq;

  int fitlowerlimit;

  attstats->Branch("n",&n,"n/I");
  attstats->Branch("hits",&a_n,"hits/I");
  attstats->Branch("mean",&a_mean,"mean/D");
  attstats->Branch("c",&a_c,"c/D");
  attstats->Branch("mpv",&a_mpv,"mpv/D");
  attstats->Branch("mpverr",&a_mpverr,"mpverr/D");
  attstats->Branch("sig",&a_sig,"sig/D");
  attstats->Branch("chisq",&a_chisq,"chisq/D");
  attstats->Branch("fitstat",&a_fitstat,"fitstat/I");
  attstats->Branch("fitlowerlimit",&fitlowerlimit,"fitlowerlimit/I");

  int i;  

  TH1D *ahisto;
  TH2I *anhisto;
  char htitle[300];


  int badfit, nofit; //counters
  int bincont, lastbin, nbins, lastbintocheck; // to flag low-gain preamp channels

  float wiregain;



  hfile->cd();

  TDirectory *hmain = gDirectory;

  outfile = fopen("cdc_new_wiregains.txt","w");

  new TCanvas;
  
    hmain->cd();

    gDirectory->mkdir("theta")->cd();

    TDirectory *hsub = gDirectory;

    fmain->cd();

    anhisto = (TH2I*)fmain->Get(Form("an%i_100ns",theta));


    badfit = 0;
    nofit = 0;


    for (i=1; i<3523; i++) {   
      //      for (i=1; i<10; i++) {   
 
      fitlowerlimit = ANL;
    
      f->SetRange(fitlowerlimit,AH);

      a_c = 0;
      a_mpv = 0;
      a_mpverr = 0;
      a_sig = 0;
      a_chisq = 0;
      a_fitstat = 0;

      n = i;

      ahisto = anhisto->ProjectionY(Form("a[%i]",i),i+1,i+1);  //straw 1 is in bin 2

      if (REBIN_HISTOS) ahisto->Rebin(4);


      sprintf(htitle,"Amplitude, tracked hits, z=52 to 78 cm, theta=%i to %i degrees, straw %i; amplitude-pedestal",theta_low,theta_high,i);

      ahisto->SetTitle(htitle);

      a_n = ahisto->GetEntries();
      a_mean = ahisto->GetMean();

      //      cout << "fitting straw " << i << endl;
 
      a_fitstat = -1;
      if (a_n > MINCOUNTS) {
        a_fitstat = ahisto->Fit(f,"QW","",fitlowerlimit,AH);  //landau   LL is for low stats
      }


      // if fit did not converge, increase lower limit 

      while (a_fitstat==4 && fitlowerlimit<200) {
        fitlowerlimit += INCREMENT_FITSTART; 
        a_fitstat = ahisto->Fit(f,"W","",fitlowerlimit,AH);  
      }

      if (a_fitstat==0) { 

        a_c      = f->GetParameter(0);
        a_mpv    = f->GetParameter(1);
        a_mpverr = f->GetParError(1);
        a_sig    = f->GetParameter(2);

        if (f->GetNDF()>0) a_chisq  = f->GetChisquare()/f->GetNDF();

      } else if (a_fitstat==4){
	  printf("straw %i unconverged fit \n",i);

      } else if (a_fitstat>0){
        printf("straw %i fitstatus %i fit MPV %f\n",i,a_fitstat,f->GetParameter(1));

      } else if (a_fitstat<0){
        printf("straw %i not enough counts \n",i);
        nofit++;

      }

    


      if (a_fitstat>0) badfit++;
    
    
      if (a_mpv<0) printf("straw %i fit MPV %f\n",i,a_mpv);
      if (a_mpv>0 && a_mpv<fitlowerlimit) printf("straw %i MPV %f below fit lower limit\n",i,a_mpv);
      if (a_mpv>0 && a_mean>3.3*a_mpv) printf("straw %i mean %f MPV %f\n",i,a_mean,a_mpv);


      attstats->Fill();

      wiregain=0;
      if (a_mpv>0) wiregain = thismpv/a_mpv;
      fprintf(outfile,"%.3f\n",wiregain);


      hsub->cd();

      ahisto->Write();

    }

    printf("\nfitstatus 4 count: %i \n",badfit);
    printf("no fit count: %i \n",nofit);



  fclose(outfile);

  hfile->cd();

  attstats->Write();


}
