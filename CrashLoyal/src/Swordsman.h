#pragma once

#include "Mob.h"

class Swordsman : public Mob
{
public:
	Swordsman(const Point& pos, bool attackingNorth) 
		: Mob(pos, attackingNorth) 
	{ 
		health = GetMaxHealth(); 
	}

	virtual int GetMaxHealth() const { return 10; }
	virtual float GetSpeed() const { return 2.f; }
	virtual float GetSize() const { return 1.f; }
	virtual int GetDamage() const { return 2; }
	virtual float GetAttackTime() const { return 200; }
};