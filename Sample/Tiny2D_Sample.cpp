#include "Tiny2D.h"
#include <math.h>

using namespace Tiny2D;

class SampleApp : public App::Callbacks
{
public:
	virtual bool OnStartup(int numArguments, const char** arguments, const App::SystemInfo& systemInfo, App::StartupParams& outParams)
	{
		outParams.name = "Tiny2D Sample";
		outParams.virtualWidth = 800;
		outParams.virtualHeight = 600;

		// If screen resolution is large enough, use large texture versions

		if (systemInfo.nativeWidth > 1024)
		{
			outParams.textureVersionSizeMultiplier = 0.5f;
			outParams.textureVersion = "@2x";
		}
		return true;
	}

	virtual bool OnInit()
	{
		// Load 'loading screen' data synchronously

		state = State_Loading;

		loadingScreen.Create("LoadingScreen.png");

		cogWheel.Create("CogWheel.png");
		cogWheelRotation = 0.0f;

		font.Create("common/courbd.ttf", 28, Font::Flags_Italic, false);

		// Load game data asynchronously

		character.Create("character", false);
		character.PlayAnimation("idle_right");
		characterPosition = Vec2(100.0f, (float) (App::GetHeight() - 178.0f));
		characterDir = Dir_Right;

		background.Create("background.png", false);

		moon.Create("moon.png", false);
		moonPosition = Vec2((float) App::GetWidth(), 50.0f);

		spinningEffectRotation = 0.0f;
		spinningEffect.Create("stars", GetSpinningEffectPosition());

		cursorEffectPosition = Vec2(0.0f, 0.0f);
		cursorEffect.Create("stars2", GetCursorEffectPosition());

		music.Create("music.ogg", true, false);

		step.Create("step.wav", false, false);
		stepTimer = 1.0f;

		Localization::LoadSet("texts");

		// Create render target

		renderTexture.CreateRenderTarget(256, 256);

		// Enable bloom postprocess

		Postprocessing::EnableBloom(true, 0.5f);

		// Enable displaying of the performance info

		App::EnableOnScreenDebugInfo(true);

		startTicks = Time::GetTicks();
		return true;
	}

	virtual void OnActivated(bool activated)
	{
		Log::Info(activated ? "My app has been restored" : "My app has been put to sleep");
	}

	virtual void OnUpdate(float deltaTime)
	{
		switch (state)
		{
			// The game data is still being (asynchronously) loaded

			case State_Loading:
				cogWheelRotation += deltaTime;
				if (Jobs::GetNumJobsInProgress() == 0 && Time::SecondsSince(startTicks) > 1.0f)
					state = State_Loaded;
				break;

			// The game data loading has just finished - wait for user input

			case State_Loaded:
				cogWheelRotation += deltaTime * 0.1f;
				if (Input::WasKeyPressed(Input::Key_Space) || Input::WasTouchpadPressed())
				{
					state = State_Active;

					// Unload loading screen

					cogWheel.Destroy();
					loadingScreen.Destroy();

					// Play music

					music.Play(false, 2.0f);
				}
				break;

			// The game is active

			case State_Active:
			{
				// Update character controllable by the user

				UpdateCharacter(deltaTime);

				// Update spinning particle effect

				spinningEffectRotation += deltaTime * 2.0f;
				spinningEffect.SetPosition( GetSpinningEffectPosition() );
				spinningEffect.Update(deltaTime);

				// Update particle effect attached to cursor

				cursorEffect.SetPosition( GetCursorEffectPosition() );
				cursorEffect.Update(deltaTime);

				// Update moon position

				moonPosition.x = moonPosition.x - deltaTime * 30.0f;
				if (moonPosition.x < (float) -moon.GetWidth())
					moonPosition.x = (float) App::GetWidth();

				// Quit an app?

				if (Input::WasKeyPressed(Input::Key_Escape) || Input::WasKeyPressed(Input::Key_Android_Back))
					App::Quit();

				// Enable 'quake' postprocess?

				if (Input::WasKeyPressed(Input::Key_Space))
					Postprocessing::StartQuake(true);

				// Toggle 'old tv' and 'rainy glass' postprocesses

				if (Input::WasKeyPressed(Input::Key_P))
					Postprocessing::EnableOldTV( !Postprocessing::IsOldTVEnabled() );
				if (Input::WasKeyPressed(Input::Key_O))
					Postprocessing::EnableRainyGlass( !Postprocessing::IsRainyGlassEnabled() );

#ifdef DESKTOP
				// Toggle fullscreen mode

				if (Input::WasKeyPressed(Input::Key_R))
				{
					App::DisplaySettings newSettings;
					newSettings.fullscreen = !App::IsFullscreen();
					newSettings.virtualWidth = 800;
					newSettings.virtualHeight = 600;

					App::ModifyDisplaySettings(newSettings);
				}
#endif
				// Open link in browser?

				if (Input::WasKeyPressed(Input::Key_W))
					App::RunCommand("http://www.pixelelephant.com");
				break;
			}
		}
	}

