#include "Mob.h"

#include <memory>
#include <limits>
#include <stdlib.h>
#include <stdio.h>
#include "Building.h"
#include "Waypoint.h"
#include "GameState.h"
#include "Point.h"

int Mob::previousUUID;

Mob::Mob() 
	: pos(-10000.f,-10000.f)
	, nextWaypoint(NULL)
	, targetPosition(new Point)
	, state(MobState::Moving)
	, uuid(Mob::previousUUID + 1)
	, attackingNorth(true)
	, health(-1)
	, targetLocked(false)
	, target(NULL)
	, lastAttackTime(0)
	, isStruct(false)
	, whereGoing(targetPosition)
{
	Mob::previousUUID += 1;
}

void Mob::Init(const Point& pos, bool attackingNorth, bool isStruct)
{
	health = GetMaxHealth();
	this->pos = pos;
	this->attackingNorth = attackingNorth;
	this->isStruct = isStruct;
	findClosestWaypoint();
}

std::shared_ptr<Point> Mob::getPosition() {
	return std::make_shared<Point>(this->pos);
}

bool Mob::findClosestWaypoint() {
	std::shared_ptr<Waypoint> closestWP = GameState::waypoints[0];
	float smallestDist = std::numeric_limits<float>::max();

	for (std::shared_ptr<Waypoint> wp : GameState::waypoints) {
		//std::shared_ptr<Waypoint> wp = GameState::waypoints[i];
		// Filter out any waypoints that are "behind" us (behind is relative to attack dir
		// Remember y=0 is in the top left
		if (attackingNorth && wp->pos.y > this->pos.y) {
			continue;
		}
		else if ((!attackingNorth) && wp->pos.y < this->pos.y) {
			continue;
		}

		float dist = this->pos.dist(wp->pos);
		if (dist < smallestDist) {
			smallestDist = dist;
			closestWP = wp;
		}
	}
	std::shared_ptr<Point> newTarget = std::shared_ptr<Point>(new Point);
	this->targetPosition->x = closestWP->pos.x;
	this->targetPosition->y = closestWP->pos.y;
	this->nextWaypoint = closestWP;
	
	return true;
}

void Mob::moveTowards(std::shared_ptr<Point> moveTarget, double elapsedTime) {
	this->whereGoing = moveTarget;
	Point movementVector;
	movementVector.x = moveTarget->x - this->pos.x;
	movementVector.y = moveTarget->y - this->pos.y;
	movementVector.normalize();
	movementVector *= (float)this->GetSpeed();
	movementVector *= (float)elapsedTime;
	pos += movementVector;
}


void Mob::findNewTarget() {
	// Find a new valid target to move towards and update this mob
	// to start pathing towards it

	if (!findAndSetAttackableMob()) { findClosestWaypoint(); }
}

// Have this mob start aiming towards the provided target
// TODO: impliment true pathfinding here
void Mob::updateMoveTarget(std::shared_ptr<Point> target) {
	this->targetPosition->x = target->x;
	this->targetPosition->y = target->y;
}

void Mob::updateMoveTarget(Point target) {
	this->targetPosition->x = target.x;
	this->targetPosition->y = target.y;
}


// Movement related
//////////////////////////////////
// Combat related

int Mob::attack(int dmg) {
	this->health -= dmg;
	return health;
}

bool Mob::findAndSetAttackableMob() {
	// Find an attackable target that's in the same quardrant as this Mob
	// If a target is found, this function returns true
	// If a target is found then this Mob is updated to start attacking it
	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
		if (otherMob->attackingNorth == this->attackingNorth) { continue; }
		if (otherMob->isStruct) { 
			continue;
		}

		bool imLeft    = this->pos.x     < (SCREEN_WIDTH / 2);
		bool otherLeft = otherMob->pos.x < (SCREEN_WIDTH / 2);

		bool imTop    = this->pos.y     < (SCREEN_HEIGHT / 2);
		bool otherTop = otherMob->pos.y < (SCREEN_HEIGHT / 2);
		if ((imLeft == otherLeft) && (imTop == otherTop)) {
			// If we're in the same quardrant as the otherMob
			// Mark it as the new target
			this->setAttackTarget(otherMob);
			return true;
		}
	}
	return false;
}

