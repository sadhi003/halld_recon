// Author: David Lawrence  June 19, 2009
//
//
// mkMaterialMap.cc
//
#include <fstream>

#include "DANA/DApplication.h"
using namespace std;

#include <DVector3.h>
#include <HDGEOMETRY/DRootGeom.h>

#include <TROOT.h>
#include <TFile.h>
#include <TH2D.h>

void ParseCommandLineArguments(int narg, char *argv[], JApplication &app);
void Usage(JApplication &app);


class Material{
	public:
		double A;
		double Z;
		double Density;
		double RadLen;
		double rhoZ_overA;		// density*Z/A
		double rhoZ_overA_logI;	// density*Z/A * log(mean excitation energy)
};


// Table dimensions
int Nr = 500;
int Nz = 1500;
double rmin = 0.0;
double rmax = 9.75;
double zmin = 15.0;
double zmax = 100.0;
int n_r = 3;
int n_z = 3;
int n_phi = 60;


//-----------
// main
//-----------
int main(int narg, char *argv[])
{
	DApplication *dapp = new DApplication(narg, argv);

	ParseCommandLineArguments(narg, argv, *dapp);


	DRootGeom *g = new DRootGeom(dapp);

	// Fill average materials table
	cout<<"Filling material table ..."; cout.flush();

	double r0 = rmin;
	double z0 = zmin;
	double dr = (rmax-rmin)/(double)(Nr);
	double dz = (zmax-zmin)/(double)(Nz);
	Material **MatTable = new Material*[Nr];
	Material *buff = new Material[Nr*Nz];
	for(int ir=0; ir<Nr; ir++){
		double r = r0 + dr/2.0 + (double)ir*dr;
		MatTable[ir]=&buff[ir*Nz];
		for(int iz=0; iz<Nz; iz++){
			double z = z0 + dz/2.0 + (double)iz*dz;
			
			// Loop over points in phi, r, and z and add up material
			double d_r = dr/(double)n_r;
			double d_z = dz/(double)n_z;
			double d_phi = 2.0*M_PI/(double)n_phi;
			Material avg_mat={0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
			for(int i_r=0; i_r<n_r; i_r++){
				double my_r = r - dr/2.0 + (double)i_r*d_r;
				for(int i_z=0; i_z<n_z; i_z++){
					double my_z = z - dz/2.0 + (double)i_z*d_z;
					for(int i_phi=0; i_phi<n_phi; i_phi++){
						double my_phi = (double)i_phi*d_phi;

						DVector3 pos(my_r*cos(my_phi), my_r*sin(my_phi), my_z);
						double A, Z, density, radlen, rhoZ_overA, rhoZ_overA_logI;
						jerror_t err = g->FindMatLL(pos, density, A, Z, radlen);
						if(err==NOERROR){
							rhoZ_overA = density*Z/A;
							double I = (Z*12.0 + 7.0)*1.0E-9; // From Leo 2nd ed. pg 25.
							rhoZ_overA_logI = rhoZ_overA*log(I);
						}else{
							A = Z = density = radlen = rhoZ_overA = rhoZ_overA_logI = 0.0;
						}

						avg_mat.A += A;
						avg_mat.Z += Z;
						avg_mat.Density += density;
						avg_mat.RadLen += 1.0/radlen; // use this to keep proper sum (will be flipped and normalized below)
						avg_mat.rhoZ_overA += rhoZ_overA;
						avg_mat.rhoZ_overA_logI += rhoZ_overA_logI;
					}
				}
			}

			// Divide by number of points to get averages
			double N = (double)(n_r*n_z*n_phi);
			avg_mat.A /= N;
			avg_mat.Z /= N;
			avg_mat.RadLen = N/avg_mat.RadLen; // see eq. 27.23 pg 273 of 2008 PDG (note: we implicitly converted to g cm^-2 and back)
			avg_mat.Density /= N;
			avg_mat.rhoZ_overA /= N;
			avg_mat.rhoZ_overA_logI /= N;
			
			if(!finite(avg_mat.RadLen))avg_mat.RadLen = 0.0;
			
			MatTable[ir][iz] = avg_mat;
		}
		cout<<"\r Filling Material table ... "<<100.0*(double)ir/(double)Nr<<"%       ";cout.flush();
	}
	cout <<"Done"<<endl;
	
	// Write table to file
	cout<<"Writing material table to file ..."<<endl;
	ofstream of("material_map");
	time_t t = time(NULL);
	of<<"#"<<endl;
	of<<"# Material map generated with src/programs/Utilities/mkMaterialMap"<<endl;
	of<<"# generated: "<<ctime(&t);
	of<<"#"<<endl;
	of<<"# Generated with the following parameters:"<<endl;
	of<<"#    Nr = "<<Nr<<endl;
	of<<"#    Nz = "<<Nz<<endl;
	of<<"#  rmin = "<<rmin<<endl;
	of<<"#  rmax = "<<rmax<<endl;
	of<<"#  zmin = "<<zmin<<endl;
	of<<"#  zmax = "<<zmax<<endl;
	of<<"#"<<endl;
	of<<"#  sampling points per cell:"<<endl;
	of<<"#   n_r = "<<n_r<<endl;
	of<<"#   n_z = "<<n_z<<endl;
	of<<"# n_phi = "<<n_phi<<endl;
	of<<"#"<<endl;

	of<<"#  r	z	A	Z	density radlen rhoZ_overA rhoZ_overA_logI"<<endl;
	for(int ir=0; ir<Nr; ir++){
		double r = r0 + dr/2.0 + (double)ir*dr;
		for(int iz=0; iz<Nz; iz++){
			double z = z0 + dz/2.0 + (double)iz*dz;
			
			Material &mat = MatTable[ir][iz];
			
			of<<r<<"\t"<<z<<"\t"<<mat.A<<"\t"<<mat.Z<<"\t"<<mat.Density<<"\t"<<mat.RadLen<<"\t"<<mat.rhoZ_overA<<"\t"<<mat.rhoZ_overA_logI<<endl;
		}
	}
	of.close();
	
	return 0;
}

//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int narg, char *argv[], JApplication &app)
{

	for(int i=1; i<narg; i++){
		string arg = argv[i];
		string arg2 = ((i+1)<narg) ? argv[i+1]:"";
		if(arg=="-h"){
			Usage(app);
			exit(0);
		}
		else if(arg=="-Nr"   ){   Nr = atoi(arg2.c_str());	i++;}
		else if(arg=="-Nz"   ){   Nz = atoi(arg2.c_str());	i++;}
		else if(arg=="-rmin" ){ rmin = atof(arg2.c_str());	i++;}
		else if(arg=="-rmax" ){ rmax = atof(arg2.c_str());	i++;}
		else if(arg=="-zmin" ){ zmin = atof(arg2.c_str());	i++;}
		else if(arg=="-zmax" ){ zmax = atof(arg2.c_str());	i++;}
		else if(arg=="-n_r"  ){  n_r = atoi(arg2.c_str());	i++;}
		else if(arg=="-n_z"  ){  n_z = atoi(arg2.c_str());	i++;}
		else if(arg=="-n_phi"){n_phi = atoi(arg2.c_str());	i++;}
		else{
			cout<<"Unknown argument \""<<arg<<"\""<<endl;
			exit(-1);
		}
		if(i>=narg){
			cout<<"option \""<<arg<<"\" requires a value!"<<endl;
			exit(-2);
		}
	}
}

//-----------
// Usage
//-----------
void Usage(JApplication &app)
{
	cout<<endl;
	cout<<"Usage:"<<endl;
	cout<<"    mkMaterialMap [options] source1 source2 source3 ..."<<endl;
	cout<<endl;
	cout<<"  -Nr #     Set number of grid points in r (def:"<<Nr<<")"<<endl;
	cout<<"  -Nz #     Set number of grid points in z (def:"<<Nz<<")"<<endl;
	cout<<"  -rmin #   Set lower boundary of map in r (def:"<<rmin<<")"<<endl;
	cout<<"  -rmax #   Set upper boundary of map in r (def:"<<rmax<<")"<<endl;
	cout<<"  -zmin #   Set lower boundary of map  in z (def:"<<zmin<<")"<<endl;
	cout<<"  -zmax #   Set upper boundary of map in z (def:"<<zmax<<")"<<endl;
	cout<<"  -n_r #    Set number of sampling points in r (def:"<<n_r<<")"<<endl;
	cout<<"  -n_z #    Set number of sampling points in z (def:"<<n_r<<")"<<endl;
	cout<<"  -n_phi #  Set number of sampling points in phi (def:"<<n_phi<<")"<<endl;
	cout<<endl;
	app.Usage();
	
	exit(0);
}

