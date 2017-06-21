#include "App.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>

App *App::app = nullptr;

vector<string> App::supportedExtensions = {
	"png",
	"bmp",
	"jpg",
	"gif"
	"jpeg",
	"lbm",
	"pcx",
	"pnm",
	"tga",
	"tiff",
	"webp",
	"xcf",
	"xpm",
	"xv"
};

int main(int argc, char *argv[])
{
	if (argc < 2) return 0;
	return App::getApp(argv[1])->run();
}

App::App(const char *nomeFile)
{
	initSDL();
	setDirectory(nomeFile);
	loadImage();
}

App::~App()
{
	SDL_Quit();
}

App *App::getApp(const char *nomeFile)
{
	if (app == nullptr) {
		app = new App(nomeFile);
	}
	return App::app;
}

void App::initSDL()
{
	assert(SDL_Init(SDL_INIT_VIDEO) == 0 && SDL_GetError());
	int flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP;
	assert(IMG_Init(flags) & flags);
	assert(SDL_GetCurrentDisplayMode(0, &displayMode) == 0 && SDL_GetError());
	win = SDL_CreateWindow("Image viewer", 0, 0, 0, 0, SDL_WINDOW_RESIZABLE);
	readSettings();
	rend = SDL_CreateRenderer(win, 0, SDL_RENDERER_ACCELERATED);
}

void App::setDirectory(const char * nomeFile)
{
	string pathString(nomeFile);
	size_t i;
	for (i = pathString.length() - 1; i >= 0; --i) {
		if (pathString.at(i) == SEPARATOR) break;
	}
	this->nomeFile = pathString.substr(i+1, pathString.length());
	currentDir = pathString.substr(0, i);
	directory = directory_iterator(currentDir.c_str());
	detectFiles();
}

void App::loadImage()
{
	string title = "Image viewer - " + this->nomeFile;
	SDL_Surface loadedImage;
	string path = currentDir + SEPARATOR + nomeFile;
	string extension = path.substr(path.find_last_of('.'), path.length());
	if (image != nullptr) {
		SDL_DestroyTexture(image);
		cout << SDL_GetError();
	}
	if (lowerString(extension) == "bmp") {
		loadedImage = *SDL_LoadBMP(path.c_str());
	}
	else {
		loadedImage = *IMG_Load(path.c_str());
	}
	SDL_GetClipRect(&loadedImage, &imageRect);
	
	originalRect = imageRect;
	resizeFactor = originalRect.w / 10;

	image = SDL_CreateTextureFromSurface(rend, &loadedImage);
	SDL_FreeSurface(&loadedImage);
	SDL_SetWindowTitle(win, title.c_str());
	onResize();
	onMouseWheel(nullptr);
	somethingChanged = true;
}

void App::onResize()
{
	bool mustWrite = true;
	SDL_Surface *screenSurface = SDL_GetWindowSurface(win);
	oldRect = screenRect;
	SDL_GetClipRect(screenSurface, &screenRect);
	float proportion;
	for (int i = 0; i > -20; --i) {
		proportion = (float)(originalRect.w + (resizeFactor * i)) / (float)originalRect.w;
		minZoomFactor = i;
		if (screenRect.w > originalRect.w + (resizeFactor * i) &&
			screenRect.h > (int)((float)originalRect.h * proportion)) {
			break;
		}
	}
	zoomFactor = minZoomFactor;
	centerToScreen();
	writeSettings(false);
#ifdef _DEBUG
	cout << oldRect.w << " - " << oldRect.h << endl;
	cout << screenRect.w << " - " << screenRect.h << endl;
	cout << ((SDL_GetWindowFlags(win) & SDL_WINDOW_MAXIMIZED) ? "Maximized": "Windowed");
	cout << endl;
#endif // _DEBUG

	somethingChanged = true;
}

void App::onMouseMove(const SDL_Event *e)
{
	if (SDL_GetMouseState(0, 0) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		imageRect.x += e->motion.x - mousex;
		imageRect.y += e->motion.y - mousey;
		fitBoundaries();
		centerToScreen();
		somethingChanged = true;
	}
	SDL_GetMouseState(&mousex, &mousey);
}

void App::fitToScreen()
{
	if (imageRect.w <= screenRect.w && imageRect.h <= screenRect.h) {
		double ratiox, ratioy, minratio;
		ratiox = (double)(screenRect.w) / (double)(imageRect.w);
		ratioy = (double)(screenRect.h) / (double)(imageRect.h);
		minratio = (ratiox > ratioy) ? ratioy : ratiox;
		imageRect.w = (ratiox == minratio) ? screenRect.w : (int)((double)imageRect.w * (double)minratio);
		imageRect.h = (ratioy == minratio) ? screenRect.h : (int)((double)imageRect.h * (double)minratio);
		imageRect.x = (screenRect.w > imageRect.w) ? (int)((screenRect.w - imageRect.w) / 2.0f) : 0;
		imageRect.y = (screenRect.h > imageRect.h) ? (int)((screenRect.h - imageRect.h) / 2.0f) : 0;
	}
}

void App::onMouseWheel(SDL_Event *e)
{
	float proportion;
	if (e != nullptr) {
		switch (e->wheel.y) {
		case 1:
			++zoomFactor;
			break;
		case -1:
			--zoomFactor;
			break;
		}
		e->type = 0;
	}
	if (zoomFactor > maxZoomFactor) --zoomFactor;
	if (zoomFactor < minZoomFactor) ++zoomFactor;
	imageRect.w = originalRect.w + (resizeFactor * zoomFactor);
	proportion = (float)imageRect.w / (float)originalRect.w;
	imageRect.h = (int)((float)originalRect.h * proportion);
	if(e != nullptr) e->wheel.y = 0;

	fitBoundaries();
	centerToScreen();
	somethingChanged = true;
}

