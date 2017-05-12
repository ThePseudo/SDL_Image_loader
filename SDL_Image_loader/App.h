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

enum Icons {
	fit_to_screen,
	unvalid
};

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
	void writeSettings(const int w, const int h);
	void readSettings(int *w, int *y);
	//utils
	string lowerString(string s);
	void getResourcePath(const string &subDir = "");

	//app
	bool quit = false;
	bool somethingChanged = true;
	static vector<string> supportedExtensions;
	string appPath;
	static App *app;

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
	SDL_Rect imageRect;
	SDL_Rect originalRect;
	SDL_Rect screenRect;
	int zoomFactor = 0;
	int resizeFactor;
	int minZoomFactor;
	int maxZoomFactor = 4;

	//various params
	int mousex, mousey;
	bool canChangeImage = true;
};