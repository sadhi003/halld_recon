// $Id$
//
//    File: FDChit_branch.h.h
// Created: Mon Dec 17 2007
// Creator: davidl
//

#ifndef _CDChit_branch_
#define _CDChit_branch_

#include <TObject.h>
#include <TVector3.h>


class CDChit_branch:public TObject{

	public:

		int ring;
		int straw;
		float q;
		float dx;
		float t;
		
		float pthrown; // taken from first DMCThrown object
		int ncdchits; // number of cdc hits in the event
		int ntothits; // number cdc+fdc hits in the event
		
		double resi_thrown;

	private:
		ClassDef(CDChit_branch,1);

};

#endif // _CDChit_branch_

