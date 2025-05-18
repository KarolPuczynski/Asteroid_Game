#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <raylib.h>
#include <raymath.h>

// --- UTILS ---
namespace Utils {
	inline static float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}
}

// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
	Vector2 position{};
	float rotation{};
};

struct Physics {
	Vector2 velocity{};
	float rotationSpeed{};
};

struct Renderable {
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, RANDOM = 0 };

// --- RENDERER ---
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer inst;
		return inst;
	}

	void Init(int w, int h, const char* title) {
		InitWindow(w, h, title);
		SetTargetFPS(60);
		screenW = w;
		screenH = h;
	}

	void Begin() {
		BeginDrawing();
		ClearBackground(BLACK);
	}

	void End() {
		EndDrawing();
	}

	void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
		DrawPolyLines(pos, sides, radius, rot, WHITE);
	}

	int Width() const {
		return screenW;
	}

	int Height() const {
		return screenH;
	}

private:
	Renderer() = default;

	int screenW{};
	int screenH{};
};

// --- ASTEROID HIERARCHY ---

class Asteroid {
public:
	Asteroid(int screenW, int screenH, bool split, Vector2 pos, int split_size, AsteroidShape shape) : shape(shape) {
		init(screenW, screenH, split, pos, split_size);
	}
	
	virtual ~Asteroid() = default;

	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		transform.rotation += physics.rotationSpeed * dt;
		if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
			transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
			return false;
		return true;
	}
	virtual void Draw() const = 0;

	Vector2 GetPosition() const {
		return transform.position;
	}

	float constexpr GetRadius() const {
		return 16.f * (float)render.size;
	}

	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}

	int GetSize() const {
		return static_cast<int>(render.size);
	}

	AsteroidShape GetShape() const {
		return shape;
	}

	void TakeDamage(int dmg) {
		if (crashed) return;
		current_asteroid_hp -= dmg;
		if (current_asteroid_hp <= 0) crashed = true;
	}

	bool IsCrashed() const {
		return crashed;
	}

protected:
	bool crashed = false;
	float starting_asteroid_hp = 100;
	float current_asteroid_hp = 1;
	AsteroidShape shape;
	void init(int screenW, int screenH, bool split, Vector2 pos, int split_size) {
		// Choosing size and asteroid HP
		if (!split) {
			render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));
			switch (shape) {
			case AsteroidShape::TRIANGLE:
				starting_asteroid_hp = 100 * render.size;
				break;
			case AsteroidShape::SQUARE:
				starting_asteroid_hp = 150 * render.size;
				break;
			case AsteroidShape::PENTAGON:
				starting_asteroid_hp = 200 * render.size;
				break;
			default:
				starting_asteroid_hp = 100 * render.size;
				break;
			}
		}
		else {
			render.size = static_cast<Renderable::Size>(split_size / 2);
			switch (shape) {
				case AsteroidShape::TRIANGLE:
					starting_asteroid_hp = 100 * split_size;
					break;
				case AsteroidShape::SQUARE:
					starting_asteroid_hp = 150 * split_size;
					break;
				case AsteroidShape::PENTAGON:
					starting_asteroid_hp = 200 * split_size;
					break;
				default:
					starting_asteroid_hp = 100 * split_size;
					break;
				}
		}
		current_asteroid_hp = starting_asteroid_hp;
		
		// Spawn at random edge
		if (!split) {
			switch (GetRandomValue(0, 3)) {
				case 0:
					transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
					break;
				case 1:
					transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
					break;
				case 2:
					transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
					break;
				default:
					transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
					break;
				}
		} // Spawning at the split place
		else {
			transform.position = pos;
		}
			
		// Aim towards center with jitter
		float maxOff = fminf(screenW, screenH) * 0.1f;
		float ang = Utils::RandomFloat(0, 2 * PI);
		float rad = Utils::RandomFloat(0, maxOff);
		Vector2 center = {
										 screenW * 0.5f + cosf(ang) * rad,
										 screenH * 0.5f + sinf(ang) * rad
		};

		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
		physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);

		transform.rotation = Utils::RandomFloat(0, 360);
	}

	TransformA transform;
	Physics    physics;
	Renderable render;

	int baseDamage = 0;
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};