// TODO Move this somewhere better like a utility class
int randomNumber(int minValue, int maxValue) {
	// Returns a random number between [min, max). Min is inclusive, max is not.
	return (rand() % maxValue) + minValue;
}

void Mob::setAttackTarget(std::shared_ptr<Attackable> newTarget) {
	this->state = MobState::Attacking;
	target = newTarget;
}

bool Mob::targetInRange() {
	float range = this->GetSize(); // TODO: change this for ranged units
	float totalSize = range + target->GetSize();
	return this->pos.insideOf(*(target->getPosition()), totalSize);
}
// Combat related
////////////////////////////////////////////////////////////
// Collisions

// PROJECT 3: 
//  1) return a vector of mobs that we're colliding with
//  2) handle collision with towers & river 
std::vector<std::shared_ptr<Mob>> Mob::checkCollision() {

	std::vector<std::shared_ptr<Mob>> colidingWith;

	//get *this* point
	std::shared_ptr<Point> posA = this->getPosition();
	float posAX = posA->x;
	float posAY = posA->y;
	float sizeA = this->GetSize();

	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
		// don't collide with yourself
		if (this->sameMob(otherMob)) { continue; }

		//Getting the other point
		std::shared_ptr<Point> posB = otherMob->getPosition();
		float posBX = posB->x;
		float posBY = posB->y;
		float sizeB = otherMob->GetSize();

		float sizeAvg = (sizeA + sizeB) / 2.0f;
		float xDif = float(abs(posAX - posBX));
		float yDif = float(abs(posAY - posBY));

		if ((xDif <= sizeAvg) && (yDif <= sizeAvg)) {
			colidingWith.push_back(otherMob);
		}
	}
	return colidingWith;
}

Point normalizeVector(Point movementVector, Mob* mob, double elapsedTime) {
	movementVector.normalize();
	movementVector *= (float)mob->GetSpeed();
	movementVector *= (float)elapsedTime;
	return movementVector;
}

void Mob::processCollision(std::shared_ptr<Mob> otherMob, double elapsedTime) {

	/*
	if (otherMob->GetIsStruct()) {
		std::cout << "COLLIDING";
	}
	*/

	//get *this* point
	std::shared_ptr<Point> posA = this->getPosition();
	float posAX = posA->x;
	float posAY = posA->y;
	float sizeA = this->GetSize();

	//Getting the other point
	std::shared_ptr<Point> posB = otherMob->getPosition();
	float posBX = posB->x;
	float posBY = posB->y;
	float sizeB = otherMob->GetSize();

	//Getting the masses
	float thisMass = this->GetMass();
	float otherMass = otherMob->GetMass();

	//Getting the movement directions
	bool thisGoingUp = this->whereGoing->y < this->pos.y;
	bool thisGoingLeft = this->whereGoing->x < this->pos.x;

	//----------

	// if the masses are the same
	if (thisMass == otherMass) {
		// moving in the same direction
		if (this->targetPosition->x == otherMob->targetPosition->x && this->targetPosition->y == otherMob->targetPosition->y) {
			//if we are going up
			if (thisGoingUp) {
				//if we are below the other, get shoved down (behind)
				if (this->pos.y >= otherMob->pos.y) {
					this->pos.y += this->GetSize() * 1.5f;
				}
			}
			//we are going down
			else {
				//if we are above the other, get shoved up (behind)
				if (this->pos.y <= otherMob->pos.y) {
					this->pos.y -= this->GetSize() * 1.5f;
				}
			}
		}
		// moving in opposite directions
		else {
			//we are going up
			if (thisGoingUp) {
				this->pos.y -= this->GetSize() / 4;
				// if our target is to the left, get shoved "out" to the right
				if (thisGoingLeft) {
					this->pos.x += this->GetSize() / 3;
					otherMob->pos.x -= this->GetSize() / 4;
				}
				//otherwise we are going right, and get shoved "out" to the left
				else {
					this->pos.x -= this->GetSize() / 3;
					otherMob->pos.x += this->GetSize() / 4;
				}
			}
			//we are going down
			else {
				this->pos.y += this->GetSize() / 4;
				//if our target is to the left, get shoved "out" to the left
				if (thisGoingLeft) {
					this->pos.x -= this->GetSize() / 3;
				}
				//otherwise we are going right, and get shoved our to the right
				else {
					this->pos.x += this->GetSize() / 3;
				}
			}
		}
	}

	//----------

	// if we have less mass than what we are colliding with
	if (thisMass < otherMass) {
		//if we are going up
		if (thisGoingUp) {
			//if we are below the other, get shoved down (behind)
			if (this->pos.y >= otherMob->pos.y) {
				std::cout << "A";
				this->pos.y += otherMob->GetSize() * 1.5f;
				if (thisGoingLeft) {
					std::cout << "B";
					this->pos.x -= this->GetSize() * 3;
				}
				else {
					std::cout << "C";
					this->pos.x += this->GetSize() * 3;
				}
			}
		}
		//we are going down
		else {
			//if we are above the other, get shoved up (behind)
			if (this->pos.y <= otherMob->pos.y) {
				std::cout << "D";
				this->pos.y -= otherMob->GetSize() * 1.5f;
				if (thisGoingLeft) {
					std::cout << "E";
					this->pos.x -= this->GetSize() * 3;
				}
				else {
					std::cout << "F";
					this->pos.x += this->GetSize() * 3;
				}
			}
		}
	}
}

