#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <unordered_map>

#include "load_save_png.hpp"
#include <iostream>

#define MAX_SPRITES_IN_LOADEDSPRITE 4

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	// Input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, r_key, f_key;

	// Some weird background animation:
	float background_fade = 0.0f;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;

	// Loaded Sprite struct
	struct LoadedSprite {
		void draw(PPU466 *ppu, uint8_t *sprite_index, glm::u8vec2 position) {
			for (auto sprite : sprites) {
				(*ppu).sprites[*sprite_index].x = position.x + sprite.x;
				(*ppu).sprites[*sprite_index].y = position.y + sprite.y;
				(*ppu).sprites[*sprite_index].attributes = sprite.attributes;
				(*ppu).sprites[*sprite_index].index = sprite.index;
				(*sprite_index)++;
			}
		}

		std::string name;
		std::array<PPU466::Sprite, MAX_SPRITES_IN_LOADEDSPRITE> sprites;
	};

private:
	// Game grid:
	const static uint8_t grid_start_x = 1;
	const static uint8_t grid_start_y = 1;

	const static uint8_t grid_width = 14;
	const static uint8_t grid_height = 10;

	const static uint8_t grid_tile_size = 16;

	// 0 - empty
	std::array<uint8_t, grid_height*grid_width> game_map;

	// Player position:
	uint8_t player_row = 0;
	uint8_t player_col = 0;
	glm::u8vec2 player_pos = glm::u8vec2();

	// Box position:
	uint8_t box_row = 0;
	uint8_t box_col = 0;
	glm::u8vec2 box_pos = glm::u8vec2();

	// Moving box animation:
	bool box_is_moving = false;
	float box_travel_time = 0.0f;
	glm::u8vec2 box_start = glm::u8vec2();
	glm::u8vec2 box_travel_dir = glm::u8vec2();
	uint8_t box_target_dist = 0;
	const uint8_t box_travel_speed = 160;

	// Memory game aspects
	// game state should loop between showing lights & reading input

	// If possible, moves the player to the new row/col
	void move_player(uint8_t new_row, uint8_t new_col);

	enum BoxAction {
		PushBox, PullBox
	};

	// Moves the box based on the given action, current player and box positions
	void move_box(BoxAction action);

	// Returns whether given row and column is valid on the grid
	bool is_valid_pos(uint8_t row, uint8_t col);

	// Given row/col, gets the position vector (x, y)
	glm::u8vec2 get_pos_vec(uint8_t row, uint8_t col);

	// Given row/col, gets the index in the grid
	uint get_index(uint8_t row, uint8_t col);

	// A dictionary containing mappings from sprite name -> loaded sprite
	std::unordered_map<std::string, LoadedSprite> sprite_mapping;
};
