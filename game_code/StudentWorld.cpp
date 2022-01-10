#include "StudentWorld.h"
#include "GameConstants.h"
#include <string>
#include "Level.h"
#include "Actor.h"
#include <cmath>
#include <sstream>
#include <iomanip>

#include "GraphObject.h"

using namespace std;

GameWorld* createStudentWorld(string assetPath)
{
	return new StudentWorld(assetPath);
}

// Students:  Add code to this file, StudentWorld.h, Actor.h and Actor.cpp

StudentWorld::StudentWorld(string assetPath)
: GameWorld(assetPath), m_actorCount(0), level_complete(false)
{}

StudentWorld::~StudentWorld()
{
	this->cleanUp();
}

int StudentWorld::init()
{
	ostringstream level_sstream;
	if (this->getLevel() < 10)
	{
		level_sstream << '0' << this->getLevel();
	}
	else if (this->getLevel() < 100)
	{
		level_sstream << this->getLevel();
	}
	else		//level 100+
	{
		return GWSTATUS_PLAYER_WON;
	}

	string level_number = level_sstream.str();

	Level lev(assetPath());
	string level_file = "level" + level_number + ".txt";

	Level::LoadResult result = lev.loadLevel(level_file);

	switch (result)
	{
	case(Level::load_fail_file_not_found):
		return GWSTATUS_PLAYER_WON;
		break;
	case(Level::load_fail_bad_format):
		return GWSTATUS_LEVEL_ERROR;
		break;
	}
	
	Level::MazeEntry actor;

	for (int y = 0; y < 16; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			actor = lev.getContentsOf(x, y);
			switch (actor)
			{
			case(Level::wall): 
				this->addActor(new Wall(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, this));
				break;
			case(Level::player): 
				this->m_penelope = new Penelope(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, this); 
				break;
			case(Level::citizen):
				this->addActor(new Citizen(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, this));
				break;
			case(Level::dumb_zombie):
				this->addActor(new DumbZombie(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, this));
				break;
			case(Level::smart_zombie):
				this->addActor(new SmartZombie(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, this));
				break;
			case(Level::exit):
				this->addActor(new Exit(x * SPRITE_WIDTH, y * SPRITE_WIDTH, this));
				break;
			case(Level::vaccine_goodie):
				this->addActor(new VaccineGoodie(x * SPRITE_WIDTH, y * SPRITE_WIDTH, this));
				break;
			case(Level::gas_can_goodie):
				this->addActor(new GasCanGoodie(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, this));
				break;
			case(Level::landmine_goodie):
				this->addActor(new LandmineGoodie(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, this));
				break;
			case(Level::pit):
				this->addActor(new Pit(x * SPRITE_WIDTH, y * SPRITE_HEIGHT, this));
				break;
			}
		}
	}

    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
	if (!this->m_penelope->isAlive())
	{
		this->decLives();
		return GWSTATUS_PLAYER_DIED;
	}
	
	ostringstream score;
	int score_abs = abs(this->getScore());

	if (score_abs < 10)
	{
		score << " 0000" << score_abs;
	}
	else if (score_abs < 100)
	{
		score << " 000" << score_abs;
	}
	else if (score_abs < 1000)
	{
		score << " 00" << score_abs;
	}
	else if (score_abs < 10000)
	{
		score << " 0" << score_abs;
	}
	else if (score_abs < 100000)
	{
		score << " " << score_abs;
	}

	string game_score = score.str();
	if (this->getScore() < 0)
	{
		game_score[0] = '-';
	}
	else
	{
		game_score[0] = '0';
	}
	
	ostringstream gameText;
	gameText << "Score: " << game_score << "  Level : " << this->getLevel() << "  Lives : ";
	gameText << this->getLives() << "  Vacc : " << this->m_penelope->vaccCount() << "  Flames : ";
	gameText << this->m_penelope->flameCount() << " Mines : " << this->m_penelope->mineCount();
	gameText << "  Infected : " << this->m_penelope->infectionCount();

	string gameText_print = gameText.str();
	this->setGameStatText(gameText_print);

	this->m_penelope->doSomething();
	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] != nullptr)
		{
			this->m_actors[i]->doSomething();
		}
	}
	
	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] != nullptr && !this->m_actors[i]->isAlive())
		{
			delete this->m_actors[i];
			this->m_actors[i] = nullptr;
		}
	}
	

	if (this->level_complete)
	{
		this->playSound(SOUND_LEVEL_FINISHED);
		return GWSTATUS_FINISHED_LEVEL;
	}
    // This code is here merely to allow the game to build, run, and terminate after you hit enter.
    // Notice that the return value GWSTATUS_PLAYER_DIED will cause our framework to end the current level.
    //decLives();
	return GWSTATUS_CONTINUE_GAME;
    //return GWSTATUS_PLAYER_DIED;
}

