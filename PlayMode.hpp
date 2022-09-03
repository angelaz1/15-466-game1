#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

#include "load_save_png.hpp"
#include <iostream>

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

	// Game grid:
	const static uint8_t grid_width = 10;
	const static uint8_t grid_height = 10;

	const static uint8_t tile_size = 16;

	// 0 - empty
	std::array<uint8_t, grid_height*grid_width> game_map = std::array<uint8_t, grid_height*grid_width>();

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

	// Given row/col, gets the index in the grid
	uint get_index(uint8_t row, uint8_t col);

	struct LoadedSprite {
		LoadedSprite() {}
		~LoadedSprite() {}	

		LoadedSprite(PPU466 *ppu, std::string file_path) {
			glm::uvec2 file_size = glm::uvec2();
			std::vector< glm::u8vec4 > data = std::vector< glm::u8vec4 >();

			load_png(file_path, &file_size, &data, LowerLeftOrigin);

			uint8_t color_count = 0;
			PPU466::Palette palette = PPU466::Palette();
			
			static uint32_t tile_index = 0;
			static uint32_t palette_index = 0;

			for (auto pixel : data) {
				if (std::find(palette.begin(), palette.end(), pixel) == palette.end()) {
					palette[color_count] = glm::u8vec4(pixel[0], pixel[1], pixel[2], pixel[3]);
					color_count++;
				}
				if (color_count > 4) {
					throw std::runtime_error("Too many colors in sprite!");
				}
			}

			ppu->palette_table[palette_index] = palette;

			sprites = std::vector<PPU466::Sprite>();

			for (uint r = 0; r < file_size.y; r += 8) {
				for (uint c = 0; c < file_size.x; c += 8) {
					// Start of a new tile
					PPU466::Tile tile = PPU466::Tile();
					PPU466::Sprite sprite = PPU466::Sprite();

					for (uint8_t r_i = 0; r_i < 8; r_i++) {
						uint8_t bit0 = 0;
						uint8_t bit1 = 0;

						for (uint8_t c_i = 0; c_i < 8; c_i++) {
							uint row = r + r_i;
							uint col = c + c_i;

							if (row >= file_size.y || col >= file_size.x) {
								// No need to fill
								break;
							}

							uint data_index = row * file_size.x + col;
							glm::u8vec4 pixel = data[data_index];

							auto find_itr = std::find(palette.begin(), palette.end(), pixel);
							uint8_t palette_index = std::distance(palette.begin(), find_itr);

							bit0 |= ((palette_index & 1) << c_i);
							bit1 |= ((palette_index & 2) >> 1 << c_i);
						}

						tile.bit0[r_i] = bit0;
						tile.bit1[r_i] = bit1;
					}
					
					ppu->tile_table[tile_index] = tile;

					sprite.x = c;
					sprite.y = r;
					sprite.index = tile_index;
					sprite.attributes = palette_index;

					sprites.push_back(sprite);
					
					tile_index++;
				}
			}

			palette_index++;
		};

		void draw(PPU466 *ppu, uint8_t *sprite_index, glm::u8vec2 position) {
			for (auto sprite : sprites) {
				(*ppu).sprites[*sprite_index].x = position.x + sprite.x;
				(*ppu).sprites[*sprite_index].y = position.y + sprite.y;
				(*ppu).sprites[*sprite_index].attributes = sprite.attributes;
				(*ppu).sprites[*sprite_index].index = sprite.index;
				(*sprite_index)++;
			}
		}

		std::vector<PPU466::Sprite> sprites;
	};

	LoadedSprite player_sprite = LoadedSprite();
	LoadedSprite box_sprite = LoadedSprite();
};
