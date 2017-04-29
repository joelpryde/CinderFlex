//
//  CinderFlex.h
//
//  Created by Joel Pryde on 5/30/2016
//
//

#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include <NvFlex.h>

namespace cinder { namespace flex {

class CinderFlex
{
	NvFlexSolver *mSolver;
	NvFlexTimers mTimers;
	NvFlexParams mParams;
	std::vector<int> mActiveIndices;
	std::vector<int> mPhases;

public:
	CinderFlex();

	// initialize flex
	void init();

	// setup a solver with a given number of particles
	void setupParticles(unsigned int particleCount, unsigned int diffuseParticleCount);

	// tick the solver
	void update(float elapsed);

	// get current flex params
	const NvFlexParams& getParams() const { return mParams; }

	// set the flex params (for some reason this has strange results if you call it multiple times per solver instance)
	void setParams(NvFlexParams& params) { mParams = params; NvFlexSetParams(mSolver, &mParams); }

	// set the particles positions and velocities (memory should be flexAlloced)
	void setParticles(float* positions, float* velocities, unsigned int particleCount, bool fluid);

	// get the current set of positions and velocities for the solver's particles (memory should be flexAlloced)
	void getParticles(float* positions, float* velocities, unsigned int particleCount);
	const NvFlexTimers& getTimers() const { return mTimers; }
};


} } // namespace flex // namespace cinder
