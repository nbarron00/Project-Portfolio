#include "Actor.h"
#include "StudentWorld.h"
#include <cmath>


// Students:  Add code to this file, Actor.h, StudentWorld.h, and StudentWorld.cpp

Actor::Actor(int id, double x, double y, StudentWorld* world, int depth, Direction start_dir)
	:GraphObject::GraphObject(id, x, y, depth, start_dir), m_alive(true)
{
	this->m_world = world;
}

Wall::Wall(double x, double y, StudentWorld* world, int depth, Direction dir)
	:Actor::Actor(IID_WALL, x, y, world, depth, dir)
{	
	this->setDirection(right);	//set direction of wall to right
}

void Wall::doSomething()
{
	//do nothing, it's a wall
}

Agent::Agent(int id, double x, double y, StudentWorld* world, int depth, Direction dir)
	: Actor::Actor(id, x, y, world, depth, dir)
{
	this->setDirection(right);		//both citizens and penelope start out facing right
}

bool Agent::moveToIfPossible(double distance, Direction d)
{
	double agent_x = this->getX();
	double agent_y = this->getY();

	this->setDirection(d);
	switch (d)
	{
	case(up): agent_y += distance; break;
	case(left): agent_x -= distance; break;
	case(right): agent_x += distance; break;
	case(down): agent_y -= distance; break;
	default:
		return false;		//invalid direction, no moement
	}

	if (this->worldPtr()->collides(agent_x, agent_y, this)) { return false; }
	else
	{
		this->moveTo(agent_x, agent_y);
	}
	return true;
}

Human::Human(int id, double x, double y, StudentWorld* world, int depth, Direction dir)
	:Agent::Agent(id, x, y, world, depth, dir), m_infected(false), m_infectionCount(0)
{}

Penelope::Penelope(double x, double y, StudentWorld* world, int depth, Direction dir)
	: n_landmines(0), n_flamethr(0), n_vacc(0), Human::Human(IID_PLAYER, x, y, world, depth, dir)
{}

void Penelope::doSomething()
{
	if (!this->isAlive())		//if penelope is dead, do nothing
	{
		return;
	}

	if (this->worldPtr()->collides_vomit(this->getX(), this->getY()))		//if penelope collides with vomit, infect penelope
	{
		this->infect();
	}

	if (this->isInfected())					//if penelope is  infected, increment count
	{										//if count reaches 500, kill penelope
		this->incrementInfCount();
		if (this->infectionCount() == 500)
		{
			this->kill();
			this->worldPtr()->playSound(SOUND_PLAYER_DIE);
			return;
		}
	}

	int key_pressed;
	double actor_x;
	double actor_y;
	bool buttonPressed = this->worldPtr()->getKey(key_pressed);

	if (buttonPressed)
	{
		actor_x = this->getX();
		actor_y = this->getY();

		switch (key_pressed)
		{
		case(KEY_PRESS_SPACE):
			if (this->flameCount() > 0)
			{
				this->n_flamethr--;
				this->worldPtr()->playSound(SOUND_PLAYER_FIRE);

				double f_x = this->getX();
				double f_y = this->getY();
				switch (this->getDirection())
				{
				case(up):
					for (int i = 1; i <= 3; i++)
					{
						if (!this->worldPtr()->collidesWallOrExit(f_x, f_y + (i * SPRITE_HEIGHT)))
						{
							this->worldPtr()->addActor(new Flame(f_x, f_y + (i * SPRITE_HEIGHT), up, this->worldPtr()));
						}
						else
						{
							break;	//ensures that fire cannot be shot through walls
						}
					}
					break;
				case(down):
					for (int i = 1; i <= 3; i++)
					{
						if (!this->worldPtr()->collidesWallOrExit(f_x, f_y - (i * SPRITE_HEIGHT)))
						{
							this->worldPtr()->addActor(new Flame(f_x, f_y - (i * SPRITE_HEIGHT), down, this->worldPtr()));
						}
						else
						{
							break;
						}
					}
					break;
				case(right):
					for (int i = 1; i <= 3; i++)
					{
						if (!this->worldPtr()->collidesWallOrExit(f_x + (i * SPRITE_WIDTH), f_y))
						{
							this->worldPtr()->addActor(new Flame(f_x + (i * SPRITE_WIDTH), f_y, right, this->worldPtr()));
						}
						else
						{
							break;
						}
					}
					break;
				case(left):
					for (int i = 1; i <= 3; i++)
					{
						if (!this->worldPtr()->collidesWallOrExit(f_x - (i * SPRITE_WIDTH), f_y))
						{
							this->worldPtr()->addActor(new Flame(f_x - (i * SPRITE_WIDTH), f_y, left, this->worldPtr()));
						}
						else
						{
							break;
						}
					}
					break;
				}
			}
			break;
		case(KEY_PRESS_TAB):
			if (this->n_landmines > 0)
			{
				this->n_landmines--;
				this->worldPtr()->addActor(new Landmine(this->getX(), this->getY(), this->worldPtr()));
			}
			break;
		case(KEY_PRESS_ENTER):
			if (this->n_vacc > 0)
			{
				this->n_vacc--;
				this->cure();
			}
			break;
		case KEY_PRESS_LEFT:
			this->moveToIfPossible(4, left);
			break;
			
		case KEY_PRESS_UP:
			this->moveToIfPossible(4, up);
			break;

		case KEY_PRESS_RIGHT:
			this->moveToIfPossible(4, right);
			break;

		case KEY_PRESS_DOWN:
			this->moveToIfPossible(4, down);
			break;
		}
	}
}

