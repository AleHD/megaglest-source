// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "ai.h"
#include "ai_interface.h"
#include "ai_rule.h"
#include "unit_type.h"
#include "unit.h"
#include "map.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
// 	class ProduceTask
// =====================================================

ProduceTask::ProduceTask(UnitClass unitClass){
	taskClass= tcProduce;
	this->unitClass= unitClass;
	unitType= NULL;
	resourceType= NULL;
}

ProduceTask::ProduceTask(const UnitType *unitType){
	taskClass= tcProduce;
	this->unitType= unitType;
	resourceType= NULL;
}

ProduceTask::ProduceTask(const ResourceType *resourceType){
	taskClass= tcProduce;
	unitType= NULL;
	this->resourceType= resourceType;
}

string ProduceTask::toString() const{
	string str= "Produce ";
	if(unitType!=NULL){
		str+= unitType->getName();
	}
	return str;
}

// =====================================================
// 	class BuildTask
// =====================================================

BuildTask::BuildTask(const UnitType *unitType){
	taskClass= tcBuild;
	this->unitType= unitType;
	resourceType= NULL;
	forcePos= false;
}

BuildTask::BuildTask(const ResourceType *resourceType){
	taskClass= tcBuild;
	unitType= NULL;
	this->resourceType= resourceType;
	forcePos= false;
}

BuildTask::BuildTask(const UnitType *unitType, const Vec2i &pos){
	taskClass= tcBuild;
	this->unitType= unitType;
	resourceType= NULL;
	forcePos= true;
	this->pos= pos;
}

string BuildTask::toString() const{
	string str= "Build ";
	if(unitType!=NULL){
		str+= unitType->getName();
	}
	return str;
}

// =====================================================
// 	class UpgradeTask
// =====================================================

UpgradeTask::UpgradeTask(const UpgradeType *upgradeType){
	taskClass= tcUpgrade;
	this->upgradeType= upgradeType;
}

string UpgradeTask::toString() const{
	string str= "Build ";
	if(upgradeType!=NULL){
		str+= upgradeType->getName();
	}
	return str;
}

// =====================================================
// 	class Ai
// =====================================================

void Ai::init(AiInterface *aiInterface, int useStartLocation) {
	this->aiInterface= aiInterface;
	if(useStartLocation == -1) {
		startLoc = random.randRange(0, aiInterface->getMapMaxPlayers()-1);
	}
	else {
		startLoc = useStartLocation;
	}
	minWarriors= minMinWarriors;
	randomMinWarriorsReached= false;
	//add ai rules
	aiRules.clear();
	aiRules.push_back(new AiRuleWorkerHarvest(this));
	aiRules.push_back(new AiRuleRefreshHarvester(this));
	aiRules.push_back(new AiRuleScoutPatrol(this));
	aiRules.push_back(new AiRuleUnBlock(this));
	aiRules.push_back(new AiRuleReturnBase(this));
	aiRules.push_back(new AiRuleMassiveAttack(this));
	aiRules.push_back(new AiRuleAddTasks(this));
	aiRules.push_back(new AiRuleProduceResourceProducer(this));
	aiRules.push_back(new AiRuleBuildOneFarm(this));
	aiRules.push_back(new AiRuleProduce(this));
	aiRules.push_back(new AiRuleBuild(this));
	aiRules.push_back(new AiRuleUpgrade(this));
	aiRules.push_back(new AiRuleExpand(this));
	aiRules.push_back(new AiRuleRepair(this));
	aiRules.push_back(new AiRuleRepair(this));
}

Ai::~Ai() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] deleting AI aiInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,aiInterface);
	deleteValues(tasks.begin(), tasks.end());
	tasks.clear();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] deleting AI aiInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,aiInterface);

	deleteValues(aiRules.begin(), aiRules.end());
	aiRules.clear();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] deleting AI aiInterface [%p]\n",__FILE__,__FUNCTION__,__LINE__,aiInterface);

	aiInterface = NULL;
}