// Collisions
///////////////////////////////////////////////
// Procedures

void Mob::attackProcedure(double elapsedTime) {
	std::vector<std::shared_ptr<Mob>> allCollisions = this->checkCollision();
	for (std::shared_ptr<Mob> otherMob : allCollisions) {
		this->processCollision(otherMob, elapsedTime);
	}

	if (this->target == nullptr || this->target->isDead()) {
		this->targetLocked = false;
		this->target = nullptr;
		this->state = MobState::Moving;
		return;
	}

	if (targetInRange()) {
		if (this->lastAttackTime >= this->GetAttackTime()) {
			// If our last attack was longer ago than our cooldown
			this->target->attack(this->GetDamage());
			this->lastAttackTime = 0; // lastAttackTime is incremented in the main update function
			return;
		}
	}
	else {
		// If the target is not in range
		moveTowards(target->getPosition(), elapsedTime);
	}
}

void Mob::moveProcedure(double elapsedTime) {
	if (targetPosition) {
		moveTowards(targetPosition, elapsedTime);

		// Check for collisions
		if (this->nextWaypoint->pos.insideOf(this->pos, (this->GetSize() + WAYPOINT_SIZE))) {
			std::shared_ptr<Waypoint> trueNextWP = this->attackingNorth ?
												   this->nextWaypoint->upNeighbor :
												   this->nextWaypoint->downNeighbor;
			setNewWaypoint(trueNextWP);
		}

		// PROJECT 3: You should not change this code very much, but this is where your 
		// collision code will be called from
		std::vector<std::shared_ptr<Mob>> allCollisions = this->checkCollision();
		for (std::shared_ptr<Mob> otherMob : allCollisions) {
			this->processCollision(otherMob, elapsedTime);
			//std::cout << "COLLIDING";
		}

		// Fighting otherMob takes priority always
		findAndSetAttackableMob();

	} else {
		// if targetPosition is nullptr
		findNewTarget();
	}
}

void Mob::update(double elapsedTime) {

	switch (this->state) {
	case MobState::Attacking:
		this->attackProcedure(elapsedTime);
		this->checkCollision();
		break;
	case MobState::Moving:
	default:
		this->moveProcedure(elapsedTime);
		break;
	}

	this->lastAttackTime += (float)elapsedTime;
}
