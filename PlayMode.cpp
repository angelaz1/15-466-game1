#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

//for loading png:
#include "data_path.hpp"

#include <random>

PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	//Also, *don't* use these tiles in your game:

	{ //use tiles 0-16 as some weird dot pattern thing:
		std::array< uint8_t, 8*8 > distance;
		for (uint32_t y = 0; y < 8; ++y) {
			for (uint32_t x = 0; x < 8; ++x) {
				float d = glm::length(glm::vec2((x + 0.5f) - 4.0f, (y + 0.5f) - 4.0f));
				d /= glm::length(glm::vec2(4.0f, 4.0f));
				distance[x+8*y] = std::max(0,std::min(255,int32_t( 255.0f * d )));
			}
		}
		for (uint32_t index = 0; index < 16; ++index) {
			PPU466::Tile tile;
			uint8_t t = (255 * index) / 16;
			for (uint32_t y = 0; y < 8; ++y) {
				uint8_t bit0 = 0;
				uint8_t bit1 = 0;
				for (uint32_t x = 0; x < 8; ++x) {
					uint8_t d = distance[x+8*y];
					if (d > t) {
						bit0 |= (1 << x);
					} else {
						bit1 |= (1 << x);
					}
				}
				tile.bit0[y] = bit0;
				tile.bit1[y] = bit1;
			}
			ppu.tile_table[index] = tile;
		}
	}

	//use sprite 32 as a "player":
	ppu.tile_table[32].bit0 = {
		0b01111110,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b01111110,
	};
	ppu.tile_table[32].bit1 = {
		0b00000000,
		0b00000000,
		0b00011000,
		0b00100100,
		0b00000000,
		0b00100100,
		0b00000000,
		0b00000000,
	};

	//makes the outside of tiles 0-16 solid:
	// ppu.palette_table[0] = {
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	// 	glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	// };

	//makes the center of tiles 0-16 solid:
	ppu.palette_table[1] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//used for the player:
	ppu.palette_table[7] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0xff, 0xff, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//used for the misc other sprites:
	ppu.palette_table[6] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x88, 0x88, 0xff, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	};

	//  TODO: Load assets
	player_sprite = LoadedSprite(&ppu, data_path("assets/player.png"));
	box_sprite = LoadedSprite(&ppu, data_path("assets/box.png"));
	// for (uint8_t i = 0; i < xx; i++) {
	// 	background_tiles[i] = new LoadedSprite(data_path("background"+i));
	// }
	// TODO: Loading in lights, floor buttons, etc.


	// Create background
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			//TODO: make weird plasma thing
			ppu.background[x+PPU466::BackgroundWidth*y] = ((x+y)%16);
		}
	}

	// TODO: initialize grid_map

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

void PlayMode::push_box() {
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

	uint8_t new_col = box_col + box_travel_dir.x;
	uint8_t new_row = box_row + box_travel_dir.y;
	while (is_valid_pos(new_row, new_col)) {
		// Check if there's obstacles
		if (game_map[get_index(new_row, new_col)] != 0) {
			break;
		}
		box_row = new_row;
		box_col = new_col;
		box_target_dist += tile_size;

		new_col += box_travel_dir.x;
		new_row += box_travel_dir.y;
	}
}

void PlayMode::pull_box() {
	if (box_is_moving) {
		return;
	}

	box_is_moving = true;
	box_target_dist = 0;
	box_travel_time = 0;
	box_start = get_pos_vec(box_row, box_col);

	if (player_row < box_row) {
		// Move up
		box_travel_dir = glm::u8vec2(0, -1);
	}
	else if (player_row > box_row) {
		// Move down
		box_travel_dir = glm::u8vec2(0, 1);
	}
	else if (player_col < box_col) {
		// Move right
		box_travel_dir = glm::u8vec2(-1, 0);
	}
	else if (player_col > box_col) {
		// Move left
		box_travel_dir = glm::u8vec2(1, 0);
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
		box_target_dist += tile_size;

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
		push_box();
		return;
	}

	player_row = new_row;
	player_col = new_col;
	player_pos = get_pos_vec(player_row, player_col);
}

glm::u8vec2 PlayMode::get_pos_vec(uint8_t row, uint8_t col) {
	return glm::u8vec2(col * tile_size, row * tile_size);
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
				pull_box();
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
	player_sprite.draw(&ppu, &sprite_index, player_pos);
	box_sprite.draw(&ppu, &sprite_index, box_pos);

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
