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

/* vlx_context 
 * 
 * The context is a structure containing objects and values shared among all functions and objects. There should be one context for the 
 * application. 
 **/

struct vlx_context;

/* vlx_buffer 
 * 
 * Generic buffer object that holds shared memory between the application and the GPU. Buffers can be created and destroyed at any point 
 * during runtime. 
 **/

struct vlx_buffer;

/* vlx_image 
 * 
 * Generic image object that holds pixel data shared between the application and the GPU. Images can be created and destroyed at any point 
 * during runtime. 
 **/

struct vlx_image;

/* vlx_surface 
 * 
 * The surface is a structure containing objects and values needed to render to a wayland client. There should be one surface per 
 * application window.
 **/

struct vlx_surface;

/* vlx_command 
 * 
 * The command buffer is the means of sending draw calls to the GPU. There should be one command buffer per application thread. 
 **/

struct vlx_command;

/* vlx_pipeline 
 * 
 * The pipeline is the means of transfering information, such as buffers and shaders, to the GPU. There can be multiple pipelines used during 
 * a frame. 
 **/

struct vlx_pipeline;

/* vlx_vertex 
 * 
 * The vertex structure contains the vertex buffer, a buffer for sharing vertices between the application and the vertex shader, and 
 * descriptions for bindings and attributes. A vertex structure can be bound to multiple heaps of memory. 
 **/

struct vlx_vertex;

/* vlx_texture 
 * 
 * The texture structure holds a vlx_image object that can be attached to a vlx_descriptor object. There should be one texture per 
 * descriptor. 
 **/

struct vlx_texture;

/* vlx_descriptor 
 * 
 * The descriptor structure contains descriptors that can be used to send textures and uniform buffers to shaders. There should be one 
 * descriptor object per pipeline. 
 **/

struct vlx_descriptor;

/* vlx_context_create 
 * 
 * int8_t					boolean for non-linear color scheme 
 * 
 * Creates a Vulkan context for the application. 
 **/

struct vlx_context* vlx_context_create(int8_t);

/* vlx_surface_create 
 * 
 * struct vlx_context*		Vulkan context 
 * void*					Wayland display (wl_display) 
 * void*					Wayland surface (wl_surface) 
 * uint16_t					width 
 * uint16_t					height 
 * 
 * Creates a Vulkan surface from a Wayland client surface. Should be called per application window. 
 **/

struct vlx_surface* vlx_surface_create(struct vlx_context*, void*, void*, uint16_t, uint16_t);

/* vlx_command_create 
 * 
 * struct vlx_context*		Vulkan context 
 * 
 * Creates command pool and buffer. 
 **/

struct vlx_command* vlx_command_create(struct vlx_context*);

/* vlx_surface_init_render_pass 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * 
 * Initializes render pass for a surface. 
 **/

void vlx_surface_init_render_pass(struct vlx_context*, struct vlx_surface*);

/* vlx_surface_init_swapchain 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * 
 * Initializes swapchain for a surface. 
 **/

void vlx_surface_init_swapchain(struct vlx_context*, struct vlx_surface*);

/* vlx_surface_init_depth_buffer 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * 
 * Initializes depth buffer for a surface. 
 **/

void vlx_surface_init_depth_buffer(struct vlx_context*, struct vlx_surface*);

/* vlx_surface_init_frame_buffer 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * 
 * Initializes frame buffer for a surface. 
 **/

void vlx_surface_init_frame_buffer(struct vlx_context*, struct vlx_surface*);

/* vlx_pipeline_create 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * int8_t*					path to the vertex shader 
 * int8_t*					path to the fragment shader 
 * struct vlx_vertex*		vertex structure 
 * struct vlx_descriptor*	descriptor structure 
 * 
 * Creates pipeline. 
 **/

struct vlx_pipeline* vlx_pipeline_create(struct vlx_context*, struct vlx_surface*, int8_t*, int8_t*, struct vlx_vertex*, struct vlx_descriptor*, uint64_t);

