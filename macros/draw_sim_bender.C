/*
Author - B.Waidyawansa
Date - 10/29/2013

A macro to open the rootfiles and draw heat deposition plots on the bender.

To use, in root do
 .L draw_sim_bender.C
Then,
raw_sim_bender("negative",500000,"lh2_4cm",11) 
*/


#include<TString.h>
#include<TFile.h>
#include<TChain.h>
#include<TChainElement.h>
#include<TH2.h>
#include<TProfile.h>
#include<TPaletteAxis.h>
#include<TH1.h>
#include<TStyle.h>
#include<iostream>
#include<TPaveText.h>
#include<TCanvas.h>
#include<TKey.h>
#include<TRandom.h>

TH1* merge_histos(TList *sourcelist);


void draw_sim_bender(TString field_status, Double_t Ne, TString target, Double_t energy)
{
 
  //Delete all the objects stored in the current directory memory
  gDirectory->Delete("*");
  //  gStyle->SetCanvasPreferGL(1); this cuases the canvas to save as a blank canvas. So saveAs option is not working..
  gStyle->SetPalette(1); 
  gStyle->SetOptStat(0); 
  
  // pads parameters
  gStyle->SetPadColor(kWhite); 
  gStyle->SetPadBorderMode(0);
  gStyle->SetPadBorderSize(0);
  gStyle->SetPadGridX(kTRUE);
  gStyle->SetPadGridY(kTRUE);
  gStyle->SetPadTopMargin(0.12);
  gStyle->SetPadBottomMargin(0.12);
  gStyle->SetPadLeftMargin(0.08);  
  gStyle->SetPadRightMargin(0.18);  
  gStyle->SetTitleSize(0.05);

  gStyle->SetLabelSize(0.05,"y");

	   
  // Create a canvas 
  TCanvas * C = new TCanvas("canvas","",1300,950);
  C->Divide(2,3);
  C->SetFillColor(0);
             

  // Open and chain the root file
  Char_t filename[200]; 
  // sprintf(filename,"~/rootfiles/rootfiles/shms_sim_benchmarking/%s_%s.000_ntuple.root",target.Data(),field_status.Data());
  
  sprintf(filename,"~/rootfiles/rootfiles/%s/%s_%s_4gev.000_ntuple.root",target.Data(),target.Data(),field_status.Data());
  //filename = "~/rootfiles/rootfiles/lh2_20cm/lh2_20cm_negative_4gev.000_ntuple.root";
  TChain * chain = new TChain("HBCoil");
  TFile *f = new TFile(filename);
  if (f->IsZombie()) {
    std::cout << "Error opening root file chain "<< filename << std::endl;
    exit(1);
  }
  else{
    //    sprintf(filename,"~/rootfiles/rootfiles/%s/%s_%s.*_ntuple.root",target.Data(),target.Data(),field_status.Data());

    sprintf(filename,"~/rootfiles/rootfiles/lh2_20cm/%s_%s_4gev.*_ntuple.root",target.Data(),field_status.Data());
    //filename = "~/rootfiles/rootfiles/lh2_20cm/lh2_20cm_negative_4gev.*_ntuple.root";


    std::cout << "Opening root file "<< filename << std::endl;
    chain->Add(filename);
    if(chain == NULL){
      std::cout<< "Unable to find the ntuple HBCoil"<< std::endl; 
      exit(1);
    }
    else{
      std::cout<<"Obtaining data from ntuple : HBCoil"<<"\n";

      std::cout<<"Number of beam electrons used for the simulation - "<<Ne<<std::endl;

      //////////////////////////////////
      // Display simulation conditions
      //////////////////////////////////
      C->cd(1);
      TPaveText *text = new TPaveText(0.1,0.1,0.9,0.9,"NDC");
      text->AddText(Form("Beam Energy = %2.0f GeV",energy));
      text->AddText(Form("Target - %s",target.Data()));
      text->AddText("Raster size 2mm x 2mm");
      text->AddText(Form("Number of electrons in the beam  %2.0f",Ne));
      text->AddText(Form("%s field",field_status.Data()));
      text->Draw();
      gPad->Update();

      //////////////////////////////////
      // Draw kinetic energy of the particles in the bender
      //////////////////////////////////
      C->cd(2);
      gPad->SetLogy();
      // secondaries generated by the scattering in the target (parentID == 1)
      chain->Draw("kineticE_MeV>>h2","parentID==1","");
      TH1F* h22 = (TH1F*)(gDirectory->Get("h2"));
      h22->Draw("");
      h22->SetFillColor(kGreen+2);
      h22->GetXaxis()->SetTitle("kinetic Energy (MeV)");
      h22->GetXaxis()->SetNoExponent(1);
      h22->GetYaxis()->SetTitle("Number of tracking steps");
      h22->SetTitle("Kinetic Energy of the Particles in the Coil");
      // primaries scattered in the target (parentID == 0)
      chain->Draw("kineticE_MeV>>h1","parentID==0","same");
      TH1F* h11 = (TH1F*)(gDirectory->Get("h1")); 
      h11->SetFillColor(kMagenta+2);
      gPad->Update();
      gPad->SetRightMargin(0.08);

      leg = new TLegend(0.6,0.7,0.9,0.9);
      leg->AddEntry(h11,"Primaries","f");
      leg->AddEntry(h22,"Secondaries","f");
      leg->Draw();

      //////////////////////////////////
      // Draw the heat deposition profile along the far side
      ////////////////////////////////// 
      C->cd(3);
      TString convert = Form("eDep_MeV/%f",Ne);
      chain->Draw(Form("%s:local_z_cm>>far",convert.Data()),"local_x_cm>12","prof");
      TProfile* h1 = (TProfile*)(gDirectory->Get("far"));
      h1->Rebin(6);
      h1->SetTitle("Profile of the heat deposited along the far side of HBCoil");
      h1->GetYaxis()->SetTitle("W#muA^{-1}");
      h1->GetXaxis()->SetTitle("Z (cm)");
      h1->SetLineColor(kBlue);
      h1->SetMarkerSize(0.5);
      h1->SetMarkerStyle(21);
      h1->SetMarkerColor(kBlue);
      h1->SetLineWidth(1.5);
      h1->Draw("E1X0");
      gPad->SetRightMargin(0.08);
      gPad->SetTopMargin(0.15);
      gPad->Update();
      gPad->Modified();
      
      //////////////////////////////////
      // Draw total heat deposition profile on the near side
      //////////////////////////////////
      C->cd(4);
      chain->Draw(Form("%s:local_z_cm>>near",convert.Data()),"local_x_cm<-5","prof");
      TProfile* h2 = (TProfile*)(gDirectory->Get("near"));
      h2->Rebin(6);
      h2->SetTitle("Profile of the heat deposited along the beam side of HBCoil");
      h2->GetYaxis()->SetTitle("W#muA^{-1}");
      h2->GetXaxis()->SetTitle("Z (cm)");
      h2->GetYaxis()->SetDecimals(kTRUE);
      h2->SetLineColor(kBlue);
      h2->SetMarkerSize(0.5);
      h2->SetMarkerStyle(21);
      h2->SetMarkerColor(kBlue);
      h2->SetLineWidth(1.5);
      h2->Draw("E1X0");
      gPad->SetRightMargin(0.08);
      gPad->SetTopMargin(0.15);
      gPad->Update();
      gPad->Modified();


      //////////////////////////////////
      // Draw total heat deposition on the far side
      //////////////////////////////////
      std::cout<<"Get heat depoisition on the far side ..\n"; 
      C->cd(5);
      //chain->Draw(Form("%s:local_z_cm>>left",convert.Data()),"local_x_cm>0","colz");
      chain->Draw("local_y_cm:local_z_cm>>left",Form("%s*(local_x_cm>12)",convert.Data()),"colz");

      TH2F* h3 = (TH2F*)(gDirectory->Get("left"));
      h3->SetStats(0);
      h3->SetTitle("Heat deposited along the far side of HBCoil");
      h3->GetYaxis()->SetTitle("Y (cm) ");
      h3->GetXaxis()->SetTitle("Z (cm)");
      gPad->Update();
      TPaletteAxis *palette1 = (TPaletteAxis*)h3->GetListOfFunctions()->FindObject("palette");
      palette1->GetAxis()->SetTitle("W#muA^{-1}");
      palette1->GetAxis()->SetTitleOffset(1.0);
      palette1->GetAxis()->SetTitleSize(0.05);
      palette1->GetAxis()->SetLabelSize(0.05);
      palette1->GetAxis()->SetDecimals(kTRUE);
      palette1->GetAxis()->SetMaxDigits(3);
      gPad->Modified();

      //////////////////////////////////
      // Draw total heat deposition on the near side
      //////////////////////////////////
      std::cout<<"Get heat depoisition on near side ..\n"; 
      C->cd(6);
      //      chain->Draw(Form("%s:local_z_cm>>right",convert.Data()),"local_x_cm<=0","colz");
      chain->Draw("local_y_cm:local_z_cm>>right",Form("%s*(local_x_cm<-5)",convert.Data()),"colz");

      TH2F* h4 = (TH2F*)(gDirectory->Get("right"));
      h4->SetStats(0);
      h4->SetTitle("Heat deposited along the beam side of HBCoil");
      h4->GetYaxis()->SetTitle("Y (cm) ");
      h4->GetXaxis()->SetTitle("Z (cm)");
      gPad->Update();
      TPaletteAxis *palette2 = (TPaletteAxis*)h4->GetListOfFunctions()->FindObject("palette");
      palette2->GetAxis()->SetTitle("W#muA^{-1}");
      palette2->GetAxis()->SetTitleOffset(1.0);
      palette2->GetAxis()->SetTitleSize(0.05);
      palette2->GetAxis()->SetLabelSize(0.05);
      palette2->GetAxis()->SetDecimals(kTRUE);
      palette2->GetAxis()->SetMaxDigits(2);
      gPad->Modified();  
    }

    C->Modified();
    C->Update();
    C->SaveAs(Form("bender_field_%s_%s_%2.0fgev_sim_plots.png",field_status.Data(),target.Data(),energy));
    //    C->SaveAs(Form("bender_field_%s_%s_%2.0fgev_sim_plots.C",field_status.Data(),target.Data(),energy));

    std::cout<<"Done!"<<std::endl;
  }
}



