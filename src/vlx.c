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

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct vlx_buffer {
	VkBuffer bfr;
	VkDeviceMemory mem;
	VkMemoryRequirements req;
};

struct vlx_image {
	VkImage img;
	VkImageView v;
	VkDeviceMemory mem;
	VkMemoryRequirements req;
};

struct vlx_context {
	VkInstance inst;
	VkDevice devc;
	VkQueue que;
	uint32_t que_i;
	VkSemaphore smph_img;
	VkSemaphore smph_drw;
	VkFence fnc;
	VkFormat img_frmt;
	VkFormat txtr_frmt;
};

struct vlx_surface {
	VkSurfaceKHR srfc;
	uint32_t w;
	uint32_t h;
	VkRenderPass rndr;
	VkSwapchainKHR swap;
	VkImage* swap_img;
	VkImageView* swap_img_v;
	VkFramebuffer* frme;
	uint32_t img_n;
	uint32_t img_i;
	struct vlx_image dpth;
	VkClearValue clr[2];
};

struct vlx_command {
	VkCommandPool pool;
	VkCommandBuffer draw;
};

struct vlx_pipeline {
	VkPipeline pipe;
	VkPipelineLayout layt;
};

struct vlx_vertex {
	VkBuffer* bfr;
	VkDeviceMemory* mem;
	VkMemoryRequirements* req;
	VkPipelineVertexInputStateCreateInfo in;
	VkVertexInputBindingDescription* bind;
	uint32_t b;
	VkVertexInputAttributeDescription* attr;
	uint32_t a;
};

struct vlx_texture {
	struct vlx_image img;
	VkSampler smpl;
};

struct vlx_descriptor {
	VkDescriptorPool pool;
	VkDescriptorSet* set;
	VkDescriptorSetLayout* layt;
	uint32_t n;
};

struct vlx_context* vlx_context_create(int8_t g) {
	struct vlx_context* cntx = malloc(sizeof(struct vlx_context));
	
	const char* instext[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
	VkInstanceCreateInfo instinfo;
		instinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instinfo.pNext = 0;
		instinfo.flags = 0;
		instinfo.pApplicationInfo = 0;
		instinfo.enabledLayerCount = 0;
		instinfo.ppEnabledLayerNames = 0;
		instinfo.enabledExtensionCount = 2;
		instinfo.ppEnabledExtensionNames = instext;
	vkCreateInstance(&instinfo, 0, &(cntx->inst));

	int32_t gpun;
	vkEnumeratePhysicalDevices(cntx->inst, &gpun, 0);
	VkPhysicalDevice* gpu = malloc(sizeof(VkPhysicalDevice) * gpun);
	vkEnumeratePhysicalDevices(cntx->inst, &gpun, gpu);
	
	VkPhysicalDeviceProperties gpuprop;
	vkGetPhysicalDeviceProperties(gpu[0], &gpuprop);
	VkPhysicalDeviceFeatures gpufeat;
	vkGetPhysicalDeviceFeatures(gpu[0], &gpufeat);
	
	cntx->que_i = 0;
	float prio = 0.f;
	VkDeviceQueueCreateInfo queinfo;
		queinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queinfo.pNext = 0;
		queinfo.flags = 0;
		queinfo.queueFamilyIndex = cntx->que_i;
		queinfo.queueCount = 1;
		queinfo.pQueuePriorities = &prio;
	
	const char* devext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	VkDeviceCreateInfo devcinfo;
		devcinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		devcinfo.pNext = 0;
		devcinfo.flags = 0;
		devcinfo.queueCreateInfoCount = 1;
		devcinfo.pQueueCreateInfos = &queinfo;
		devcinfo.enabledLayerCount = 0;
		devcinfo.ppEnabledLayerNames = 0;
		devcinfo.enabledExtensionCount = 1;
		devcinfo.ppEnabledExtensionNames = &devext;
		devcinfo.pEnabledFeatures = &gpufeat;
	vkCreateDevice(gpu[0], &devcinfo, 0, &(cntx->devc));
	vkGetDeviceQueue(cntx->devc, 0, 0, &(cntx->que));
	
	free(gpu);
	
	VkSemaphoreCreateInfo smphinfo;
		smphinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		smphinfo.pNext = 0;
		smphinfo.flags = 0;
	vkCreateSemaphore(cntx->devc, &smphinfo, 0, &(cntx->smph_img));
	vkCreateSemaphore(cntx->devc, &smphinfo, 0, &(cntx->smph_drw));
	
	VkFenceCreateInfo fncinfo;
		fncinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fncinfo.pNext = 0;
		fncinfo.flags = 0;
	vkCreateFence(cntx->devc, &fncinfo, 0, &(cntx->fnc));
	
	cntx->img_frmt = VK_FORMAT_B8G8R8A8_UNORM;
	cntx->txtr_frmt = VK_FORMAT_R8G8B8A8_UNORM;
	if (g) {
		cntx->img_frmt = VK_FORMAT_B8G8R8A8_SRGB;
		cntx->txtr_frmt = VK_FORMAT_R8G8B8A8_SRGB;
	}
	
	return cntx;
}

struct vlx_surface* vlx_surface_create(struct vlx_context* cntx, struct wl_display* disp, struct wl_surface* wrfc, uint16_t w, uint16_t h) {
	struct vlx_surface* srfc = calloc(1, sizeof(struct vlx_surface));
	
	VkWaylandSurfaceCreateInfoKHR wayinfo;
		wayinfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
		wayinfo.pNext = 0;
		wayinfo.flags = 0;
		wayinfo.display = disp;
		wayinfo.surface = wrfc;
	vkCreateWaylandSurfaceKHR(cntx->inst, &wayinfo, 0, &(srfc->srfc));
	
	srfc->w = w;
	srfc->h = h;
	
	return srfc;
}

struct vlx_command* vlx_command_create(struct vlx_context* cntx) {
	struct vlx_command* cmd = malloc(sizeof(struct vlx_command));
	
	VkCommandPoolCreateInfo poolinfo;
		poolinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolinfo.pNext = 0;
		poolinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolinfo.queueFamilyIndex = cntx->que_i;
	vkCreateCommandPool(cntx->devc, &poolinfo, 0, &(cmd->pool));
	
