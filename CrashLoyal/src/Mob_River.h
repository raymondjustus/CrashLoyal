#pragma once

#include "Mob.h"

class Mob_River : public Mob
{
public:
	virtual int GetMaxHealth() const { return 1000000; }
	virtual float GetSpeed() const { return 0.0f; }
	virtual float GetSize() const { return 1.5f; }
	virtual float GetMass() const { return 50000.f; }
	virtual int GetDamage() const { return 0; }
	virtual int GetIsRiver() const { return true;  }
	virtual float GetAttackTime() const { return 2.5f; }
	const char* GetDisplayLetter() const { return "R"; }
};