class TriangleAsteroid : public Asteroid {
public:
	TriangleAsteroid(int w, int h, bool split, Vector2 pos, int split_size, AsteroidShape shape) : Asteroid(w, h, split, pos, split_size, shape) { baseDamage = 5; }
	void Draw() const override {
		float green_bar_width = 2 * GetRadius() * float(current_asteroid_hp / starting_asteroid_hp);
		float bar_height = 5;
		DrawRectangle(transform.position.x - GetRadius(), transform.position.y - GetRadius() - 6, green_bar_width, bar_height, GREEN);
		Renderer::Instance().DrawPoly(transform.position, 3, GetRadius(), transform.rotation);
	}
};
class SquareAsteroid : public Asteroid {
public:
	SquareAsteroid(int w, int h, bool split, Vector2 pos, int split_size, AsteroidShape shape) : Asteroid(w, h, split, pos, split_size, shape) { baseDamage = 10; }
	void Draw() const override {
		float green_bar_width = 2 * GetRadius() * float(current_asteroid_hp / starting_asteroid_hp);
		float bar_height = 5;
		DrawRectangle(transform.position.x - GetRadius(), transform.position.y - GetRadius() - 6, green_bar_width, bar_height, GREEN);
		Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
	}
};
class PentagonAsteroid : public Asteroid {
public:
	PentagonAsteroid(int w, int h, bool split, Vector2 pos, int split_size, AsteroidShape shape) : Asteroid(w, h, split, pos, split_size, shape) { baseDamage = 15; }
	void Draw() const override {
		float green_bar_width = 2 * GetRadius() * float(current_asteroid_hp / starting_asteroid_hp);
		float bar_height = 5;
		DrawRectangle(transform.position.x - GetRadius(), transform.position.y - GetRadius() - 6, green_bar_width, bar_height, GREEN);;
		//DrawRectangle(100, 10, hp_percent * BAR_WIDTH, BAR_HEIGHT, GREEN);
		//DrawRectangle(100 + hp_percent * BAR_WIDTH, 10, (1 - hp_percent) * BAR_WIDTH, BAR_HEIGHT, BLACK);
		Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
	}
};

// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape, bool split, Vector2 pos, int split_size) {
	switch (shape) {
	case AsteroidShape::TRIANGLE:
		return std::make_unique<TriangleAsteroid>(w, h, split, pos, split_size, shape);
	case AsteroidShape::SQUARE:
		return std::make_unique<SquareAsteroid>(w, h, split, pos, split_size, shape);
	case AsteroidShape::PENTAGON:
		return std::make_unique<PentagonAsteroid>(w, h, split, pos, split_size, shape);
	default: {
		return MakeAsteroid(w, h, static_cast<AsteroidShape>(3 + GetRandomValue(0, 2)), split, pos, split_size);
	}
	}
}

// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, SHOTGUN, COUNT };
class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt)
	{
		transform.position = pos;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
	}
	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));

		if (transform.position.x < 0 ||
			transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 ||
			transform.position.y > Renderer::Instance().Height())
		{
			return true;
		}
		return false;
	}
	void Draw() const {
		Vector2 offset = { 0, 40 };
		Vector2 offset_shotgun = { -20, 40 };
		switch (type) {
			case WeaponType::BULLET:
				DrawCircleV(transform.position + offset, 5.f, WHITE);
				break;
			case WeaponType::LASER: {
				static constexpr float LASER_LENGTH = 30.f;
				Rectangle lr = { transform.position.x - 2.f, transform.position.y + offset.y, 4.f, LASER_LENGTH };
				DrawRectangleRec(lr, RED);
				break;
			}
			case WeaponType::SHOTGUN:
				DrawCircleV(transform.position + offset_shotgun, 3.f, BLUE);
				break;
		}
	}
	Vector2 GetPosition() const {
		return transform.position;
	}

	float GetRadius() const {
		return (type == WeaponType::BULLET) ? 5.f : 2.f;
	}

	int GetDamage() const {
		return baseDamage;
	}

private:
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;
};

inline static Projectile MakeProjectile(WeaponType wt,
	const Vector2 pos,
	const Vector2 speed)
{
	switch (wt) {
	case WeaponType::BULLET:
		return Projectile(pos, speed, 10, wt);
		break;
	case WeaponType::LASER:
		return Projectile(pos, speed, 200, wt);
		break;
	case WeaponType::SHOTGUN:
		return Projectile(pos, speed, 5, wt);
		break;
	}
}