Citizen::Citizen(double x, double y, StudentWorld* world, int depth, Direction dir)
	:Human::Human(IID_CITIZEN, x, y, world, depth, dir), m_thinking(true)
{
}

void Citizen::doSomething()
{
	this->m_thinking = !(this->m_thinking);

	if (!this->isAlive())
	{
		return;
	}

	if (this->worldPtr()->collides_vomit(this->getX(), this->getY()))		//if citizen collides with vomit, infect the citizen
	{
		if (!this->isInfected())
		{
			this->worldPtr()->playSound(SOUND_CITIZEN_INFECTED);
		}
		this->infect();
	}

	if (this->isInfected())
	{
		this->incrementInfCount();				//if citizen is infected, increment citizen infection count
		if (this->infectionCount() == 500)
		{
			this->kill();									//if infection count reaches 500, kill the citizen 
			this->worldPtr()->playSound(SOUND_ZOMBIE_BORN);
			this->worldPtr()->increaseScore(-1000);

			if (randInt(0, 9) < 3)			//30% chance new smart zombie is introduced, 70% a dumb zombie is introduced
			{
				this->worldPtr()->addActor(new SmartZombie(this->getX(), this->getY(), this->worldPtr()));
			}
			else
			{
				this->worldPtr()->addActor(new DumbZombie(this->getX(), this->getY(), this->worldPtr()));
			}
			return;
		}
	}
	
	if (this->m_thinking)		//skips every other tick
	{
		return;
	}
	
	double dist_p = sqrt(pow((this->getX() - this->worldPtr()->getPenelopeX()), 2) + pow((this->getY() - this->worldPtr()->getPenelopeY()), 2));
	double dist_z;
	this->worldPtr()->nearestZombie(this->getX(), this->getY(), dist_z);

	if ((dist_z == -1 || dist_p < dist_z) && dist_p <= 80)	//if no zombies present, or person is closer than closest zombie:
	{
		if (this->getX() == this->worldPtr()->getPenelopeX())
		{
			if (this->getY() < this->worldPtr()->getPenelopeY())
			{
				this->moveToIfPossible(2, up);
			}

			else
			{
				this->moveToIfPossible(2, down);
			}
			
			return;
		}

		else if (this->getY() == this->worldPtr()->getPenelopeY())
		{
			if (this->getX() < this->worldPtr()->getPenelopeX())
			{
				this->moveToIfPossible(2, right);
			}

			else
			{
				this->moveToIfPossible(2, left);
			}
			
			return;
		}

		else	//citizen not on same row or column as penelope
		{
			switch (randInt(0, 1))
			{
			case(0):	//horozontal movement
				if (this->getX() < this->worldPtr()->getPenelopeX())
				{
					if (this->moveToIfPossible(2, right)) { break; }
					else
					{
						if (this->getY() < this->worldPtr()->getPenelopeY())
						{
							if (!this->moveToIfPossible(2, up)) { return; }
						}

						else
						{
							if (!this->moveToIfPossible(2, down)) { return; }
						}
						break;
					}
				}
				else
				{
					if (this->moveToIfPossible(2, left)) { break; }
					else
					{
						if (this->getY() < this->worldPtr()->getPenelopeY())
						{
							if (!this->moveToIfPossible(2, up)) { return; }
						}

						else
						{
							if (!this->moveToIfPossible(2, down)) { return; }
						}
						break;
					}
				}
			
			case(1):	//vertical movement
				if (this->getY() < this->worldPtr()->getPenelopeY())
				{
					if (this->moveToIfPossible(2, up)) { break; }
					else
					{
						if (this->getX() < this->worldPtr()->getPenelopeX())
						{
							if (!this->moveToIfPossible(2, right)) { return; }
						}

						else
						{
							if (!this->moveToIfPossible(2, left)) { return; }
						}
						break;
					}
				}
				else
				{
					if (this->moveToIfPossible(2, down)) { break; }
					else
					{
						if (this->getX() < this->worldPtr()->getPenelopeX())
						{
							if (!this->moveToIfPossible(2, right)) { return; }
						}

						else
						{
							if (!this->moveToIfPossible(2, left)) { return; }
						}
						break;
					}
				}
			}
		}
	}

	double cit_x = this->getX();
	double cit_y = this->getY();
	double max_dist = dist_z;
	Direction move_this_way;

	if (dist_z <= 80)
	{
		if (!this->worldPtr()->collides(cit_x + 2, cit_y, this))
		{
			double dist_right;
			this->worldPtr()->nearestZombie(cit_x + 2, cit_y, dist_right);
			if (dist_right > max_dist)
			{
				max_dist = dist_right;
				move_this_way = right;
			}
		}
		if (!this->worldPtr()->collides(cit_x - 2, cit_y, this))
		{
			double dist_left;
			this->worldPtr()->nearestZombie(cit_x - 2, cit_y, dist_left);
			if (dist_left > max_dist)
			{
				max_dist = dist_left;
				move_this_way = left;
			}
		}
		if (!this->worldPtr()->collides(cit_x, cit_y + 2, this))
		{
			double dist_up;
			this->worldPtr()->nearestZombie(cit_x, cit_y + 2, dist_up);
			if (dist_up > max_dist)
			{
				max_dist = dist_up;
				move_this_way = up;
			}
		}
		if (!this->worldPtr()->collides(cit_x, cit_y - 2, this))
		{
			double dist_down;
			this->worldPtr()->nearestZombie(cit_x, cit_y - 2, dist_down);
			if (dist_down > max_dist)
			{
				max_dist = dist_down;
				move_this_way = down;
			}
		}

		if (max_dist > dist_z)
		{
			this->moveToIfPossible(2, move_this_way);
		}
	}

	return;
	//implement after implementing zombies
}

