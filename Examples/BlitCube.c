#include "Common.h"

static SDL_GpuGraphicsPipeline* Pipeline;
static SDL_GpuBuffer* VertexBuffer;
static SDL_GpuBuffer* IndexBuffer;
static SDL_GpuTexture* SourceTexture;
static SDL_GpuTexture* DestinationTexture;
static SDL_GpuSampler* Sampler;

static Vector3 CamPos = { 0, 0, 4 };

static int Init(Context* context)
{
	int result = CommonInit(context, 0);
	if (result < 0)
	{
		return result;
	}

	// Create the shaders
	SDL_GpuShader* vertexShader = LoadShader(context->Device, "Skybox.vert", 0, 1, 0, 0);
	if (vertexShader == NULL)
	{
		SDL_Log("Failed to create vertex shader!");
		return -1;
	}

	SDL_GpuShader* fragmentShader = LoadShader(context->Device, "Skybox.frag", 1, 0, 0, 0);
	if (fragmentShader == NULL)
	{
		SDL_Log("Failed to create fragment shader!");
		return -1;
	}

	// Create the pipeline
	SDL_GpuGraphicsPipelineCreateInfo pipelineCreateInfo = {
		.attachmentInfo = {
			.colorAttachmentCount = 1,
			.colorAttachmentDescriptions = (SDL_GpuColorAttachmentDescription[]){{
				.format = SDL_GpuGetSwapchainTextureFormat(context->Device, context->Window),
				.blendState = {
					.blendEnable = SDL_TRUE,
					.alphaBlendOp = SDL_GPU_BLENDOP_ADD,
					.colorBlendOp = SDL_GPU_BLENDOP_ADD,
					.colorWriteMask = 0xF,
					.srcColorBlendFactor = SDL_GPU_BLENDFACTOR_ONE,
					.srcAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ONE,
					.dstColorBlendFactor = SDL_GPU_BLENDFACTOR_ZERO,
					.dstAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ZERO
				}
			}},
		},
		.vertexInputState = (SDL_GpuVertexInputState){
			.vertexBindingCount = 1,
			.vertexBindings = (SDL_GpuVertexBinding[]){{
				.binding = 0,
				.inputRate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instanceStepRate = 0,
				.stride = sizeof(PositionVertex)
			}},
			.vertexAttributeCount = 1,
			.vertexAttributes = (SDL_GpuVertexAttribute[]){{
				.binding = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 0,
				.offset = 0
			}}
		},
		.multisampleState.sampleMask = 0xFFFF,
		.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertexShader = vertexShader,
		.fragmentShader = fragmentShader
	};

	Pipeline = SDL_GpuCreateGraphicsPipeline(context->Device, &pipelineCreateInfo);

	SDL_GpuReleaseShader(context->Device, vertexShader);
	SDL_GpuReleaseShader(context->Device, fragmentShader);

	// Create the GPU resources
	VertexBuffer = SDL_GpuCreateBuffer(
		context->Device,
		&(SDL_GpuBufferCreateInfo) {
			.usageFlags = SDL_GPU_BUFFERUSAGE_VERTEX_BIT,
			.sizeInBytes = sizeof(PositionVertex) * 24
		}
	);

	IndexBuffer = SDL_GpuCreateBuffer(
		context->Device,
		&(SDL_GpuBufferCreateInfo) {
			.usageFlags = SDL_GPU_BUFFERUSAGE_INDEX_BIT,
			.sizeInBytes = sizeof(Uint16) * 36
		}
	);

	SourceTexture = SDL_GpuCreateTexture(context->Device, &(SDL_GpuTextureCreateInfo){
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.type = SDL_GPU_TEXTURETYPE_CUBE,
		.width = 32,
		.height = 32,
		.layerCountOrDepth = 6,
		.levelCount = 1,
		.usageFlags = SDL_GPU_TEXTUREUSAGE_SAMPLER_BIT
	});

	DestinationTexture = SDL_GpuCreateTexture(context->Device, &(SDL_GpuTextureCreateInfo){
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.type = SDL_GPU_TEXTURETYPE_CUBE,
		.width = 32,
		.height = 32,
		.layerCountOrDepth = 6,
		.levelCount = 1,
		.usageFlags = SDL_GPU_TEXTUREUSAGE_SAMPLER_BIT | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET_BIT
	});

	Sampler = SDL_GpuCreateSampler(context->Device, &(SDL_GpuSamplerCreateInfo){
		.minFilter = SDL_GPU_FILTER_NEAREST,
		.magFilter = SDL_GPU_FILTER_NEAREST,
		.mipmapMode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.addressModeU = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.addressModeV = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.addressModeW = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	});

	// Set up buffer data
	SDL_GpuTransferBuffer* bufferTransferBuffer = SDL_GpuCreateTransferBuffer(
		context->Device,
		&(SDL_GpuTransferBufferCreateInfo) {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.sizeInBytes = (sizeof(PositionVertex) * 24) + (sizeof(Uint16) * 36)
		}
	);

	PositionVertex* bufferTransferData = SDL_GpuMapTransferBuffer(
		context->Device,
		bufferTransferBuffer,
		SDL_FALSE
	);

	bufferTransferData[0] = (PositionVertex) { -10, -10, -10 };
	bufferTransferData[1] = (PositionVertex) { 10, -10, -10 };
	bufferTransferData[2] = (PositionVertex) { 10, 10, -10 };
	bufferTransferData[3] = (PositionVertex) { -10, 10, -10 };

	bufferTransferData[4] = (PositionVertex) { -10, -10, 10 };
	bufferTransferData[5] = (PositionVertex) { 10, -10, 10 };
	bufferTransferData[6] = (PositionVertex) { 10, 10, 10 };
	bufferTransferData[7] = (PositionVertex) { -10, 10, 10 };

	bufferTransferData[8] = (PositionVertex) { -10, -10, -10 };
	bufferTransferData[9] = (PositionVertex) { -10, 10, -10 };
	bufferTransferData[10] = (PositionVertex) { -10, 10, 10 };
	bufferTransferData[11] = (PositionVertex) { -10, -10, 10 };

	bufferTransferData[12] = (PositionVertex) { 10, -10, -10 };
	bufferTransferData[13] = (PositionVertex) { 10, 10, -10 };
	bufferTransferData[14] = (PositionVertex) { 10, 10, 10 };
	bufferTransferData[15] = (PositionVertex) { 10, -10, 10 };

	bufferTransferData[16] = (PositionVertex) { -10, -10, -10 };
	bufferTransferData[17] = (PositionVertex) { -10, -10, 10 };
	bufferTransferData[18] = (PositionVertex) { 10, -10, 10 };
	bufferTransferData[19] = (PositionVertex) { 10, -10, -10 };

	bufferTransferData[20] = (PositionVertex) { -10, 10, -10 };
	bufferTransferData[21] = (PositionVertex) { -10, 10, 10 };
	bufferTransferData[22] = (PositionVertex) { 10, 10, 10 };
	bufferTransferData[23] = (PositionVertex) { 10, 10, -10 };

	Uint16* indexData = (Uint16*) &bufferTransferData[24];
	Uint16 indices[] = {
		 0,  1,  2,  0,  2,  3,
		 6,  5,  4,  7,  6,  4,
		 8,  9, 10,  8, 10, 11,
		14, 13, 12, 15, 14, 12,
		16, 17, 18, 16, 18, 19,
		22, 21, 20, 23, 22, 20
	};
	SDL_memcpy(indexData, indices, sizeof(indices));

	SDL_GpuUnmapTransferBuffer(context->Device, bufferTransferBuffer);

	// Set up texture data
	const Uint32 bytesPerImage = 32 * 32 * 4;
	SDL_GpuTransferBuffer* textureTransferBuffer = SDL_GpuCreateTransferBuffer(
		context->Device,
		&(SDL_GpuTransferBufferCreateInfo) {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.sizeInBytes = bytesPerImage * 6
		}
	);
	Uint8* textureTransferData = SDL_GpuMapTransferBuffer(
		context->Device,
		textureTransferBuffer,
		SDL_FALSE
	);

	const char* imageNames[] = {
		"cube0.bmp", "cube1.bmp", "cube2.bmp",
		"cube3.bmp", "cube4.bmp", "cube5.bmp",
	};

	for (int i = 0; i < SDL_arraysize(imageNames); i += 1) {
		SDL_Surface* imageData = LoadImage(imageNames[i], 4);
		if (imageData == NULL)
		{
			SDL_Log("Could not load image data!");
			return -1;
		}
		SDL_memcpy(textureTransferData + (bytesPerImage * i), imageData->pixels, bytesPerImage);
		SDL_DestroySurface(imageData);
	}
	SDL_GpuUnmapTransferBuffer(context->Device, textureTransferBuffer);

	// Upload the transfer data to the GPU buffers
	SDL_GpuCommandBuffer* cmdbuf = SDL_GpuAcquireCommandBuffer(context->Device);
	SDL_GpuCopyPass* copyPass = SDL_GpuBeginCopyPass(cmdbuf);

	SDL_GpuUploadToBuffer(
		copyPass,
		&(SDL_GpuTransferBufferLocation) {
			.transferBuffer = bufferTransferBuffer,
			.offset = 0
		},
		&(SDL_GpuBufferRegion) {
			.buffer = VertexBuffer,
			.offset = 0,
			.size = sizeof(PositionVertex) * 24
		},
		SDL_FALSE
	);

	SDL_GpuUploadToBuffer(
		copyPass,
		&(SDL_GpuTransferBufferLocation) {
			.transferBuffer = bufferTransferBuffer,
			.offset = sizeof(PositionVertex) * 24
		},
		&(SDL_GpuBufferRegion) {
			.buffer = IndexBuffer,
			.offset = 0,
			.size = sizeof(Uint16) * 36
		},
		SDL_FALSE
	);

	for (int i = 0; i < 6; i += 1) {
		SDL_GpuUploadToTexture(
			copyPass,
			&(SDL_GpuTextureTransferInfo) {
				.transferBuffer = textureTransferBuffer,
				.offset = bytesPerImage * i
			},
			&(SDL_GpuTextureRegion) {
				.texture = SourceTexture,
				.layer = i,
				.w = 32,
				.h = 32,
				.d = 1,
			},
			SDL_FALSE
		);
	}

	SDL_GpuEndCopyPass(copyPass);

	// Blit to destination texture.
	// This serves no real purpose other than demonstrating cube->cube blits are possible!
	for (int i = 0; i < 6; i += 1) {
		SDL_GpuBlit(
			cmdbuf,
			&(SDL_GpuBlitRegion){
				.texture = SourceTexture,
				.layerOrDepthPlane = i,
				.w = 32,
				.h = 32,
			},
			&(SDL_GpuBlitRegion){
				.texture = DestinationTexture,
				.layerOrDepthPlane = i,
				.w = 32,
				.h = 32,
			},
			SDL_FLIP_NONE,
			SDL_GPU_FILTER_LINEAR,
			SDL_FALSE
		);
	}

	SDL_GpuReleaseTransferBuffer(context->Device, bufferTransferBuffer);
	SDL_GpuReleaseTransferBuffer(context->Device, textureTransferBuffer);

	SDL_GpuSubmit(cmdbuf);

	// Print the instructions
	SDL_Log("Press Left/Right to view the opposite direction!");

	return 0;
}