void Ai::update() {

	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(aiInterface->getMyFaction()->getFirstSwitchTeamVote() != NULL) {
		const SwitchTeamVote *vote = aiInterface->getMyFaction()->getFirstSwitchTeamVote();
		aiInterface->getMyFaction()->setCurrentSwitchTeamVoteFactionIndex(vote->factionIndex);

		factionSwitchTeamRequestCount[vote->factionIndex]++;
		int factionSwitchTeamRequestCountCurrent = factionSwitchTeamRequestCount[vote->factionIndex];

		//int allowJoinTeam = random.randRange(0, 100);
		srand(time(NULL) + aiInterface->getMyFaction()->getIndex());
		int allowJoinTeam = rand() % 100;

		SwitchTeamVote *voteResult = aiInterface->getMyFaction()->getSwitchTeamVote(vote->factionIndex);
		voteResult->voted = true;
		voteResult->allowSwitchTeam = false;

		const GameSettings *settings =  aiInterface->getWorld()->getGameSettings();
		// Can only ask the AI player 2 times max per game
		if(factionSwitchTeamRequestCountCurrent <= 2) {
			// x% chance the AI will answer yes
			if(settings->getAiAcceptSwitchTeamPercentChance() >= 100) {
				voteResult->allowSwitchTeam = true;
			}
			else if(settings->getAiAcceptSwitchTeamPercentChance() <= 0) {
				voteResult->allowSwitchTeam = false;
			}
			else {
				voteResult->allowSwitchTeam = (allowJoinTeam >= (100 - settings->getAiAcceptSwitchTeamPercentChance()));
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] AI for faction# %d voted %s [%d] factionSwitchTeamRequestCountCurrent [%d] settings->getAiAcceptSwitchTeamPercentChance() [%d]\n",__FILE__,__FUNCTION__,__LINE__,aiInterface->getMyFaction()->getIndex(),(voteResult->allowSwitchTeam ? "Yes" : "No"),allowJoinTeam,factionSwitchTeamRequestCountCurrent,settings->getAiAcceptSwitchTeamPercentChance());
		//printf("AI for faction# %d voted %s [%d] factionSwitchTeamRequestCountCurrent [%d]\n",aiInterface->getMyFaction()->getIndex(),(voteResult->allowSwitchTeam ? "Yes" : "No"),allowJoinTeam,factionSwitchTeamRequestCountCurrent);

		aiInterface->giveCommandSwitchTeamVote(aiInterface->getMyFaction(),voteResult);
	}

	//process ai rules
	for(int ruleIdx = 0; ruleIdx < aiRules.size(); ++ruleIdx) {
		AiRule *rule = aiRules[ruleIdx];
		if(rule == NULL) {
			throw runtime_error("rule == NULL");
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx);

		if((aiInterface->getTimer() % (rule->getTestInterval() * GameConstants::updateFps / 1000)) == 0) {

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d, before rule->test()]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx);

			//printf("???? Testing AI Faction # %d RULE Name[%s]\n",aiInterface->getFactionIndex(),rule->getName().c_str());

			if(rule->test()) {
				if(outputAIBehaviourToConsole()) printf("\n\nYYYYY Executing AI Faction # %d RULE Name[%s]\n\n",aiInterface->getFactionIndex(),rule->getName().c_str());

				aiInterface->printLog(3, intToStr(1000 * aiInterface->getTimer() / GameConstants::updateFps) + ": Executing rule: " + rule->getName() + '\n');

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d, before rule->execute() [%s]]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx,rule->getName().c_str());

				rule->execute();

				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [ruleIdx = %d, after rule->execute() [%s]]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),ruleIdx,rule->getName().c_str());
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [END]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}


// ==================== state requests ====================

int Ai::getCountOfType(const UnitType *ut){
    int count= 0;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(ut == aiInterface->getMyUnit(i)->getType()){
			count++;
		}
	}
    return count;
}