Zombie::Zombie(double x, double y, StudentWorld* world, int depth, Direction dir)
	:Agent::Agent(IID_ZOMBIE, x, y, world, depth, dir), m_movementDistPlan(0), m_paralyzed(true)
{}

bool Zombie::humanInFront()
{
	double x = this->getX();
	double y = this->getY();
	Direction d = this->getDirection();
	bool human_collides;
	switch (d)
	{
	case(up): 
		this->worldPtr()->collides_human(x, y + SPRITE_HEIGHT, human_collides);
		if (human_collides)
		{
			return true;
		}
		break;
	case(down):
		this->worldPtr()->collides_human(x, y - SPRITE_HEIGHT, human_collides);
		if (human_collides)
		{
			return true;
		}
		break;
	case(right):
		this->worldPtr()->collides_human(x + SPRITE_WIDTH, y, human_collides);
		if (human_collides)
		{
			return true;
		}
		break;
	case(left): 
		this->worldPtr()->collides_human(x - SPRITE_WIDTH, y, human_collides);
		if (human_collides)
		{
			return true;
		}
		break;
	}
	
	return false;
}

void Zombie::newMovementPlan()
{
	this->m_movementDistPlan = randInt(3, 10);
	switch (randInt(0, 3))
	{
	case(0): this->setDirection(up); break;
	case(1): this->setDirection(right); break;
	case(2): this->setDirection(down); break;
	case(3): this->setDirection(left); break;
	}
}

