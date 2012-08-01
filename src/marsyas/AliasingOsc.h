/*
** Copyright (C) 1998-2010 George Tzanetakis <gtzan@cs.uvic.ca>
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

#ifndef MARSYAS_MARSYSTEMTEMPLATEBASIC_H
#define MARSYAS_MARSYSTEMTEMPLATEBASIC_H

#include "MarSystem.h"

namespace Marsyas
{
/**
	\class MarSystemTemplateBasic
	\ingroup Processing
	\brief Basic example on how to use controls efficiently in MarSystems

	This example is the same as Gain; it scales the output by
	multiplying each sample with a real number.

	Controls:
	- \b mrs_real/gain [w] : sets the gain multiplier.
	- \b mrs_bool/dummy [rw] : does nothing.
*/

class AliasingOsc: public MarSystem
{
private:
	// Add specific controls needed by this MarSystem.
	void addControls();

	// Reads changed controls and sets up variables if necessary.
	void myUpdate(MarControlPtr sender);

	mrs_real currentValue_;
	mrs_real incr_;
	mrs_real cyclicRate_;

public:
	// AliasingOsc constructor.
	AliasingOsc(std::string name);

	// AliasingOsc copy constructor.
	AliasingOsc(const AliasingOsc& a);

	// AliasingOsc destructor.
	~AliasingOsc();

	// Implementation of the MarSystem::clone() method.
	MarSystem* clone() const;

	// Implementation of the MarSystem::myProcess method.
	void myProcess(realvec& in, realvec& out);
};

}
//namespace Marsyas

#endif
//MARSYAS_MARSYSTEMTEMPLATEBASIC_H

