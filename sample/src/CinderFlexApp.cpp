#include "cinder/app/App.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/Rand.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/GeomIO.h"
#include "cinder/ImageIo.h"
#include "cinder/params/Params.h"

#include "CinderFlex.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CinderFlexApp : public App
{
	flex::CinderFlex mFlex;

	unsigned int mParticleCount;
	float mParticleSize;
	bool mFluid;
	float mDynamicFriction;
	float mViscosity;
	float mVorticityConfinement;
	unsigned int mNumPlanes;
	ColorA mColor;

	NvFlexBuffer*		mPositions;
	NvFlexBuffer*		mVelocities;
	NvFlexBuffer*		mPhases;

	CameraPersp			mCamera;
	CameraUi			mCamUi;
	vec3				mCameraTarget;

	gl::BatchRef		mBatch;
	gl::GlslProgRef		mShader, mPlaneShader;
	gl::VboRef			mInstanceDataVbo;

	gl::BatchRef mPlaneBatch[6];

	params::InterfaceGlRef	mGuiParams;

	void setupParticles();

public:
	void setup() override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void resize() override;
	void update() override;
	void draw() override;
};

void CinderFlexApp::setupParticles()
{
	NvFlexFreeBuffer(mPositions);
	NvFlexFreeBuffer(mVelocities);
	NvFlexFreeBuffer(mPhases);

	mPositions = nullptr;
	mVelocities = nullptr;
	mPhases = nullptr;

	mFlex.setupParticles(mParticleCount, 0);

	mPositions = NvFlexAllocBuffer(mFlex.getLibrary(), mParticleCount, sizeof(vec4), eNvFlexBufferHost);
	mVelocities = NvFlexAllocBuffer(mFlex.getLibrary(), mParticleCount, sizeof(vec3), eNvFlexBufferHost);
	mPhases = NvFlexAllocBuffer(mFlex.getLibrary(), mParticleCount, sizeof(int), eNvFlexBufferHost);

	// map buffers for reading / writing
	vec4* positions = (vec4*)NvFlexMap(mPositions, eNvFlexMapWait);
	vec3* velocities = (vec3*)NvFlexMap(mVelocities, eNvFlexMapWait);
	int* phases = (int*)NvFlexMap(mPhases, eNvFlexMapWait);

	// set positions in a grid and some random velocities to start particles with
	float mass = 1.0f;
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float delta = mParticleSize;
	for (size_t i = 0; i < mParticleCount; ++i)
	{
		positions[i] = vec4(x, y, z, 1.0f / mass);
		x += delta;
		if (x > 5.0f)
		{
			x = 0.0f;
			y += delta;
		}
		if (y > 5.0f)
		{
			x = 0.0f;
			y = 0.0f;
			z += delta;
		}
	}

	int phase = NvFlexMakePhase(0, mFluid ? (eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid) : eNvFlexPhaseSelfCollide);

	for (size_t i = 0; i < mParticleCount; ++i)
	{
		velocities[i] = randVec3();
		phases[i] = phase;
	}

	// setup some parameters
	float boxSize = 4.0f;
	vec3 up = vec3(0.0f, 1.0f, 0.0f);
	NvFlexParams params = mFlex.getParams();
	params.radius = mParticleSize * 2.0f;
	params.solidRestDistance = mParticleSize;

	if (mFluid)
	{
		params.fluidRestDistance = mParticleSize;
		params.dynamicFriction = mDynamicFriction;
		params.viscosity = mViscosity;
		params.numIterations = 3;
		params.vorticityConfinement = mVorticityConfinement;
		params.anisotropyScale = 25.0f;
	}

	(vec4&)params.planes[0] = vec4(up.x, up.y, up.z, 0.0f);
	(vec4&)params.planes[1] = vec4(0.0f, 0.0f, 1.0f, boxSize);
	(vec4&)params.planes[2] = vec4(1.0f, 0.0f, 0.0f, boxSize);
	(vec4&)params.planes[3] = vec4(-1.0f, 0.0f, 0.0f, boxSize);
	(vec4&)params.planes[4] = vec4(0.0f, 0.0f, -1.0f, boxSize);
	(vec4&)params.planes[5] = vec4(0.0f, -1.0f, 0.0f, boxSize);

	params.numPlanes = mNumPlanes;
	mFlex.setParams(params);

	// camera
	mCamera.lookAt(normalize(vec3(3, 3, 6)) * 5.0f, mCameraTarget);
	mCamUi = CameraUi(&mCamera);

	// shaders
	mPlaneShader = gl::context()->getStockShader(gl::ShaderDef().color());
	mShader = gl::GlslProg::create(loadAsset("shader.vert"), loadAsset("shader.frag"));

	// create plane vbos
	float planeSize = boxSize / 2.0f;
	mPlaneBatch[0] = gl::Batch::create(gl::VboMesh::create(geom::Plane().origin(vec3(0.0f, 0.0f, 0.0f)).normal(up).size(vec2(planeSize))), mPlaneShader);
	mPlaneBatch[1] = gl::Batch::create(gl::VboMesh::create(geom::Plane().origin(vec3(0.0f, 0.0f + planeSize / 2.0f, -boxSize / 4.0f)).normal(vec3(0.0f, 0.0f, 1.0f)).size(vec2(planeSize))), mPlaneShader);
	mPlaneBatch[2] = gl::Batch::create(gl::VboMesh::create(geom::Plane().origin(vec3(-boxSize / 4.0f, 0.0f + planeSize / 2.0f, 0.0f)).normal(vec3(1.0f, 0.0f, 0.0f)).size(vec2(planeSize))), mPlaneShader);
	mPlaneBatch[3] = gl::Batch::create(gl::VboMesh::create(geom::Plane().origin(vec3(boxSize / 4.0f, 0.0f + planeSize / 2.0f, 0.0f)).normal(vec3(1.0f, 0.0f, 0.0f)).size(vec2(planeSize))), mPlaneShader);
	mPlaneBatch[4] = gl::Batch::create(gl::VboMesh::create(geom::Plane().origin(vec3(0.0f, 0.0f + planeSize / 2.0f, -boxSize / 4.0f)).normal(vec3(0.0f, 0.0f, -1.0f)).size(vec2(planeSize))), mPlaneShader);
	mPlaneBatch[5] = gl::Batch::create(gl::VboMesh::create(geom::Plane().origin(vec3(0.0f, -boxSize / 4.0f + planeSize / 2.0f, 0.0f)).normal(vec3(0.0f, -1.0f, 0.0f)).size(vec2(planeSize))), mPlaneShader);

	// create particle batch
	gl::VboMeshRef mesh = gl::VboMesh::create(geom::Sphere().subdivisions(10).radius(mParticleSize / 4.0f));
	mInstanceDataVbo = gl::Vbo::create(GL_ARRAY_BUFFER, mParticleCount * sizeof(vec4), positions, GL_DYNAMIC_DRAW);
	geom::BufferLayout instanceDataLayout;
	instanceDataLayout.append(geom::Attrib::CUSTOM_0, 4, 0, 0, 1 /* per instance */);
	mesh->appendVbo(instanceDataLayout, mInstanceDataVbo);
	mBatch = gl::Batch::create(mesh, mShader, { { geom::Attrib::CUSTOM_0, "vInstancePosition" } });
}

