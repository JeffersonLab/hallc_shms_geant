//******************************************************************
/*
Date - 10/11/2013
Author - B.Waidyawansa

SHMSDetectorConstruction class implementation

10-16-2013 Moved from using sensitive detectors to scoreres.
11-01-2013 Start using sensitive detectors.
12-05-13 Buddhini- Added  pGlobalFieldPropagator->SetLargestAcceptableStep(1 cm) to solve lack of stepping in vacuum. 
                   Without this, particles just take one large step from the gun to the end wall without considering 
		   the magnetic field.
		   Switched to using g4 vacuum, G4_Galactic
*/
//******************************************************************

#include "SHMSDetectorConstruction.hh"
#include "SHMSMagTabulatedField3D.hh"
#include "SHMSDetectorSD.hh"
#include "G4PropagatorInField.hh"

// Geant4 includes
//
#include "globals.hh"
#include "math.h"
#include "G4GeometryManager.hh"
#include "G4VisAttributes.hh"
#include "G4NistManager.hh"
#include "G4FieldManager.hh"
#include "G4SDManager.hh"
#include "G4SDChargedFilter.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4VPrimitiveScorer.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4VSensitiveDetector.hh"

// Materials
//
#include "G4Material.hh"

// Geometry includes
//
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4PVParameterised.hh"
#include "G4PVPlacement.hh"
#include "G4Box.hh"
#include "G4Trap.hh"
#include "G4Tubs.hh"
#include "G4SDManager.hh"
#include "G4ChordFinder.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4UniformMagField.hh"

// Solids
//
#include "G4RotationMatrix.hh"

// GDML parser include
//
#include "G4GDMLParser.hh"


// Possibility to turn off (0) magnetic field and measurement volume. 
#define MAG 1        
#define MAG_POS 0        

SHMSDetectorConstruction* SHMSDetectorConstruction::fgInstance = 0;

SHMSDetectorConstruction* SHMSDetectorConstruction::Instance()
{
// Static acces function via G4RunManager 
  return fgInstance;
} 

SHMSDetectorConstruction::SHMSDetectorConstruction(G4String fGDMLFile)
{
  // Sensitive Detector Manager
  SDman = G4SDManager::GetSDMpointer();
  DefineMaterials();
  fGeomFile =fGDMLFile; // Read from the command prompt  
  G4cout  << "Found geometry file "<< fGeomFile+".gdml"<<G4endl;
}


SHMSDetectorConstruction::~SHMSDetectorConstruction()
{
  if (MAG) delete SHMSMagField;
}

