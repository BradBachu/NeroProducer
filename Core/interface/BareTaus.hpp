#ifndef BARE_TAUS_H
#define BARE_TAUS_H

#include "NeroProducer/Core/interface/BareCollection.hpp"

class BareTaus : virtual public BareCollection
{
public:
	BareTaus();
	~BareTaus();
	void clear();
	void defineBranches(TTree *t);
	void setBranchAddresses(TTree*) ;
	virtual inline string name(){return "BareTaus";};

	// --  members
	TClonesArray *p4;
	vector<float>           *id ;// byMediumCombinedIsolationDeltaBetaCorr3Hit ?
	vector<int>       	*Q ;// charge
	vector<float>           *M ; // mass
	vector<float>           *iso ;

};
#endif
