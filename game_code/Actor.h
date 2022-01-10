#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"
class StudentWorld;

// Students:  Add code to this file, Actor.cpp, StudentWorld.h, and StudentWorld.cpp

class Actor : public GraphObject
{
public:
	Actor(int id, double x , double y, StudentWorld* world, int depth = 0, Direction start_dir = right);
	virtual void doSomething() = 0;
	bool isAlive() { return this->m_alive; }
	void kill() { this->m_alive = false; }
	StudentWorld* worldPtr() { return this->m_world; }
	virtual bool blocksMovement() const { return false; }
	virtual bool isZombie() { return false; }
	virtual bool isSmartZombie() { return false; }
	virtual bool isHuman() { return false; }
	virtual bool isPenelope() { return false; }
	virtual bool isExit() { return false; }
	virtual bool isVomit() { return false; }
	virtual bool isWall() { return false; }
	virtual bool isMine() { return false; }
	virtual bool isGoodie() { return false; }
	virtual bool isFlame() { return false; }

private:
	StudentWorld* m_world;
	bool m_alive;
};

class Wall : public Actor
{
public:
	Wall(double x, double y, StudentWorld* world, int depth = 0, Direction dir = right);
	virtual void doSomething();
	virtual bool blocksMovement() const { return true; }
	virtual bool isWall() { return true; }
};

class Agent : public Actor
{
public:
	Agent(int id, double x, double y, StudentWorld* world, int depth, Direction dir);
	virtual void doSomething() = 0;
	virtual bool blocksMovement() const { return true; }

	bool moveToIfPossible(double distance, Direction d);

private:
	
};

class Human : public Agent
{
public:
	Human(int id, double x, double y, StudentWorld* world, int depth, Direction dir);
	bool isInfected() { return this->m_infected; }
	int infectionCount() { return this->m_infectionCount; }
	void incrementInfCount() { this->m_infectionCount++; }
	virtual bool isHuman() { return true; }
	void infect() { this->m_infected = true; }
	void cure() { this->m_infected = false; this->m_infectionCount = 0; }
private:
	bool m_infected;
	int m_infectionCount;
};

class Penelope : public Human
{
public:
	Penelope(double x, double y, StudentWorld* world, int depth = 0, Direction dir = right);
	virtual void doSomething();
	virtual bool isPenelope() { return true; }
	int vaccCount() { return this->n_vacc; }
	int flameCount() { return this->n_flamethr; }
	int mineCount() { return this->n_landmines; }
	void addVacc() { this->n_vacc++; }
	void addFlame() { this->n_flamethr += 5; }
	void addMines() { this->n_landmines += 2; }

private:
	int n_landmines;
	int n_flamethr;
	int n_vacc;
};

class Citizen : public Human
{
public:
	Citizen(double x, double y, StudentWorld* world, int depth = 0, Direction dir = right);
	virtual void doSomething();
private:
	bool m_thinking;
};

class Zombie : public Agent
{
public:
	Zombie(double x, double y, StudentWorld* world, int depth, Direction dir);
	virtual bool isZombie() { return true; }
	int movesLeft() { return this->m_movementDistPlan; }
	void newMovementPlan();
	void newMovementDist() { this->m_movementDistPlan = randInt(3, 10); }
	bool humanInFront();
	void decrementPlan() { this->m_movementDistPlan--; }
	void endPlan() { this->m_movementDistPlan = 0; }
	bool isParalyzed() { return (m_paralyzed = !(this->m_paralyzed)); }
	
private:
	int m_movementDistPlan;
	bool m_paralyzed;
};

class DumbZombie : public Zombie
{
public:
	DumbZombie(double x, double y, StudentWorld* world, int depth = 0, Direction dir = right);
	~DumbZombie();
	virtual void doSomething();
private:
};

class SmartZombie : public Zombie
{
public:
	SmartZombie(double x, double y, StudentWorld* world, int depth = 0, Direction dir = right);
	virtual void doSomething();
	virtual bool isSmartZombie() { return true; }
private:
};

class ActivatingObject : public Actor
{
public:
	ActivatingObject(int id, double x, double y, int depth, int dir, StudentWorld* world);
private:
};

class Exit : public ActivatingObject
{
public:
	Exit(int x, int y, StudentWorld* world);
	virtual void doSomething();
	virtual bool isExit() { return true; }

private:
};

class Pit : public ActivatingObject
{
public:
	Pit(double x, double y, StudentWorld* world);
	virtual void doSomething();
private:
};

class Flame : public ActivatingObject
{
public:
	Flame(double x, double y, Direction d, StudentWorld* world);
	virtual void doSomething();
	virtual bool isFlame() { return true; }
private:
	int ticks_left;
};

class Vomit : public ActivatingObject
{
public:
	Vomit(double x, double y, Direction d, StudentWorld* world);
	virtual void doSomething();
	virtual bool isVomit() { return true; }

private:
	int existanceCount;
};

class Landmine : public ActivatingObject
{
public:
	Landmine(double x, double y, StudentWorld* world);
	virtual void doSomething();
	virtual bool isMine() { return true; }
	
private:
	bool m_active;
	int m_safteyTicks;
};

class Goodie : public ActivatingObject
{
public:
	Goodie(int id, double x, double y, int depth, int dir, StudentWorld* world);
	virtual bool isGoodie() { return true; }
private:
};

class VaccineGoodie : public Goodie
{
public:
	VaccineGoodie(double x, double y, StudentWorld* world);
	virtual void doSomething();
private:
};

class GasCanGoodie : public Goodie
{
public:
	GasCanGoodie(double x, double y, StudentWorld* world);
	virtual void doSomething();
private:
};

class LandmineGoodie : public Goodie
{
public:
	LandmineGoodie(double x, double y, StudentWorld* world);
	virtual void doSomething();
private:
};



#endif // ACTOR_H_