// Constructs geometries and materials
//
G4VPhysicalVolume* SHMSDetectorConstruction::Construct()
{ 
  // Read geometry from the GDML file
  //
  G4cout << "Attempting to read the GDML geometry file." << G4endl;
  if (getenv("G4LIB_USE_GDML")) {

    // ACTIVATING OVERLAP CHECK when read volumes are placed.
    // Can take long time in case of complex geometries
    //
    fGDMLParser.SetOverlapCheck(true);
    fGDMLParser.Read("gdml/"+fGeomFile+".gdml");
    
    // Prints the material information
    //
    G4cout << *(G4Material::GetMaterialTable() ) << G4endl;
    
    // Retrieve the world volume from GDML processor 
    //
    fWorldPhysVol = fGDMLParser.GetWorldVolume();
  }
  else {
    G4cout << "No support for GDML geometry!" << G4endl;
    G4cout << "exiting program.." << G4endl;
    fWorldPhysVol = NULL;
    exit(1);
  }

 
  // Retrieve Auxiliary Information for sensitive detector
  //
  char detectorname[200];

  G4VSensitiveDetector* magnetcoils[5];
  G4int k=0;

  const G4GDMLAuxMapType* auxmap = fGDMLParser.GetAuxMap();
  G4cout << "Found " << auxmap->size()
         << " volume(s) with auxiliary information."
         << G4endl << G4endl;

  for(G4GDMLAuxMapType::const_iterator iter  = auxmap->begin();
      iter != auxmap->end(); iter++) 
    {
      G4LogicalVolume* myvol = (*iter).first;     
      G4cout << " Volume " << myvol->GetName()
	     << " has the following list of auxiliary information: "<< G4endl;
   
      for (G4GDMLAuxListType::const_iterator vit  = (*iter).second.begin();
	   vit != (*iter).second.end(); vit++)
	{
	  G4cout << "--> Type: " << (*vit).type
		 << " Value: "   << (*vit).value << std::endl;
	 
 	  if ((*vit).type=="SensDet")
	    {
	      G4String det_type = (*vit).value;
	      snprintf(detectorname,200,"/detector%i",k+1);
	      magnetcoils[k] = new SHMSDetectorSD(detectorname, det_type);

	      if (magnetcoils[k] != 0)
		{
		  
		  G4cout << "  Creating sensitive detector " << detectorname 
			 <<" of type "<< det_type
			 <<"  for volume " << myvol->GetName()
			 <<G4endl;
		  
		  SDman->AddNewDetector(magnetcoils[k]);
		  myvol->SetSensitiveDetector(magnetcoils[k]);
		  detNames.push_back(det_type);
		}
	      else
		{
		  G4cout <<det_type << " sensitive detector type not found" << G4endl;
		}
	      G4cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << G4endl;
	    }
	}
      k++;
    }


  // Set Visualization attributes and missing materials definitions in GDML
  //
  G4VisAttributes* WorldVisAtt= new G4VisAttributes(G4Colour(1.0,1.0,1.0));  // white
  G4VisAttributes* greenVisAtt= new G4VisAttributes(G4Colour(0.0,1.0,0.0));  // green
  G4VisAttributes* vaccumVisAtt= new G4VisAttributes(G4Colour(0.5,0.5,0.5)); // grey
  G4VisAttributes* copperVisAtt= new G4VisAttributes(G4Colour(1.0,0.0,0.0)); // red
  G4VisAttributes* ssteelVisAtt= new G4VisAttributes(G4Colour(1.0,1.0,1.0)); // white
  G4VisAttributes* lheVisAtt= new G4VisAttributes(G4Colour(0.0,0.0,1.0));    // blue
  G4VisAttributes* aluminumVisAtt= new G4VisAttributes(G4Colour(1.0,0.0,1.0)); // magneta

  fWorldPhysVol->GetLogicalVolume()->SetVisAttributes(WorldVisAtt);  
  WorldVisAtt->SetVisibility(true);
  greenVisAtt->SetVisibility(true);
  greenVisAtt->SetForceSolid(false);
  vaccumVisAtt->SetVisibility(true);
  vaccumVisAtt->SetForceSolid(false);
  copperVisAtt->SetVisibility(true);
  copperVisAtt->SetForceSolid(true);
  ssteelVisAtt->SetVisibility(true);
  ssteelVisAtt->SetForceSolid(true);
  lheVisAtt->SetVisibility(true);
  lheVisAtt->SetForceSolid(true);
  aluminumVisAtt->SetVisibility(true);
  aluminumVisAtt->SetForceSolid(true);

  fWorldPhysVol->GetLogicalVolume()->SetVisAttributes(WorldVisAtt);
  if (fWorldPhysVol->GetLogicalVolume()->GetMaterial()->GetName().compare("Vacuum")==0)
    {
      fWorldPhysVol->GetLogicalVolume()->SetMaterial(fVacuum);
    }
  
  for(int i=0;i<fWorldPhysVol->GetLogicalVolume()->GetNoDaughters();i++)
    {
     
      //G4cout<<" daughter"<<fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)->GetName()<<G4endl; 
      //G4cout<<" material name "<<fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
      //	->GetLogicalVolume()->GetMaterial()->GetName()<<G4endl;
     
      if (fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	  ->GetLogicalVolume()->GetMaterial()->GetName().compare("Vacuum")==0)
 	{
	  fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	    ->GetLogicalVolume()->SetVisAttributes(vaccumVisAtt); 
 	  fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	    ->GetLogicalVolume()->SetMaterial(fVacuum);
 	}
      
      if (fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	  ->GetLogicalVolume()->GetMaterial()->GetName().compare("Copper")==0)
 	{
	  fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	    ->GetLogicalVolume()->SetVisAttributes(copperVisAtt); 
  	}
      
      if (fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	  ->GetLogicalVolume()->GetMaterial()->GetName().compare("SSteel")==0)
 	{
	  fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	    ->GetLogicalVolume()->SetVisAttributes(ssteelVisAtt);
	  fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	    ->GetLogicalVolume()->SetMaterial(fSSteel);
 	}
  
      if (fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	  ->GetLogicalVolume()->GetMaterial()->GetName().compare("He")==0)
 	{
	  fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	    ->GetLogicalVolume()->SetVisAttributes(lheVisAtt);
 	}
    if (fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	  ->GetLogicalVolume()->GetMaterial()->GetName().compare("Al")==0)
 	{
	  fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	    ->GetLogicalVolume()->SetVisAttributes(aluminumVisAtt);
 	}

      // set visualization based on object names
      if (fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	  ->GetLogicalVolume()->GetName().compare("HBCryoBoxLogic")==0)
 	{
	  fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	    ->GetLogicalVolume()->SetVisAttributes(greenVisAtt);
 	}
      
      
      if (fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	  ->GetLogicalVolume()->GetName().compare("TargetLogic")==0)
 	{
	  fWorldPhysVol->GetLogicalVolume()->GetDaughter(i)
	    ->GetLogicalVolume()->SetVisAttributes(lheVisAtt);
 	}

    }


  //  Magnetic Field 
  //
#if MAG

  static G4bool fieldIsInitialized = false;
  if(!fieldIsInitialized)
    {
     
      G4FieldManager   *pFieldMgr;

      if(MAG_POS){
	SHMSMagField= new SHMSMagTabulatedField3D("HB_positive.TABLE");
        G4cout << "************************************" << G4endl
	       << "*                                  *" << G4endl
	       << "*  Positive Magnetic Field is ON   *" << G4endl
	       << "*                                  *" << G4endl
	       << "************************************" << G4endl;
      }
      else {
	SHMSMagField= new SHMSMagTabulatedField3D("HB_negative.TABLE");


        G4cout << "************************************" << G4endl
	       << "*                                  *" << G4endl
	       << "*  Negative Magnetic Field is ON   *" << G4endl
	       << "*                                  *" << G4endl
	       << "************************************" << G4endl;
	
      }

      // Get the global field manager 
      pFieldMgr=G4TransportationManager::GetTransportationManager()->GetFieldManager();
    
      // perform navigation/propagation in a general non-uni-form magnetic field
      G4PropagatorInField* pGlobalFieldPropagator = G4TransportationManager::GetTransportationManager()->GetPropagatorInField();
  
      /*
	12-05-13 Buddhini- Used to solve lack of stepping in vacuum. Without this, particles
	just take one large step from the gun to the end wall..
	from - http://geant4.web.cern.ch/geant4/G4UsersDocuments/UsersGuides/ForApplicationDeveloper/html/Detector/electroMagneticField.html
      The maximum acceptable step should be set to a distance larger than 
      the biggest reasonable step. If the apparatus in a setup has a diameter 
      of two meters, a likely maximum acceptable steplength would be 10 meters. 
      A particle could then take large spiral steps, but would not attempt 
      to take, for example, a 1000-meter-long step in the case of a 
      very low-density material.
  
      */
      G4double maxStep = 1*cm; // should be acceptables for particle tracking through the beamline
      pGlobalFieldPropagator->SetLargestAcceptableStep(maxStep);

      // Set this field to the global field manager 
      pFieldMgr->SetDetectorField(SHMSMagField);


      // Accuracy for one tracking/physics step.
      //
      G4cout<< "// Accuracy for one tracking/physics step "<<pFieldMgr->GetDeltaOneStep()/cm <<"cm" <<endl;
      
      G4ChordFinder *pChordFinder = new G4ChordFinder(SHMSMagField);
      pFieldMgr->SetChordFinder( pChordFinder );
      
      fieldIsInitialized = true;

      if(!(pFieldMgr->DoesFieldExist()))
	G4cerr<<"Unable to find the field. No magneitc field !"<<G4endl;


    }
  

#endif

  return fWorldPhysVol;
}
  
