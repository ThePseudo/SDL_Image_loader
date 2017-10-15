#pragma once
#include <string>
#include <filesystem>
#include <list>
#include <SDL.h>
#include <SDL_image.h>

#ifdef _WIN32
#define SEPARATOR L'\\'
#else
#define SEPARATOR L'/'
#endif

using namespace std::experimental::filesystem;
using namespace std;

class App
{
private:
	App(const wchar_t *nomeFile);
public:
	~App();
	static App *getApp(const wchar_t *nomeFile = L"");
	int run();


	//getters
	inline SDL_Renderer *getRenderer() { return rend; }
	inline SDL_Window *getWindow() { return win; }
	inline wstring getAppPath() { return appPath; }

private:
	void initSDL();
	void setDirectory(const wchar_t * nomeFile);
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
	wstring lowerString(wstring s);
	void getResourcePath(const wstring &subDir = L"");

	//app
	static App *app;
	bool quit = false;
	bool somethingChanged = true;
	bool canChangeImage = true;
	int mousex = 0, mousey = 0;
	double angle = 0.0f;
	static vector<wstring> supportedExtensions;
	wstring appPath;

	//directory things
	wstring currentDir;
	wstring nomeFile;
	directory_iterator directory;
	list<wstring> files;
	list<wstring>::iterator curFile;

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