// --- SHIP HIERARCHY ---
class Ship {
public:
	Ship(int screenW, int screenH) {
		transform.position = {
												 screenW * 0.5f,
												 screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 7.f; // shots/sec
		fireRateBullet = 14.f;
		fireRateShotgun = 10.f;
		spacingLaser = 80.f; // px between lasers
		spacingBullet = 60.f;
		spacingShotgun = 50.f;
	}
	virtual ~Ship() = default;
	virtual void Update(float dt) = 0;
	virtual void Draw() const = 0;

	void TakeDamage(int dmg) {
		if (!alive) return;
		hp -= dmg;
		if (hp <= 0) {
			alive = false;
			hp = 0;
		}
	}

	bool IsAlive() const {
		return alive;
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const = 0;

	float GetHP() const {
		return hp;
	}

	float GetFireRate(WeaponType wt) const {
		switch (wt) {
		case WeaponType::BULLET:
			return fireRateBullet;
			break;
		case WeaponType::LASER:
			return fireRateLaser;
			break;
		case WeaponType::SHOTGUN:
			return fireRateShotgun;
			break;
		}
	}

	float GetSpacing(WeaponType wt) const {
		switch (wt) {
		case WeaponType::BULLET:
			return spacingBullet;
			break;
		case WeaponType::LASER:
			return spacingLaser;
			break;
		case WeaponType::SHOTGUN:
			return spacingShotgun;
			break;
		}
	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	bool       alive;
	float      fireRateLaser;
	float      fireRateBullet;
	float	   fireRateShotgun;
	float      spacingLaser;
	float      spacingBullet;
	float	   spacingShotgun;
};

class PlayerShip :public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		texture = LoadTexture("spaceship1.png");
		GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
		SetTextureFilter(texture, 2);
		scale = 0.25f;
	}
	~PlayerShip() {
		UnloadTexture(texture);
	}

	void Update(float dt) override {
		if (alive) {
			if (IsKeyDown(KEY_W)) transform.position.y -= speed * dt;
			if (IsKeyDown(KEY_S)) transform.position.y += speed * dt;
			if (IsKeyDown(KEY_A)) transform.position.x -= speed * dt;
			if (IsKeyDown(KEY_D)) transform.position.x += speed * dt;
		}
		else {
			transform.position.y += speed * dt;
		}
	}

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
		Vector2 dstPos = {
										 transform.position.x - (texture.width * scale) * 0.5f,
										 transform.position.y - (texture.height * scale) * 0.5f
		};
		DrawTextureEx(texture, dstPos, 0.0f, scale, WHITE);
	}

	float GetRadius() const override {
		return (texture.width * scale) * 0.5f;
	}

private:
	Texture2D texture;
	float     scale;
};

// --- APPLICATION ---
class Application {
public:
	static Application& Instance() {
		static Application inst;
		return inst;
	}

