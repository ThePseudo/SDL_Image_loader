#pragma once
#include <string>
#include <filesystem>
#include <list>
#include <SDL.h>
#include <SDL_image.h>

#ifdef _WIN32
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

using namespace std::experimental::filesystem;
using namespace std;

class App
{
private:
	App(const char *nomeFile);
public:
	~App();
	static App *getApp(const char *nomeFile = "");
	int run();


	//getters
	inline SDL_Renderer *getRenderer() { return rend; }
	inline SDL_Window *getWindow() { return win; }
	inline string getAppPath() { return appPath; }

private:
	void initSDL();
	void setDirectory(const char * nomeFile);
	void loadImage();
	void detectFiles();
	void centerToScreen();
	void fitBoundaries();
	//evend handling
	void onResize();
	void onMouseMove(const SDL_Event *e);
	void fitToScreen();
	void onMouseWheel(SDL_Event *e);
	void writeSettings(const bool reset);
	void readSettings();
	//utils
	string lowerString(string s);
	void getResourcePath(const string &subDir = "");

	//app
	static App *app;
	bool quit = false;
	bool somethingChanged = true;
	bool canChangeImage = true;
	int mousex, mousey;
	static vector<string> supportedExtensions;
	string appPath;

	//directory things
	string currentDir;
	string nomeFile;
	directory_iterator directory;
	list<string> files;
	list<string>::iterator curFile;

	//window and SDL things
	SDL_Window *win;
	SDL_Renderer *rend;
	SDL_Texture *image;
	SDL_DisplayMode displayMode;
	SDL_Rect imageRect;
	SDL_Rect originalRect;
	SDL_Rect screenRect;
	SDL_Rect oldRect;
	int zoomFactor = 0;
	int resizeFactor;
	int minZoomFactor;
	int maxZoomFactor = 4;
};