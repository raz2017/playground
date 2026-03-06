#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include <cassert>
#include <vector>


struct Frame {
	SDL_FRect source_rect;
	Frame(float x, float y, float w, float h) {
		source_rect.x = x;
		source_rect.y = y;
		source_rect.w = w;
		source_rect.h = h;
	}
};

struct Animation {
	std::vector<Frame> mFrames;
	std::size_t mCurrentFrame = 0;
	std::size_t mMaxFrames = 0;

	Animation() {

	}

	void LoadFrames(float starting_x, float starting_y, float w, float h, int numberOfFrames) {
		mMaxFrames = numberOfFrames;
		for (std::size_t i = 0; i < numberOfFrames; i++) {
			mFrames.push_back(Frame(starting_x + (w * i), starting_y, w, h));
		}
	}

	SDL_FRect GetFrameAsSDL_FRect(int index) {
		return mFrames.at(index).source_rect;
	}


	SDL_FRect GetFrameAsSDL_FRect() {
		return mFrames.at(mCurrentFrame).source_rect;
	}


	void Loop_Animation() {
		mCurrentFrame++;
		if (mCurrentFrame >= mMaxFrames) {
			mCurrentFrame = 0;
		}
	}
};

struct Sprite {
	SDL_Texture* mTexture;
	SDL_FRect destination_rect;
	SDL_FRect source_rect;
	Animation mAnimation;
	SDL_FlipMode mFlipMode = SDL_FLIP_NONE;
	float mVelocityX = 0.0f;
	float mVelocityY = 0.0f;
	float mGroundY = 800.0f;
	bool mIsGrounded = true;
	bool mHasDoubleJump = true;

	static constexpr float kGravity = 2.5f;
	static constexpr float kJumpForce = -20.0f;

	void Jump() {
		if (mIsGrounded) {
			mVelocityY = kJumpForce;
			mIsGrounded = false;
			mHasDoubleJump = true;
		}
		else if (mHasDoubleJump && mVelocityY < 0) {
			mVelocityY = kJumpForce;
			mHasDoubleJump = false;
		}
	}

	Sprite(SDL_Renderer* renderer, std::string filename) {
		SDL_Surface* surface = SDL_LoadPNG(filename.c_str());

		const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(surface->format);
		SDL_Palette* palette = SDL_GetSurfacePalette(surface);
		Uint32 colorKey = SDL_MapRGB(details, palette, 140, 198, 255);
		SDL_SetSurfaceColorKey(surface, true, colorKey);

		mTexture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_DestroySurface(surface);

		destination_rect = {
			.x = 50, .y = 25, .w = 24, .h = 28
		};

		mAnimation.LoadFrames(8, 14, 32, 36, 6);
	}

	Sprite() {

	}


	~Sprite() {
		SDL_DestroyTexture(mTexture);
	}

	void SetSpritePosition(float x, float y) {
		destination_rect.x = x;
		destination_rect.y = y;
	}

	void SetSpriteDimensions(float w, float h) {
		destination_rect.w = w;
		destination_rect.h = h;
	}

	void Update() {
		if (!mIsGrounded) mVelocityY += kGravity;

		destination_rect.x += mVelocityX;
		destination_rect.y += mVelocityY;

		if (destination_rect.x < 0) destination_rect.x = 0;
		if (destination_rect.x > 976) destination_rect.x = 976;

		if (destination_rect.y >= mGroundY) {
			destination_rect.y = mGroundY;
			mVelocityY = 0.0f;
			mIsGrounded = true;
			mHasDoubleJump = true;
		}

		if (mVelocityX > 0) mFlipMode = SDL_FLIP_NONE;
		else if (mVelocityX < 0) mFlipMode = SDL_FLIP_HORIZONTAL;

		source_rect = mAnimation.GetFrameAsSDL_FRect();
		if (mVelocityX != 0.0f || mVelocityY != 0.0f) {
			mAnimation.Loop_Animation();
		}
	}

	void Render(SDL_Renderer* renderer) {
		SDL_FPoint center{
			.x = 0.0,
			.y = 0.0
		};

		SDL_RenderTextureRotated(renderer, mTexture, &source_rect, &destination_rect, 0.0, &center, mFlipMode);

		SDL_SetTextureScaleMode(mTexture, SDL_SCALEMODE_PIXELART);
	}
};

struct SDLApplication {
	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	Sprite* mSprite;

	SDL_Texture* mTexture;
	SDL_Gamepad* mGamepad = nullptr;
	bool mJumpPressed = false;
	//Particles mParticlesSystem{ 10000 };
	bool running = true;
	bool isFullScreen = false;
	bool debug_mode = false;

	//A test surface

