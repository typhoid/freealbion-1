/*
 *  Albion Remake
 *  Copyright (C) 2007 - 2015 Florian Ziesche
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "map/npc.hpp"
#include <SDL2/SDL.h>

/*! npc constructor
 */
npc::npc() : char_type(CHARACTER_TYPE::NPC) {
	time_per_tile = TIME_PER_TILE_NPC;
	last_frame = last_anim_frame = last_move = SDL_GetTicks();
	npc_data = nullptr;
	enabled = true;
	pos_interp = 0.0f;
	clock_cb = nullptr;
	view_direction = MOVE_DIRECTION::NONE;
}

/*! npc destructor
 */
npc::~npc() {
	if(clock_cb_added) {
		clck->delete_tick_callback(clock_cb);
	}
}

void npc::set_npc_data(const map_npcs::map_npc* npc_data_) {
	npc::npc_data = npc_data_;
	
	// set start position from map data (-1, b/c we start at 0)
	if(npc_data->position[0].x == 0 || npc_data->position[0].y == 0) {
		// pos (0, 0) = disabled npc
		set_enabled(false);
		set_pos(0, 0);
	}
	else set_pos(npc_data->position[0].x-1, npc_data->position[0].y-1);
	
	// if the npc uses a track, add a clock callback
	if(npc_data->movement_type == MOVEMENT_TYPE::TRACK) {
		if(!clock_cb_added) {
			clock_cb = bind(&npc::clock_tick_cb, this, placeholders::_1);
			clck->add_tick_callback(ar_clock::CCBT_TICK, clock_cb);
			clock_cb_added = true;
		}
		time_per_tile = clck->get_ms_per_tick();
	}
	else if(npc_data->movement_type == MOVEMENT_TYPE::RANDOM && clock_cb_added) {
		clck->delete_tick_callback(clock_cb);
		clock_cb_added = false;
	}
}

unsigned int npc::get_object_num() const {
	if(npc_data == nullptr) return 0;
	return npc_data->object_num;
}

void npc::set_pos(const size_t& x, const size_t& y) {
	pos.set(x, y);
	next_pos.set(x, y);
}

const size2& npc::get_pos() const {
	return next_pos;
}

float2 npc::get_interpolated_pos() const {
	return float2 { pos } * (1.0f - pos_interp) + float2(next_pos) * pos_interp;
}

void npc::set_enabled(const bool& state) {
	enabled = state;
}

bool npc::is_enabled() const {
	return enabled;
}

void npc::compute_move() {
	if(!enabled) return;
	
	// compute next move
	if(npc_data->movement_type == MOVEMENT_TYPE::RANDOM) {
		// TODO: think of a better method ;)
		if(SDL_GetTicks() - last_move < time_per_tile) return;
		last_move = SDL_GetTicks();
		
		const size_t rand_dir = rand() % 8;
		MOVE_DIRECTION dir = MOVE_DIRECTION::NONE;
		switch(rand_dir) {
			case 0: dir = MOVE_DIRECTION::UP; break;
			case 1: dir = (MOVE_DIRECTION)(MOVE_DIRECTION::UP | MOVE_DIRECTION::RIGHT); break;
			case 2: dir = MOVE_DIRECTION::RIGHT; break;
			case 3: dir = (MOVE_DIRECTION)(MOVE_DIRECTION::DOWN | MOVE_DIRECTION::RIGHT); break;
			case 4: dir = MOVE_DIRECTION::DOWN; break;
			case 5: dir = (MOVE_DIRECTION)(MOVE_DIRECTION::DOWN | MOVE_DIRECTION::LEFT); break;
			case 6: dir = MOVE_DIRECTION::LEFT; break;
			case 7: dir = (MOVE_DIRECTION)(MOVE_DIRECTION::UP | MOVE_DIRECTION::LEFT); break;
			default: break;
		}
		move(dir);
	}
	else {
		// track movement is handled in clock_tick_cb
	}
}

void npc::clock_tick_cb(size_t tick) {
	if(npc_data->position[tick].x == 0 && npc_data->position[tick].y == 0) {
		// TODO: better npc disable when not visible!
		set_pos(0, 0);
		return;
	}
	
	// follow track dependent on game time (-> tick)
	move(npc_data->position[tick] - size2(1, 1));
}

bool npc::has_moved() const {
	return moved;
}

void npc::set_moved(const bool& state) {
	moved = state;
}

void npc::set_view_direction(const MOVE_DIRECTION& vdirection) {
	view_direction = vdirection;
}
