/*
** Copyright (C) 1998-2006 George Tzanetakis <gtzan@cs.uvic.ca>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "WHaSp.h"
#include "peakView.h"
#include "PeakFeatureSelect.h"
#include "SimilarityMatrix.h"
#include "HWPS.h"

using namespace std;
using namespace Marsyas;

WHaSp::WHaSp(string name):MarSystem("WHaSp", name)
{
	HWPSnet_ = NULL;

	addControls();
}

WHaSp::WHaSp(const WHaSp& a) : MarSystem(a)
{
	ctrl_histSize_ = getctrl("mrs_natural/histSize");
	ctrl_totalNumPeaks_ = getctrl("mrs_natural/totalNumPeaks");
	ctrl_frameMaxNumPeaks_ = getctrl("mrs_natural/frameMaxNumPeaks");
	
	HWPSnet_ = NULL;
}

WHaSp::~WHaSp()
{
	delete HWPSnet_;
}

MarSystem* 
WHaSp::clone() const 
{
  return new WHaSp(*this);
}

void 
WHaSp::addControls()
{
	addctrl("mrs_natural/histSize", 20, ctrl_histSize_);
	addctrl("mrs_natural/totalNumPeaks",0, ctrl_totalNumPeaks_);
	addctrl("mrs_natural/frameMaxNumPeaks", 0, ctrl_frameMaxNumPeaks_);
}

void
WHaSp::createSimMatrixNet()
{
	if(HWPSnet_)
		return;

	HWPSnet_ = new Series("HWPSnet");

	//add a feat selector and 
	//set the features needed for HWPS
	MarSystem* peFeatSelect = new PeakFeatureSelect("peFeatSelect");
	peFeatSelect->updctrl("mrs_natural/selectedFeatures",
		PeakFeatureSelect::pkFrequency | PeakFeatureSelect::pkSetFrequencies| PeakFeatureSelect::pkSetAmplitudes);
	HWPSnet_->addMarSystem(peFeatSelect);

	//create a similarityMatrix MarSystem that uses the HPWS metric
	SimilarityMatrix* simMat = new SimilarityMatrix("simMat");
	simMat->addMarSystem(new HWPS("hwps"));

	HWPSnet_->addMarSystem(simMat);

	//link totalNumPeaks control to PeakFeatureSelect
	HWPSnet_->getctrl("PeakFeatureSelect/peFeatSelect/mrs_natural/totalNumPeaks")->linkTo(ctrl_totalNumPeaks_, NOUPDATE);
	HWPSnet_->update(); //only call update to HWPSnet_ since this is being called from WHaSp::update()! -> avoid potential infinite recursion!
	
	//link frameMaxNumPeaks control to PeakFeatureSelect
	HWPSnet_->getctrl("PeakFeatureSelect/peFeatSelect/mrs_natural/frameMaxNumPeaks")->linkTo(ctrl_frameMaxNumPeaks_, NOUPDATE);
	HWPSnet_->update(); //only call update to HWPSnet_ since this is being called from WHaSp::update()! -> avoid potential infinite recursion!

	//link histSize control to HWPS metric
	HWPSnet_->getctrl("SimilarityMatrix/simMat/HWPS/hwps/mrs_natural/histSize")->linkTo(ctrl_histSize_, NOUPDATE);
	HWPSnet_->update(); //only call update to HWPSnet_ since this is being called from WHaSp::update()! -> avoid potential infinite recursion!
	
	HWPSnet_->setctrl("SimilarityMatrix/simMat/HWPS/hwps/mrs_natural/histSize", 20);
	HWPSnet_->update(); //only call update to HWPSnet_ since this is being called from WHaSp::update()! -> avoid potential infinite recursion!

	//HWPSnet_->updctrl("SimilarityMatrix/simMat/HWPS/hwps/mrs_bool/calcDistance", true);

	HWPSnet_->setctrl("SimilarityMatrix/simMat/HWPS/hwps/mrs_natural/histSize", 100);
	//HWPSnet_->update(); //only call update to HWPSnet_ since this is being called from WHaSp::update()! -> avoid potential infinite recursion!

}

void
WHaSp::myUpdate(MarControlPtr sender)
{
	//output has the same flow format as input (see myProcess())
	MarSystem::myUpdate(sender);

	if(!HWPSnet_)
			createSimMatrixNet();

	//pass input flow configuration to internal MarSystem
	HWPSnet_->setctrl("mrs_natural/inSamples",ctrl_inSamples_);
	HWPSnet_->setctrl("mrs_natural/inObservations", ctrl_inObservations_);
	HWPSnet_->setctrl("mrs_real/israte", ctrl_israte_);
	HWPSnet_->updctrl("mrs_string/inObsNames", ctrl_inObsNames_); //this calls update()
}

void 
WHaSp::myProcess(realvec& in, realvec& out)
{
	peakView inPkView(in);
	peakView outPkView(out);

	//copy input to output, so we can fill in
	//the WHASP values calculated bellow
	out = in;
	
	mrs_natural numPeaks = inPkView.getFrameNumPeaks();

	if(numPeaks > 0)
	{
		simMatrix_.create(numPeaks, numPeaks);
		
		HWPSnet_->process(in, simMatrix_);
		
		mrs_real maxDist = MINREAL;
		for(mrs_natural r=0; r<numPeaks; ++r)
		{
			outPkView(r, peakView::pkVolume) = 0;
			for(mrs_natural c=0; c < numPeaks; ++c)
			{
				if(r != c)
					outPkView(r, peakView::pkVolume) += simMatrix_(r, c); 
			}
			if(outPkView(r, peakView::pkVolume) > maxDist)
				maxDist = outPkView(r, peakView::pkVolume);
		}
		
		mrs_real dist;
		for(mrs_natural i=0; i < numPeaks; ++i)
		{
			//"enhance" all spectral peaks that have a high harmonicity
			//similarity with other peaks (i.e. high HWPS)
			outPkView(i, peakView::pkAmplitude) *= outPkView(i, peakView::pkVolume)/maxDist;

			//filter peaks with Volumes below a defined threshold
			//
			//dist = outPkView(i, peakView::pkVolume)/maxDist;
			//if(dist < 0.75)
			//	outPkView(i, peakView::pkAmplitude) = 0;
		}
	}

	realvec table;
	outPkView.toTable(table);
	MATLAB_PUT(table, "P");
	MATLAB_EVAL("stemPeaks(P)");
}







	
