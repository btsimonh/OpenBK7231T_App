#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include "sim_local.h"

class CSimulator {
	// window
	CString projectPath;
	bool bMouseButtonStates[10];
	SDL_Window *Window;
	SDL_GLContext Context;
	u32 WindowFlags;
	bool Running;
	bool FullScreen;
	class Tool_Base *activeTool;
	class CSimulation *sim;
	class CursorManager *cur;
	class CSolver *solver;
	class CWinMenuBar *winMenu;
	class PrefabManager *prefabs;
	class CSaveLoad *saveLoad;
	class CProject *project;
	bool bSchematicModified;
	class CText *currentlyEditingText;

	void onKeyDown(int keyCode);
	void setTool(class Tool_Base *tb);
public:
	CSimulator();
	void drawWindow();
	void createWindow();
	void onUserClose();
	void showExitSaveMessageBox();
	void destroyObject(class CShape *s);
	void beginEditingText(class CText *txt);
	class CShape *getShapeUnderCursor();
	class CSimulation*getSim() {
		return sim;
	}
	class PrefabManager*getPfbs() {
		return prefabs;
	}
	class CursorManager*getCursorMgr() {
		return cur;
	}
	class CShape *allocByClassName(const char *className);
	bool isMouseButtonHold(int idx) const {
		return bMouseButtonStates[idx];
	}
	bool createSimulation(bool bDemo);
	bool loadSimulation(const char *s);
	bool saveSimulationAs(const char *s);
	bool saveSimulation();
	void saveOrShowSaveAsDialogIfNeeded();

	void markAsModified() {
		bSchematicModified = true;
	}
	bool hasProjectPath() const {
		if (projectPath.size() > 0)
			return true;
		return false;
	}
};

#endif