// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_load_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "network_message.h"
#include "auto_test.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateLoadGame
// =====================================================

MenuStateLoadGame::MenuStateLoadGame(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "root")
{
	containerName = "LoadGame";
	Lang &lang= Lang::getInstance();

	int buttonWidth = 120;
	int yPos=30;
	int xPos=20;
	int xSpacing=20;
	int slotsLineHeight=20;
	int slotLinesYBase=650;
	int slotsToRender=30;
	int slotWidth=200;

	lines[0].init(0,slotLinesYBase+slotsLineHeight);
	lines[1].init(0, slotLinesYBase-(slotsToRender-1)*slotsLineHeight-5);
	//lines[1].setHorizontal(false);

	headerLabel.registerGraphicComponent(containerName,"headerLabel");
	headerLabel.init(400, 730);
	headerLabel.setFont(CoreData::getInstance().getMenuFontBig());
	headerLabel.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	headerLabel.setText(lang.get("LoadGameMenu"));

	noSavedGamesLabel.registerGraphicComponent(containerName,"noSavedGamesLabel");
	noSavedGamesLabel.init(20, 400);
	noSavedGamesLabel.setFont(CoreData::getInstance().getMenuFontBig());
	noSavedGamesLabel.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	noSavedGamesLabel.setText(lang.get("NoSavedGames"));

	savedGamesLabel.registerGraphicComponent(containerName,"savedGamesLabel");
	savedGamesLabel.init(120, slotLinesYBase+30);
	savedGamesLabel.setFont(CoreData::getInstance().getMenuFontBig());
	savedGamesLabel.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	savedGamesLabel.setText(lang.get("SavedGames"));

	infoHeaderLabel.registerGraphicComponent(containerName,"infoHeaderLabel");
	infoHeaderLabel.init(650, slotLinesYBase+30);
	infoHeaderLabel.setFont(CoreData::getInstance().getMenuFontBig());
	infoHeaderLabel.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	infoHeaderLabel.setText(lang.get("SavegameInfo"));

	infoTextLabel.registerGraphicComponent(containerName,"infoTextLabel");
	infoTextLabel.init(600, 400);
//	infoTextLabel.setFont(CoreData::getInstance().getMenuFontBig());
//	infoTextLabel.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	infoTextLabel.setText("Info block for the current slot, maybe screenshot above \ntest\ntest2");

    abortButton.registerGraphicComponent(containerName,"abortButton");
    abortButton.init(xPos, yPos, buttonWidth);
	abortButton.setText(lang.get("Abort"));
	xPos+=buttonWidth+xSpacing;
    loadButton.registerGraphicComponent(containerName,"loadButton");
    loadButton.init(xPos, yPos, buttonWidth);
    loadButton.setText(lang.get("LoadGame"));
	xPos+=buttonWidth+xSpacing;
    deleteButton.registerGraphicComponent(containerName,"deleteButton");
    deleteButton.init(xPos, yPos, buttonWidth);
    deleteButton.setText(lang.get("Delete"));

    slotsScrollBar.init(500-20,slotLinesYBase-slotsLineHeight*(slotsToRender-1),false,slotWidth,20);
    slotsScrollBar.setLength(slotsLineHeight*slotsToRender);
    slotsScrollBar.setElementCount(0);
    slotsScrollBar.setVisibleSize(slotsToRender);
    slotsScrollBar.setVisibleStart(0);


	GraphicComponent::applyAllCustomProperties(containerName);
}

MenuStateLoadGame::~MenuStateLoadGame() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	clearSlots();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] END\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateLoadGame::clearSlots() {
	while(!slots.empty()) {
		delete slots.back();
		slots.pop_back();
	}
}


void MenuStateLoadGame::reloadUI() {
	Lang &lang= Lang::getInstance();

	abortButton.setText(lang.get("Abort"));
	deleteButton.setText(lang.get("Delete"));
	loadButton.setText(lang.get("LoadGame"));

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

void MenuStateLoadGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData=  CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

    if(abortButton.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
    else if(deleteButton.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
    else if(loadButton.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
	else if(slotsScrollBar.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
	}
    else {
    	if(slotsScrollBar.getElementCount()!=0){
    		for(int i = slotsScrollBar.getVisibleStart(); i <= slotsScrollBar.getVisibleEnd(); ++i) {
				if(slots[i]->mouseClick(x, y)) {
					break;
				}
    		}
    	}
    }
}

void MenuStateLoadGame::mouseMove(int x, int y, const MouseState *ms){

    abortButton.mouseMove(x, y);
    deleteButton.mouseMove(x, y);
    loadButton.mouseMove(x, y);
	if(slotsScrollBar.getElementCount()!=0){
		for(int i = slotsScrollBar.getVisibleStart(); i <= slotsScrollBar.getVisibleEnd(); ++i) {
			if(slots[i]->mouseMove(x, y)) {
				break;
			}
		}
	}
	slotsScrollBar.mouseMove(x,y);

}

void MenuStateLoadGame::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderLabel(&headerLabel);
	renderer.renderLabel(&savedGamesLabel);
	renderer.renderLabel(&infoHeaderLabel);
	renderer.renderLabel(&infoTextLabel);

	renderer.renderButton(&abortButton);
	renderer.renderButton(&deleteButton);
	renderer.renderButton(&loadButton);

    for(int i=0; i<sizeof(lines) / sizeof(lines[0]); ++i){
    	renderer.renderLine(&lines[i]);
    }

	if(slotsScrollBar.getElementCount()==0 ) {
		renderer.renderLabel(&noSavedGamesLabel);
	}
	else{
		for(int i = slotsScrollBar.getVisibleStart(); i <= slotsScrollBar.getVisibleEnd(); ++i) {
			renderer.renderButton(slots[i]);
		}
	}
	renderer.renderScrollBar(&slotsScrollBar);

	renderer.renderConsole(&console,false,true);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateLoadGame::update(){
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateNewGame(program, mainMenu);
	}
	console.update();
}

void MenuStateLoadGame::keyDown(SDL_KeyboardEvent key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	//if(key == configKeys.getCharKey("SaveGUILayout")) {
	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
		//Lang &lang= Lang::getInstance();
		//console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
	}
}

}}//end namespace