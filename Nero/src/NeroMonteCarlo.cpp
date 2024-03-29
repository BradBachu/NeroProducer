#include "NeroProducer/Nero/interface/NeroMonteCarlo.hpp"
#include "NeroProducer/Nero/interface/Nero.hpp"

#include "TStopwatch.h"
#define VERBOSE 0

NeroMonteCarlo::NeroMonteCarlo(): BareMonteCarlo()
{
	mMinGenParticlePt = 5.;
	mMinGenJetPt = 20.;
	isRealData = 0;
}

NeroMonteCarlo::~NeroMonteCarlo(){
}

// -- void NeroMonteCarlo::defineBranches(TTree *t)
// -- {
// -- 	//if (I know it is data return;
// -- 	return BareMonteCarlo::defineBranches(t);
// -- }

int NeroMonteCarlo::analyze(const edm::Event& iEvent){

	if ( iEvent.isRealData() ) return 0;
	isRealData = iEvent.isRealData() ? 1 : 0 ; // private, not the one in the tree

	TStopwatch sw;
	if(VERBOSE)sw.Start();
	// maybe handle should be taken before
	iEvent.getByToken(info_token, info_handle);
	iEvent.getByToken(packed_token, packed_handle);
	iEvent.getByToken(pruned_token, pruned_handle);
	iEvent.getByToken(pu_token, pu_handle);
	iEvent.getByToken(jet_token, jet_handle);

	if(VERBOSE){ sw.Stop() ; cout<<"[NeroMonteCarlo]::[analyze] getToken took "<<sw.CpuTime()<<" Cpu and "<<sw.RealTime()<<" RealTime"<<endl; sw.Reset(); sw.Start();}
	// INFO
	mcWeight = info_handle -> weight();
	//weights() 
	qScale   = info_handle -> qScale();
	alphaQED = info_handle -> alphaQED();
	alphaQCD = info_handle -> alphaQCD();
	x1       = info_handle -> pdf() -> x.first;
	x2       = info_handle -> pdf() -> x.second;
	pdf1Id   = info_handle -> pdf() -> id.first;
	pdf2Id   = info_handle -> pdf() -> id.second;
	scalePdf = info_handle -> pdf() -> scalePDF;
		
	//PU
	puTrueInt = 0;
	for(const auto & pu : *pu_handle)
		{
		//Intime
		if (pu.getBunchCrossing() == 0)
			puTrueInt += pu.getTrueNumInteractions();
			//puInt += getPU_NumInteractions(); //old
		//Out-of-time
		}

	if(VERBOSE){ sw.Stop() ; cout<<"[NeroMonteCarlo]::[analyze] pu&info took "<<sw.CpuTime()<<" Cpu and "<<sw.RealTime()<<" RealTime"<<endl; sw.Reset(); sw.Start();}
	// GEN PARTICLES
	TLorentzVector genmet(0,0,0,0);
	//for ( auto & gen : *packed_handle)
	for ( unsigned int i=0;i < packed_handle->size() ;++i)
		{
		const auto gen  = & (*packed_handle)[i];
		if (gen->pt()  < 5 ) continue;
		if (gen->pt() < mMinGenParticlePt ) continue;
		int pdg = gen->pdgId();
		int apdg = abs(pdg);

		//neutrinos
		if (apdg != 12 and apdg !=14 and apdg !=16
			and apdg <1000000 //SUSY
			and fabs(gen->eta() ) <4.7 
			)
			{ 
			 TLorentzVector tmp( gen->px(),gen->py(),gen->pz(),gen->energy() ); 
			 genmet += tmp;
			}

		
		//FILL
		//    e mu photons
		if ( apdg == 11 or apdg == 13 or apdg == 22)
			{
			new ( (*p4)[p4->GetEntriesFast()]) TLorentzVector(gen->px(), gen->py(), gen->pz(), gen->energy());
			pdgId -> push_back( pdg );
			}
		
		} //end packed


	if(VERBOSE){ sw.Stop() ; cout<<"[NeroMonteCarlo]::[analyze] packed took "<<sw.CpuTime()<<" Cpu and "<<sw.RealTime()<<" RealTime"<<endl; sw.Reset(); sw.Start();}
	// LOOP over PRUNED PARTICLES
	//for (auto & gen : *pruned_handle)
	for (unsigned int i=0;i<pruned_handle->size() ;++i)
		{
		const auto gen = &(*pruned_handle)[i];
		if (gen->pt()  < 5 ) continue;
		if (gen->pt()  < mMinGenParticlePt ) continue;
		int pdg = gen->pdgId();
		int apdg = abs(pdg);
		if (gen->status() == 1) continue; //packed

		// tau Z (23)  W (24) H (25) H+ (37)
		if ( apdg == 15 or 
			(apdg >= 23 and apdg <26 ) or 
			apdg == 37)
			{
			new ( (*p4)[p4->GetEntriesFast()]) TLorentzVector(gen->px(), gen->py(), gen->pz(), gen->energy());
			pdgId -> push_back( pdg );
			}
		}

	if(VERBOSE){ sw.Stop() ; cout<<"[NeroMonteCarlo]::[analyze] pruned took "<<sw.CpuTime()<<" Cpu and "<<sw.RealTime()<<" RealTime"<<endl; sw.Reset(); sw.Start();}
	// GEN JETS
	for (const auto & j : *jet_handle)
		{
		if (j.pt() < 20 ) continue;
		if (j.pt() < mMinGenJetPt ) continue;
		// --- FILL
		new ( (*jetP4)[jetP4->GetEntriesFast()]) TLorentzVector(j.px(), j.py(), j.pz(), j.energy());
		}
	if(VERBOSE){ sw.Stop() ; cout<<"[NeroMonteCarlo]::[analyze] jets took "<<sw.CpuTime()<<" Cpu and "<<sw.RealTime()<<" RealTime"<<endl; sw.Reset();}
	return 0;
}