	VkCommandBufferAllocateInfo cmdinfo;
		cmdinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdinfo.pNext = 0;
		cmdinfo.commandPool = cmd->pool;
		cmdinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdinfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(cntx->devc, &cmdinfo, &(cmd->draw));
	
	return cmd;
}

void vlx_surface_init_render_pass(struct vlx_context* cntx, struct vlx_surface* srfc) {
	VkAttachmentDescription atch[2];
		atch[0].flags = 0;
		atch[0].format = cntx->img_frmt;
		atch[0].samples = VK_SAMPLE_COUNT_1_BIT;
		atch[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		atch[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		atch[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		atch[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		atch[0].initialLayout = 0;
		atch[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		atch[1].flags = 0;
		atch[1].format = VK_FORMAT_D32_SFLOAT;
		atch[1].samples = VK_SAMPLE_COUNT_1_BIT;
		atch[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		atch[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		atch[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		atch[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		atch[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		atch[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference colref;
		colref.attachment = 0;
		colref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depref;
		depref.attachment = 1;
		depref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subdesc;
		subdesc.flags = 0;
		subdesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subdesc.inputAttachmentCount = 0;
		subdesc.pInputAttachments = 0;
		subdesc.colorAttachmentCount = 1;
		subdesc.pColorAttachments = &colref;
		subdesc.pResolveAttachments = 0;
		subdesc.pDepthStencilAttachment = &depref;
		subdesc.preserveAttachmentCount = 0;
		subdesc.pPreserveAttachments = 0;
	VkRenderPassCreateInfo rndrinfo;
		rndrinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rndrinfo.pNext = 0;
		rndrinfo.flags = 0;
		rndrinfo.attachmentCount = 2;
		rndrinfo.pAttachments = atch;
		rndrinfo.subpassCount = 1;
		rndrinfo.pSubpasses = &subdesc;
		rndrinfo.dependencyCount = 0;
		rndrinfo.pDependencies = 0;
	vkCreateRenderPass(cntx->devc, &rndrinfo, 0, &(srfc->rndr));
}

void vlx_surface_init_swapchain(struct vlx_context* cntx, struct vlx_surface* srfc) {
	VkSwapchainKHR swap_anc = srfc->swap;
	VkSwapchainCreateInfoKHR swapinfo;
		swapinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapinfo.pNext = 0;
		swapinfo.flags = 0;
		swapinfo.surface = srfc->srfc;
		swapinfo.minImageCount = 3;
		swapinfo.imageFormat = cntx->img_frmt;
		swapinfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		swapinfo.imageExtent.width = srfc->w;
		swapinfo.imageExtent.height = srfc->h;
		swapinfo.imageArrayLayers = 1;
		swapinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapinfo.queueFamilyIndexCount = 1;
		swapinfo.pQueueFamilyIndices = &(cntx->que_i);
		swapinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapinfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		swapinfo.clipped = 1;
		swapinfo.oldSwapchain = swap_anc;
	vkCreateSwapchainKHR(cntx->devc, &swapinfo, 0, &(srfc->swap));
	
	if (swap_anc != 0) {
		vkDestroySwapchainKHR(cntx->devc, swap_anc, 0);
	}
	
	vkGetSwapchainImagesKHR(cntx->devc, srfc->swap, &(srfc->img_n), 0);
	srfc->swap_img = malloc(sizeof(VkImage) * srfc->img_n);
	vkGetSwapchainImagesKHR(cntx->devc, srfc->swap, &(srfc->img_n), srfc->swap_img);
	srfc->swap_img_v = malloc(sizeof(VkImageView) * srfc->img_n);
	
	VkImageViewCreateInfo imgvinfo;
		imgvinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgvinfo.pNext = 0;
		imgvinfo.flags = 0;
		imgvinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgvinfo.format = cntx->img_frmt;
		imgvinfo.components.r = 0;
		imgvinfo.components.g = 0;
		imgvinfo.components.b = 0;
		imgvinfo.components.a = 0;
		imgvinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgvinfo.subresourceRange.baseMipLevel = 0;
		imgvinfo.subresourceRange.levelCount = 1;
		imgvinfo.subresourceRange.baseArrayLayer = 0;
		imgvinfo.subresourceRange.layerCount = 1;
	for (uint32_t i = 0; i < srfc->img_n; i++) {
		imgvinfo.image = srfc->swap_img[i];
		vkCreateImageView(cntx->devc, &imgvinfo, 0, &(srfc->swap_img_v)[i]);
	}
}

void vlx_surface_init_depth_buffer(struct vlx_context* cntx, struct vlx_surface* srfc) {
	VkImageCreateInfo imginfo;
		imginfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imginfo.pNext = 0;
		imginfo.flags = 0;
		imginfo.imageType = VK_IMAGE_TYPE_2D;
		imginfo.format = VK_FORMAT_D32_SFLOAT;
		imginfo.extent.width = srfc->w;
		imginfo.extent.height = srfc->h;
		imginfo.mipLevels = 1;
		imginfo.arrayLayers = 1;
		imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imginfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imginfo.sharingMode = 0;
		imginfo.queueFamilyIndexCount = 1;
		imginfo.pQueueFamilyIndices = &(cntx->que_i);
		imginfo.initialLayout = 0;
	vkCreateImage(cntx->devc, &imginfo, 0, &(srfc->dpth.img));
	
	vkGetImageMemoryRequirements(cntx->devc, srfc->dpth.img, &(srfc->dpth.req));
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = srfc->dpth.req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(cntx->devc, &meminfo, 0, &(srfc->dpth.mem));
	vkBindImageMemory(cntx->devc, srfc->dpth.img, srfc->dpth.mem, 0);
	
	VkImageViewCreateInfo imgvinfo;
		imgvinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgvinfo.pNext = 0;
		imgvinfo.flags = 0;
		imgvinfo.image = srfc->dpth.img;
		imgvinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgvinfo.format = VK_FORMAT_D32_SFLOAT;
		imgvinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imgvinfo.subresourceRange.baseMipLevel = 0;
		imgvinfo.subresourceRange.levelCount = 1;
		imgvinfo.subresourceRange.baseArrayLayer = 0;
		imgvinfo.subresourceRange.layerCount = 1;
	vkCreateImageView(cntx->devc, &imgvinfo, 0, &(srfc->dpth.v));
}

struct vlx_pipeline* vlx_pipeline_create(struct vlx_context* cntx, struct vlx_surface* srfc, int8_t* pthv, int8_t* pthf, struct vlx_vertex* vrtx, struct vlx_descriptor* dscr, uint64_t push_sz) {
	struct vlx_pipeline* pipe = malloc(sizeof(struct vlx_pipeline));
	
	FILE* f = fopen(pthv, "rb");
	fseek(f, 0, SEEK_END);
	uint64_t sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint32_t* src = malloc(sz);
	fread(src, sz, 1, f);
	fclose(f);
	
	VkShaderModule shdv;
	VkShaderModuleCreateInfo shdinfo;
		shdinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shdinfo.pNext = 0;
		shdinfo.flags = 0;
		shdinfo.codeSize = sz;
		shdinfo.pCode = src;
	vkCreateShaderModule(cntx->devc, &shdinfo, 0, &shdv);
	free(src);
	
	f = fopen(pthf, "rb");
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	src = malloc(sz);
	fread(src, sz, 1, f);
	fclose(f);
	
	VkShaderModule shdf;
		shdinfo.codeSize = sz;
		shdinfo.pCode = src;
	vkCreateShaderModule(cntx->devc, &shdinfo, 0, &shdf);
	free(src);
	
	VkPipelineShaderStageCreateInfo stginfo[2];
		stginfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stginfo[0].pNext = 0;
		stginfo[0].flags = 0;
		stginfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stginfo[0].module = shdv;
		stginfo[0].pName = "main";
		stginfo[0].pSpecializationInfo = 0;
		stginfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stginfo[1].pNext = 0;
		stginfo[1].flags = 0;
		stginfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stginfo[1].module = shdf;
		stginfo[1].pName = "main";
		stginfo[1].pSpecializationInfo = 0;
	
	VkPipelineInputAssemblyStateCreateInfo inasminfo;
		inasminfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inasminfo.pNext = 0;
		inasminfo.flags = 0;
		inasminfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		inasminfo.primitiveRestartEnable = 1;
		
	VkViewport vprt;
		vprt.x = 0.f;
		vprt.y = 0.f;
		vprt.width = 1.f;
		vprt.height = 1.f;
		vprt.minDepth = 0.f;
		vprt.maxDepth = 1.f;
		
	VkRect2D scsr;
		scsr.offset.x = 0;
		scsr.offset.y = 0;
		scsr.extent.width = 1;
		scsr.extent.height = 1;
		
	VkPipelineViewportStateCreateInfo vprtinfo;
		vprtinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vprtinfo.pNext = 0;
		vprtinfo.flags = 0;
		vprtinfo.viewportCount = 1;
		vprtinfo.pViewports = &vprt;
		vprtinfo.scissorCount = 1;
		vprtinfo.pScissors = &scsr;
	
	VkPipelineRasterizationStateCreateInfo rstrinfo;
		rstrinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rstrinfo.pNext = 0;
		rstrinfo.flags = 0;
		rstrinfo.depthClampEnable = 0;
		rstrinfo.rasterizerDiscardEnable = 0;
		rstrinfo.polygonMode = VK_POLYGON_MODE_FILL;
		rstrinfo.cullMode = VK_CULL_MODE_NONE;
		rstrinfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rstrinfo.depthBiasEnable = 0;
		rstrinfo.depthBiasConstantFactor = 0.f;
		rstrinfo.depthBiasClamp = 0.f;
		rstrinfo.depthBiasSlopeFactor = 0.f;
		rstrinfo.lineWidth = 1.f;
		
	VkPipelineMultisampleStateCreateInfo multinfo;
		multinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multinfo.pNext = 0;
		multinfo.flags = 0;
		multinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multinfo.sampleShadingEnable = 0;
		multinfo.minSampleShading = 1.f;
		multinfo.pSampleMask = 0;
		multinfo.alphaToCoverageEnable = 0;
		multinfo.alphaToOneEnable = 0;
		
	VkPipelineDepthStencilStateCreateInfo dpthinfo;
		dpthinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		dpthinfo.pNext = 0;
		dpthinfo.flags = 0;
		dpthinfo.depthTestEnable = 1;
		dpthinfo.depthWriteEnable = 1;
		dpthinfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		dpthinfo.depthBoundsTestEnable = 0;
		dpthinfo.stencilTestEnable = 0;
		dpthinfo.front.failOp = 0;
		dpthinfo.front.passOp = 0;
		dpthinfo.front.depthFailOp = 0;
		dpthinfo.front.compareOp = 0;
		dpthinfo.front.compareMask = 0;
		dpthinfo.front.writeMask = 0;
		dpthinfo.front.reference = 0;
		dpthinfo.back.failOp = 0;
		dpthinfo.back.passOp = 0;
		dpthinfo.back.depthFailOp = 0;
		dpthinfo.back.compareOp = 0;
		dpthinfo.back.compareMask = 0;
		dpthinfo.back.writeMask = 0;
		dpthinfo.back.reference = 0;
		dpthinfo.minDepthBounds = 0.f;
		dpthinfo.maxDepthBounds = 1.f;
		
	VkPipelineColorBlendAttachmentState colblndatch;
		colblndatch.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colblndatch.blendEnable = 1;
		colblndatch.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colblndatch.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colblndatch.colorBlendOp = VK_BLEND_OP_ADD;
		colblndatch.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colblndatch.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colblndatch.alphaBlendOp = VK_BLEND_OP_ADD;
		
	VkPipelineColorBlendStateCreateInfo colblndinfo;
		colblndinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colblndinfo.pNext = 0;
		colblndinfo.flags = 0;
		colblndinfo.logicOpEnable = 0;
		colblndinfo.logicOp = VK_LOGIC_OP_COPY;
		colblndinfo.attachmentCount = 1;
		colblndinfo.pAttachments = &colblndatch;
		colblndinfo.blendConstants[0] = 0.f;
		colblndinfo.blendConstants[1] = 0.f;
		colblndinfo.blendConstants[2] = 0.f;
		colblndinfo.blendConstants[3] = 0.f;
		
	VkDynamicState dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dyninfo;
		dyninfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dyninfo.pNext = 0;
		dyninfo.flags = 0;
		dyninfo.dynamicStateCount = 2;
		dyninfo.pDynamicStates = dyn;
		
	VkPushConstantRange pushrng;
		pushrng.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushrng.offset = 0;
		pushrng.size = push_sz;
	
	VkPipelineLayoutCreateInfo pipelaytinfo;
		pipelaytinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelaytinfo.pNext = 0;
		pipelaytinfo.flags = 0;
		pipelaytinfo.setLayoutCount = 0;
		pipelaytinfo.pSetLayouts = 0;
	if (dscr != 0) {
		pipelaytinfo.setLayoutCount = dscr->n;
		pipelaytinfo.pSetLayouts = dscr->layt;
	}
		pipelaytinfo.pushConstantRangeCount = 1;
		pipelaytinfo.pPushConstantRanges = &pushrng;
	vkCreatePipelineLayout(cntx->devc, &pipelaytinfo, 0, &(pipe->layt));
	
	VkGraphicsPipelineCreateInfo pipeinfo;
		pipeinfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeinfo.pNext = 0;
		pipeinfo.flags = 0;
		pipeinfo.stageCount = 2;
		pipeinfo.pStages = stginfo;
		pipeinfo.pVertexInputState = &(vrtx->in);
		pipeinfo.pInputAssemblyState = &inasminfo;
		pipeinfo.pTessellationState = 0;
		pipeinfo.pViewportState = &vprtinfo;
		pipeinfo.pRasterizationState = &rstrinfo;
		pipeinfo.pMultisampleState = &multinfo;
		pipeinfo.pDepthStencilState = &dpthinfo;
		pipeinfo.pColorBlendState = &colblndinfo;
		pipeinfo.pDynamicState = &dyninfo;
		pipeinfo.layout = pipe->layt;
		pipeinfo.renderPass = srfc->rndr;
		pipeinfo.subpass = 0;
		pipeinfo.basePipelineHandle = 0;
		pipeinfo.basePipelineIndex = 0;
	vkCreateGraphicsPipelines(cntx->devc, 0, 1, &pipeinfo, 0, &(pipe->pipe));
	
	vkDestroyShaderModule(cntx->devc, shdv, 0);
	vkDestroyShaderModule(cntx->devc, shdf, 0);
	
	return pipe;
}

void vlx_surface_init_frame_buffer(struct vlx_context* cntx, struct vlx_surface* srfc) {
	VkImageView atch[2];
	atch[1] = srfc->dpth.v;
	VkFramebufferCreateInfo fbfrinfo;
		fbfrinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbfrinfo.pNext = 0;
		fbfrinfo.flags = 0;
		fbfrinfo.renderPass = srfc->rndr;
		fbfrinfo.attachmentCount = 2;
		fbfrinfo.pAttachments = atch;
		fbfrinfo.width = srfc->w;
		fbfrinfo.height = srfc->h;
		fbfrinfo.layers = 1;
		srfc->frme = malloc(sizeof(VkFramebuffer) * srfc->img_n);
	for (uint32_t i = 0; i < srfc->img_n; i++) {
		atch[0] = srfc->swap_img_v[i];
		vkCreateFramebuffer(cntx->devc, &fbfrinfo, 0, &(srfc->frme)[i]);
	}
}

void vlx_buffer_refresh(struct vlx_context* cntx, struct vlx_buffer* bfr, void* data, uint64_t sz) {
	void* memdata;
	vkMapMemory(cntx->devc, bfr->mem, 0, bfr->req.size, 0, &memdata);
	memcpy(memdata, data, sz);
	vkUnmapMemory(cntx->devc, bfr->mem);
	vkBindBufferMemory(cntx->devc, bfr->bfr, bfr->mem, 0);
}

struct vlx_vertex* vlx_vertex_create(struct vlx_context* cntx, uint32_t b, uint32_t a, uint64_t sz) {
	struct vlx_vertex* vrtx = malloc(sizeof(struct vlx_vertex));
	vrtx->bfr = malloc(sizeof(VkBuffer) * b);
	vrtx->mem = malloc(sizeof(VkDeviceMemory) * b);
	vrtx->req = malloc(sizeof(VkMemoryRequirements) * b);
	vrtx->bind = malloc(sizeof(VkVertexInputBindingDescription) * b);
	vrtx->b = b;
	vrtx->attr = malloc(sizeof(VkVertexInputAttributeDescription) * a);
	vrtx->a = a;
	
	for (uint32_t i = 0; i < b; i++) {
		VkBufferCreateInfo bfrinfo;
			bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bfrinfo.pNext = 0;
			bfrinfo.flags = 0;
			bfrinfo.size = sz;
			bfrinfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bfrinfo.sharingMode = 0;
			bfrinfo.queueFamilyIndexCount = 1;
			bfrinfo.pQueueFamilyIndices = &(cntx->que_i);
		vkCreateBuffer(cntx->devc, &bfrinfo, 0, &(vrtx->bfr[i]));
	
		vkGetBufferMemoryRequirements(cntx->devc, vrtx->bfr[i], &(vrtx->req[i]));
		VkMemoryAllocateInfo meminfo;
			meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			meminfo.pNext = 0;
			meminfo.allocationSize = vrtx->req[i].size;
			meminfo.memoryTypeIndex = 0;
		vkAllocateMemory(cntx->devc, &meminfo, 0, &(vrtx->mem[i]));
	}
	return vrtx;
}

void vlx_vertex_bind(struct vlx_vertex* vrtx, uint32_t b, uint32_t s) {
	vrtx->bind[b].binding = b;
	vrtx->bind[b].stride = s;
	vrtx->bind[b].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void vlx_vertex_attr(struct vlx_vertex* vrtx, uint32_t l, uint32_t b, int8_t sz, uint32_t off) {
	vrtx->attr[l].location = l;
	vrtx->attr[l].binding = b;
	if (sz == -4) vrtx->attr[l].format = VK_FORMAT_R32_SINT;
	else if (sz == -8) vrtx->attr[l].format = VK_FORMAT_R32G32_SINT;
	else if (sz == -12) vrtx->attr[l].format = VK_FORMAT_R32G32B32_SINT;
	else if (sz == -16) vrtx->attr[l].format = VK_FORMAT_R32G32B32A32_SINT;
	else if (sz == 4) vrtx->attr[l].format = VK_FORMAT_R32_UINT;
	else if (sz == 8) vrtx->attr[l].format = VK_FORMAT_R32G32_UINT;
	else if (sz == 12) vrtx->attr[l].format = VK_FORMAT_R32G32B32_UINT;
	else if (sz == 16) vrtx->attr[l].format = VK_FORMAT_R32G32B32A32_UINT;
	vrtx->attr[l].offset = off;
}

void vlx_vertex_conf(struct vlx_vertex* vrtx) {
	vrtx->in.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vrtx->in.pNext = 0;
	vrtx->in.flags = 0;
	vrtx->in.vertexBindingDescriptionCount = vrtx->b;
	vrtx->in.pVertexBindingDescriptions = vrtx->bind;
	vrtx->in.vertexAttributeDescriptionCount = vrtx->a;
	vrtx->in.pVertexAttributeDescriptions = vrtx->attr;
}

void vlx_vertex_refresh(struct vlx_context* cntx, struct vlx_vertex* vrtx, uint32_t b, void* data, uint64_t sz) {
	void* memdata;
	vkMapMemory(cntx->devc, vrtx->mem[b], 0, vrtx->req[b].size, 0, &memdata);
	memcpy(memdata, data, sz);
	vkUnmapMemory(cntx->devc, vrtx->mem[b]);
	vkBindBufferMemory(cntx->devc, vrtx->bfr[b], vrtx->mem[b], 0);
}

struct vlx_buffer* vlx_index_create(struct vlx_context* cntx, uint64_t sz) {
	struct vlx_buffer* indx = malloc(sizeof(struct vlx_buffer));

	VkBufferCreateInfo bfrinfo;
		bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bfrinfo.pNext = 0;
		bfrinfo.flags = 0;
		bfrinfo.size = sz;
		bfrinfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bfrinfo.sharingMode = 0;
		bfrinfo.queueFamilyIndexCount = 1;
		bfrinfo.pQueueFamilyIndices = &(cntx->que_i);
	vkCreateBuffer(cntx->devc, &bfrinfo, 0, &(indx->bfr));
	
	vkGetBufferMemoryRequirements(cntx->devc, indx->bfr, &(indx->req));
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = indx->req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(cntx->devc, &meminfo, 0, &(indx->mem));
	
	return indx;
}

struct vlx_buffer* vlx_uniform_create(struct vlx_context* cntx, uint64_t sz) {
	struct vlx_buffer* unif = malloc(sizeof(struct vlx_buffer));

	VkBufferCreateInfo bfrinfo;
		bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bfrinfo.pNext = 0;
		bfrinfo.flags = 0;
		bfrinfo.size = sz;
		bfrinfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bfrinfo.sharingMode = 0;
		bfrinfo.queueFamilyIndexCount = 1;
		bfrinfo.pQueueFamilyIndices = &(cntx->que_i);
	vkCreateBuffer(cntx->devc, &bfrinfo, 0, &(unif->bfr));
	
	vkGetBufferMemoryRequirements(cntx->devc, unif->bfr, &(unif->req));
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = unif->req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(cntx->devc, &meminfo, 0, &(unif->mem));
	
	return unif;
}

struct vlx_texture* vlx_texture_create(struct vlx_context* cntx, struct vlx_command* cmd, uint8_t* pix, uint32_t w, uint32_t h) {
	struct vlx_texture* txtr = malloc(sizeof(struct vlx_texture));
	struct vlx_buffer bfr;

	VkBufferCreateInfo bfrinfo;
		bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bfrinfo.pNext = 0;
		bfrinfo.flags = 0;
		bfrinfo.size = w * h * 4;
		bfrinfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bfrinfo.sharingMode = 0;
		bfrinfo.queueFamilyIndexCount = 1;
		bfrinfo.pQueueFamilyIndices = &(cntx->que_i);
	vkCreateBuffer(cntx->devc, &bfrinfo, 0, &(bfr.bfr));
	
	vkGetBufferMemoryRequirements(cntx->devc, bfr.bfr, &(bfr.req));
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = bfr.req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(cntx->devc, &meminfo, 0, &(bfr.mem));
	
	vlx_buffer_refresh(cntx, &(bfr), pix, w * h * 4);
	
	VkImageCreateInfo imginfo;
		imginfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imginfo.pNext = 0;
		imginfo.flags = 0;
		imginfo.imageType = VK_IMAGE_TYPE_2D;
		imginfo.format = cntx->txtr_frmt;
		imginfo.extent.width = w;
		imginfo.extent.height = h;
		imginfo.extent.depth = 1;
		imginfo.mipLevels = 1;
		imginfo.arrayLayers = 1;
		imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imginfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imginfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imginfo.queueFamilyIndexCount = 1;
		imginfo.pQueueFamilyIndices = &(cntx->que_i);
		imginfo.initialLayout = 0;
	vkCreateImage(cntx->devc, &imginfo, 0, &(txtr->img.img));
	
	vkGetImageMemoryRequirements(cntx->devc, txtr->img.img, &(txtr->img.req));
		meminfo.allocationSize = txtr->img.req.size;
	vkAllocateMemory(cntx->devc, &meminfo, 0, &(txtr->img.mem));
	vkBindImageMemory(cntx->devc, txtr->img.img, txtr->img.mem, 0);
	
	VkCommandBuffer cmd_txtr;
	VkCommandBufferAllocateInfo cmdinfo;
		cmdinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdinfo.pNext = 0;
		cmdinfo.commandPool = cmd->pool;
		cmdinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdinfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(cntx->devc, &cmdinfo, &cmd_txtr);
	
	VkCommandBufferBeginInfo cbfrinfo;
		cbfrinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbfrinfo.pNext = 0;
		cbfrinfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cbfrinfo.pInheritanceInfo = 0;
	vkBeginCommandBuffer(cmd_txtr, &cbfrinfo);
	
	VkImageMemoryBarrier imgmembar;
		imgmembar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imgmembar.pNext = 0;
		imgmembar.srcAccessMask = 0;
		imgmembar.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imgmembar.oldLayout = 0;
		imgmembar.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgmembar.srcQueueFamilyIndex = cntx->que_i;
		imgmembar.dstQueueFamilyIndex = cntx->que_i;
		imgmembar.image = txtr->img.img;
		imgmembar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgmembar.subresourceRange.baseMipLevel = 0;
		imgmembar.subresourceRange.levelCount = 1;
		imgmembar.subresourceRange.baseArrayLayer = 0;
		imgmembar.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd_txtr, 0, 0, 0, 0, 0, 0, 0, 1, &imgmembar);
	
	VkBufferImageCopy cp;
		cp.bufferOffset = 0;
		cp.bufferRowLength = 0;
		cp.bufferImageHeight = 0;
		cp.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		cp.imageSubresource.mipLevel = 0;
		cp.imageSubresource.baseArrayLayer = 0;
		cp.imageSubresource.layerCount = 1;
		cp.imageOffset.x = 0;
		cp.imageOffset.y = 0;
		cp.imageOffset.z = 0;
		cp.imageExtent.width = w;
		cp.imageExtent.height = h;
		cp.imageExtent.depth = 1;
	vkCmdCopyBufferToImage(cmd_txtr, bfr.bfr, txtr->img.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cp);
	
	vkEndCommandBuffer(cmd_txtr);
	
	VkSubmitInfo sbmtinfo;
		sbmtinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		sbmtinfo.pNext = 0;
		sbmtinfo.waitSemaphoreCount = 0;
		sbmtinfo.pWaitSemaphores = 0;
		sbmtinfo.pWaitDstStageMask = 0;
		sbmtinfo.commandBufferCount = 1;
		sbmtinfo.pCommandBuffers = &cmd_txtr;
		sbmtinfo.signalSemaphoreCount = 0;
		sbmtinfo.pSignalSemaphores = 0;
	vkQueueSubmit(cntx->que, 1, &sbmtinfo, cntx->fnc);
	
	vkWaitForFences(cntx->devc, 1, &(cntx->fnc), 1, UINT64_MAX);
	vkResetFences(cntx->devc, 1, &(cntx->fnc));
	
	vkFreeCommandBuffers(cntx->devc, cmd->pool, 1, &cmd_txtr);
	
	VkImageViewCreateInfo imgvinfo;
		imgvinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgvinfo.pNext = 0;
		imgvinfo.flags = 0;
		imgvinfo.image = txtr->img.img;
		imgvinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgvinfo.format = cntx->txtr_frmt;
		imgvinfo.components.r = 0;
		imgvinfo.components.g = 0;
		imgvinfo.components.b = 0;
		imgvinfo.components.a = 0;
		imgvinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgvinfo.subresourceRange.baseMipLevel = 0;
		imgvinfo.subresourceRange.levelCount = 1;
		imgvinfo.subresourceRange.baseArrayLayer = 0;
		imgvinfo.subresourceRange.layerCount = 1;
	vkCreateImageView(cntx->devc, &imgvinfo, 0, &(txtr->img.v));
	
	VkSamplerCreateInfo smplinfo;
		smplinfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		smplinfo.pNext = 0;
		smplinfo.flags = 0;
		smplinfo.magFilter = 0;
		smplinfo.minFilter = 0;
		smplinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		smplinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		smplinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		smplinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		smplinfo.mipLodBias = 0.f;
		smplinfo.anisotropyEnable = 0;
		smplinfo.maxAnisotropy = 0;
		smplinfo.compareEnable = 0;
		smplinfo.compareOp = VK_COMPARE_OP_ALWAYS;
		smplinfo.minLod = 0.f;
		smplinfo.maxLod = 0.f;
		smplinfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		smplinfo.unnormalizedCoordinates = 0;
	vkCreateSampler(cntx->devc, &smplinfo, 0, &(txtr->smpl));
	
	vkDestroyBuffer(cntx->devc, bfr.bfr, 0);
	vkFreeMemory(cntx->devc, bfr.mem, 0);
	
	return txtr;
}
struct vlx_descriptor* vlx_descriptor_create(struct vlx_context* cntx, uint32_t n) {
	struct vlx_descriptor* dscr = malloc(sizeof	(struct vlx_descriptor));
	dscr->set = malloc(sizeof(VkDescriptorSet) * n);
	dscr->layt = malloc(sizeof(VkDescriptorSetLayout) * n);
	dscr->n = n;
	
	VkDescriptorPoolSize poolsz[2];
		poolsz[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolsz[0].descriptorCount = n;
		poolsz[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolsz[1].descriptorCount = n;
	VkDescriptorPoolCreateInfo poolinfo;
		poolinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolinfo.pNext = 0;
		poolinfo.flags = 0;
		poolinfo.maxSets = n;
		poolinfo.poolSizeCount = 2;
		poolinfo.pPoolSizes = poolsz;
	vkCreateDescriptorPool(cntx->devc, &poolinfo, 0, &(dscr->pool));
	
	VkDescriptorSetLayoutBinding bind[2];
		bind[0].binding = 0;
		bind[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bind[0].descriptorCount = 1;
		bind[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bind[0].pImmutableSamplers = 0;
		bind[1].binding = 1;
		bind[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bind[1].descriptorCount = 1;
		bind[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bind[1].pImmutableSamplers = 0;
	
	VkDescriptorSetLayoutCreateInfo laytinfo;
		laytinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		laytinfo.pNext = 0;
		laytinfo.flags = 0;
		laytinfo.bindingCount = 2;
		laytinfo.pBindings = bind;
	for (uint32_t i = 0; i < n; i++) {
		vkCreateDescriptorSetLayout(cntx->devc, &laytinfo, 0, &(dscr->layt[i]));
	}
	VkDescriptorSetAllocateInfo setalc;
		setalc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setalc.pNext = 0;
		setalc.descriptorPool = dscr->pool;
		setalc.descriptorSetCount = n;
		setalc.pSetLayouts = dscr->layt;
	vkAllocateDescriptorSets(cntx->devc, &setalc, dscr->set);
	
	return dscr;
}

void vlx_dsecriptor_write(struct vlx_context* cntx, struct vlx_descriptor* dscr, uint32_t i, struct vlx_buffer* unif, void* data, uint64_t sz, struct vlx_texture* txtr) {
	VkWriteDescriptorSet writ[2];
	uint8_t n = 0;
	
	VkDescriptorBufferInfo bfr;
	if (unif != 0) {
		bfr.buffer = unif->bfr;
		bfr.offset = 0;
		bfr.range = sz;
		vlx_buffer_refresh(cntx, unif, data, sz);
		
		writ[n].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writ[n].pNext = 0;
		writ[n].dstSet = dscr->set[i];
		writ[n].dstBinding = 0;
		writ[n].dstArrayElement = 0;
		writ[n].descriptorCount = 1;
		writ[n].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writ[n].pImageInfo = 0;
		writ[n].pBufferInfo = &bfr;
		writ[n].pTexelBufferView = 0;
		n++;
	}
	
	VkDescriptorImageInfo img;
	if (txtr != 0) {
		img.sampler = txtr->smpl;
		img.imageView = txtr->img.v;
		img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		writ[n].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writ[n].pNext = 0;
		writ[n].dstSet = dscr->set[i];
		writ[n].dstBinding = 1;
		writ[n].dstArrayElement = 0;
		writ[n].descriptorCount = 1;
		writ[n].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writ[n].pImageInfo = &img;
		writ[n].pBufferInfo = 0;
		writ[n].pTexelBufferView = 0;
		n++;
	}
	
	vkUpdateDescriptorSets(cntx->devc, n, writ, 0, 0);
}

void vlx_surface_clear(struct vlx_surface* srfc, uint8_t r, uint8_t g, uint8_t b) {
	srfc->clr[0].color.float32[0] = (float) r / 255;
	srfc->clr[0].color.float32[1] = (float) g / 255;
	srfc->clr[0].color.float32[2] = (float) b / 255;
	srfc->clr[0].color.float32[3] = (float) 1;
	srfc->clr[1].depthStencil.depth = 1.f;
	srfc->clr[1].depthStencil.stencil = 0;
}

void vlx_surface_new_frame(struct vlx_context* cntx, struct vlx_surface* srfc, struct vlx_command* cmd) {
	vkAcquireNextImageKHR(cntx->devc, srfc->swap, UINT64_MAX, cntx->smph_img, 0, &(srfc->img_i));
	
	VkCommandBufferBeginInfo cbfrinfo;
		cbfrinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbfrinfo.pNext = 0;
		cbfrinfo.flags = 0;
		cbfrinfo.pInheritanceInfo = 0;
	vkBeginCommandBuffer(cmd->draw, &cbfrinfo);
	
	VkImageMemoryBarrier imgmembar;
		imgmembar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imgmembar.pNext = 0;
		imgmembar.srcAccessMask = 0;
		imgmembar.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imgmembar.oldLayout = 0;
		imgmembar.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgmembar.srcQueueFamilyIndex = cntx->que_i;
		imgmembar.dstQueueFamilyIndex = cntx->que_i;
		imgmembar.image = srfc->swap_img[srfc->img_i];
		imgmembar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgmembar.subresourceRange.baseMipLevel = 0;
		imgmembar.subresourceRange.levelCount = 1;
		imgmembar.subresourceRange.baseArrayLayer = 0;
		imgmembar.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd->draw, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, &imgmembar);
	
	VkRenderPassBeginInfo rndrinfo;
		rndrinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rndrinfo.pNext = 0;
		rndrinfo.renderPass = srfc->rndr;
		rndrinfo.framebuffer = srfc->frme[srfc->img_i];
		rndrinfo.renderArea.offset.x = 0;
		rndrinfo.renderArea.offset.y = 0;
		rndrinfo.renderArea.extent.width = srfc->w;
		rndrinfo.renderArea.extent.height = srfc->h;
		rndrinfo.clearValueCount = 2;
		rndrinfo.pClearValues = srfc->clr;
	vkCmdBeginRenderPass(cmd->draw, &rndrinfo, VK_SUBPASS_CONTENTS_INLINE);
}

void vlx_surface_draw_frame(struct vlx_context* cntx, struct vlx_surface* srfc, struct vlx_pipeline* pipe, struct vlx_command* cmd, struct vlx_buffer* indx, struct vlx_vertex* vrtx, struct vlx_descriptor* dscr, void* push, uint64_t push_sz, uint32_t n, uint32_t indx_off, uint32_t vrtx_off) {
	vkCmdBindPipeline(cmd->draw, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->pipe);
	
	VkViewport vprt;
		vprt.x = 0.f;
		vprt.y = 0.f;
		vprt.width = (float) srfc->w;
		vprt.height = (float) srfc->h;
		vprt.minDepth = 0.f;
		vprt.maxDepth = 1.f;
	vkCmdSetViewport(cmd->draw, 0, 1, &vprt);
	
	VkRect2D scsr;
		scsr.offset.x = 0;
		scsr.offset.y = 0;
		scsr.extent.width = srfc->w;
		scsr.extent.height = srfc->h;
	vkCmdSetScissor(cmd->draw, 0, 1, &scsr);
	
	VkDeviceSize offset = {0};
	vkCmdBindVertexBuffers(cmd->draw, 0, vrtx->b, vrtx->bfr, &offset);
	vkCmdBindIndexBuffer(cmd->draw, indx->bfr, 0, VK_INDEX_TYPE_UINT32);
	if (dscr != 0) vkCmdBindDescriptorSets(cmd->draw, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->layt, 0, dscr->n, dscr->set, 0, 0);
	vkCmdPushConstants(cmd->draw, pipe->layt, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, push_sz, push);
	vkCmdDrawIndexed(cmd->draw, n, 1, indx_off, vrtx_off, 0);
}

void vlx_surface_swap_frame(struct vlx_context* cntx, struct vlx_surface* srfc, struct vlx_command* cmd) {
	vkCmdEndRenderPass(cmd->draw);
	
	VkImageMemoryBarrier imgmembar;
		imgmembar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imgmembar.pNext = 0;
		imgmembar.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imgmembar.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imgmembar.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgmembar.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imgmembar.srcQueueFamilyIndex = cntx->que_i;
		imgmembar.dstQueueFamilyIndex = cntx->que_i;
		imgmembar.image = srfc->swap_img[srfc->img_i];
		imgmembar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgmembar.subresourceRange.baseMipLevel = 0;
		imgmembar.subresourceRange.levelCount = 1;
		imgmembar.subresourceRange.baseArrayLayer = 0;
		imgmembar.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd->draw, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, &imgmembar);
	
	vkEndCommandBuffer(cmd->draw);

	VkPipelineStageFlags pipeflag = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo sbmtinfo;
		sbmtinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		sbmtinfo.pNext = 0;
		sbmtinfo.waitSemaphoreCount = 1;
		sbmtinfo.pWaitSemaphores = &cntx->smph_img;
		sbmtinfo.pWaitDstStageMask = &pipeflag;
		sbmtinfo.commandBufferCount = 1;
		sbmtinfo.pCommandBuffers = &(cmd->draw);
		sbmtinfo.signalSemaphoreCount = 1;
		sbmtinfo.pSignalSemaphores = &cntx->smph_drw;
	vkQueueSubmit(cntx->que, 1, &sbmtinfo, cntx->fnc);
	
	VkPresentInfoKHR preinfo;
		preinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		preinfo.pNext = 0;
		preinfo.waitSemaphoreCount = 1;
		preinfo.pWaitSemaphores = &cntx->smph_drw;
		preinfo.swapchainCount = 1;
		preinfo.pSwapchains = &(srfc->swap);
		preinfo.pImageIndices = &(srfc->img_i);
		preinfo.pResults = 0;
	vkQueuePresentKHR(cntx->que, &preinfo);
	
	vkWaitForFences(cntx->devc, 1, &(cntx->fnc), 1, UINT64_MAX);
	vkResetFences(cntx->devc, 1, &(cntx->fnc));
}

void vlx_surface_resize(struct vlx_context* cntx, struct vlx_surface* srfc, uint32_t w, uint32_t h) {
	for (uint32_t i = 0; i < srfc->img_n; i++) {
		vkDestroyFramebuffer(cntx->devc, srfc->frme[i], 0);
	}
	free(srfc->frme);
	
	vkDestroyImageView(cntx->devc, srfc->dpth.v, 0);
	vkDestroyImage(cntx->devc, srfc->dpth.img, 0);
	vkFreeMemory(cntx->devc, srfc->dpth.mem, 0);
	
	for (uint32_t i = 0; i < srfc->img_n; i++) {
		vkDestroyImageView(cntx->devc, srfc->swap_img_v[i], 0);
	}
	free(srfc->swap_img);
	free(srfc->swap_img_v);
	
	srfc->w = w;
	srfc->h = h;
	
	vlx_surface_init_swapchain(cntx, srfc);
	vlx_surface_init_depth_buffer(cntx, srfc);
	vlx_surface_init_frame_buffer(cntx, srfc);
}

void vlx_buffer_destroy(struct vlx_context* cntx, struct vlx_buffer* bfr) {
	vkDestroyBuffer(cntx->devc, bfr->bfr, 0);
	vkFreeMemory(cntx->devc, bfr->mem, 0);
	free(bfr);
}

void vlx_image_destroy(struct vlx_context* cntx, struct vlx_image* img) {
	vkDestroyImageView(cntx->devc, img->v, 0);
	vkDestroyImage(cntx->devc, img->img, 0);
	vkFreeMemory(cntx->devc, img->mem, 0);
	free(img);
}

void vlx_vertex_destroy(struct vlx_context* cntx, struct vlx_vertex* vrtx) {
	for (uint32_t i = 0; i < vrtx->b; i++) {
		vkDestroyBuffer(cntx->devc, vrtx->bfr[i], 0);
		vkFreeMemory(cntx->devc, vrtx->mem[i], 0);
	}
	free(vrtx->bfr);
	free(vrtx->mem);
	free(vrtx->req);
	free(vrtx->bind);
	free(vrtx->attr);
	free(vrtx);
}

void vlx_texture_destroy(struct vlx_context* cntx, struct vlx_texture* txtr) {
	vkDestroyImageView(cntx->devc, txtr->img.v, 0);
	vkDestroyImage(cntx->devc, txtr->img.img, 0);
	vkFreeMemory(cntx->devc, txtr->img.mem, 0);
	vkDestroySampler(cntx->devc, txtr->smpl, 0);
	
	free(txtr);
}

void vlx_descriptor_destroy(struct vlx_context* cntx, struct vlx_descriptor* dscr) {
	vkFreeDescriptorSets(cntx->devc, dscr->pool, dscr->n, dscr->set);
	for (uint32_t i = 0; i < dscr->n; i++) {
		vkDestroyDescriptorSetLayout(cntx->devc, dscr->layt[i], 0);
	}
	vkDestroyDescriptorPool(cntx->devc, dscr->pool, 0);
	
	free(dscr->set);
	free(dscr->layt);
	free(dscr);
}

void vlx_pipeline_destroy(struct vlx_context* cntx, struct vlx_pipeline* pipe) {
	vkDestroyPipeline(cntx->devc, pipe->pipe, 0);
	vkDestroyPipelineLayout(cntx->devc, pipe->layt, 0);
	free(pipe);
}

void vlx_command_destroy(struct vlx_context* cntx, struct vlx_command* cmd) {
	vkFreeCommandBuffers(cntx->devc, cmd->pool, 1, &(cmd->draw));
	vkDestroyCommandPool(cntx->devc, cmd->pool, 0);
	free(cmd);
}

void vlx_surface_destroy(struct vlx_context* cntx, struct vlx_surface* srfc) {
	for (uint32_t i = 0; i < srfc->img_n; i++) {
		vkDestroyFramebuffer(cntx->devc, srfc->frme[i], 0);
	}
	free(srfc->frme);
	
	vkDestroyImageView(cntx->devc, srfc->dpth.v, 0);
	vkDestroyImage(cntx->devc, srfc->dpth.img, 0);
	vkFreeMemory(cntx->devc, srfc->dpth.mem, 0);
	
	for (uint32_t i = 0; i < srfc->img_n; i++) {
		vkDestroyImageView(cntx->devc, srfc->swap_img_v[i], 0);
	}
	free(srfc->swap_img);
	free(srfc->swap_img_v);
	vkDestroySwapchainKHR(cntx->devc, srfc->swap, 0);
	
	vkDestroyRenderPass(cntx->devc, srfc->rndr, 0);
	
	vkDestroySurfaceKHR(cntx->inst, srfc->srfc, 0);
	free(srfc);
}

void vlx_context_destroy(struct vlx_context* cntx) {
	vkDestroySemaphore(cntx->devc, cntx->smph_img, 0);
	vkDestroySemaphore(cntx->devc, cntx->smph_drw, 0);
	vkDestroyFence(cntx->devc, cntx->fnc, 0);
	
	vkDestroyDevice(cntx->devc, 0);
	vkDestroyInstance(cntx->inst, 0);
	free(cntx);
}