int Ai::getCountOfClass(UnitClass uc,UnitClass *additionalUnitClassToExcludeFromCount) {
    int count= 0;
    for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		if(aiInterface->getMyUnit(i)->getType()->isOfClass(uc)) {
			// Skip unit if it ALSO contains the exclusion unit class type
			if(additionalUnitClassToExcludeFromCount != NULL) {
				if(aiInterface->getMyUnit(i)->getType()->isOfClass(*additionalUnitClassToExcludeFromCount)) {
					continue;
				}
			}
            ++count;
		}
    }
    return count;
}

float Ai::getRatioOfClass(UnitClass uc,UnitClass *additionalUnitClassToExcludeFromCount) {
	if(aiInterface->getMyUnitCount() == 0) {
		return 0;
	}
	else {
		return static_cast<float>(getCountOfClass(uc,additionalUnitClassToExcludeFromCount)) / aiInterface->getMyUnitCount();
	}
}

const ResourceType *Ai::getNeededResource(int unitIndex) {
	int amount = INT_MAX;
	const ResourceType *neededResource= NULL;
    const TechTree *tt= aiInterface->getTechTree();
    const Unit *unit = aiInterface->getMyUnit(unitIndex);

    for(int i = 0; i < tt->getResourceTypeCount(); ++i) {
        const ResourceType *rt= tt->getResourceType(i);
        const Resource *r= aiInterface->getResource(rt);

		if( rt->getClass() != rcStatic && rt->getClass() != rcConsumable &&
			r->getAmount() < amount) {

			// Now MAKE SURE the unit has a harvest command for this resource
			// AND that the resource is within eye-sight to avoid units
			// standing around doing nothing.
			const HarvestCommandType *hct= unit->getType()->getFirstHarvestCommand(rt,unit->getFaction());

			Vec2i resPos;
			if(hct != NULL && aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), resPos, false)) {
				amount= r->getAmount();
				neededResource= rt;
			}
        }
    }
    return neededResource;
}

bool Ai::beingAttacked(Vec2i &pos, Field &field, int radius){

	const Unit *enemy = aiInterface->getFirstOnSightEnemyUnit(pos, field, radius);
	return (enemy != NULL);
/*
    int count= aiInterface->onSightUnitCount();
    const Unit *unit;

    for(int i=0; i<count; ++i){
        unit= aiInterface->getOnSightUnit(i);
        if(!aiInterface->isAlly(unit) && unit->isAlive()){
            pos= unit->getPos();
			field= unit->getCurrField();
            if(pos.dist(aiInterface->getHomeLocation())<radius){
                aiInterface->printLog(2, "Being attacked at pos "+intToStr(pos.x)+","+intToStr(pos.y)+"\n");
                return true;
            }
        }
    }
    return false;
*/
}

bool Ai::isStableBase() {
	UnitClass ucWorkerType = ucWorker;
    if(getCountOfClass(ucWarrior,&ucWorkerType) > minWarriors) {
        aiInterface->printLog(4, "Base is stable\n");
        return true;
    }
    else{
        aiInterface->printLog(4, "Base is not stable\n");
        return false;
    }
}

bool Ai::findAbleUnit(int *unitIndex, CommandClass ability, bool idleOnly){
	vector<int> units;

	*unitIndex= -1;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *unit= aiInterface->getMyUnit(i);
		if(unit->getType()->hasCommandClass(ability)){
			if(!idleOnly || !unit->anyCommand() || unit->getCurrCommand()->getCommandType()->getClass()==ccStop){
				units.push_back(i);
			}
		}
	}

	if(units.empty()){
		return false;
	}
	else{
		*unitIndex= units[random.randRange(0, units.size()-1)];
		return true;
	}
}

bool Ai::findAbleUnit(int *unitIndex, CommandClass ability, CommandClass currentCommand){
	vector<int> units;

	*unitIndex= -1;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *unit= aiInterface->getMyUnit(i);
		if(unit->getType()->hasCommandClass(ability)){
			if(unit->anyCommand() && unit->getCurrCommand()->getCommandType()->getClass()==currentCommand){
				units.push_back(i);
			}
		}
	}

	if(units.empty()){
		return false;
	}
	else{
		*unitIndex= units[random.randRange(0, units.size()-1)];
		return true;
	}
}