// from http://root.cern.ch/root/html/tutorials/io/hadd.C.html

TH1* merge_histos(TList *sourcelist){
  TH1 *htot;

  TFile *first_source = (TFile*)sourcelist->First();
  TDirectory *current_sourcedir = gDirectory;
  //gain time, do not add the objects in the list in memory
  Bool_t status = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  
  
  // loop over all keys in this directory
  TChain *globChain = 0;
  TIter nextkey( current_sourcedir->GetListOfKeys() );
  TKey *key, *oldkey=0;
  while ( (key = (TKey*)nextkey())) {

    //keep only the highest cycle number for each key
    if (oldkey && !strcmp(oldkey->GetName(),key->GetName())) continue;
    
    // read object from first source file
    TObject *obj = key->ReadObj();
    
    // if the object is named "tot_edp_hbcoil" merge it
    if ( obj->FindObject("Tot_Edep_HBCoil")) {

      //      cout << "Merging histogram " << obj->GetName() << endl;
      htot = (TH1*)obj;
      
      // loop over all source files and add the content of the
      // correspondant histogram to the one pointed to by "h1"
      TFile *nextsource = (TFile*)sourcelist->After( first_source );
      while ( nextsource ) {
	// make sure we are at the correct directory level by cd'ing to path
	TKey *key2 = (TKey*)gDirectory->GetListOfKeys()->FindObject(htot->GetName());
	if (key2) {
	  TH1 *htemp = (TH1*)key2->ReadObj();
	  htot->Add(htemp);
	  delete htemp;
	}
	
	nextsource = (TFile*)sourcelist->After( nextsource );
      }
    }   
  }

  return htot;
}