/* vlx_buffer_refresh 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_buffer*		Vulkan buffer 
 * void*					memory 
 * uint64_t					size of memory to refresh 
 * 
 * Refreshes Vulkan buffer. This function can be used to transfer data from CPU memory to GPU memory. 
 **/

void vlx_buffer_refresh(struct vlx_context*, struct vlx_buffer*, void*, uint64_t);

/* vlx_vertex_create 
 * 
 * struct vlx_context*		Vulkan context 
 * uint32_t					number of bindings 
 * uint32_t					number of attributes 
 * uint64_t					size of vertex buffer 
 * 
 * Creates vertex structure.
 **/

struct vlx_vertex* vlx_vertex_create(struct vlx_context*, uint32_t, uint32_t, uint64_t);

/* vlx_vertex_bind 
 * 
 * struct vlx_context*		Vulkan context 
 * uint32_t					binding index 
 * uint32_t					binding stride 
 * 
 * Sets the stride of a binding in the vertex buffer. Should be called per binding. 
 **/

void vlx_vertex_bind(struct vlx_vertex*, uint32_t, uint32_t);

/* vlx_vertex_attr 
 * 
 * struct vlx_context*		Vulkan context 
 * uint32_t					attribute location 
 * uint32_t					attribute binding 
 * int8_t					attribute format 
 * uint32_t					attribute offset 
 * 
 * Sets the binding, format, and offset of an attribute in the vertex buffer. The format is represented as the number of bytes of the 
 * attribute, with a negative value representing a signed format, and a positive value representing an unsigned format. Should be called per 
 * attribute.
 **/

void vlx_vertex_attr(struct vlx_vertex*, uint32_t, uint32_t, int8_t, uint32_t);

/* vlx_vertex_conf 
 * 
 * struct vlx_vertex*		vertex structure 
 * 
 * Configures vertex structure. Should be called after all bindings and attributes have been set. 
 **/

void vlx_vertex_conf(struct vlx_vertex*);

/* vlx_vertex_refresh
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_vertex*		vertex structure 
 * void*					memory 
 * uint64_t					size of memory to refresh 
 * uint64_t					offset of memory to refresh 
 * 
 * Refreshes vertex buffer. This function can be used to transfer data from CPU memory to GPU memory. 
 **/

void vlx_vertex_refresh(struct vlx_context*, struct vlx_vertex*, uint32_t, void*, uint64_t);

/* vlx_index_create 
 * 
 * struct vlx_context*		Vulkan context 
 * uint64_t					size of buffer 
 * 
 * Creates an index buffer. 
 **/

struct vlx_buffer* vlx_index_create(struct vlx_context*, uint64_t);

/* vlx_uniform_create 
 * 
 * struct vlx_context*		Vulkan context 
 * uint64_t					size of buffer 
 * 
 * Creates a uniform buffer. 
 **/

struct vlx_buffer* vlx_uniform_create(struct vlx_context*, uint64_t);

/* vlx_texture_create 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_command*		command structure 
 * uint8_t*					pixel data (rgba)
 * uint32_t					width 
 * uint32_t					height 
 * 
 * Creates texture.
 **/

struct vlx_texture* vlx_texture_create(struct vlx_context*, struct vlx_command*, uint8_t*, uint32_t, uint32_t);

/* vlx_descriptor_create 
 * 
 * struct vlx_context*		Vulkan context 
 * uint32_t					number of descriptors 
 * 
 * Creates descriptors.
 **/

struct vlx_descriptor* vlx_descriptor_create(struct vlx_context*, uint32_t);

/* vlx_descriptor_write 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_descriptor*	descriptor structure 
 * uint32_t					descriptor index
 * struct vlx_buffer*		uniform buffer 
 * void*					uniform memory 
 * uint64_t					uniform size 
 * struct vlx_texture*		texture 
 * 
 * Writes descriptors. 
 **/

void vlx_descriptor_write(struct vlx_context*, struct vlx_descriptor*, uint32_t, struct vlx_buffer*, void*, uint64_t, struct vlx_texture*);