bool Ai::findPosForBuilding(const UnitType* building, const Vec2i &searchPos, Vec2i &outPos){

	const int spacing= 1;

    for(int currRadius = 0; currRadius < maxBuildRadius; ++currRadius) {
        for(int i=searchPos.x - currRadius; i < searchPos.x + currRadius; ++i) {
            for(int j=searchPos.y - currRadius; j < searchPos.y + currRadius; ++j) {
                outPos= Vec2i(i, j);
                if(aiInterface->isFreeCells(outPos - Vec2i(spacing), building->getSize() + spacing * 2, fLand)) {
               		return true;
                }
            }
        }
    }

    return false;

}


// ==================== tasks ====================

void Ai::addTask(const Task *task){
	tasks.push_back(task);
	aiInterface->printLog(2, "Task added: " + task->toString());
}

void Ai::addPriorityTask(const Task *task){
	deleteValues(tasks.begin(), tasks.end());
	tasks.clear();

	tasks.push_back(task);
	aiInterface->printLog(2, "Priority Task added: " + task->toString());
}

bool Ai::anyTask(){
	return !tasks.empty();
}

const Task *Ai::getTask() const{
	if(tasks.empty()){
		return NULL;
	}
	else{
		return tasks.front();
	}
}

void Ai::removeTask(const Task *task){
	aiInterface->printLog(2, "Task removed: " + task->toString());
	tasks.remove(task);
	delete task;
}

void Ai::retryTask(const Task *task){
	tasks.remove(task);
	tasks.push_back(task);
}
// ==================== expansions ====================

void Ai::addExpansion(const Vec2i &pos){

	//check if there is a nearby expansion
	for(Positions::iterator it= expansionPositions.begin(); it!=expansionPositions.end(); ++it){
		if((*it).dist(pos)<villageRadius){
			return;
		}
	}

	//add expansion
	expansionPositions.push_front(pos);

	//remove expansion if queue is list is full
	if(expansionPositions.size()>maxExpansions){
		expansionPositions.pop_back();
	}
}

Vec2i Ai::getRandomHomePosition() {

	if(expansionPositions.empty() || random.randRange(0, 1) == 0){
		return aiInterface->getHomeLocation();
	}

	return expansionPositions[random.randRange(0, expansionPositions.size()-1)];
}

// ==================== actions ====================

void Ai::sendScoutPatrol(){
	Vec2i pos;
	int unit;
	bool possibleTargetFound= false;

	if((aiInterface->getControlType() == ctCpuMega || aiInterface->getControlType() == ctNetworkCpuMega)
	        && random.randRange(0, 1) == 1)
			{
		Map *map= aiInterface->getMap();

		const TechTree *tt= aiInterface->getTechTree();
		const ResourceType *rt= tt->getResourceType(0);
		int tryCount= 0;
		int height= map->getH();
		int width= map->getW();

		for(int i= 0; i < tt->getResourceTypeCount(); ++i){
			const ResourceType *rt_= tt->getResourceType(i);
			//const Resource *r= aiInterface->getResource(rt);

			if(rt_->getClass() == rcTech){
				rt=rt_;
				break;
			}
		}
		//printf("looking for resource %s\n",rt->getName().c_str());
		while(possibleTargetFound == false){
			tryCount++;
			if(tryCount == 4){
				//printf("no target found\n");
				break;
			}
			pos= Vec2i(random.randRange(2, width - 2), random.randRange(2, height - 2));
			if(map->isInside(pos) && map->isInsideSurface(map->toSurfCoords(pos))){
				//printf("is inside map\n");
				// find first resource in this area
				Vec2i resPos;
				if(aiInterface->isResourceInRegion(pos, rt, resPos, 20)){
					// found a possible target.
					pos= resPos;
					//printf("lets try the new target\n");
					possibleTargetFound= true;
					break;
				}
			}
			//else printf("is outside map\n");
		}
	}
	if(possibleTargetFound == false){
		startLoc= (startLoc + 1) % aiInterface->getMapMaxPlayers();
		pos= aiInterface->getStartLocation(startLoc);
		//printf("normal target used\n");
	}

	if(aiInterface->getHomeLocation() != pos){
		if(findAbleUnit(&unit, ccAttack, false)){
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			aiInterface->giveCommand(unit, ccAttack, pos);
			aiInterface->printLog(2, "Scout patrol sent to: " + intToStr(pos.x) + "," + intToStr(pos.y) + "\n");
		}
	}
}