	void Run() {
		srand(static_cast<unsigned>(time(nullptr)));
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");

		auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;
		Vector2 pos = {};

		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;

			// Update player
			player->Update(dt);

			// Restart logic
			if (!player->IsAlive()) {
				int textWidth = MeasureText("GAME OVER", 80);
				DrawText("GAME OVER", (C_WIDTH - textWidth) / 2, (C_HEIGHT / 2) - 80, 80, RED);

				char scoreText[64];
				sprintf_s(scoreText, "Your score: %d", score);
				textWidth = MeasureText(scoreText, 60);
				DrawText(scoreText, (C_WIDTH - textWidth) / 2, (C_HEIGHT / 2) + 30, 60, GOLD);

				const char* retryText = "Press R to try again";
				textWidth = MeasureText(retryText, 40);
				DrawText(retryText, (C_WIDTH - textWidth) / 2, (C_HEIGHT / 2) + 120, 40, GREEN);
				if (IsKeyPressed(KEY_R)) {
					player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
					asteroids.clear();
					projectiles.clear();
					spawnTimer = 0.f;
					spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
					score = 0;
				}
			}
			// Asteroid shape switch
			if (IsKeyPressed(KEY_ONE)) {
				currentShape = AsteroidShape::TRIANGLE;
			}
			if (IsKeyPressed(KEY_TWO)) {
				currentShape = AsteroidShape::SQUARE;
			}
			if (IsKeyPressed(KEY_THREE)) {
				currentShape = AsteroidShape::PENTAGON;
			}
			if (IsKeyPressed(KEY_FOUR)) {
				currentShape = AsteroidShape::RANDOM;
			}

			// Weapon switch
			if (IsKeyPressed(KEY_TAB)) {
				currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT));
			}

			// Shooting
			{
				if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
					shotTimer += dt;
					float interval = 1.f / player->GetFireRate(currentWeapon);
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);
					Vector2 vel = {};

					while (shotTimer >= interval) {
						Vector2 p = player->GetPosition();
						p.y -= player->GetRadius();
						if (currentWeapon == WeaponType::SHOTGUN) {
							for (int i = 0; i < 3; i++) {
								switch (i) {
									case 0: {
										vel = { projSpeed, -projSpeed };		// bullet heading to the left
										break;
									}
									case 1:{
										vel = { 0, -projSpeed };				// bullet heading up
										break;
									}
									case 2: {
										vel = { -projSpeed, -projSpeed };	    // bullet heading to the right
										break;
									}
								}
								projectiles.push_back(MakeProjectile(currentWeapon, p, vel));
								p.x += 20;
							}
						}
						else {
							vel = { 0, -projSpeed };
							projectiles.push_back(MakeProjectile(currentWeapon, p, vel));
						}
						shotTimer -= interval;
					}
				}
				else {
					float maxInterval = 1.f / player->GetFireRate(currentWeapon);

					if (shotTimer > maxInterval) {
						shotTimer = fmodf(shotTimer, maxInterval);
					}
				}
			}

			// Spawn asteroids
			if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST) {
				asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, currentShape, false, pos, 0));
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}

			// Update projectiles - check if in boundries and move them forward
			{
				auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
					[dt](auto& projectile) {
						return projectile.Update(dt);
					});
				projectiles.erase(projectile_to_remove, projectiles.end());
			}

			// Projectile-Asteroid collisions O(n^2)
			for (auto pit = projectiles.begin(); pit != projectiles.end();) {
				bool removed = false;

				for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
					float dist = Vector2Distance((*pit).GetPosition(), (*ait)->GetPosition());
					if (dist < (*pit).GetRadius() + (*ait)->GetRadius()) {
						
						(*ait)->TakeDamage((*pit).GetDamage());

						if ((*ait)->IsCrashed()) {
							score += (*ait)->GetSize() * 5;
							if ((*ait)->GetSize() > 1) {									// if size of the asteroid is larger than small size it splits up into two parts
								Vector2 split_position = (*ait)->GetPosition();
								asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, (*ait)->GetShape(), true, split_position, (*ait)->GetSize()));
								asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, (*ait)->GetShape(), true, split_position, (*ait)->GetSize()));
								score += (*ait)->GetSize() * 2;
							}
							ait = asteroids.erase(ait);
						}
						pit = projectiles.erase(pit);
						removed = true;
						break;
					}
				}
				if (!removed) {
					++pit;
				}
			}

			// Asteroid-Ship collisions
			{
				auto remove_collision =
					[&player, dt](auto& asteroid_ptr_like) -> bool {
					if (player->IsAlive()) {
						float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

						if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
							player->TakeDamage(asteroid_ptr_like->GetDamage());
							return true; // Mark asteroid for removal due to collision
						}
					}
					if (!asteroid_ptr_like->Update(dt)) {
						return true;
					}
					return false; // Keep the asteroid
					};
				auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
				asteroids.erase(asteroid_to_remove, asteroids.end());
			}

			// Render everything
			{
				Renderer::Instance().Begin();

				int current_hp = player->GetHP();
				float hp_percent = (float)current_hp / 100;
				DrawText(TextFormat("HP: %d", current_hp),
					10, 10, 20, GREEN);
				DrawRectangle(97, 12, BAR_WIDTH + 6, BAR_HEIGHT + 6, DARKGRAY);
				DrawRectangle(100, 15, hp_percent * BAR_WIDTH, BAR_HEIGHT, GREEN);
				DrawRectangle(100 + hp_percent * BAR_WIDTH, 15, (1 - hp_percent) * BAR_WIDTH, BAR_HEIGHT, BLACK);

				const char* weaponName = "";

				switch (currentWeapon) { 
				case WeaponType::BULLET:
					weaponName = "BULLET";
					break;
				case WeaponType::LASER:
					weaponName = "LASER";
					break;
				case WeaponType::SHOTGUN:
					weaponName = "SHOTGUN";
					break;
				}

				DrawText(TextFormat("Weapon: %s", weaponName),         
					10, 40, 20, BLUE);

				DrawText(TextFormat("Score: %d", score),
					10, 70, 20, GOLD);

				for (const auto& projPtr : projectiles) {
					projPtr.Draw();
				}
				for (const auto& astPtr : asteroids) {
					astPtr->Draw();
				}

				player->Draw();

				Renderer::Instance().End();
			}
		}
	}

private:
	Application()
	{
		asteroids.reserve(1000);
		projectiles.reserve(10'000);
	};

	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;

	AsteroidShape currentShape = AsteroidShape::TRIANGLE;

	static constexpr int C_WIDTH = 1400;
	static constexpr int C_HEIGHT = 800;
	static constexpr int BAR_WIDTH = 0.2 * C_WIDTH;
	static constexpr int BAR_HEIGHT = 0.01 * C_HEIGHT;
	static constexpr size_t MAX_AST = 150;
	static constexpr float C_SPAWN_MIN = 0.5f;
	static constexpr float C_SPAWN_MAX = 3.0f;
	static constexpr int C_MAX_ASTEROIDS = 1000;
	static constexpr int C_MAX_PROJECTILES = 10'000;
	int score = 0;
};

int main() {
	Application::Instance().Run();
	return 0;
}