DumbZombie::DumbZombie(double x, double y, StudentWorld* world, int depth, Direction dir)
	:Zombie::Zombie(x, y, world, depth, dir)
{}

DumbZombie::~DumbZombie()
{
	Direction d[] = { up, down, left, right };
	int directions = 4;

	if (this->worldPtr()->overlapsAnything(this->getX(), this->getY() + SPRITE_HEIGHT))
	{
		Direction temp = d[directions - 1];
		d[directions - 1] = up;
		d[0] = temp;
		directions--;
	}

	if (this->worldPtr()->overlapsAnything(this->getX() - SPRITE_WIDTH, this->getY()))
	{
		Direction temp = d[directions - 1];
		d[directions - 1] = left;
		d[0] = temp;
		directions--;
	}

	if (this->worldPtr()->overlapsAnything(this->getX(), this->getY() - SPRITE_HEIGHT))
	{
		Direction temp = d[directions - 1];
		d[directions - 1] = down;
		d[0] = temp;
		directions--;
	}

	if (this->worldPtr()->overlapsAnything(this->getX() + SPRITE_WIDTH, this->getY()))
	{
		Direction temp = d[directions - 1];
		d[directions - 1] = right;
		d[0] = temp;
		directions--;
	}

	if (directions != 0)
	{
		if (randInt(0, 9) == 0)
		{
			switch (d[randInt(0, directions - 1)])
			{
			case(up): this->worldPtr()->addActor(new VaccineGoodie(this->getX(), this->getY() + SPRITE_HEIGHT, this->worldPtr())); break;
			case(down): this->worldPtr()->addActor(new VaccineGoodie(this->getX(), this->getY() - SPRITE_HEIGHT, this->worldPtr())); break;
			case(left): this->worldPtr()->addActor(new VaccineGoodie(this->getX() - SPRITE_WIDTH, this->getY(), this->worldPtr())); break;
			case(right): this->worldPtr()->addActor(new VaccineGoodie(this->getX() + SPRITE_WIDTH, this->getY(), this->worldPtr())); break;
			}
		}
	}

}

void DumbZombie::doSomething()
{
	if (!isAlive())
	{
		return;
	}
	
	if (this->isParalyzed())	//zombie is 'paralyzed' every other tick
	{
		return;
	}

	if (this->humanInFront())
	{
		double vomit_x = this->getX();
		double vomit_y = this->getY();
	
		switch (this->getDirection())
		{
		case(up): { vomit_y += SPRITE_HEIGHT; } break;
		case(down): { vomit_y -= SPRITE_HEIGHT; } break;
		case(left): { vomit_x -= SPRITE_WIDTH; } break;
		case(right): { vomit_x += SPRITE_WIDTH; } break;
		}

		double dist_to_nearest_human;
		double near_human_x;
		double near_human_y;
		this->worldPtr()->nearestHuman(vomit_x, vomit_y, dist_to_nearest_human, near_human_x, near_human_y);
		if (dist_to_nearest_human <= 10)
		{
			if (randInt(0, 2) == 0)
			{
				this->worldPtr()->addActor(new Vomit(vomit_x, vomit_y, this->getDirection(), this->worldPtr()));
				this->worldPtr()->playSound(SOUND_ZOMBIE_VOMIT);
				return;
			}
		}
	}

	if (this->movesLeft() == 0)
	{
		this->newMovementPlan();
	}

	if (this->moveToIfPossible(1, this->getDirection()))
	{
		this->decrementPlan();
		return;
	}

	this->endPlan();
	return;
}

