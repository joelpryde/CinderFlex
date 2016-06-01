//
//  CinderFlex.cpp
//
//  Created by Joel Pryde on 5/30/2016
//
//

#include "CinderFlex.h"

using namespace std;
using namespace ci;
using namespace ci::app;

namespace cinder { namespace  flex {

CinderFlex::CinderFlex()
{
	mSolver = NULL;
	memset(&mTimers, 0, sizeof(mTimers));
}

void CinderFlex::init()
{
	flexInit();
}

void CinderFlex::setupParticles(unsigned int particleCount, unsigned int diffuseParticleCount)
{
	flexSetFence();
	flexWaitFence();

	// create new solver
	if (mSolver)
		flexDestroySolver(mSolver);
	mSolver = flexCreateSolver(particleCount, diffuseParticleCount);

	// setup some decent default params
	float particleSize = 0.1f;
	mParams.mGravity[0] = 0.0f;
	mParams.mGravity[1] = -9.8f;
	mParams.mGravity[2] = 0.0f;
	mParams.mWind[0] = 0.0f;
	mParams.mWind[1] = 0.0f;
	mParams.mWind[2] = 0.0f;
	mParams.mViscosity = 0.0f;
	mParams.mDynamicFriction = 0.0f;
	mParams.mStaticFriction = 0.0f;
	mParams.mParticleFriction = 0.0f; // scale friction between particles by default
	mParams.mFreeSurfaceDrag = 0.0f;
	mParams.mDrag = 0.0f;
	mParams.mLift = 0.0f;
	mParams.mNumIterations = 3;
	mParams.mAnisotropyScale = 1.0f;
	mParams.mAnisotropyMin = 0.1f;
	mParams.mAnisotropyMax = 2.0f;
	mParams.mSmoothing = 1.0f;
	mParams.mDissipation = 0.0f;
	mParams.mDamping = 0.0f;
	mParams.mParticleCollisionMargin = 0.0f;
	mParams.mShapeCollisionMargin = 0.0f;
	mParams.mCollisionDistance = 0.0f;
	mParams.mPlasticThreshold = 0.0f;
	mParams.mPlasticCreep = 0.0f;
	mParams.mFluid = false;
	mParams.mSleepThreshold = 0.0f;
	mParams.mShockPropagation = 0.0f;
	mParams.mRestitution = 0.0f;
	mParams.mMaxSpeed = FLT_MAX;
	mParams.mRelaxationMode = eFlexRelaxationLocal;
	mParams.mRelaxationFactor = 1.0f;
	mParams.mSolidPressure = 1.0f;
	mParams.mAdhesion = 0.0f;
	mParams.mCohesion = 0.025f;
	mParams.mSurfaceTension = 0.0f;
	mParams.mVorticityConfinement = 0.0f;
	mParams.mBuoyancy = 1.0f;
	mParams.mDiffuseThreshold = 100.0f;
	mParams.mDiffuseBuoyancy = 1.0f;
	mParams.mDiffuseDrag = 0.8f;
	mParams.mDiffuseBallistic = 16;
	mParams.mDiffuseSortAxis[0] = 0.0f;
	mParams.mDiffuseSortAxis[1] = 0.0f;
	mParams.mDiffuseSortAxis[2] = 0.0f;
	mParams.mDiffuseLifetime = 2.0f;
	mParams.mInertiaBias = 0.001f;
	mParams.mRadius = particleSize * 2.0f;
	mParams.mFluidRestDistance = particleSize;
	mParams.mSolidRestDistance = particleSize;
}

void CinderFlex::update(float elapsed)
{
	// tick solver
	flexUpdateSolver(mSolver, elapsed, 1, NULL); // &mTimers);
}

void CinderFlex::setParticles(float* positions, float* velocities, unsigned int particleCount, bool fluid)
{
	flexSetParticles(mSolver, positions, particleCount, eFlexMemoryHostAsync);
	flexSetVelocities(mSolver, velocities, particleCount, eFlexMemoryHostAsync);

	mActiveIndices.resize(particleCount);
	for (size_t i = 0; i < mActiveIndices.size(); ++i)
		mActiveIndices[i] = i;
	flexSetActive(mSolver, &mActiveIndices[0], particleCount, eFlexMemoryHost);

	int phase = flexMakePhase(0, fluid ? (eFlexPhaseSelfCollide | eFlexPhaseFluid) : eFlexPhaseSelfCollide);
	mPhases.resize(particleCount);
	for (size_t i = 0; i < mPhases.size(); ++i)
		mPhases[i] = phase;
	flexSetPhases(mSolver, &mPhases[0], particleCount, eFlexMemoryHost);
}

void CinderFlex::getParticles(float* positions, float* velocities, unsigned int particleCount)
{
	// kick off async memory reads from device
	flexGetParticles(mSolver, positions, particleCount, eFlexMemoryHostAsync);
	flexGetVelocities(mSolver, velocities, particleCount, eFlexMemoryHostAsync);

	// wait for GPU to finish working (can perform async. CPU work here)
	flexSetFence();
	flexWaitFence();
}

}}  // namespace flex // namespace cinder