void Ai::massiveAttack(const Vec2i &pos, Field field, bool ultraAttack){
	const int minWorkerAttackersHarvesting = 3;

	int producerWarriorCount=0;
	int maxProducerWarriors=random.randRange(1,11);
	int unitCount = aiInterface->getMyUnitCount();
	int unitGroupCommandId = -1;

	int attackerWorkersHarvestingCount = 0;
    for(int i = 0; i < unitCount; ++i) {
    	bool isWarrior=false;
    	bool productionInProgress=false;
        const Unit *unit= aiInterface->getMyUnit(i);
		const AttackCommandType *act= unit->getType()->getFirstAttackCommand(field);

		if(	aiInterface->getControlType() == ctCpuMega ||
			aiInterface->getControlType() == ctNetworkCpuMega) {
			if(producerWarriorCount > maxProducerWarriors) {
				if(
					unit->getCommandSize()>0 &&
					unit->getCurrCommand()->getCommandType()!=NULL && (
				    unit->getCurrCommand()->getCommandType()->getClass()==ccBuild ||
					unit->getCurrCommand()->getCommandType()->getClass()==ccMorph ||
					unit->getCurrCommand()->getCommandType()->getClass()==ccProduce
					)
				 ) {
					productionInProgress=true;
					isWarrior=false;
					producerWarriorCount++;
				}
				else {
					isWarrior =! unit->getType()->hasCommandClass(ccHarvest);
				}

			}
			else {
				isWarrior= !unit->getType()->hasCommandClass(ccHarvest) && !unit->getType()->hasCommandClass(ccProduce);
			}
		}
		else {
			isWarrior= !unit->getType()->hasCommandClass(ccHarvest) && !unit->getType()->hasCommandClass(ccProduce);
		}

		bool alreadyAttacking= (unit->getCurrSkill()->getClass() == scAttack);

		bool unitSignalledToAttack = false;
		if(alreadyAttacking == false && unit->getType()->hasSkillClass(scAttack) && (aiInterface->getControlType()
		        == ctCpuUltra || aiInterface->getControlType() == ctCpuMega || aiInterface->getControlType()
		        == ctNetworkCpuUltra || aiInterface->getControlType() == ctNetworkCpuMega)){
			//printf("~~~~~~~~ Unit [%s - %d] checking if unit is being attacked\n",unit->getFullName().c_str(),unit->getId());

			std::pair<bool, Unit *> beingAttacked= aiInterface->getWorld()->getUnitUpdater()->unitBeingAttacked(unit);
			if(beingAttacked.first == true){
				Unit *enemy= beingAttacked.second;
				const AttackCommandType *act_forenemy= unit->getType()->getFirstAttackCommand(enemy->getCurrField());

				//printf("~~~~~~~~ Unit [%s - %d] attacked by enemy [%s - %d] act_forenemy [%p] enemy->getCurrField() = %d\n",unit->getFullName().c_str(),unit->getId(),enemy->getFullName().c_str(),enemy->getId(),act_forenemy,enemy->getCurrField());

				if(act_forenemy != NULL){
					bool shouldAttack= true;
					if(unit->getType()->hasSkillClass(scHarvest)){
						shouldAttack= (attackerWorkersHarvestingCount > minWorkerAttackersHarvesting);
						if(shouldAttack == false){
							attackerWorkersHarvestingCount++;
						}
					}
					if(shouldAttack) {
						if(unitGroupCommandId == -1) {
							unitGroupCommandId = aiInterface->getWorld()->getNextCommandGroupId();
						}

						aiInterface->giveCommand(i, act_forenemy, beingAttacked.second->getPos(), unitGroupCommandId);
						unitSignalledToAttack= true;
					}
				}
				else{
					const AttackStoppedCommandType *asct_forenemy= unit->getType()->getFirstAttackStoppedCommand(
					        enemy->getCurrField());
					//printf("~~~~~~~~ Unit [%s - %d] found enemy [%s - %d] asct_forenemy [%p] enemy->getCurrField() = %d\n",unit->getFullName().c_str(),unit->getId(),enemy->getFullName().c_str(),enemy->getId(),asct_forenemy,enemy->getCurrField());
					if(asct_forenemy != NULL){
						bool shouldAttack= true;
						if(unit->getType()->hasSkillClass(scHarvest)){
							shouldAttack= (attackerWorkersHarvestingCount > minWorkerAttackersHarvesting);
							if(shouldAttack == false){
								attackerWorkersHarvestingCount++;
							}
						}
						if(shouldAttack){
//								printf("~~~~~~~~ Unit [%s - %d] WILL AttackStoppedCommand [%s - %d]\n", unit->getFullName().c_str(),
//								        unit->getId(), enemy->getFullName().c_str(), enemy->getId());

							if(unitGroupCommandId == -1) {
								unitGroupCommandId = aiInterface->getWorld()->getNextCommandGroupId();
							}

							aiInterface->giveCommand(i, asct_forenemy, beingAttacked.second->getCenteredPos(), unitGroupCommandId);
							unitSignalledToAttack= true;
						}
					}
				}
			}
		}
		if(alreadyAttacking == false && act != NULL && (ultraAttack || isWarrior) &&
			unitSignalledToAttack == false) {
			bool shouldAttack = true;
			if(unit->getType()->hasSkillClass(scHarvest)) {
				shouldAttack = (attackerWorkersHarvestingCount > minWorkerAttackersHarvesting);
				if(shouldAttack == false) {
					attackerWorkersHarvestingCount++;
				}
			}

			// Mega CPU does not send ( far away ) units which are currently producing something
			if(aiInterface->getControlType() == ctCpuMega || aiInterface->getControlType() == ctNetworkCpuMega){
				if(!isWarrior ){
					if(!productionInProgress){
						shouldAttack= false;
						//printf("no attack \n ");
					}
				}
			}
			if(shouldAttack) {
				if(unitGroupCommandId == -1) {
					unitGroupCommandId = aiInterface->getWorld()->getNextCommandGroupId();
				}

				aiInterface->giveCommand(i, act, pos, unitGroupCommandId);
			}
		}
    }

    if(	aiInterface->getControlType() == ctCpuEasy ||
    	aiInterface->getControlType() == ctNetworkCpuEasy) {
		minWarriors+= 1;
	}
	else if(aiInterface->getControlType() == ctCpuMega ||
			aiInterface->getControlType() == ctNetworkCpuMega) {
		minWarriors+= 3;
		if(minWarriors>maxMinWarriors-1 || randomMinWarriorsReached) {
			randomMinWarriorsReached=true;
			minWarriors=random.randRange(maxMinWarriors-10, maxMinWarriors*2);
		}
	}
	else if(minWarriors<maxMinWarriors) {
		minWarriors+= 3;
	}
	aiInterface->printLog(2, "Massive attack to pos: "+ intToStr(pos.x)+", "+intToStr(pos.y)+"\n");
}

