#pragma once;

#include <QMap>
#include <QVector>
#include <QString>
#include <queue>
#include "Stacker/EditingSuggestion.h"

struct PrimitiveState
{
	 void* geometry;
	 bool isFrozen;
};

class ShapeState
{
public:
	QMap< QString, PrimitiveState > primStates;
	double deltaStackability;
	double distortion;

	// N history states have a N-1 long trajectory
	QVector<ShapeState> history;
	QVector<EditingSuggestion> trajectory;

	double energy();

	// 
	QVector<QString> seeds;
};

struct lessDistortion
{
	bool operator () (ShapeState a, ShapeState b);
};

struct lessEnergy
{
	bool operator () (ShapeState a, ShapeState b);
};

typedef std::priority_queue< ShapeState, QVector<ShapeState>, lessEnergy >		PQShapeStateLessEnergy;
typedef std::priority_queue< ShapeState, QVector<ShapeState>, lessDistortion >	PQShapeStateLessDistortion;