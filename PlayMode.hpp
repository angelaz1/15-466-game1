#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, r_key, f_key;

	//some weird background animation:
	float background_fade = 0.0f;

	// game grid:
	const static uint8_t grid_width = 20;
	const static uint8_t grid_height = 20;

	const static uint8_t tile_size = 8;

	// 0 - empty
	uint8_t game_map[grid_width][grid_height];

	//player position:
	uint8_t player_row = 0;
	uint8_t player_col = 0;
	glm::u8vec2 player_pos;

	//box position:
	uint8_t box_row;
	uint8_t box_col;
	glm::u8vec2 box_pos;

	//moving box animation:
	bool box_is_moving = false;
	float box_travel_time;
	glm::u8vec2 box_start;
	glm::u8vec2 box_travel_dir;
	uint8_t box_target_dist;
	const uint8_t box_travel_speed = 80;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;

private:
	// If possible, moves the player to the new row/col
	void move_player(uint8_t new_row, uint8_t new_col);

	// Pushes the box based on the current player and box positions
	void push_box();

	// Pulls the box based on the current player and box positions
	// FIXME: code is essentially the same as push box, can combine
	void pull_box();

	// Returns whether given row and column is valid on the grid
	bool is_valid_pos(uint8_t row, uint8_t col);

	// Given row/col, gets the position vector (x, y)
	glm::u8vec2 get_pos_vec(uint8_t row, uint8_t col);
};
