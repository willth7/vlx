//   Copyright 2022 Will Thomas
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#ifndef _GFX_GFX_H
#define _GFX_GFX_H

#include <stdint.h>

struct vlx_context;

struct vlx_buffer;

struct vlx_image;

struct vlx_surface;

struct vlx_command;

struct vlx_pipeline;

struct vlx_vertex;

struct vlx_texture;

struct vlx_descriptor;

struct vlx_context* vlx_context_create(int8_t);

struct vlx_surface* vlx_surface_create(struct vlx_context*, void*, void*, uint16_t, uint16_t);

struct vlx_command* vlx_command_create(struct vlx_context*);

void vlx_render_pass_init(struct vlx_context*, struct vlx_surface*);

void vlx_swapchain_init(struct vlx_context*, struct vlx_surface*);

void vlx_depth_buffer_init(struct vlx_context*, struct vlx_surface*);

void vlx_frame_buffer_init(struct vlx_context*, struct vlx_surface*);

struct vlx_pipeline* vlx_pipeline_create(struct vlx_context*, struct vlx_surface*, int8_t*, int8_t*, struct vlx_vertex*, struct vlx_descriptor*, uint64_t);

void vlx_buffer_refresh(struct vlx_context*, struct vlx_buffer*, void*, uint64_t);

struct vlx_vertex* vlx_vertex_create(struct vlx_context*, uint32_t, uint32_t, uint64_t);

void vlx_vertex_bind(struct vlx_vertex*, uint32_t, uint32_t);

void vlx_vertex_attr(struct vlx_vertex*, uint32_t, uint32_t, int8_t, uint32_t);

void vlx_vertex_conf(struct vlx_vertex*);

void vlx_vertex_refresh(struct vlx_context*, struct vlx_vertex*, uint32_t, void*, uint64_t);

struct vlx_buffer* vlx_index_create(struct vlx_context*, uint64_t);

struct vlx_buffer* vlx_uniform_create(struct vlx_context*, uint64_t);

struct vlx_texture* vlx_texture_create(struct vlx_context*, struct vlx_command*, uint8_t*, uint32_t, uint32_t);

struct vlx_descriptor* vlx_dscr_create(struct vlx_context*, uint32_t);

void vlx_descriptor_write(struct vlx_context*, struct vlx_descriptor*, uint32_t, struct vlx_buffer*, void*, uint64_t, struct vlx_texture*);

void vlx_surface_clear(struct vlx_surface*, uint8_t, uint8_t, uint8_t, uint8_t);

void vlx_surface_new_frame(struct vlx_context*, struct vlx_surface*, struct vlx_command*);

void vlx_surface_draw_frame(struct vlx_context*, struct vlx_surface*, struct vlx_pipeline*, struct vlx_command*, struct vlx_buffer*, struct vlx_vertex*, struct vlx_descriptor*, void*, uint64_t, uint32_t, uint32_t, uint32_t);

void vlx_surface_swap_frame(struct vlx_context*, struct vlx_surface*, struct vlx_command*);

void vlx_surface_resize(struct vlx_context*, struct vlx_surface*, uint32_t, uint32_t);

void vlx_buffer_destroy(struct vlx_context*, struct vlx_buffer*);

void vlx_image_destroy(struct vlx_context*, struct vlx_image*);

void vlx_vertex_destroy(struct vlx_context*, struct vlx_vertex*);

void vlx_texture_destroy(struct vlx_context*, struct vlx_texture*);

void vlx_descriptor_destroy(struct vlx_context*, struct vlx_descriptor*);

void vlx_pipeline_destroy(struct vlx_context*, struct vlx_pipeline*);

void vlx_command_destroy(struct vlx_context*, struct vlx_command*);

void vlx_surface_destroy(struct vlx_context*, struct vlx_surface*);

void vlx_context_destroy(struct vlx_context*);

#endif