static int Update(Context* context)
{
	if (context->LeftPressed || context->RightPressed)
	{
		CamPos.z *= -1;
	}

	return 0;
}

static int Draw(Context* context)
{
	SDL_GpuCommandBuffer* cmdbuf = SDL_GpuAcquireCommandBuffer(context->Device);
	if (cmdbuf == NULL)
	{
		SDL_Log("GpuAcquireCommandBuffer failed");
		return -1;
	}

	Uint32 w, h;
	SDL_GpuTexture* swapchainTexture = SDL_GpuAcquireSwapchainTexture(cmdbuf, context->Window, &w, &h);
	if (swapchainTexture != NULL)
	{
		Matrix4x4 proj = Matrix4x4_CreatePerspectiveFieldOfView(
			75.0f * SDL_PI_F / 180.0f,
			640.0f / 480.0f,
			0.01f,
			100.0f
		);
		Matrix4x4 view = Matrix4x4_CreateLookAt(
			CamPos,
			(Vector3) { 0, 0, 0 },
			(Vector3) { 0, 1, 0 }
		);

		Matrix4x4 viewproj = Matrix4x4_Multiply(view, proj);

		SDL_GpuColorAttachmentInfo colorAttachmentInfo = {
			.texture = swapchainTexture,
			.clearColor = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f },
			.loadOp = SDL_GPU_LOADOP_CLEAR,
			.storeOp = SDL_GPU_STOREOP_STORE
		};

		SDL_GpuRenderPass* renderPass = SDL_GpuBeginRenderPass(cmdbuf, &colorAttachmentInfo, 1, NULL);

		SDL_GpuBindGraphicsPipeline(renderPass, Pipeline);
		SDL_GpuBindVertexBuffers(renderPass, 0, &(SDL_GpuBufferBinding){ VertexBuffer, 0 }, 1);
		SDL_GpuBindIndexBuffer(renderPass, &(SDL_GpuBufferBinding){ IndexBuffer, 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		SDL_GpuBindFragmentSamplers(renderPass, 0, &(SDL_GpuTextureSamplerBinding){ DestinationTexture, Sampler }, 1);
		SDL_GpuPushVertexUniformData(cmdbuf, 0, &viewproj, sizeof(viewproj));
		SDL_GpuDrawIndexedPrimitives(renderPass, 0, 0, 36, 1, 0);

		SDL_GpuEndRenderPass(renderPass);
	}

	SDL_GpuSubmit(cmdbuf);

	return 0;
}

static void Quit(Context* context)
{
	SDL_GpuReleaseGraphicsPipeline(context->Device, Pipeline);
	SDL_GpuReleaseBuffer(context->Device, VertexBuffer);
	SDL_GpuReleaseBuffer(context->Device, IndexBuffer);
	SDL_GpuReleaseTexture(context->Device, SourceTexture);
	SDL_GpuReleaseTexture(context->Device, DestinationTexture);
	SDL_GpuReleaseSampler(context->Device, Sampler);

	CamPos.z = SDL_fabsf(CamPos.z);

	CommonQuit(context);
}

Example BlitCube_Example = { "BlitCube", Init, Update, Draw, Quit };