	virtual void OnDraw(Texture& renderTarget)
	{
		switch (state)
		{
			// The game is still loading or waiting for user input

			case State_Loading:
			case State_Loaded:
				DrawLoadingScreen(renderTarget);
				break;

			// The game is active

			case State_Active:
				DrawToRenderTexture(renderTexture);
				DrawScene(renderTarget);
				break;
		}
	}

	virtual void OnDrawAfterPostprocessing(Texture& renderTarget)
	{
		switch (state)
		{
			case State_Loading:
			case State_Loaded:
			{
				Text::DrawParams fontParams;
				fontParams.position = Vec2(0.0f, 500.0f);
				fontParams.width = (float) App::GetWidth();
				fontParams.horizontalAlignment = Text::HorizontalAlignment_Center;
				fontParams.drawShadow = true;
				switch (state)
				{
				case State_Loading:
					fontParams.text = Localization::Get("texts.LoadingScreen.Loading");
					break;
				case State_Loaded:
					fontParams.text = Localization::Get("texts.LoadingScreen.Continue");
					break;
				}
				font.Draw(&fontParams);
				break;
			}

		case State_Active:

			// Spinning text
			{
				const Localization::Param localizationParam("name", "Tiny2D");

				Text::DrawParams params;
				params.text = Localization::Get("texts.General.Info", &localizationParam, 1);
				params.position = Vec2(0.0f, 0.0f);
				params.width = (float) App::GetWidth();
				params.height = (float) App::GetHeight();
				params.horizontalAlignment = Text::HorizontalAlignment_Center;
				params.verticalAlignment = Text::VerticalAlignment_Center;
				params.rotation = moonPosition.x * 0.01f;
				params.drawShadow = true;

				font.Draw(&params);
			}

			break;
		}
	}

private:

	void DrawCogWheel(float x, float y, float radius, float rotation, const Color& color)
	{
		const float scale = radius * 2.0f / (float) cogWheel.GetWidth();
		x -= 400.0f;
		y += 100.0f;
		cogWheel.Draw(Vec2(x, y), rotation, scale, color);
	}

	void DrawLoadingScreen(Texture& renderTarget)
	{
		// Begin drawing to render target

		renderTarget.BeginDrawing();

		// Draw background

		loadingScreen.Draw();

		// Draw rotating cog wheels

		DrawCogWheel(640, 930 - 750, 110, cogWheelRotation, Color(0.7f, 0.4f, 0.1f, 1));
		DrawCogWheel(820, 880 - 750, 70, -cogWheelRotation * 1.3f, Color(0.6f, 0.3f, 0.1f, 1));
		DrawCogWheel(900, 970 - 750, 42, cogWheelRotation * 1.8f, Color(0.6f, 0.4f, 0.1f, 1));
		DrawCogWheel(952, 914 - 750, 30, -cogWheelRotation * 2.8f, Color(0.7f, 0.4f, 0.1f, 1));
		DrawCogWheel(990, 957 - 750, 22, cogWheelRotation * 3.4f, Color(0.5f, 0.3f, 0.1f, 1));

		// End drawing to render target

		renderTarget.EndDrawing();
	}

	void DrawToRenderTexture(Texture& renderTarget)
	{
		renderTarget.BeginDrawing(&Color::Black);

		// Draw the moon
		{
			const Rect rect(0.0f, 0.0f, 256.0f, 256.0f);

			Sprite::DrawParams params;
			params.rect = &rect;
			moon.Draw(&params);
		}

		// Draw some debug shapes

		Shape::DrawCircle(Vec2(50, 50), 50, 5, moonPosition.x * 0.1f, Color(1, 0, 0, 0.7f));
		Shape::DrawCircle(Vec2(130, 130), 70, 15, -moonPosition.x * 0.1f, Color(0, 1, 0, 0.7f));
		Shape::DrawRectangle(Rect(150, 150, 90, 70), moonPosition.x * 0.05f, Color(0, 0, 1, 0.7f));
		Shape::DrawLine(Vec2(10, 10), Vec2(200, 200), Color::White);

		// Draw debug text

		font.Draw("This is\nrender\ntexture", Vec2(20, 70), Color::Yellow);

		renderTarget.EndDrawing();
	}

