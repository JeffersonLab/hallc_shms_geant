//******************************************************************
/*
Date - 10/11/2013
Author - B.Waidyawansa

SHMSPrimaryGeneratorAction class



*/
//******************************************************************

#ifndef SHMSPrimaryGeneratorAction_h
#define SHMSPrimaryGeneratorAction_h 1

#include "G4ParticleTable.hh"
#include "G4ParticleGun.hh"
#include "G4Event.hh"

#include "G4VUserPrimaryGeneratorAction.hh"

class SHMSPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
  
  // Constructor and destructor
  //
  SHMSPrimaryGeneratorAction();
  ~SHMSPrimaryGeneratorAction();
  
  // Used by Geant4 to generate the primary particles of the event
  //
  void GeneratePrimaries(G4Event* anEvent);
  
  private:
  
  G4ParticleGun* fParticleGun;
  G4ParticleTable * fParticleTable;
  G4double gun_x_pos;
  G4double gun_y_pos;
  G4double gun_z_pos;

};


#endif