void StudentWorld::cleanUp()
{
	if (this->m_penelope != nullptr)
	{
		delete this->m_penelope;
	}

	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] != nullptr)
		{
			delete this->m_actors[i];
		}
	}

	while(this->m_actors.size() != 0)
	{
		this->m_actors.pop_back();
	}
	this->m_actorCount = 0;
	this->level_complete = false;
}

bool StudentWorld::collidesWallOrExit(double f_x, double f_y)
{
	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] == nullptr)
		{
			continue;
		}
		if (abs(m_actors[i]->getX() - f_x) < SPRITE_WIDTH && abs(m_actors[i]->getY() - f_y) < SPRITE_HEIGHT && (m_actors[i]->isWall() || m_actors[i]->isExit()))
		{
			return true;
		}
	}
	return false;
}

void StudentWorld::collides_human(double x, double y, bool &collides)
{
	double other_x;
	double other_y;
	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] == nullptr)
		{
			continue;
		}
		other_x = m_actors[i]->getX();
		other_y = m_actors[i]->getY();

		if ((abs(x - other_x) < SPRITE_WIDTH) && (abs(y - other_y) < SPRITE_HEIGHT) && m_actors[i]->isHuman())
		{
			collides = true;
			return;
		}
	}

	if (this->m_penelope != nullptr)
	{
		other_x = this->m_penelope->getX();
		other_y = this->m_penelope->getY();
		if ((abs(x - other_x) < SPRITE_WIDTH) && (abs(y - other_y) < SPRITE_HEIGHT))
		{
			collides = true;
			return;
		}
	}
	collides = false;
	return;
}

bool StudentWorld::collides(double x, double y, Actor* self)
{
	double other_x;
	double other_y;
	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] == nullptr || this->m_actors[i] == self)
		{
			continue;
		}
		other_x = m_actors[i]->getX();
		other_y = m_actors[i]->getY();
		
		if ((abs(x - other_x) < SPRITE_WIDTH) && (abs(y - other_y) < SPRITE_HEIGHT) && m_actors[i]->blocksMovement())
		{
			return true;
		}
	}

	if (self != this->m_penelope && abs(x - this->m_penelope->getX()) < SPRITE_WIDTH && abs(y - this->m_penelope->getY()) < SPRITE_HEIGHT)
	{
		return true;
	}

	return false;
}

void StudentWorld::citizensExit(double exit_x, double exit_y)
{
	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] != nullptr && this->m_actors[i]->isHuman() && this->overlaps(this->m_actors[i], exit_x, exit_y))
		{
			m_actors[i]->kill();
			this->playSound(SOUND_CITIZEN_SAVED);
			this->increaseScore(500);
		}
	}

	if (!(this->citizensLeft()) && this->overlaps(this->m_penelope, exit_x, exit_y))
	{
		this->completeLevel();
	}

}

void StudentWorld::nearestZombie(double x, double y, double& distance)
{
	double low_distance = 1000000000;
	double temp_distance;

	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] != nullptr && this->m_actors[i]->isZombie())
		{
			temp_distance = sqrt(pow(x - m_actors[i]->getX(), 2) + pow(y - m_actors[i]->getY(), 2));
			if (temp_distance < low_distance)
			{
				low_distance = temp_distance;
			}
		}
	}

	distance = low_distance;
	return;
}

void StudentWorld::nearestHuman(double x, double y, double &distance, double& hum_x, double& hum_y) const
{
	double min_distance = 1000000000;
	double min_x = 1000000000;
	double min_y = 1000000000;
	double temp_distance;

	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] != nullptr && this->m_actors[i]->isHuman())
		{
			temp_distance = sqrt(pow(x - m_actors[i]->getX(), 2) + pow(y - m_actors[i]->getY(), 2));
			if (temp_distance < min_distance)
			{
				min_distance = temp_distance;
				min_x = this->m_actors[i]->getX();
				min_y = this->m_actors[i]->getY();
			}
		}
	}

	if (this->m_penelope != nullptr)
	{
		temp_distance = sqrt(pow((this->m_penelope->getX() - x), 2) + pow((this->m_penelope->getY() - y), 2));
		if (temp_distance < min_distance)
		{
			min_distance = temp_distance;
			min_x = this->m_penelope->getX();
			min_y = this->m_penelope->getY();
		}
	}
	distance = min_distance;
	hum_x = min_x;
	hum_y = min_y;
	return;
}

void StudentWorld::addActor(Actor* a)
{
	m_actors.push_back(a);
	this->m_actorCount++;
}

bool StudentWorld::citizensLeft()
{
	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] != nullptr && this->m_actors[i]->isHuman() && this->m_actors[i]->isAlive())
		{
			return true;
		}
	}
	return false;
}