SmartZombie::SmartZombie(double x, double y, StudentWorld* world, int depth, Direction dir)
	:Zombie::Zombie(x, y, world, depth, dir)
{}

void SmartZombie::doSomething()
{
	if (!isAlive())
	{
		return;
	}

	if (this->isParalyzed())	//zombie is 'paralyzed' every other tick
	{
		return;
	}

	if (this->humanInFront())
	{
		double vomit_x = this->getX();
		double vomit_y = this->getY();

		switch (this->getDirection())						//determine coordinates of the vomit
		{
		case(up): { vomit_y += SPRITE_HEIGHT; } break;
		case(down): { vomit_y -= SPRITE_HEIGHT; } break;
		case(left): { vomit_x -= SPRITE_WIDTH; } break;
		case(right): { vomit_x += SPRITE_WIDTH; } break;
		}

		double dist_to_near_hum;
		double human_x;
		double human_y;
		this->worldPtr()->nearestHuman(vomit_x, vomit_y, dist_to_near_hum, human_x, human_y);		
		if (dist_to_near_hum <= 10)												//if distance to nearest vomit is 10 or less, 1/3 chance to create vomit object
		{
			if (randInt(0, 2) == 0)
			{
				this->worldPtr()->addActor(new Vomit(vomit_x, vomit_y, this->getDirection(), this->worldPtr()));
				this->worldPtr()->playSound(SOUND_ZOMBIE_VOMIT);
				return;
			}
		}
	}

	if (this->movesLeft() == 0)			//if movememnt plan has ended, begin process of creating new movement plan
	{
		this->newMovementDist();
		double distance_to_human;
		double human_x;
		double human_y;
		this->worldPtr()->nearestHuman(this->getX(), this->getY(), distance_to_human, human_x, human_y);
		
		if (distance_to_human > 80)							//if nearest human is farther than 80 pixels away, pick a random direction
		{
			switch (randInt(0, 3))
			{
			case(0): this->setDirection(up); break;
			case(1): this->setDirection(right); break;
			case(2): this->setDirection(down); break;
			case(3): this->setDirection(left); break;
			}
		}

		else		//nearest human 80 pixels or less away
		{
			double zombie_x = this->getX();
			double zombie_y = this->getY();

			if (zombie_x == human_x)
			{
				if (zombie_x < human_x)
				{
					this->setDirection(right);
				}
				else
				{
					this->setDirection(left);
				}
			}

			else if (zombie_y == human_y)
			{
				if (zombie_y < human_y)
				{
					this->setDirection(up);
				}
				else
				{
					this->setDirection(down);
				}
			}
			else
			{
				Direction d_list[] = { up, right };
				if (zombie_y > human_y)
				{
					d_list[0] = down;
				}
				if (zombie_x > human_x)
				{
					d_list[0] = left;
				}

				this->setDirection(d_list[randInt(0, 1)]);
			}
		}
	}
	if (this->moveToIfPossible(1, this->getDirection()))
	{
		this->decrementPlan();
	}
	else
	{
		this->endPlan();
	}
	
}

ActivatingObject::ActivatingObject(int id, double x, double y, int depth, int dir, StudentWorld* world)
	:Actor::Actor(id, x, y, world, depth, dir)
{}

Exit::Exit(int x, int y, StudentWorld* world)
	:ActivatingObject::ActivatingObject(IID_EXIT, x, y, 1, right, world)
{}

