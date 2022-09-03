#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

//for loading chunks:
#include "data_path.hpp"
#include "read_write_chunk.hpp"

#include <fstream>
#include <iostream>
#include <random>

#define TILE_WIDTH 8

PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	//Also, *don't* use these tiles in your game:

	// { //use tiles 0-16 as some weird dot pattern thing:
	// 	std::array< uint8_t, 8*8 > distance;
	// 	for (uint32_t y = 0; y < 8; ++y) {
	// 		for (uint32_t x = 0; x < 8; ++x) {
	// 			float d = glm::length(glm::vec2((x + 0.5f) - 4.0f, (y + 0.5f) - 4.0f));
	// 			d /= glm::length(glm::vec2(4.0f, 4.0f));
	// 			distance[x+8*y] = std::max(0,std::min(255,int32_t( 255.0f * d )));
	// 		}
	// 	}
	// 	for (uint32_t index = 0; index < 16; ++index) {
	// 		PPU466::Tile tile;
	// 		uint8_t t = (255 * index) / 16;
	// 		for (uint32_t y = 0; y < 8; ++y) {
	// 			uint8_t bit0 = 0;
	// 			uint8_t bit1 = 0;
	// 			for (uint32_t x = 0; x < 8; ++x) {
	// 				uint8_t d = distance[x+8*y];
	// 				if (d > t) {
	// 					bit0 |= (1 << x);
	// 				} else {
	// 					bit1 |= (1 << x);
	// 				}
	// 			}
	// 			tile.bit0[y] = bit0;
	// 			tile.bit1[y] = bit1;
	// 		}
	// 		ppu.tile_table[index] = tile;
	// 	}
	// }

	// //use sprite 32 as a "player":
	// ppu.tile_table[32].bit0 = {
	// 	0b01111110,
	// 	0b11111111,
	// 	0b11111111,
	// 	0b11111111,
	// 	0b11111111,
	// 	0b11111111,
	// 	0b11111111,
	// 	0b01111110,
	// };
	// ppu.tile_table[32].bit1 = {
	// 	0b00000000,
	// 	0b00000000,
	// 	0b00011000,
	// 	0b00100100,
	// 	0b00000000,
	// 	0b00100100,
	// 	0b00000000,
	// 	0b00000000,
	// };

	// //makes the outside of tiles 0-16 solid:
	// // ppu.palette_table[0] = {
	// // 	glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	// // 	glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	// // 	glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	// // 	glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	// // };

	// //makes the center of tiles 0-16 solid:
	// ppu.palette_table[1] = {
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	// };

	// //used for the player:
	// ppu.palette_table[7] = {
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	// 	glm::u8vec4(0xff, 0xff, 0x00, 0xff),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	// };

	// //used for the misc other sprites:
	// ppu.palette_table[6] = {
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	// 	glm::u8vec4(0x88, 0x88, 0xff, 0xff),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	// };

	// Read in asset chunks
	// Referenced documentation for istream https://cplusplus.com/reference/istream/istream/istream/
	std::filebuf fb;
	uint8_t index;

	std::cout << "Loading palettes\n";
	{ // Load Palettes
		std::vector< PPU466::Palette > palette_table = std::vector< PPU466::Palette >();
		fb.open(data_path("resources/palette_table.chunk"), std::ios::in);
		std::istream palette_is(&fb);
		read_chunk(palette_is, "pltt", &palette_table);
		fb.close();

		index = 0;
		for (PPU466::Palette palette : palette_table) {
			ppu.palette_table[index] = palette;
			index++;
		}
	}
    
	std::cout << "Loading tiles\n";
	{ // Load Tiles
		std::vector< PPU466::Tile > tile_table = std::vector< PPU466::Tile >();
		fb.open(data_path("resources/tile_table.chunk"), std::ios::in);
		std::istream tile_is(&fb);
		read_chunk(tile_is, "tile", &tile_table);
		fb.close();

		index = 0;
		for (PPU466::Tile tile : tile_table) {
			ppu.tile_table[index] = tile;
			index++;
		}
	}
    
	std::cout << "Loading sprites\n";
	{ // Load sprites
		std::vector< LoadedSprite > sprite_table = std::vector< LoadedSprite >();
		fb.open(data_path("resources/game_sprites.chunk"), std::ios::in);
		std::istream sprite_is(&fb);
		read_chunk(sprite_is, "sprt", &sprite_table);
		fb.close();

		sprite_mapping = std::unordered_map<std::string, LoadedSprite>();		
		for (LoadedSprite sprite : sprite_table) {
			sprite_mapping[sprite.name] = sprite;
		}
	}

	std::cout << "Finished loading assets\n";

	// Create background
	LoadedSprite floor_sprite = sprite_mapping["floor"];
	LoadedSprite wall_sprite = sprite_mapping["wall"];

	uint8_t tile_incr = grid_tile_size / TILE_WIDTH;

	// Draw walls
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; y += tile_incr) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; x += tile_incr) {
			for (auto sprite : wall_sprite.sprites) {
				uint32_t x_pos = x + sprite.x / TILE_WIDTH;
				uint32_t y_pos = y + sprite.y / TILE_WIDTH;

				if (x_pos < PPU466::BackgroundWidth && y_pos < PPU466::BackgroundHeight) {
					ppu.background[x_pos + PPU466::BackgroundWidth * y_pos] = (sprite.attributes << 8) | sprite.index;
				}
			}
		}
	}

	// Draw floor
	for (uint32_t y = 0; y < grid_height; y++) {
		for (uint32_t x = 0; x < grid_width; x++) {
			for (auto sprite : floor_sprite.sprites) {
				uint32_t x_pos = ((grid_start_x + x) * tile_incr) + sprite.x / TILE_WIDTH;
				uint32_t y_pos = ((grid_start_y + y) * tile_incr) + sprite.y / TILE_WIDTH;

				if (x_pos < PPU466::BackgroundWidth && y_pos < PPU466::BackgroundHeight) {
					ppu.background[x_pos + PPU466::BackgroundWidth * y_pos] = (sprite.attributes << 8) | sprite.index;
				}
			}
		}
	}

	// Initialize grid_map
	game_map = std::array<uint8_t, grid_height*grid_width>();

	// Initialize player position
	player_pos = get_pos_vec(player_row, player_col);

	// Initialize box position
	box_row = (rand() % grid_height - 1) + 1;
	box_col = (rand() % grid_width - 1) + 1;
	box_pos = get_pos_vec(box_row, box_col);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			r_key.downs += 1;
			r_key.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_f) {
			f_key.downs += 1;
			f_key.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::move_box(BoxAction boxAction) {
	if (box_is_moving) {
		return;
	}

	box_is_moving = true;
	box_target_dist = 0;
	box_travel_time = 0;
	box_start = get_pos_vec(box_row, box_col);

	if (player_row < box_row) {
		// Move up
		box_travel_dir = glm::u8vec2(0, 1);
	}
	else if (player_row > box_row) {
		// Move down
		box_travel_dir = glm::u8vec2(0, -1);
	}
	else if (player_col < box_col) {
		// Move right
		box_travel_dir = glm::u8vec2(1, 0);
	}
	else if (player_col > box_col) {
		// Move left
		box_travel_dir = glm::u8vec2(-1, 0);
	}

	if (boxAction == PullBox) {
		box_travel_dir = -box_travel_dir;
	}

	uint8_t new_col = box_col + box_travel_dir.x;
	uint8_t new_row = box_row + box_travel_dir.y;
	while (is_valid_pos(new_row, new_col)) {
		// Check if there's obstacles
		if (game_map[get_index(new_row, new_col)] != 0) {
			break;
		}

		if (new_row == player_row && new_col == player_col) {
			break;
		}

		box_row = new_row;
		box_col = new_col;
		box_target_dist += grid_tile_size;

		new_col += box_travel_dir.x;
		new_row += box_travel_dir.y;
	}
}

bool PlayMode::is_valid_pos(uint8_t row, uint8_t col) {
	return row < grid_height && col < grid_width;
}

void PlayMode::move_player(uint8_t new_row, uint8_t new_col) {
	if (!is_valid_pos(new_row, new_col)) {
		return;
	}

	if (new_row == box_row && new_col == box_col) {
		move_box(PushBox);
		return;
	}

	player_row = new_row;
	player_col = new_col;
	player_pos = get_pos_vec(player_row, player_col);
}

glm::u8vec2 PlayMode::get_pos_vec(uint8_t row, uint8_t col) {
	return glm::u8vec2((grid_start_x + col) * grid_tile_size, (grid_start_y + row) * grid_tile_size);
}

uint PlayMode::get_index(uint8_t row, uint8_t col) {
	return row * grid_width + col;
}

void PlayMode::update(float elapsed) {
	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	if (!box_is_moving) {
		if (left.downs) move_player(player_row, player_col - 1);
		else if (right.downs) move_player(player_row, player_col + 1);
		else if (down.downs) move_player(player_row - 1, player_col);
		else if (up.downs) move_player(player_row + 1, player_col);
		else if (r_key.downs) {
			// FIXME: remove this, just for testing
			box_row = (rand() % grid_height - 1) + 1;
			box_col = (rand() % grid_width - 1) + 1;
			box_pos = get_pos_vec(box_row, box_col);
		}
		else if (f_key.downs) {
			// Pull box
			if (box_row == player_row || box_col == player_col) {
				move_box(PullBox);
			}
		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	r_key.downs = 0;
	f_key.downs = 0;

	if (box_is_moving) {
		box_travel_time += elapsed;
		uint8_t travel_dist = static_cast <uint8_t> (std::floor(box_travel_time * box_travel_speed));
		if (travel_dist >= box_target_dist) {
			box_pos = box_start + box_target_dist * box_travel_dir;
			box_is_moving = false;
		}
		else {
			box_pos = box_start + travel_dist * box_travel_dir;
		}
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
		0xff
	);

	// //background scroll:
	// ppu.background_position.x = int32_t(-0.5f * player_at.x);
	// ppu.background_position.y = int32_t(-0.5f * player_at.y);

	uint8_t sprite_index = 0;
	sprite_mapping["player"].draw(&ppu, &sprite_index, player_pos);
	sprite_mapping["box"].draw(&ppu, &sprite_index, box_pos);

	// //some other misc sprites:
	// for (uint32_t i = 1; i < 63; ++i) {
	// 	float amt = (i + 2.0f * background_fade) / 62.0f;
	// 	ppu.sprites[i].x = int32_t(0.5f * PPU466::ScreenWidth + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * PPU466::ScreenWidth);
	// 	ppu.sprites[i].y = int32_t(0.5f * PPU466::ScreenHeight + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * PPU466::ScreenWidth);
	// 	ppu.sprites[i].index = 32;
	// 	ppu.sprites[i].attributes = 6;
	// 	if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	// }

	//--- actually draw ---
	ppu.draw(drawable_size);
}