int NeroMonteCarlo::crossSection(edm::Run const & iRun, TH1F* h)
{
	// will be run at the end of run
	if ( isRealData ) return 0; // if the first run has no events ?  TODO

	//iRun.getByToken( runinfo_token, runinfo_handle);
	iRun.getByLabel( "generator", runinfo_handle);
	cout<<"in begin Run  intXS/extXSLO/extXSNLO "<<runinfo_handle->internalXSec().value()<<"/"<<runinfo_handle->externalXSecLO().value()<<"/"<<runinfo_handle->externalXSecNLO().value()<<endl;	

	// Internal xSec =  h(0) / h(1) 
	if ( runinfo_handle->internalXSec().error() != 0 )
		{
	  	h->Fill(0. ,runinfo_handle->internalXSec().value()/pow(runinfo_handle->internalXSec().error(),2)    );
	  	h->Fill(1 ,1./pow(runinfo_handle->internalXSec().error(),2) );
		}
	else 
		{
		cout <<" Warning: ERROR on xSec is 0. Setting it to 1"<<endl;
	  	h->Fill(0. ,runinfo_handle->internalXSec().value()    );
	  	h->Fill(1 ,1.);
		}
  	
	// External xSec =  h(2) / h(3) 
  	h->Fill(2 ,runinfo_handle->externalXSecLO().value()/pow(runinfo_handle->externalXSecLO().error(),2)  );
  	h->Fill(3 ,1./pow(runinfo_handle->externalXSecLO().error(),2)  );
  	
  	h->Fill(4 ,runinfo_handle->externalXSecNLO().value()/pow(runinfo_handle->externalXSecNLO().error(),2) );
  	h->Fill(5 ,1./pow(runinfo_handle->externalXSecNLO().error(),2) );
  	
  	h->Fill(6 , 1 );
  	
  	h->Fill(7 ,pow(runinfo_handle->internalXSec().value(),1) );
  	h->Fill(6 ,pow(runinfo_handle->externalXSecLO().value(),1)  );
  	h->Fill(9 ,pow(runinfo_handle->externalXSecNLO().value(),1) );
  	
  	h->Fill(10 ,pow(runinfo_handle->internalXSec().value(),2) );
  	h->Fill(11 ,pow(runinfo_handle->externalXSecLO().value(),2)  );
  	h->Fill(12 ,pow(runinfo_handle->externalXSecNLO().value(),2) );

	return 0;
}

