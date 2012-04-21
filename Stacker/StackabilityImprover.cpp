#include "StackabilityImprover.h"

#include "Offset.h"
#include "Primitive.h"
#include "Controller.h"
#include "Propagator.h"


QVector<EditingSuggestion> suggestions;

StackabilityImprover::StackabilityImprover( Offset *offset )
{
	activeOffset = offset;
	isSuggesting = false;
}

std::vector< Vec3d > StackabilityImprover::getLocalMoves( HotSpot& HS )
{
	std::vector< Vec3d > result;

	// pos
	Vec3d hotPoint = HS.rep.first();

	// step
	Vec3d step = (constraint_bbmax - constraint_bbmin) / 20;
	int K = 6;

	// Horizontal moves
	if (HS.defineHeight)
	{
		double min_x = - step[0] * K;
		double max_x = - min_x;
		double min_y = - step[1] * K;
		double max_y = - min_y;

		for (double x = min_x; x <= max_x; x += step[0]){
			for (double y = min_y; y <= max_y; y += step[1]){
				// for (double z = min_z; z <= max_z; z += step[2])
				double z = 0;
				{
					Vec3d delta(x,y,z);
					Vec3d pos = hotPoint + delta;

					// check whether \pos is in BB
					if (RANGE(pos[0], constraint_bbmin[0], constraint_bbmax[0])
						&& RANGE(pos[1], constraint_bbmin[1], constraint_bbmax[1])
						&& RANGE(pos[2], constraint_bbmin[2], constraint_bbmax[2]))
					{
						result.push_back(delta);
					}
				}			
			}
		}
	}

	// Vertical moves
	if (!HS.defineHeight)
	{
		double min_z = - step[2] * K;
		double max_z = - min_z;
		if (1 == HS.side)
			max_z = 0;
		else
			min_z = 0;

		for (double z = min_z; z <= max_z; z += step[2])
		{
			Vec3d delta = Vec3d(0, 0, z);
			Vec3d pos = hotPoint + delta;

			// check whether \pos is in BB
			if (RANGE(pos[0], constraint_bbmin[0], constraint_bbmax[0])
				&& RANGE(pos[1], constraint_bbmin[1], constraint_bbmax[1])
				&& RANGE(pos[2], constraint_bbmin[2], constraint_bbmax[2]))
			{
				result.push_back(delta);
			}
		}
	}
	return result;
}

void StackabilityImprover::deformNearPointHotspot( HotSpot& freeHS, HotSpot& fixedHS )
{
	Primitive* free_prim = ctrl()->getPrimitive(freeHS.segmentID);
	Primitive* fixed_prim = ctrl()->getPrimitive(fixedHS.segmentID);

	Point free_handle = freeHS.rep.first();
	Point fixed_hanble = fixedHS.rep.first();

	// The constraints solver
	Propagator propagator(ctrl());

	// Move hot spot side away or closer to each other
	std::vector< Vec3d > Ts = getLocalMoves(freeHS);
	
	// Modify the shape
	for (int i=0;i<Ts.size();i++)
	{
		// Clear the frozen flags
		ctrl()->setPrimitivesFrozen(false);		

		// Modify the hot segment(s)
		fixed_prim->addFixedPoint(fixed_hanble); // original position for fixedHS
		free_prim->movePoint(free_handle, Ts[i]); // locally move freeHS
		free_prim->addFixedPoint(free_handle + Ts[i]); // moved position for freeHS

		// Regroup hot segments pair
		propagator.regroupPair(free_prim->id, fixed_prim->id);
				
		// Propagate the deformation
		free_prim->isFrozen = true;
		fixed_prim->isFrozen = true;
		propagator.execute();

		// BB constraint is hard		
		if ( !satisfyBBConstraint() ) continue;

		// 
		double stackability = activeOffset->getStackability(true);

		// Save state history
		ShapeState state = ctrl()->getShapeState();
		state.history = currentCandidate.history;
		state.history.push_back(state);
		state.deltaStackability = stackability - orgStackability;
		state.distortion = ctrl()->getDistortion();

		// Evaluate suggestion
		EditingSuggestion suggest;
		suggest.center = free_handle;
		suggest.direction = Ts[i];
		suggest.deltaS = state.deltaStackability;
		suggest.deltaV = state.distortion;
		suggest.side = freeHS.side;
		suggest.value = state.energy();

		state.trajectory = currentCandidate.trajectory;
		state.trajectory.push_back(suggest);

		// Weither or not we are tracking candidate solutions
		if (isSuggesting)
		{
			suggestions.push_back(suggest);
			suggestSolutions.push(state);
		}
		else
		{
			if (stackability > TARGET_STACKABILITY)
				solutions.push(state);
			else if (isUnique(state, 0))
				candidateSolutions.push(state);
		}		


		// Restore the shape state of current candidate
		ctrl()->setShapeState(currentCandidate);
	}
}

