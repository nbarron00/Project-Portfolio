#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include <string>
#include <vector>
class Actor;
class Penelope;
class Landmine;
class ActivatingObject;

// Students:  Add code to this file, StudentWorld.cpp, Actor.h, and Actor.cpp

class StudentWorld : public GameWorld
{
public:
    StudentWorld(std::string assetPath);
	virtual ~StudentWorld();
    virtual int init();
    virtual int move();
    virtual void cleanUp();

	bool collidesWallOrExit(double f_x, double f_y);
	void collides_human(double x, double y, bool &a);
	bool collides(double x, double y, Actor* self);		//returns true if player collides with another actor that blocks movement
	void citizensExit(double exit_x, double exit_y);
	bool collides_vomit(double h_x, double h_y);
	void nearestZombie(double x, double y, double& distance);
	void nearestHuman(double x, double y, double& distance, double& human_x, double& human_y) const;
	void addActor(Actor* a);
	bool citizensLeft();
	void completeLevel() { this->level_complete = true; }
	double getPenelopeX();
	double getPenelopeY();
	bool overlaps(Actor* a, double x, double y) const;
	void givePenVacc();
	void givePenGas();
	void givePenMines();
	void activateMineIfCollides(Landmine* m);
	void killIfCollides(ActivatingObject* fp);
	bool overlapsAnything(double x, double y);
	

private:

	std::vector<Actor*> m_actors;	//vector holding pointers to all actors in a level
	int m_actorCount;
	bool level_complete;
	Penelope* m_penelope;
};

#endif // STUDENTWORLD_H_
