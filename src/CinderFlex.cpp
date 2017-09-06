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

namespace cinder {
	namespace  flex {

		CinderFlex::CinderFlex()
		{
			mLibrary = nullptr;
			mSolver = nullptr;
			memset(&mTimers, 0, sizeof(mTimers));
		}

		void CinderFlex::init()
		{
			mLibrary = NvFlexInit();
		}

		void CinderFlex::setupParticles(unsigned int particleCount, unsigned int diffuseParticleCount)
		{
			// create new solver
			NvFlexSolverDesc solverDesc;
			NvFlexSetSolverDescDefaults(&solverDesc);
			solverDesc.maxParticles = particleCount;
			solverDesc.maxDiffuseParticles = 0;

			mSolver = std::shared_ptr<NvFlexSolver>(NvFlexCreateSolver(mLibrary, &solverDesc));

			// setup some decent default params
			float particleSize = 0.1f;
			mParams.gravity[0] = 0.0f;
			mParams.gravity[1] = -9.8f;
			mParams.gravity[2] = 0.0f;
			mParams.wind[0] = 0.0f;
			mParams.wind[1] = 0.0f;
			mParams.wind[2] = 0.0f;
			mParams.viscosity = 0.0f;
			mParams.dynamicFriction = 0.0f;
			mParams.staticFriction = 0.0f;
			mParams.particleFriction = 0.0f; // scale friction between particles by default
			mParams.freeSurfaceDrag = 0.0f;
			mParams.drag = 0.0f;
			mParams.lift = 0.0f;
			mParams.numIterations = 3;
			mParams.anisotropyScale = 1.0f;
			mParams.anisotropyMin = 0.1f;
			mParams.anisotropyMax = 2.0f;
			mParams.smoothing = 1.0f;
			mParams.dissipation = 0.0f;
			mParams.damping = 0.0f;
			mParams.particleCollisionMargin = 0.0f;
			mParams.shapeCollisionMargin = 0.0f;
			mParams.collisionDistance = 0.0f;
			mParams.sleepThreshold = 0.0f;
			mParams.shockPropagation = 0.0f;
			mParams.restitution = 0.0f;
			mParams.maxSpeed = FLT_MAX;
			mParams.relaxationMode = eNvFlexRelaxationLocal;
			mParams.relaxationFactor = 1.0f;
			mParams.solidPressure = 1.0f;
			mParams.adhesion = 0.0f;
			mParams.cohesion = 0.025f;
			mParams.surfaceTension = 0.0f;
			mParams.vorticityConfinement = 0.0f;
			mParams.buoyancy = 1.0f;
			mParams.diffuseThreshold = 100.0f;
			mParams.diffuseBuoyancy = 1.0f;
			mParams.diffuseDrag = 0.8f;
			mParams.diffuseBallistic = 16;
			mParams.radius = particleSize * 2.0f;
			mParams.fluidRestDistance = particleSize;
			mParams.solidRestDistance = particleSize;
		}

		void CinderFlex::update(float elapsed)
		{
			// tick solver
			NvFlexUpdateSolver(mSolver.get(), elapsed, 1, NULL); // &mTimers);
		}

		void CinderFlex::setParticles(NvFlexBuffer* positions, NvFlexBuffer* velocities, NvFlexBuffer* phases, unsigned int particleCount, bool fluid)
		{
			// unmap buffers
			NvFlexUnmap(positions);
			NvFlexUnmap(velocities);
			NvFlexUnmap(phases);

			// set active particles
			NvFlexBuffer* activeBuffer = NvFlexAllocBuffer(mLibrary, particleCount, sizeof(int), eNvFlexBufferHost);
			int* activeIndices = (int*)NvFlexMap(activeBuffer, eNvFlexMapWait);

			for (int i = 0; i < particleCount; ++i) {
				activeIndices[i] = i;
			}

			NvFlexUnmap(activeBuffer);

			NvFlexSetActive(mSolver.get(), activeBuffer, NULL);
			NvFlexSetActiveCount(mSolver.get(), particleCount);

			// write to device (async)
			NvFlexSetParticles(mSolver.get(), positions, NULL);
			NvFlexSetVelocities(mSolver.get(), velocities, NULL);
			NvFlexSetPhases(mSolver.get(), phases, NULL);
		}

		void CinderFlex::getParticles(NvFlexBuffer* positions, NvFlexBuffer* velocities, NvFlexBuffer* phases, unsigned int particleCount)
		{
			// kick off async memory reads from device
			NvFlexGetParticles(mSolver.get(), positions, NULL);
			NvFlexGetVelocities(mSolver.get(), velocities, NULL);
			NvFlexGetPhases(mSolver.get(), phases, NULL);
		}

	}
}  // namespace flex // namespace cinder