void Ai::returnBase(int unitIndex) {
    Vec2i pos;
    CommandResult r;
    aiInterface->getFactionIndex();
    pos= Vec2i(
		random.randRange(-villageRadius, villageRadius), random.randRange(-villageRadius, villageRadius)) +
		getRandomHomePosition();

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    r= aiInterface->giveCommand(unitIndex, ccMove, pos);

    //aiInterface->printLog(1, "Order return to base pos:" + intToStr(pos.x)+", "+intToStr(pos.y)+": "+rrToStr(r)+"\n");
}

void Ai::harvest(int unitIndex) {
	const ResourceType *rt= getNeededResource(unitIndex);
	if(rt != NULL) {
		const HarvestCommandType *hct= aiInterface->getMyUnit(unitIndex)->getType()->getFirstHarvestCommand(rt,aiInterface->getMyUnit(unitIndex)->getFaction());

		Vec2i resPos;
		if(hct != NULL && aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), resPos, false)) {
			resPos= resPos+Vec2i(random.randRange(-2, 2), random.randRange(-2, 2));
			aiInterface->giveCommand(unitIndex, hct, resPos, -1);
			//aiInterface->printLog(4, "Order harvest pos:" + intToStr(resPos.x)+", "+intToStr(resPos.y)+": "+rrToStr(r)+"\n");
		}
	}
}