	void DrawScene(Texture& renderTarget)
	{
		// Begin drawing to render target and clear it

		renderTarget.BeginDrawing(&Color::Black);

		// Background with moving moon

		background.Draw();
		moon.Draw(moonPosition, sinf(moonPosition.x * 0.1f) * 0.3f);

		// Particle effects

		spinningEffect.Draw();
		cursorEffect.Draw();

		// Draw content of the render texture

		renderTexture.Draw(Vec2(100, 100), -moonPosition.x * 0.01f);

		// Walking character
		{
			Sprite::DrawParams params;
			params.position = characterPosition;
			params.flipX = characterDir == Dir_Left;
			params.scale = 1.4f;

			character.Draw(&params);
		}

		// End drawing to render target

		renderTarget.EndDrawing();
	}

	// Updates character based on user input
	void UpdateCharacter(float deltaTime)
	{
		const float speed = 200.0f;

		bool isWalking = false;
		if (Input::IsKeyDown(Input::Key_Left))
		{
			characterPosition.x -= deltaTime * speed;
			isWalking = true;
			characterDir = Dir_Left;
		}
		else if (Input::IsKeyDown(Input::Key_Right))
		{
			characterPosition.x += deltaTime * speed;
			isWalking = true;
			characterDir = Dir_Right;
		}
		else if (Input::IsKeyDown(Input::Key_Up))
		{
			characterPosition.y -= deltaTime * speed;
			isWalking = true;
			characterDir = Dir_Up;
		}
		else if (Input::GetKeyState(Input::Key_Down))
		{
			characterPosition.y += deltaTime * speed;
			isWalking = true;
			characterDir = Dir_Down;
		}

		// Determine animation to play depending on character orientation and state

		std::string animationName = isWalking ? "walk_" : "idle_";
		switch (characterDir)
		{
			case Dir_Left:
			case Dir_Right:
				animationName += "right";
				break;
			case Dir_Up:
				animationName += "up";
				break;
			case Dir_Down:
				animationName += "down";
				break;
		}

		// Set/change animation

		character.PlayAnimation(animationName);

		// Update sprite state (mostly animation blending)

		character.Update(deltaTime);

		// When walking, play 'step' sound every second

		if (!isWalking)
			stepTimer = 0.0f;
		else
		{
			stepTimer -= deltaTime;
			if (stepTimer <= 0.0f)
			{
				stepTimer = 1.0f;
				step.Play();
			}
		}
	}

	// Gets position of spinning particle effect
	Vec2 GetSpinningEffectPosition() const
	{
		const Vec2 screenCenter( (float) App::GetWidth() * 0.5f, (float) App::GetHeight() * 0.5f );
		Vec2 spinningEffectPosition = screenCenter * 1.3f;
		spinningEffectPosition.Rotate(spinningEffectRotation, screenCenter);
		return spinningEffectPosition;
	}

	// Gets position of particle effect attached to cursor
	Vec2 GetCursorEffectPosition()
	{
		if (Input::HasMouse())
			cursorEffectPosition = Input::GetMouseState().position;
		else if (Input::HasTouchpad() && Input::GetNumTouches() >= 1)
			cursorEffectPosition = Input::GetTouch(0).position;
		return cursorEffectPosition;
	}

	// State

	enum State
	{
		State_Loading,	// The game data is being loaded
		State_Loaded,	// The game data loading finished; now waiting for user input
		State_Active	// Game is active
	};
	State state;

	Time::Ticks startTicks;

	// Loading screen

	Texture cogWheel;
	Texture loadingScreen;
	float cogWheelRotation;

	// Background

	Texture background;

	Sprite moon;
	Vec2 moonPosition;

	// Character

	Sprite character;
	Vec2 characterPosition;
	enum Dir
	{
		Dir_Left = 0,
		Dir_Right,
		Dir_Up,
		Dir_Down
	};
	Dir characterDir;

	// Particle effects

	Effect spinningEffect;
	float spinningEffectRotation;

	Effect cursorEffect;
	Vec2 cursorEffectPosition;

	// Audio

	Sound music;
	Sound step;
	float stepTimer;

	// Misc

	Texture renderTexture;

	Font font;
};

DEFINE_APP(SampleApp);