void Exit::doSomething()
{
	bool collides;
	this->worldPtr()->collides_human(this->getX(), this->getY(), collides);
	if (collides)
	{
		this->worldPtr()->citizensExit(this->getX(), this->getY());
	}
	return;
}

Pit::Pit(double x, double y, StudentWorld* world)
	:ActivatingObject::ActivatingObject(IID_PIT, x, y, 0, right, world)
{}

void Pit::doSomething()
{
	this->worldPtr()->killIfCollides(this);
}

Flame::Flame(double x, double y, Direction d, StudentWorld* world)
	:ActivatingObject::ActivatingObject(IID_FLAME, x, y, 0, d, world), ticks_left(2)
{}

void Flame::doSomething()
{
	if (!this->isAlive())
	{
		return;
	}
	if (this->ticks_left-- == 0)
	{
		this->kill();
	}
	this->worldPtr()->killIfCollides(this);
}

Vomit::Vomit(double x, double y, Direction d, StudentWorld* world)
	:ActivatingObject::ActivatingObject(IID_VOMIT, x, y, 0, d, world), existanceCount(2)
{}

void Vomit::doSomething()
{
	if (!this->isAlive())
	{
		return;
	}

	if (this->existanceCount == 0)
	{
		this->kill();
	}
	this->existanceCount--;
	
	//infecting the human implemented in Citizen and Penelope classes

	return;
}

Landmine::Landmine(double x, double y, StudentWorld* world)
	:ActivatingObject::ActivatingObject(IID_LANDMINE, x, y, 1, right, world), m_active(false), m_safteyTicks(30)
{}

void Landmine::doSomething()
{
	if (!this->isAlive())
	{
		return;
	}
	if (!this->m_active)
	{
		this->m_safteyTicks--;
		if (this->m_safteyTicks == 0)
		{
			this->m_active = true;
		}
		return;
	}

	if (this->m_active)
	{
		this->worldPtr()->activateMineIfCollides(this);
	}
	return;
}

Goodie::Goodie(int id, double x, double y, int depth, int dir, StudentWorld* world)
	:ActivatingObject::ActivatingObject(id, x, y, depth, dir, world)
{}

VaccineGoodie::VaccineGoodie(double x, double y, StudentWorld* world)
	:Goodie::Goodie(IID_VACCINE_GOODIE, x, y, 1, right, world)
{}

void VaccineGoodie::doSomething()
{
	if (!this->isAlive())
	{
		return;
	}
	if (this->worldPtr()->overlaps(this, this->worldPtr()->getPenelopeX(), this->worldPtr()->getPenelopeY()))
	{
		this->kill();
		this->worldPtr()->playSound(SOUND_GOT_GOODIE);
		this->worldPtr()->givePenVacc();
		this->worldPtr()->increaseScore(50);
	}
}

GasCanGoodie::GasCanGoodie(double x, double y, StudentWorld* world)
	:Goodie::Goodie(IID_GAS_CAN_GOODIE, x, y, 1, right, world)
{}

void GasCanGoodie::doSomething()
{
	if (!this->isAlive())
	{
		return;
	}

	if (this->worldPtr()->overlaps(this, this->worldPtr()->getPenelopeX(), this->worldPtr()->getPenelopeY()))
	{
		this->kill();
		this->worldPtr()->playSound(SOUND_GOT_GOODIE);
		this->worldPtr()->givePenGas();
		this->worldPtr()->increaseScore(50);
	}
}

LandmineGoodie::LandmineGoodie(double x, double y, StudentWorld* world)
	:Goodie::Goodie(IID_LANDMINE_GOODIE, x, y, 1, right, world)
{}

void LandmineGoodie::doSomething()
{
	if (!this->isAlive())
	{
		return;
	}
	if (this->worldPtr()->overlaps(this, this->worldPtr()->getPenelopeX(), this->worldPtr()->getPenelopeY()))
	{
		this->kill();
		this->worldPtr()->playSound(SOUND_GOT_GOODIE);
		this->worldPtr()->givePenMines();
		this->worldPtr()->increaseScore(50);
	}
}