void StackabilityImprover::deformNearLineHotspot( HotSpot& freeHS, HotSpot& fixedHS )
{

}

void StackabilityImprover::deformNearRingHotspot( HotSpot& freeHS )
{
	Primitive* prim = ctrl()->getPrimitive(freeHS.segmentID);

	// The hot region	
	std::vector< Vec2i > &hotRegion = activeOffset->hotRegions[freeHS.hotRegionID];

	Vec2i center = centerOfRegion(hotRegion);
	Vec2i p = hotRegion[0];

	// Save the initial hot shape state
	ShapeState initialHotShapeState = ctrl()->getShapeState();

	// Translations
	std::vector< Vec3d > Ts;
	Ts.push_back(Vec3d(1,0,0));
	Ts.push_back(Vec3d(-1,0,0));

	Propagator propagator(ctrl());

	Vec3d hotPoint = freeHS.hotSamples[0];
	for (int i=0;i<Ts.size();i++)
	{
		// Clear the frozen flags
		ctrl()->setPrimitivesFrozen(false);		

		// Move the current hot spot
		prim->movePoint(hotPoint, Ts[i]);

		// Propagation the deformation
		prim->isFrozen = true;

		propagator.execute();

		// Check if this is a candidate solution

		//		if ( satisfyBBConstraint() )
		{
			double stackability = activeOffset->getStackability(true);
			//			if (stackability > orgStackability + 0.1)
			{
				ShapeState state = ctrl()->getShapeState();
				//				if (isUnique(state, 0))
				{
					state.distortion = 0;
					state.deltaStackability = stackability - orgStackability;

					EditingSuggestion suggest;
					suggest.center = hotPoint;

					Vec3d t = hotPoint;
					t[2] = 0;
					if (Ts[i].x() < 0) t *= -1;
					t.normalize();
					t *= 0.15;

					suggest.direction = t;
					suggest.deltaS = state.deltaStackability;
					suggest.deltaV = state.distortion;
					suggest.side = freeHS.side;
					suggest.value = state.energy();

					state.trajectory = currentCandidate.trajectory;
					state.trajectory.push_back(suggest);

					if (isSuggesting)
					{
						suggestions.push_back(suggest);
						suggestSolutions.push(state);
					}
				}			

			}
		}

		// Restore the initial hot shape state
		ctrl()->setShapeState(initialHotShapeState);
	}


}

void StackabilityImprover::deformNearHotspot( HotSpot& freeHS, HotSpot& fixedHS )
{
	switch (freeHS.type)
	{
	case POINT_HOTSPOT:
		deformNearPointHotspot(freeHS, fixedHS);
		break;
	case LINE_HOTSPOT:
		deformNearLineHotspot(freeHS, fixedHS);
		break;
	case RING_HOTSPOT:
		deformNearRingHotspot(freeHS);
		break;
	}
}

// Explorer local successors by modifying the shape near hotspots
void StackabilityImprover::localSearch()
{
	// Step 1: Detect hot spots
	activeOffset->computeOffsetOfShape();
	orgStackability = activeOffset->getStackability();
	activeOffset->detectHotspots();

	// Step 2: Deform the object driven by locally modifying the hot spots
	// Suppose all hotspot pairs have symmetry relationships (direct or indirect)
	// Modifying one pair of hotspot will be propagated to others naturally later on
	int hsid = 0;
	HotSpot& upperHS = activeOffset->upperHotSpots[hsid];
	HotSpot& lowerHS = activeOffset->lowerHotSpots[hsid];

	// Modify one hotspot while keeping the other fixed
	deformNearHotspot(upperHS, lowerHS);
	deformNearHotspot(lowerHS, upperHS);	
}