// Utility to build and list necessary materials
//
void SHMSDetectorConstruction::DefineMaterials()
{
   
  // Use NIST database for elements and materials whereever possible.
  G4NistManager* man = G4NistManager::Instance();
  man->SetVerbose(1);
   
  // Define elements from NIST 
  G4Element* C  = man->FindOrBuildElement("C");
  G4Element* Si = man->FindOrBuildElement("Si");
  G4Element* Cr = man->FindOrBuildElement("Cr");
  G4Element* Mn = man->FindOrBuildElement("Mn");
  G4Element* Fe = man->FindOrBuildElement("Fe");
  G4Element* Ni = man->FindOrBuildElement("Ni");
  
  // Define materials not in NIST
  G4double density;
  G4int ncomponents;
  G4double fractionmass;
  
  // Stainless steel
  fSSteel = new G4Material("SSteel",density= 8.06*g/cm3, ncomponents=6);
  fSSteel->AddElement(C, fractionmass=0.001);
  fSSteel->AddElement(Si, fractionmass=0.007);
  fSSteel->AddElement(Cr, fractionmass=0.18);
  fSSteel->AddElement(Mn, fractionmass=0.01);
  fSSteel->AddElement(Fe, fractionmass=0.712);
  fSSteel->AddElement(Ni, fractionmass=0.09); 

  // Vacuum
  fVacuum = man->FindOrBuildMaterial("G4_Galactic");

  G4cout << G4endl << *(G4Material::GetMaterialTable()) << G4endl;

  G4cout << "end material"<< G4endl;  

}
