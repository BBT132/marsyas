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

/** 
    \class PeSynFFT
    \brief  perform amplitude modification of Power Spectrum using Peaks Info

*/

#include "PeSynFFT.h"
#include "PeUtilities.h"

using namespace std;
using namespace Marsyas;

PeSynFFT::PeSynFFT(string name):MarSystem("PeSynFFT", name)
{
 
  addControls();
}

PeSynFFT::PeSynFFT(const PeSynFFT& a) : MarSystem(a)
{
	ctrl_Peaks_ = getctrl("mrs_realvec/peaks");
	ctrl_NbChannels_ = getctrl("mrs_natural/nbChannels");
}

PeSynFFT::~PeSynFFT()
{
}

MarSystem* 
PeSynFFT::clone() const 
{
  return new PeSynFFT(*this);
}

void 
PeSynFFT::addControls()
{
	addctrl("mrs_realvec/peaks", realvec(), ctrl_Peaks_);
	addctrl("mrs_natural/nbChannels", 1, ctrl_NbChannels_);
	setctrlState("mrs_natural/nbChannels", true);
	addctrl("mrs_string/panning", EMPTYSTRING);
	setctrlState("mrs_string/panning", true);
}

void
PeSynFFT::myUpdate(MarControlPtr sender)
{
	MarSystem::myUpdate(sender);
  setctrl("mrs_natural/onSamples", getctrl("mrs_natural/inSamples")->toNatural()*getctrl("mrs_natural/nbChannels")->toNatural());

	realvec conv(4);
	conv.setval(-1);
	if(getctrl("mrs_string/panning")->toString() != EMPTYSTRING)
	{
	string2parameters(getctrl("mrs_string/panning")->toString(), conv, '_');
	}
	fgVolume_ = conv(0); 
  fgPanning_ = conv(1);
	bgVolume_ = conv(2); 
  bgPanning_ = conv(3);

	mask_ = realvec(getctrl("mrs_natural/inObservations")->toNatural()/2);
}

void
PeSynFFT::generateMask(mrs_natural type)
{
	mrs_natural i, j;
	realvec peaks = ctrl_Peaks_->toVec();

	//cout << peaks;
	mrs_natural nbPeaks = peaks.getSize()/nbPkParameters;

	// intialize background
	if(bgVolume_ != -1)
	{
		mask_.setval(bgVolume_);
		if(type == 1)
			mask_*=(1-bgPanning_)/2;
		if(type == 2)
			mask_*=(1+bgPanning_)/2;
	}
	else
		mask_.setval(0);

	// set level info for foreground clusters
	for (i=0 ; i<onObservations_/2; i++)
	{
		for(j=0 ; j<nbPeaks; j++)
		{
			if(peaks(j+pkGroup*nbPeaks) > -1 &&
				 i>=peaks(j+nbPeaks*pkBinLow)*onObservations_ && i<peaks(j+nbPeaks*pkBinHigh)*onObservations_)
			{
				mrs_real vol, pan;
				if(fgVolume_ != -1)
				{
					// use global info
					vol = fgVolume_;
					pan = fgPanning_;
				}
				else
				{
					// use peaks info
					vol = peaks(j+nbPeaks*pkVolume);
					pan = peaks(j+nbPeaks*pkPan);
				}

				mask_(i) = vol;
				// left
				if(type == 1)
					mask_(i) *= (1-pan)/2;
				// right
				if(type == 2)
					mask_(i) *= (1+pan)/2;
				break;
			}
		}
	}
//	cout << mask_;
}

void 
PeSynFFT::myProcess(realvec& in, realvec& out)
{	
	mrs_natural nbChannels = ctrl_NbChannels_->toNatural();
//	cout << in;
	for (t = 0; t < onSamples_; t++)
	{
		generateMask(t+(nbChannels-1));
		for (o=0; o < onObservations_/2; o++)
		{
			//apply PeSynFFT to all channels
			out(o,t) = mask_(o) * in(o, 0);		
		}
		for (o=onObservations_/2; o < onObservations_; o++)
		{
			//apply PeSynFFT to all channels
			out(o,t) = in(o, 0);		
		}
	}
//	cout << out;
}







	