bool StackabilityImprover::satisfyBBConstraint()
{
	bool result = true;

	Vec3d preBB = constraint_bbmax - constraint_bbmin;
	activeObject()->computeBoundingBox();
	Vec3d currBB = activeObject()->bbmax - activeObject()->bbmin;

	Vec3d diff = preBB - currBB;

	// Current BB is within expanded BB (with tolerence)
	if ( diff[0] < 0 || diff[1] < 0 || diff[2] < 0 )
		result = false;

	// debug
	//std::cout << "-----------------------------------\n"
	//	<<"The preBB size: (" << preBB <<")\n";	
	//std::cout << "The currBB size:(" << currBB <<")\n";
	//std::cout << "BB-satisfying: " << result <<std::endl;

	return result;
}

bool StackabilityImprover::isUnique( ShapeState state, double threshold )
{
	// dissimilar to \solutions
	PQShapeShateLessDistortion solutionsCopy = solutions;
	while(!solutionsCopy.empty())
	{
		ShapeState ss = solutionsCopy.top();

		if (ctrl()->similarity(ss, state) < threshold)
			return false;

		solutionsCopy.pop();
	}

	// dissimilar to used candidate solutions
	foreach(ShapeState ss, usedCandidateSolutions)
	{
		if (ctrl()->similarity(ss, state) < threshold)
			return false;
	}

	// dissimilar to candidate solutions
	PQShapeShateLessEnergy candSolutionsCopy = candidateSolutions;
	while(!candSolutionsCopy.empty())
	{
		ShapeState ss = candSolutionsCopy.top();

		if (ctrl()->similarity(ss, state) < threshold)
			return false;

		candSolutionsCopy.pop();
	}


	//std::cout << "Unique: " << result <<std::endl;

	return true;
}

void StackabilityImprover::showSolution( int i )
{
	if (solutions.empty())
	{
		std::cout << "There is no solution.\n";
		return;
	}

	int id = i % solutions.size();
	Controller* ctrl = (Controller*)activeObject()->ptr["controller"];

	PQShapeShateLessDistortion solutionsCopy = solutions;
	for (int i=0;i<id;i++)
		solutionsCopy.pop();

	ShapeState sln = solutionsCopy.top();
	ctrl->setShapeState(sln);

	std::cout << "Histrory length: " << sln.history.size() << std::endl; 
	//std::cout << "Showing the " << id << "th solution out of " << solutions.size() <<".\n";
}

// Suggestions
QVector<EditingSuggestion> StackabilityImprover::getSuggestions()
{
	// Clear
	suggestions.clear();
	suggestSolutions = PQShapeShateLessEnergy();
	solutions = PQShapeShateLessDistortion();

	// Current candidate
	currentCandidate = ctrl()->getShapeState();
	currentCandidate.deltaStackability = activeOffset->getStackability() - orgStackability;
	currentCandidate.distortion = ctrl()->getDistortion();


	// The bounding box constraint is hard
	constraint_bbmin = activeObject()->bbmin * BB_TOLERANCE;
	constraint_bbmax = activeObject()->bbmax * BB_TOLERANCE;

	isSuggesting = true;
	localSearch();
	isSuggesting = false;

	normalizeSuggestions();
	return suggestions;
}