/* vlx_surface_clear 
 * 
 * struct vlx_context*		Vulkan context 
 * uint8_t					red value 
 * uint8_t					blue value 
 * uint8_t					green value 
 * 
 * Sets the clear color for the surface. 
 **/

void vlx_surface_clear(struct vlx_surface*, uint8_t, uint8_t, uint8_t);

/* vlx_surface_new_frame 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * struct vlx_command*		command structure 
 * 
 * Signals new surface frame to the command buffer. Should be called once per frame before any drawing. 
 **/

void vlx_surface_new_frame(struct vlx_context*, struct vlx_surface*, struct vlx_command*);

/* vlx_surface_draw_frame 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * struct vlx_pipeline*		Vulkan pipeline 
 * struct vlx_command*		command structure 
 * struct vlx_buffer*		index buffer 
 * struct vlx_vertex*		vertex buffer 
 * struct vlx_descriptor*	descriptor structure 
 * void*					push constant
 * uint64_t					size of push constanst 
 * uint32_t					number of indices 
 * uint32_t					index offset 
 * uint32_t					vertex offset 
 * 
 * Draws frame given pipeline and buffers. Can be called multiple times per frame. 
 **/

void vlx_surface_draw_frame(struct vlx_context*, struct vlx_surface*, struct vlx_pipeline*, struct vlx_command*, struct vlx_buffer*, struct vlx_vertex*, struct vlx_descriptor*, void*, uint64_t, uint32_t, uint32_t, uint32_t);

/* vlx_surface_swap_frame 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * struct vlx_command*		command structure 
 * 
 * Swaps frame buffers. Should be called once per frame after all drawing is done. 
 **/

void vlx_surface_swap_frame(struct vlx_context*, struct vlx_surface*, struct vlx_command*);

/* vlx_surface_resize 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * uint32_t					width 
 * uint32_t					height 
 * 
 * Resizes surface. 
 **/

void vlx_surface_resize(struct vlx_context*, struct vlx_surface*, uint32_t, uint32_t);

/* vlx_buffer_destroy 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_buffer*		buffer 
 * 
 * Frees buffer resources. 
 **/

void vlx_buffer_destroy(struct vlx_context*, struct vlx_buffer*);

/* vlx_image_destroy 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_image*		image 
 * 
 * Frees image resources. 
 **/

void vlx_image_destroy(struct vlx_context*, struct vlx_image*);

/* vlx_vertex_destroy 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_vertex*		vertex structure 
 * 
 * Frees vertex buffer resources. 
 **/

void vlx_vertex_destroy(struct vlx_context*, struct vlx_vertex*);

/* vlx_texture_destroy 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_texture*		texture 
 * 
 * Frees texture resources. 
 **/

void vlx_texture_destroy(struct vlx_context*, struct vlx_texture*);

/* vlx_descriptor_destroy 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_descriptor*	descriptor structure 
 * 
 * Frees descriptor resources. 
 **/

void vlx_descriptor_destroy(struct vlx_context*, struct vlx_descriptor*);

/* vlx_pipeline_destroy 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_pipeline*		Vulkan pipeline
 * 
 * Frees pipeline resources. 
 **/

void vlx_pipeline_destroy(struct vlx_context*, struct vlx_pipeline*);

/* vlx_command_destroy 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_command*		command structure
 * 
 * Frees command resources. 
 **/

void vlx_command_destroy(struct vlx_context*, struct vlx_command*);

/* vlx_surface_destroy 
 * 
 * struct vlx_context*		Vulkan context 
 * struct vlx_surface*		Vulkan surface 
 * 
 * Frees surface resources. 
 **/

void vlx_surface_destroy(struct vlx_context*, struct vlx_surface*);

/* vlx_context_destroy 
 * 
 * struct vlx_context*		Vulkan context 
 * 
 * Frees context resources. Should be called after all other Vulkan resources have been freed. 
 **/

void vlx_context_destroy(struct vlx_context*);

#endif
