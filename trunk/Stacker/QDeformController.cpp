#include "QDeformController.h"
#include "SimpleDraw.h"

QDeformController::QDeformController()
{
	this->frame = new qglviewer::ManipulatedFrame;

	connect(frame, SIGNAL(manipulated()), SLOT(updateController()));
}

qglviewer::ManipulatedFrame * QDeformController::getFrame()
{
	return frame;
}

Vec3d QDeformController::pos()
{
	qglviewer::Vec q = frame->position();
	return Vec3d (q.x,q.y,q.z);
}