void StackabilityImprover::normalizeSuggestions()
{
	if (suggestions.isEmpty())
		return;

	if (suggestions.size() == 1)
	{
		suggestions.first().value = 1;
		return;
	}


	// Normalize \deltaS and \deltaV respectively
	bool computeValue = false;
	if (computeValue)
	{
		double minS = DBL_MAX;
		double maxS = DBL_MIN;
		double minV = DBL_MAX;
		double maxV = DBL_MIN;

		foreach(EditingSuggestion sg, suggestions)
		{
			double s = sg.deltaS;
			double v = sg.deltaV;

			minS = Min(minS, s);
			maxS = Max(maxS, s);
			minV = Min(minV, v);
			maxV = Max(maxV, v);
		}

		double rangeS = maxS - minS;
		double rangeV = maxV - minV;

		bool zeroRangeS = abs(rangeS) < 1e-10;
		bool zeroRangeV = abs(rangeV) < 1e-10;
		for(int i = 0; i < suggestions.size(); i++)
		{
			double s = zeroRangeS? 1 : (suggestions[i].deltaS - minS) / rangeS;
			double v = zeroRangeV? 1 : (suggestions[i].deltaV - minV) / rangeV;

			double alpha = 0.7;
			suggestions[i].value = alpha * s - (1-alpha)*v;

		}
	}



	// Normalize \value
	double minValue = DBL_MAX;
	double maxValue = DBL_MIN;

	foreach(EditingSuggestion sg, suggestions)
	{
		double s = sg.value;

		minValue = Min(minValue, s);
		maxValue = Max(maxValue, s);

	}

	double range = maxValue - minValue;
	bool zeroRang = abs(range) < 1e-10;
	for(int i = 0; i < suggestions.size(); i++)
	{
		suggestions[i].value = zeroRang? 1 : (suggestions[i].value - minValue) / range;
	}

	//Output
	std::cout << "There are " << suggestions.size() << " suggestions (from offset):\n";
	for (int i=0;i<suggestions.size();i++)
	{
		std::cout << " deltaS = " << suggestions[i].deltaS 
			<< "\tdeltaV = " << suggestions[i].deltaV
			<< "\tvalue = " << suggestions[i].value << std::endl;
	}
}

void StackabilityImprover::showSuggestion( int i )
{
	if (suggestSolutions.empty())
	{
		std::cout << "There is no suggestion.\n";
		return;
	}

	int id = i % suggestSolutions.size();

	PQShapeShateLessEnergy suggestSolutionsCopy = suggestSolutions;
	for (int i=0;i<id;i++)
		suggestSolutionsCopy.pop();

	Controller* ctrl = (Controller*)activeObject()->ptr["controller"];

	ctrl->setShapeState(suggestSolutionsCopy.top());
}


QSegMesh* StackabilityImprover::activeObject()
{
	return activeOffset->activeObject();
}

Controller* StackabilityImprover::ctrl()
{
	return (Controller*)activeObject()->ptr["controller"];
}


// === Main access
void StackabilityImprover::executeImprove()
{
	// Clear
	candidateSolutions = PQShapeShateLessEnergy();
	solutions = PQShapeShateLessDistortion();
	usedCandidateSolutions.clear();

	// The bounding box constraint is hard
	constraint_bbmin = activeObject()->bbmin * BB_TOLERANCE;
	constraint_bbmax = activeObject()->bbmax * BB_TOLERANCE;

	// Push the current shape as the initial candidate solution
	ShapeState state = ctrl()->getShapeState();
	state.deltaStackability = activeOffset->getStackability() - orgStackability;
	state.distortion = ctrl()->getDistortion();
	state.history.push_back(state);
	candidateSolutions.push(state);

	while(!candidateSolutions.empty())
	{
		currentCandidate = candidateSolutions.top();
		candidateSolutions.pop();
		usedCandidateSolutions.push_back(currentCandidate);

		ctrl()->setShapeState(currentCandidate);

		// Explorer all local successors
		localSearch();

		std::cout << "#Candidates = " << candidateSolutions.size() << std::endl;
		std::cout << "#Solutions = " << solutions.size() << std::endl;

		if (solutions.size() >= NUM_EXPECTED_SOLUTION)
		{
			std::cout << NUM_EXPECTED_SOLUTION << " solutions have been found." << std::endl;
			break;
		}

	}

	//// Debug: add candidate solutions to solutions
	//while (!candidateSolutions.empty())
	//{
	//	solutions.push( candidateSolutions.top() );
	//	candidateSolutions.pop();
	//}

	ctrl()->setShapeState(state);
	activeObject()->computeBoundingBox();
}