void CinderFlexApp::setup()
{
	mPositions = NULL;
	mVelocities = NULL;
	mPhases = NULL;

	setWindowSize(ivec2(1024, 768));
	mParticleCount = 60000;
	mParticleSize = 0.1f;
	mFluid = false;
	mDynamicFriction = 0.5f;
	mViscosity = 0.1f;
	mVorticityConfinement = 0.0f;
	mNumPlanes = 3;
	mColor = ColorA(1.0f, 1.0f, 1.0f, 1.0f);

	// init flex
	mFlex.init();

	// setup Particles
	setupParticles();

	// params
	mGuiParams = params::InterfaceGl::create(getWindow(), "Flex Demo", toPixels(ivec2(300, 400)));
	mGuiParams->setOptions("", "valueswidth=100 refresh=0.1");
	mGuiParams->addParam("Particle Count", &mParticleCount).step(1).updateFn(std::bind(&CinderFlexApp::setupParticles, this));
	mGuiParams->addParam("Particle Size", &mParticleSize).step(0.01f).max(2.0f).updateFn(std::bind(&CinderFlexApp::setupParticles, this));
	mGuiParams->addParam("Num Planes", &mNumPlanes).max(6).updateFn(std::bind(&CinderFlexApp::setupParticles, this));
	mGuiParams->addParam("Color", &mColor);
	mGuiParams->addSeparator();
	mGuiParams->addParam("Fluid", &mFluid).updateFn(std::bind(&CinderFlexApp::setupParticles, this));
	mGuiParams->addParam("Dynamic Friction", &mDynamicFriction).step(0.1f).updateFn(std::bind(&CinderFlexApp::setupParticles, this));
	mGuiParams->addParam("Viscosity", &mViscosity).step(0.1f).updateFn(std::bind(&CinderFlexApp::setupParticles, this));
	mGuiParams->addParam("Vorticity Confinement", &mVorticityConfinement).step(0.1f).updateFn(std::bind(&CinderFlexApp::setupParticles, this));
	mGuiParams->addSeparator();
	mGuiParams->addButton("Reset", std::bind(&CinderFlexApp::setupParticles, this));
	mGuiParams->addSeparator();
}

void CinderFlexApp::mouseDown(MouseEvent event)
{
	mCamUi.mouseDown(event);
}

void CinderFlexApp::mouseDrag(MouseEvent event)
{
	mCamUi.mouseDrag(event);
}

void CinderFlexApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'r')
		setupParticles();
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void CinderFlexApp::update()
{
	mFlex.setParticles(mPositions, mVelocities, mPhases, mParticleCount, mFluid);

	// do flex update
	mFlex.update(1.0f / 60.0f);

	// get particle positions and velocities
	mFlex.getParticles(mPositions, mVelocities, mPhases, mParticleCount);

	// update our instance positions; map our instance data VBO, write new positions, unmap	
	auto vboPositions = (vec4*)mInstanceDataVbo->mapReplace();

	vec4* positions = (vec4*)NvFlexMap(mPositions, eNvFlexMapWait);
	memcpy(vboPositions, positions, sizeof(vec4) * mParticleCount);

	//for (int i = 0; i < mParticleCount; i++) {
	//	*vboPositions = positions[i];
	//	vboPositions++;
	//}

	mInstanceDataVbo->unmap();
	NvFlexUnmap(mPositions);
}

void CinderFlexApp::resize()
{
	mCamera.setAspectRatio(getWindowAspectRatio());
}

void CinderFlexApp::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow(getWindowSize());
	gl::drawString("FPS: " + std::to_string(getAverageFps()), vec2(getWindowWidth() - 50.0f, 10.0f), Color::white());
	gl::setMatrices(mCamera);

	// draw planes
	gl::disableDepthWrite();
	gl::color(1.0f, 1.0f, 1.0f, 0.2f);
	for (size_t p = 0; p < mNumPlanes; p++)
		mPlaneBatch[p]->draw();

	// draw particles
	gl::color(mColor);
	gl::enableDepthWrite();
	gl::enableDepthRead();
	mBatch->drawInstanced(mParticleCount);

	mGuiParams->draw();
}

CINDER_APP(CinderFlexApp, RendererGl)
