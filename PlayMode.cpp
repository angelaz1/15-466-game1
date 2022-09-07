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

	// Store light sprites
	lightSpriteOrder = std::vector<std::string>();
	lightSpriteOrder.push_back("red_light");
	lightSpriteOrder.push_back("blue_light");
	lightSpriteOrder.push_back("yellow_light");
	lightSpriteOrder.push_back("green_light");

	// Initialize grid_map
	game_map = std::array<GridContents, grid_height*grid_width>();

	// Initialize player position
	player_pos = get_pos_vec(player_row, player_col, true);

	// Initialize buttons
	randomize_buttons();
	
	// Initialize box position
	do {
		box_row = (rand() % grid_height - 1) + 1;
		box_col = (rand() % grid_width - 1) + 1;
	} while (game_map[get_index(box_row, box_col)] != CellEmpty);
	box_pos = get_pos_vec(box_row, box_col, true);

	// Start game on memorizing
	gameState = MemorizeSequence;
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

void PlayMode::randomize_buttons() {
	for (uint i = 0; i < grid_width * grid_height; i++) {
		game_map[i] = CellEmpty;
	}

	for (uint8_t i = 0; i < num_lights; i++) {
		uint8_t button_row, button_col;
		do {
			button_row = (rand() % grid_height - 1) + 1;
			button_col = (rand() % grid_width - 1) + 1;
		} while (game_map[get_index(button_row, button_col)] != CellEmpty);

		game_map[get_index(button_row, button_col)] = GridContents(i + 1);
	}
}

void PlayMode::move_box(BoxAction boxAction) {
	if (box_is_moving) {
		return;
	}

	box_is_moving = true;
	box_target_dist = 0;
	box_travel_time = 0;
	box_start = get_pos_vec(box_row, box_col, true);

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
	player_pos = get_pos_vec(player_row, player_col, true);
}

PlayMode::LightColor PlayMode::get_light_from_button(GridContents cell) {
	switch (cell) {
		case RedButton: return LightRed;
		case BlueButton: return LightBlue;
		case YellowButton: return LightYellow;
		case GreenButton: return LightGreen;
		default: break;
	}
	return LightOff;
}

glm::u8vec2 PlayMode::get_pos_vec(uint8_t row, uint8_t col, bool use_grid_start) {
	if (use_grid_start) {
		return glm::u8vec2((grid_start_x + col) * grid_tile_size, (grid_start_y + row) * grid_tile_size);
	}
	return glm::u8vec2(col * grid_tile_size, row * grid_tile_size);
}

uint PlayMode::get_index(uint8_t row, uint8_t col) {
	return row * grid_width + col;
}

