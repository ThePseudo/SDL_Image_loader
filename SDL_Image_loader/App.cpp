#include "App.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>

App *App::app = nullptr;

vector<wstring> App::supportedExtensions = {
	L"png",
	L"bmp",
	L"jpg",
	L"jpeg",
	L"gif",
	L"lbm",
	L"pcx",
	L"pnm",
	L"tga",
	L"tiff",
	L"webp",
	L"xcf",
	L"xpm",
	L"xv"
};

int main(int argc, char *argv[])
{
	if (argc < 2) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "No file selected", "No image was selected to be shown", nullptr);
		return 0;
	}
	string arg = argv[1];
	wstring wArg(arg.begin(), arg.end());
	return App::getApp(wArg.c_str())->run();
}

App::App(const wchar_t *nomeFile)
{
	images.resize(3);
	imageNames.resize(3);
	imageRects.resize(3);
	initSDL();
	setDirectory(nomeFile);
	loadImages();
}

App::~App()
{
	SDL_Quit();
}

App *App::getApp(const wchar_t *nomeFile)
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

void App::setDirectory(const wchar_t * nomeFile)
{
	wstring pathstring(nomeFile);
	size_t i;
	for (i = pathstring.length() - 1; i >= 0; --i) {
		if (pathstring.at(i) == SEPARATOR) break;
	}
	this->nomeFile = pathstring.substr(i+1, pathstring.length());
	currentDir = pathstring.substr(0, i);
	directory = directory_iterator(currentDir.c_str());
	detectFiles();
}

void App::loadImages()
{
	updateTitle();
	prevImageTask = create_task([this]() {
		loadImage(prev_image);
	});
	nextImageTask = create_task([this]() {
		loadImage(next_image);
	});
	loadImage(current_image);
	resetSize();
}

void App::loadImage(int index)
{
	destroyImage(index);
	SDL_Surface loadedImage;
	wstring path = currentDir + SEPARATOR + *imageNames[index];
	wstring extension = path.substr(path.find_last_of('.'), path.length());
	string temp(path.begin(), path.end());
	if (lowerString(extension) == "bmp") {
		loadedImage = *SDL_LoadBMP(temp.c_str());
	}
	else {
		loadedImage = *IMG_Load(temp.c_str());
	}
	if (&loadedImage == nullptr) {
		cout << SDL_GetError() << endl;
	}
	images[index] = SDL_CreateTextureFromSurface(rend, &loadedImage);
	SDL_GetClipRect(&loadedImage, &imageRects[index]);
	SDL_FreeSurface(&loadedImage);
}

void App::nextImage()
{
	nextImageTask.wait();
	if (canChangeImage) {
		imageNames[prev_image] = imageNames[current_image];
		imageNames[current_image] = imageNames[next_image];
		imageNames[next_image]++;
		if (imageNames[next_image] == files.end()) {
			imageNames[next_image] = files.begin();
		}
		nomeFile = *imageNames[current_image];
		updateTitle();
		destroyImage(prev_image);
		images[prev_image] = images[current_image];
		images[current_image] = images[next_image];
		imageRects[prev_image] = imageRects[current_image];
		imageRects[current_image] = imageRects[next_image];
		nextImageTask = create_task([this]() {
			images[next_image] = nullptr;
			loadImage(next_image);
		});
		resetSize();
		canChangeImage = false;
		somethingChanged = true;
	}
}

void App::prevImage()
{
	prevImageTask.wait();
	if (canChangeImage) {
		imageNames[next_image] = imageNames[current_image];
		imageNames[current_image] = imageNames[prev_image];
		if (imageNames[prev_image] == files.begin()) {
			imageNames[prev_image] = files.end()--;
		}
		else {
			imageNames[prev_image]--;
		}
		nomeFile = *imageNames[current_image];
		updateTitle();
		destroyImage(next_image);
		images[next_image] = images[current_image];
		images[current_image] = images[prev_image];
		imageRects[next_image] = imageRects[current_image];
		imageRects[current_image] = imageRects[prev_image];
		prevImageTask = create_task([this]() {
			images[prev_image] = nullptr;
			loadImage(prev_image);
		});
		resetSize();
		canChangeImage = false;
		somethingChanged = true;
	}
}

