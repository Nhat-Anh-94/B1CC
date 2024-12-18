#include "B1RunAction.hh"
#include "B1PrimaryGeneratorAction.hh"
#include "B1DetectorConstruction.hh"
#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4AccumulableManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "time.h" 
#include "iostream"
#include <iomanip>      // put_time
#include <ctime>        // time_t
#include <chrono>       // system_clock

B1RunAction::B1RunAction()
    : G4UserRunAction(),
    fEdep(0.),
    fEdep2(0.),
    count(0.),
    numberUseful(0.),
    numberUseless(0.),
    OneScatter(0.),
    MoreScatter(0.),
    OneScatterEscape(0.),
    MoreScatterEscape(0.),
    photonScattererCount(0.),
    photonAbsorberCount(0.),
    fOutput("/path/to/output/")  // Khởi tạo fOutput (đảm bảo rằng đường dẫn này hợp lệ)
{
    // add new units for dose
    const G4double milligray = 1.e-3 * gray;
    const G4double microgray = 1.e-6 * gray;
    const G4double nanogray = 1.e-9 * gray;
    const G4double picogray = 1.e-12 * gray;

    new G4UnitDefinition("milligray", "milliGy", "Dose", milligray);
    new G4UnitDefinition("microgray", "microGy", "Dose", microgray);
    new G4UnitDefinition("nanogray", "nanoGy", "Dose", nanogray);
    new G4UnitDefinition("picogray", "picoGy", "Dose", picogray);

    G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
    accumulableManager->RegisterAccumulable(fEdep);
    accumulableManager->RegisterAccumulable(fEdep2);
    photonScattererCount = 0;
    photonAbsorberCount = 0;
}

B1RunAction::~B1RunAction() {}

void B1RunAction::BeginOfRunAction(const G4Run*)
{
    G4RunManager::GetRunManager()->SetRandomNumberStore(false);
    G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
    accumulableManager->Reset();
}

void B1RunAction::EndOfRunAction(const G4Run* run)
{
    G4int nofEvents = run->GetNumberOfEvent();
    if (nofEvents == 0) return;

    G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
    accumulableManager->Merge();

    G4double edep = fEdep.GetValue();
    G4double edep2 = fEdep2.GetValue();

    G4double rms = 0.0;
    if (nofEvents > 0) {
        rms = edep2 - edep * edep / nofEvents;
        if (rms > 0.) rms = std::sqrt(rms);
        else rms = 0.;
    }

    const B1PrimaryGeneratorAction* generatorAction
        = static_cast<const B1PrimaryGeneratorAction*>
        (G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction());
    G4String runCondition;
    if (generatorAction) {
        const G4ParticleGun* particleGun = generatorAction->GetParticleGun();
        runCondition += particleGun->GetParticleDefinition()->GetParticleName();
        runCondition += " of ";
        G4double particleEnergy = particleGun->GetParticleEnergy();
        runCondition += G4BestUnit(particleEnergy, "Energy");
    }

    // Save efficiency data to file
    std::ofstream Efficiency;
    G4double Scent = 0.0, Lcent = 0.0;
    if (count > 0) {
        Scent = (numberUseless / count) * 100;
        Lcent = (numberUseful / count) * 100;
    }

    Efficiency.open(fOutput + "Efficiency.txt", std::ios_base::app);
    if (Efficiency.is_open()) {
        Efficiency << count << " " << numberUseless << " " << Scent << " " << numberUseful << " " << Lcent << "\n";
    }
    else {
        Efficiency << "Unable to open file\n";
    }

    // Save total compton data
    std::ofstream totalComptons;
    totalComptons.open("totalComptons.txt", std::ios_base::app);
    if (totalComptons.is_open()) {
        totalComptons << count << " " << OneScatter << " " << MoreScatter << " " << OneScatterEscape << " " << MoreScatterEscape << "\n";
    }
    else {
        totalComptons << "Unable to open file\n";
    }

    // Save photon count in volumes
    std::ofstream photonsInVolumeFile;
    photonsInVolumeFile.open(fOutput + "totalPhotonsInUsefulVolumes.txt");
    if (photonsInVolumeFile.is_open()) {
        photonsInVolumeFile << photonScattererCount << " " << photonAbsorberCount << std::endl;
    }

    std::cout << "numberUseful=" << numberUseful << "\n";
    std::cout << "numberUseless=" << numberUseless << "\n";

    if (IsMaster()) {
        G4cout << G4endl << "--------------------End of Global Run-----------------------";
    }
    else {
        G4cout << G4endl << "--------------------End of Local Run------------------------";
    }

    G4cout << G4endl
        << " The run consists of " << nofEvents << " " << runCondition
        << G4endl
        << " Cumulated dose per run, in scoring volume : "
        << G4BestUnit(edep, "Dose") << " rms = " << G4BestUnit(rms, "Dose")
        << G4endl
        << "------------------------------------------------------------"
        << G4endl
        << G4endl;
}

void B1RunAction::AddEdep(G4double edep)
{
    fEdep += edep;
    fEdep2 += edep * edep;
}