bool Ai::haveBlockedUnits() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	int unitCount = aiInterface->getMyUnitCount();
	Map *map = aiInterface->getMap();
	//If there is no close store
	for(int j=0; j < unitCount; ++j) {
		const Unit *u= aiInterface->getMyUnit(j);
		const UnitType *ut= u->getType();

		// If this building is a store
		if(u->isAlive() && ut->isMobile() && u->getPath() != NULL && (u->getPath()->isBlocked() || u->getPath()->getBlockCount())) {
			Vec2i unitPos = u->getPos();

			//printf("#1 AI found blocked unit [%d - %s]\n",u->getId(),u->getFullName().c_str());

			int failureCount = 0;
			int cellCount = 0;

			for(int i = -1; i <= 1; ++i) {
				for(int j = -1; j <= 1; ++j) {
					Vec2i pos = unitPos + Vec2i(i, j);
					if(map->isInside(pos) && map->isInsideSurface(map->toSurfCoords(pos))) {
						if(pos != unitPos) {
							bool canUnitMoveToCell = map->aproxCanMove(u, unitPos, pos);
							if(canUnitMoveToCell == false) {
								failureCount++;
							}
							cellCount++;
						}
					}
				}
			}
			bool unitImmediatelyBlocked = (failureCount == cellCount);
			//printf("#1 unitImmediatelyBlocked = %d, failureCount = %d, cellCount = %d\n",unitImmediatelyBlocked,failureCount,cellCount);

			if(unitImmediatelyBlocked) {
				//printf("#1 AI unit IS BLOCKED [%d - %s]\n",u->getId(),u->getFullName().c_str());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
				return true;
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
	return false;
}

bool Ai::getAdjacentUnits(std::map<float, std::map<int, const Unit *> > &signalAdjacentUnits, const Unit *unit) {
	//printf("In getAdjacentUnits...\n");

	bool result = false;
	Map *map = aiInterface->getMap();
	Vec2i unitPos = unit->getPos();
	for(int i = -1; i <= 1; ++i) {
		for(int j = -1; j <= 1; ++j) {
			Vec2i pos = unitPos + Vec2i(i, j);
			if(map->isInside(pos) && map->isInsideSurface(map->toSurfCoords(pos))) {
				if(pos != unitPos) {
					Unit *adjacentUnit = map->getCell(pos)->getUnit(unit->getCurrField());
					if(adjacentUnit != NULL && adjacentUnit->getFactionIndex() == unit->getFactionIndex()) {
						if(adjacentUnit->getType()->isMobile() && adjacentUnit->getPath() != NULL) {
							//signalAdjacentUnits.push_back(adjacentUnit);
							float dist = unitPos.dist(adjacentUnit->getPos());

							std::map<float, std::map<int, const Unit *> >::iterator iterFind1 = signalAdjacentUnits.find(dist);
							if(iterFind1 == signalAdjacentUnits.end()) {
								signalAdjacentUnits[dist][adjacentUnit->getId()] = adjacentUnit;

								getAdjacentUnits(signalAdjacentUnits, adjacentUnit);
								result = true;
							}
							else {
								std::map<int, const Unit *>::iterator iterFind2 = iterFind1->second.find(adjacentUnit->getId());
								if(iterFind2 == iterFind1->second.end()) {
									signalAdjacentUnits[dist][adjacentUnit->getId()] = adjacentUnit;
									getAdjacentUnits(signalAdjacentUnits, adjacentUnit);
									result = true;
								}
							}
						}
					}
				}
			}
		}
	}
	return result;
}

void Ai::unblockUnits() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	int unitCount = aiInterface->getMyUnitCount();
	Map *map = aiInterface->getMap();
	// Find blocked units and move surrounding units out of the way
	std::map<float, std::map<int, const Unit *> > signalAdjacentUnits;
	for(int idx=0; idx < unitCount; ++idx) {
		const Unit *u= aiInterface->getMyUnit(idx);
		const UnitType *ut= u->getType();

		// If this building is a store
		if(u->isAlive() && ut->isMobile() && u->getPath() != NULL && (u->getPath()->isBlocked() || u->getPath()->getBlockCount())) {
			Vec2i unitPos = u->getPos();

			//printf("#2 AI found blocked unit [%d - %s]\n",u->getId(),u->getFullName().c_str());

			int failureCount = 0;
			int cellCount = 0;

			for(int i = -1; i <= 1; ++i) {
				for(int j = -1; j <= 1; ++j) {
					Vec2i pos = unitPos + Vec2i(i, j);
					if(map->isInside(pos) && map->isInsideSurface(map->toSurfCoords(pos))) {
						if(pos != unitPos) {
							bool canUnitMoveToCell = map->aproxCanMove(u, unitPos, pos);
							if(canUnitMoveToCell == false) {
								failureCount++;
								getAdjacentUnits(signalAdjacentUnits, u);
							}
							cellCount++;
						}
					}
				}
			}
			//bool unitImmediatelyBlocked = (failureCount == cellCount);
			//printf("#2 unitImmediatelyBlocked = %d, failureCount = %d, cellCount = %d, signalAdjacentUnits.size() = %d\n",unitImmediatelyBlocked,failureCount,cellCount,signalAdjacentUnits.size());
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

	if(signalAdjacentUnits.empty() == false) {
		//printf("#2 AI units ARE BLOCKED about to unblock\n");

		int unitGroupCommandId = -1;

		for(std::map<float, std::map<int, const Unit *> >::reverse_iterator iterMap = signalAdjacentUnits.rbegin();
			iterMap != signalAdjacentUnits.rend(); ++iterMap) {

			for(std::map<int, const Unit *>::iterator iterMap2 = iterMap->second.begin();
				iterMap2 != iterMap->second.end(); ++iterMap2) {
				//int idx = iterMap2->first;
				const Unit *adjacentUnit = iterMap2->second;
				if(adjacentUnit != NULL && adjacentUnit->getType()->getFirstCtOfClass(ccMove) != NULL) {
					const CommandType *ct = adjacentUnit->getType()->getFirstCtOfClass(ccMove);

					for(int moveAttempt = 1; moveAttempt <= villageRadius; ++moveAttempt) {
						Vec2i pos= Vec2i(
							random.randRange(-villageRadius*2, villageRadius*2), random.randRange(-villageRadius*2, villageRadius*2)) +
											 adjacentUnit->getPos();

						bool canUnitMoveToCell = map->aproxCanMove(adjacentUnit, adjacentUnit->getPos(), pos);
						if(canUnitMoveToCell == true) {

							if(ct != NULL) {
								if(unitGroupCommandId == -1) {
									unitGroupCommandId = aiInterface->getWorld()->getNextCommandGroupId();
								}

								CommandResult r = aiInterface->giveCommand(adjacentUnit,ct, pos, unitGroupCommandId);
							}
						}
					}
				}
			}
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] took msecs: %lld [START]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
}

bool Ai::outputAIBehaviourToConsole() const {
	return false;
}

}}//end namespace