void App::writeSettings(bool reset)
{
	string settingsPath = this->appPath + "settings.ini";
	ofstream settingsO(settingsPath.c_str(), ios::out);
	if (reset) {
		settingsO << "800 600\nwindowed" << endl;
	}
	else {
		if ((SDL_GetWindowFlags(win) & SDL_WINDOW_MAXIMIZED) == 0) {
			settingsO << screenRect.w << " " << screenRect.h << endl << "windowed" << endl;
		}
		else {
			settingsO  << oldRect.w << " " << oldRect.h << endl << "maximized" << endl;
		}
	}
	settingsO.close();
}

void App::readSettings()
{
	int x = 800, y = 600;
	getResourcePath();
	string settingsPath = this->appPath + "settings.ini";
	ifstream settings(settingsPath.c_str(), ios::in);
	if (!settings) {
		writeSettings(true);
		settings.close();
		settings.open("settings.ini", ios::in);
	}
	string maximized;
	settings >> x >> y >> maximized;
	settings.close();

	SDL_SetWindowSize(win, x, y);
	SDL_SetWindowPosition(win, (displayMode.w - x) / 2, (displayMode.h - y) / 2);
	if (maximized == "maximized") {
		SDL_MaximizeWindow(win);
	}
}

void App::detectFiles()
{
	bool found = false;
	string temp1, temp2;
	for (auto i = directory; i != directory_iterator(); ++i) {
		if (!is_directory(i->path())) {
			temp1 = i->path().extension().string();
			for (auto iter = supportedExtensions.begin(); iter != supportedExtensions.end(); ++iter) {
				if (lowerString(i->path().extension().string()) == (string)("." + *iter)) {
					files.push_back(i->path().filename().string());
					break;
				}
			}
		}
	}
	for (curFile = files.begin(); curFile != files.end(); ++curFile) {
		if (*curFile == nomeFile) {
			found = true;
			break;
		}
	}
	assert(found);
}

void App::centerToScreen()
{
	if (imageRect.w - imageRect.x <= screenRect.w && imageRect.h - imageRect.h <= screenRect.h) {
		fitToScreen();
	}
	if (imageRect.w - imageRect.x < screenRect.w) {
		imageRect.x = (int)((float)(screenRect.w - imageRect.w) / 2.0f);
	}
	else if (imageRect.h - imageRect.y < screenRect.h) {
		imageRect.y = (int)((float)(screenRect.h - imageRect.h) / 2.0f);
	}
}

void App::fitBoundaries()
{
	if (imageRect.x > screenRect.x) {
		imageRect.x = screenRect.x;
	}
	if (imageRect.y > screenRect.y) {
		imageRect.y = screenRect.y;
	}
	if (imageRect.x + imageRect.w < screenRect.x + screenRect.w)
		imageRect.x = screenRect.x + screenRect.w - imageRect.w;
	if (imageRect.y + imageRect.h < screenRect.y + screenRect.h)
		imageRect.y = screenRect.y + screenRect.h - imageRect.h;
}

int App::run()
{
	SDL_Event e;
	while (!quit) {
		
		SDL_PollEvent(&e);
		switch (e.type) {
		case SDL_QUIT:
			quit = true;
			break;

		case SDL_KEYDOWN:
			switch(e.key.keysym.sym) {
			case SDLK_LEFT:
				if (canChangeImage) {
					if (curFile != files.begin()) {
						curFile--;
						nomeFile = *curFile;
						loadImage();
						canChangeImage = false;
					}
				}
				break;
			case SDLK_RIGHT:
				if (canChangeImage) {
					curFile++;
					if (curFile == files.end()) {
						curFile--;
					}
					else {
						nomeFile = *curFile;
						loadImage();
						canChangeImage = false;
					}
				}
				break;
			}
			break;
		case SDL_KEYUP:
			canChangeImage = true;
			break;
		case SDL_MOUSEMOTION:
			onMouseMove(&e);
			break;
		case SDL_WINDOWEVENT:
			switch (e.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				onResize();
				onMouseWheel(&e);
				break;
			}
			break;
		case SDL_MOUSEWHEEL:
			onMouseWheel(&e);
			break;
		}
		if (somethingChanged) {
			SDL_RenderClear(rend);
			SDL_RenderCopy(rend, image, nullptr, &imageRect);
			SDL_RenderPresent(rend);
			somethingChanged = false;
		}
		else {
			/*Since the SDL_PollEvent is an implementation of the Windows 
			PeekMessage call, it uses a lot of CPU.
			The aim of this portion of code is to avoid 
			peeking too many messages when it's not needed,
			yet not giving the impression to the user that his messages get dropped:
			first, when something changes PeekMessage is fully operational;
			then, when everything is static, the program doesn't need any message to be peeked,
			so I emulate the GetMessage call by slowing down the PeekMessage.
			*/
			SDL_Delay(50);
		}
	}
	return 0;
}

string App::lowerString(string s)
{
	for (unsigned i = 0; i < s.length(); ++i) {
		if(s[i] <= 'Z' && s[i] >= 'A')
			s[i] = s[i] + 'a' - 'A';
	}
	return s;
}

void App::getResourcePath(const string &subDir)
{
	string baseRes;
	assert(baseRes.empty());
	char *basePath = SDL_GetBasePath();
	if (basePath)
	{
		baseRes = basePath;
		SDL_free(basePath);
	}
	size_t pos = baseRes.rfind("bin");
		baseRes = baseRes.substr(0, pos);
	string filePath = subDir.empty() ? baseRes : baseRes + subDir + SEPARATOR;
	appPath = filePath;
}