	SDLApplication(const char* title) {
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
		mWindow = SDL_CreateWindow(title, 1000, 1000, SDL_WINDOW_RESIZABLE);
		//mSurface = SDL_LoadPNG("data/bg/bg_layer1.png");
		//if (mSurface == nullptr) {
		//	assert(0 && "Improper image file path");
		//}
		mRenderer = SDL_CreateRenderer(mWindow, nullptr);
		if (mRenderer == nullptr) {
			assert(0 && "Renderer not created");
		}
		SDL_SetRenderLogicalPresentation(mRenderer, 1000, 1000, SDL_LOGICAL_PRESENTATION_STRETCH);

		/*SDL_Surface* mSurface = SDL_LoadPNG("data/slide.png");
		if (mSurface == nullptr) {
			assert(0 && "Could not load surface");
		}
		mTexture = SDL_CreateTextureFromSurface(mRenderer, mSurface);
		SDL_DestroySurface(mSurface);*/

		SetupSceneData();
	}

	void SetupSceneData() {
		mSprite = new Sprite{ mRenderer, "data/mario/mario.png" };
		mSprite->SetSpritePosition(50, 800);
		mSprite->SetSpriteDimensions(32, 28);
	}

	~SDLApplication() {
		if (mGamepad) SDL_CloseGamepad(mGamepad);
		SDL_Quit();
		SDL_DestroyTexture(mTexture);
		SDL_DestroyRenderer(mRenderer);
		SDL_DestroyWindow(mWindow);
		delete mSprite;
	}





	void Tick() {
		Input();
		Update();
		Render();
	}

	void Input() {
		SDL_Event event;
		const bool* keystate = SDL_GetKeyboardState(nullptr);

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			else if (event.type == SDL_EVENT_KEY_DOWN) {
				SDL_Log("a key was pressed: %d", event.key.key);

				if (keystate[SDL_SCANCODE_L] == true) {
					SDL_Log("SDL_SCANCODE_L was pressed");
				}
				if (keystate[SDL_SCANCODE_F11] == true) {
					isFullScreen = !isFullScreen;
					SDL_SetWindowFullscreen(mWindow, isFullScreen);
				}
				if (keystate[SDL_SCANCODE_F10] == true) {
					debug_mode = !debug_mode;
				}
			}
			else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
				if (event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
					mJumpPressed = true;
				}
			}
			else if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
				if (!mGamepad) {
					mGamepad = SDL_OpenGamepad(event.gdevice.which);
					SDL_Log("Gamepad connected: %s", SDL_GetGamepadName(mGamepad));
				}
			}
			else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
				if (mGamepad && SDL_GetGamepadID(mGamepad) == event.gdevice.which) {
					SDL_CloseGamepad(mGamepad);
					mGamepad = nullptr;
					SDL_Log("Gamepad disconnected");
				}
			}
			else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
				float x, y;
				SDL_MouseButtonFlags mouse = SDL_GetMouseState(&x, &y);
				SDL_Log("x,y: %f,%f", x, y);
			}
		}

	}

	void Update() {
		const float speed = 3.0f;
		const float deadzone = 8000.0f;
		float vx = 0.0f, vy = 0.0f;

		if (mGamepad) {
			float axisX = (float)SDL_GetGamepadAxis(mGamepad, SDL_GAMEPAD_AXIS_LEFTX);
			float axisY = (float)SDL_GetGamepadAxis(mGamepad, SDL_GAMEPAD_AXIS_LEFTY);
			if (SDL_fabsf(axisX) > deadzone) vx = (axisX / 32767.0f) * speed;
			if (SDL_fabsf(axisY) > deadzone) vy = (axisY / 32767.0f) * speed;

			if (SDL_GetGamepadButton(mGamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))  vx = -speed;
			if (SDL_GetGamepadButton(mGamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) vx = speed;
			if (SDL_GetGamepadButton(mGamepad, SDL_GAMEPAD_BUTTON_DPAD_UP))    vy = -speed;
			if (SDL_GetGamepadButton(mGamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))  vy = speed;
		}

		mSprite->mVelocityX = vx;
		if (mJumpPressed) {
			mSprite->Jump();
			mJumpPressed = false;
		}
		mSprite->Update();
	}

	void Render() {
		SDL_SetRenderDrawColor(mRenderer, 0x00, 0xAA, 0xFF, 0xFF);
		SDL_RenderClear(mRenderer);

		if (debug_mode) {
			SDL_SetRenderDrawColor(mRenderer, 0x00, 0x00, 0x00, 0xFF);
			SDL_RenderDebugText(mRenderer, 0.0f, 0.0f, "Debug mode");
		}


		mSprite->Render(mRenderer);
		SDL_RenderPresent(mRenderer);

	}

	void MainLoop() {

		Uint64 frames_per_second = 0;
		Uint64 lastTime = 0;

		while (running) {
			Uint64 currentTick = SDL_GetTicks();
			Tick();
			SDL_Delay(60);
			frames_per_second++;
			Uint64 deltaTime = SDL_GetTicks() - currentTick;
			if (currentTick > lastTime + 1000) {
				lastTime = currentTick;
				std::string title;
				title += "Raz's playground FPS: " + std::to_string(frames_per_second);
				SDL_SetWindowTitle(mWindow, title.c_str());
				frames_per_second = 0;

			}
		}
	}
};

int main(int argc, char* argv[])
{
	SDLApplication app("Raz's playground");
	app.MainLoop();
	return 0;
}