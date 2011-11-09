#include "Contoller.h"
#include "Cuboid.h"

Controller::Controller( QSegMesh* mesh )
{
	m_mesh = mesh;
	
	fitPrimitives();
}



Controller::~Controller()
{
	for (int i=0;i<m_mesh->nbSegments();i++)
	{
		delete primitives[i];
	}
}



void Controller::fitPrimitives()
{
	for (int i=0;i<m_mesh->nbSegments();i++)
	{
		Cuboid* cub = new Cuboid(m_mesh->getSegment(i));
		primitives.push_back(cub);
	}
}



void Controller::draw()
{
	for (int i=0;i<m_mesh->nbSegments();i++)
	{
		primitives[i]->draw();
	}
}


void Controller::test1()
{
	// Deform the primitive
	Cuboid* cp = ( Cuboid* )primitives[0];
	cp->scaleAlongAxis(0, 0.6);
	cp->scaleAlongAxis(1, 0.6);
}

void Controller::test2()
{
	primitives[0]->deformMesh();
	m_mesh->build_up();
}