void App::onResize()
{
	bool mustWrite = true;
	SDL_Surface *screenSurface = SDL_GetWindowSurface(win);
	oldRect = screenRect;
	SDL_GetClipRect(screenSurface, &screenRect);
	float proportion;
	for (int i = 0; i > -20; --i) {
		proportion = static_cast<float>(originalRect.w + (resizeFactor * i)) / (float)originalRect.w;
		minZoomFactor = i;
		if (screenRect.w > originalRect.w + (resizeFactor * i) &&
			screenRect.h > static_cast<int>(static_cast<float>(originalRect.h) * proportion)) {
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
		imageRects[current_image].x += e->motion.x - mousex;
		imageRects[current_image].y += e->motion.y - mousey;
		fitBoundaries();
		centerToScreen();
		somethingChanged = true;
	}
	SDL_GetMouseState(&mousex, &mousey);
}

void App::fitToScreen()
{
	if (imageRects[current_image].w <= screenRect.w && imageRects[current_image].h <= screenRect.h) {
		double ratiox, ratioy, minratio;
		ratiox = static_cast<double>(screenRect.w) / static_cast<double>(imageRects[current_image].w);
		ratioy = static_cast<double>(screenRect.h) / static_cast<double>(imageRects[current_image].h);
		minratio = (ratiox > ratioy) ? ratioy : ratiox;
		imageRects[current_image].w = (ratiox == minratio) ? screenRect.w : (int)((double)imageRects[current_image].w * (double)minratio);
		imageRects[current_image].h = (ratioy == minratio) ? screenRect.h : (int)((double)imageRects[current_image].h * (double)minratio);
		imageRects[current_image].x = (screenRect.w > imageRects[current_image].w) ? (int)((screenRect.w - imageRects[current_image].w) / 2.0f) : 0;
		imageRects[current_image].y = (screenRect.h > imageRects[current_image].h) ? (int)((screenRect.h - imageRects[current_image].h) / 2.0f) : 0;
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
	imageRects[current_image].w = originalRect.w + (resizeFactor * zoomFactor);
	proportion = (float)imageRects[current_image].w / (float)originalRect.w;
	imageRects[current_image].h = (int)((float)originalRect.h * proportion);
	if(e != nullptr) e->wheel.y = 0;

	fitBoundaries();
	centerToScreen();
	somethingChanged = true;
}

void App::writeSettings(bool reset)
{
	wstring settingsPath = this->appPath + L"settings.ini";
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
	wstring settingsPath = this->appPath + L"settings.ini";
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
	wstring temp1, temp2;
	for (auto i = directory; i != directory_iterator(); ++i) {
		if (!is_directory(i->path())) {
			temp1 = i->path().extension().wstring();
			for (auto iter = supportedExtensions.begin(); iter != supportedExtensions.end(); ++iter) {
				if (lowerString(i->path().extension().wstring()) == (wstring)(L"." + *iter)) {
					wstring nameFile = i->path().filename().wstring();
					files.push_back(nameFile);
#ifdef _DEBUG
					cout << nameFile << endl;
#endif
					break;
				}
			}
		}
	}
	for (imageNames[current_image] = files.begin(); imageNames[current_image] != files.end(); ++imageNames[current_image]) {
		if (*imageNames[current_image] == nomeFile) {
			imageNames[prev_image] = --imageNames[current_image];
			++imageNames[current_image];
			imageNames[next_image] = ++imageNames[current_image];
			--imageNames[current_image];
			found = true;
			break;
		}
	}
	assert(found);
}

void App::centerToScreen()
{
	if (imageRects[current_image].w - imageRects[current_image].x <= screenRect.w && imageRects[current_image].h - imageRects[current_image].h <= screenRect.h) {
		fitToScreen();
	}
	if (imageRects[current_image].w - imageRects[current_image].x < screenRect.w) {
		imageRects[current_image].x = static_cast<int>(static_cast<float>(screenRect.w - imageRects[current_image].w) / 2.0f);
	}
	else if (imageRects[current_image].h - imageRects[current_image].y < screenRect.h) {
		imageRects[current_image].y = static_cast<int>(static_cast<float>(screenRect.h - imageRects[current_image].h) / 2.0f);
	}
}

void App::fitBoundaries()
{
	if ((angle > 45 && angle < 135) || (angle > 225 && angle < 335)) {
		return;
	}
	if (imageRects[current_image].x > screenRect.x) {
		imageRects[current_image].x = screenRect.x;
	}
	if (imageRects[current_image].y > screenRect.y) {
		imageRects[current_image].y = screenRect.y;
	}
	if (imageRects[current_image].x + imageRects[current_image].w < screenRect.x + screenRect.w)
		imageRects[current_image].x = screenRect.x + screenRect.w - imageRects[current_image].w;
	if (imageRects[current_image].y + imageRects[current_image].h < screenRect.y + screenRect.h)
		imageRects[current_image].y = screenRect.y + screenRect.h - imageRects[current_image].h;
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
			case SDLK_a:
				angle += 270.0f;
				if (angle > 360) angle -= 360;
				somethingChanged = true;
				break;
			case SDLK_d:
				angle += 90.0f;
				if (angle > 360) angle -= 360;
				somethingChanged = true;
				break;
			case SDLK_LEFT:
				prevImage();
				break;
			case SDLK_RIGHT:
				nextImage();
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
			SDL_RenderCopyEx(rend, images[current_image], nullptr, &imageRects[current_image], angle, nullptr, SDL_FLIP_NONE);
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
			SDL_Delay(25);
		}
	}
	return 0;
}

wstring App::lowerString(wstring s)
{
	for (unsigned i = 0; i < s.length(); ++i) {
		if(s[i] <= L'Z' && s[i] >= L'A')
			s[i] = s[i] + L'a' - L'A';
	}
	return s;
}

void App::getResourcePath(const wstring &subDir)
{
	string basePath = SDL_GetBasePath();
	wstring baseRes(basePath.begin(), basePath.end());

	size_t pos = baseRes.rfind(L"bin");
		baseRes = baseRes.substr(0, pos);
	wstring filePath = subDir.empty() ? baseRes : baseRes + subDir + SEPARATOR;
	appPath = filePath;
}

void App::destroyImage(int index)
{
	if (images[index] != nullptr) {
		SDL_DestroyTexture(images[index]);
	}
	images[index] = nullptr;
}

void App::updateTitle()
{
	wstring title = L"Image viewer - " + this->nomeFile;
	string tempTitle(title.begin(), title.end());
	SDL_SetWindowTitle(win, tempTitle.c_str());
}

void App::resetSize()
{
	originalRect = imageRects[current_image];
	resizeFactor = originalRect.w / 10;
	onResize();
	onMouseWheel(nullptr);
	angle = 0.0f;
}