void PlayMode::read_player_input() {
	if (!box_is_moving) {
		if (left.downs) move_player(player_row, player_col - 1);
		else if (right.downs) move_player(player_row, player_col + 1);
		else if (down.downs) move_player(player_row - 1, player_col);
		else if (up.downs) move_player(player_row + 1, player_col);
		else if (f_key.downs) {
			if (box_row == player_row || box_col == player_col) {
				// Pull box
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
}

void PlayMode::generate_light_sequence() {
	player_lights = std::vector<LightColor>();
	player_light_count = 0;
	prev_player_light_count = 0;

	// Generate lights order
	if (!failed_sequence) {
		randomize_buttons();
		light_sequence_count++;
		light_sequence_count = std::min(light_sequence_count, max_light_sequence_count);
		lights_order = std::vector<LightColor>();
		for (uint8_t i = 0; i < light_sequence_count; i++) {
			lights_order.push_back(LightColor((rand() % num_lights)));
		}
	}

	showing_lights = true;
	showing_lights_index = 0;
	showing_light_time = 0;
	light_state = LightShowFeedback;
}

void PlayMode::show_lights(float elapsed) {	
	current_light = LightOff;

	if (showing_lights_index >= light_sequence_count) {
		showing_lights = false;
		gameState = RepeatSequence;
		return;
	}

	showing_light_time += elapsed;

	switch (light_state) {
		case LightShowSequence: {
			current_light = lights_order[showing_lights_index];
			if (showing_light_time >= light_show_time) {
				showing_lights_index++;
				showing_light_time = 0;
				light_state = LightShowBreak;
			}
			break;
		}
		case LightShowBreak: {
			if (showing_light_time >= light_break_time) {
				showing_light_time = 0;
				light_state = LightShowSequence;
			}
			break;
		}
		case LightShowFeedback: {
			current_light = failed_sequence ? AllLightsRed : AllLightsGreen;

			if (showing_light_time >= light_break_time) {
				if (lives == 0) {
					end_game();
				}
				showing_light_time = 0;
				light_state = LightShowBreak;
			}
			break;
		}
		default: break;
	}
}

void PlayMode::end_game() {
	std::cout << "\n------------------------------\n";
	std::cout << "Thanks for playing KaraSoko!\n";
	std::cout << "Your score was: " << score << "\n";
	std::cout << "------------------------------\n\n";
	Mode::set_current(nullptr);	
}

void PlayMode::update(float elapsed) {
	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	switch (gameState) {
		case MemorizeSequence:
			if (!showing_lights) {
				generate_light_sequence();
			}
			show_lights(elapsed);
			break;
		case RepeatSequence:
			read_player_input();

			current_light = LightOff;
			if (player_light_count > prev_player_light_count) {
				current_light = player_lights[prev_player_light_count];
				showing_light_time += elapsed;

				if (showing_light_time >= light_break_time) {
					showing_light_time = 0;
					prev_player_light_count++;
				}
			}
			break;
	}

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

		// Check for button press
		uint8_t box_row = box_pos.y / grid_tile_size - grid_start_y;
		uint8_t box_col = box_pos.x / grid_tile_size - grid_start_x;
		uint current_index = get_index(box_row, box_col);
		if (current_index != box_last_index && game_map[current_index] != CellEmpty && gameState == RepeatSequence) {
			// Count button press
			LightColor hit_light = get_light_from_button(game_map[current_index]);
			if (lights_order[player_light_count] != hit_light) {
				failed_sequence = true;
				lives--;
				gameState = MemorizeSequence;
			} else {
				player_lights.push_back(hit_light);
				player_light_count++;

				if (player_light_count == light_sequence_count) {
					failed_sequence = false;
					score++;
					gameState = MemorizeSequence;
				}
			}

		}
		box_last_index = current_index;
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

	uint8_t sprite_index = 0;

	// Draw buttons 
	for (uint8_t r = 0; r < grid_height; r++) {
		for (uint8_t c = 0; c < grid_width; c++) {
			switch (game_map[get_index(r, c)]) {
				case RedButton:
					sprite_mapping["red_light"].draw(&ppu, &sprite_index, get_pos_vec(r, c, true));
					break;
				case BlueButton:
					sprite_mapping["blue_light"].draw(&ppu, &sprite_index, get_pos_vec(r, c, true));
					break;
				case YellowButton:
					sprite_mapping["yellow_light"].draw(&ppu, &sprite_index, get_pos_vec(r, c, true));
					break;
				case GreenButton:
					sprite_mapping["green_light"].draw(&ppu, &sprite_index, get_pos_vec(r, c, true));
					break;
				default: break;
			}
		}
	}

	sprite_mapping["player"].draw(&ppu, &sprite_index, player_pos);
	sprite_mapping["box"].draw(&ppu, &sprite_index, box_pos);

	// Draw lights
	for (uint8_t i = 0; i < num_lights; i++) {
		LoadedSprite light_sprite = sprite_mapping["off_light"];

		if (current_light == AllLightsRed) {
			light_sprite = sprite_mapping["red_light"];
		}
		else if (current_light == AllLightsGreen) {
			light_sprite = sprite_mapping["green_light"];
		} 
		else if (current_light == i) {
			light_sprite = sprite_mapping[lightSpriteOrder[i]];
		}

		light_sprite.draw(&ppu, &sprite_index, get_pos_vec(29, 3 + i * 3, false));
	}

	// Draw lives
	for (uint8_t i = 0; i < lives; i++) {
		sprite_mapping["heart"].draw(&ppu, &sprite_index, get_pos_vec(0, i, false));
	}
	for (uint8_t i = lives; i < max_lives; i++) {
		sprite_mapping["heart"].draw(&ppu, &sprite_index, get_pos_vec(31, 31, false)); // Off-screen
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