bool StudentWorld::collides_vomit(double h_x, double h_y)
{
	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] != nullptr && this->m_actors[i]->isVomit() && this->overlaps(m_actors[i], h_x, h_y))
		{
			return true;
		}
	}
	return false;
}

bool StudentWorld::overlaps(Actor* a, double x, double y) const
{
	if (abs(a->getX() - x) < SPRITE_WIDTH && abs(a->getY() - y) < SPRITE_HEIGHT && a != nullptr)
	{
		return true;
	}
	return false;
}

double StudentWorld::getPenelopeX()
{
	return this->m_penelope->getX();
}

double StudentWorld::getPenelopeY()
{
	return this->m_penelope->getY();
}

void StudentWorld::givePenVacc()
{
	this->m_penelope->addVacc();
}

void StudentWorld::givePenGas()
{
	this->m_penelope->addFlame();
}

void StudentWorld::givePenMines()
{
	this->m_penelope->addMines();
}

void StudentWorld::activateMineIfCollides(Landmine* m)
{
	double m_x = m->getX();
	double m_y = m->getY();
	int actorCount = this->m_actorCount;
	for (int i = 0; i < actorCount; i++)
	{
		if (this->m_actors[i] == nullptr)
		{
			continue;
		}
		if (this->overlaps(m_actors[i], m_x, m_y) && (this->m_actors[i]->isHuman() || this->m_actors[i]->isZombie() || this->m_actors[i]->isFlame()))
		{
			this->m_actors[i]->kill();
			this->playSound(SOUND_LANDMINE_EXPLODE);
			double lm_top = m_y + SPRITE_HEIGHT;
			double lm_left = m_x - SPRITE_WIDTH;
			for (int row = 0; row < 3; row++)
			{
				for (int col = 0; col < 3; col++)
				{
					if (!this->collidesWallOrExit(lm_top - (row * SPRITE_HEIGHT), lm_left + (col * SPRITE_WIDTH)))
					{
						this->addActor(new Flame(lm_left + (col * SPRITE_WIDTH), lm_top - (row * SPRITE_HEIGHT), GraphObject::up, this));
					}
				}
			}

			m->kill();
			this->addActor(new Pit(m->getX(), m->getY(), this));
		}
	}

	if (this->overlaps(this->m_penelope, m_x, m_y))
	{
		this->m_penelope->kill();
		this->playSound(SOUND_LANDMINE_EXPLODE);
		double lm_top = m_y + SPRITE_HEIGHT;
		double lm_left = m_x - SPRITE_WIDTH;
		for (int row = 0; row < 3; row++)
		{
			for (int col = 0; col < 3; col++)
			{
				if (!this->collidesWallOrExit(lm_top - (row * SPRITE_HEIGHT), lm_left + (col * SPRITE_WIDTH)))
				{
					this->addActor(new Flame(lm_left + (col * SPRITE_WIDTH), lm_top - (row * SPRITE_HEIGHT), GraphObject::up, this));
				}
			}
		}

		m->kill();
		this->addActor(new Pit(m->getX(), m->getY(), this));
	}
	return;
}

void StudentWorld::killIfCollides(ActivatingObject* f)	//use for flames and pits
{
	double f_x = f->getX();
	double f_y = f->getY();

	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] == nullptr)
		{
			continue;
		}
		if (this->overlaps(m_actors[i], f_x, f_y) && this->m_actors[i]->isAlive() && (m_actors[i]->isHuman() || m_actors[i]->isZombie() || m_actors[i]->isGoodie()))
		{
			m_actors[i]->kill();
			if (m_actors[i]->isZombie() && !(m_actors[i]->isSmartZombie())) { this->increaseScore(1000); }
			if (m_actors[i]->isHuman()) { this->increaseScore(-1000); this->playSound(SOUND_CITIZEN_DIE); }
			if (m_actors[i]->isSmartZombie()) { this->increaseScore(2000); }
		}
	}

	if (this->overlaps(this->m_penelope, f_x, f_y))
	{
		this->m_penelope->kill();
	}
}

bool StudentWorld::overlapsAnything(double x, double y)
{
	for (int i = 0; i < this->m_actorCount; i++)
	{
		if (this->m_actors[i] != nullptr && this->m_actors[i]->isAlive() &&  abs(m_actors[i]->getX() - x) < SPRITE_WIDTH && abs(m_actors[i]->getY() - y) < SPRITE_HEIGHT)
		{
			return true;
		}
	}

	if (abs(this->m_penelope->getX() - x) < SPRITE_WIDTH && abs(this->m_penelope->getY() - y) < SPRITE_HEIGHT)
	{
		return true;
	}

	return false;
}


