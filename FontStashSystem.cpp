#include "FontStashSystem.hpp"

#include <Usagi/Runtime/Graphics/GpuImageCreateInfo.hpp>
#include <Usagi/Game/Game.hpp>
#include <Usagi/Runtime/Runtime.hpp>
#include <Usagi/Runtime/Graphics/GpuDevice.hpp>
#include <Usagi/Runtime/Graphics/GpuImageViewCreateInfo.hpp>
#include <Usagi/Runtime/Graphics/GpuSamplerCreateInfo.hpp>
#include <Usagi/Runtime/Graphics/GpuImage.hpp>
#include <Usagi/Runtime/Graphics/GpuImageView.hpp>
#include <Usagi/Runtime/Graphics/GpuSampler.hpp>
#include <Usagi/Runtime/Graphics/GraphicsCommandList.hpp>
#include <Usagi/Runtime/Graphics/GpuBuffer.hpp>
#include <Usagi/Graphics/RenderTarget/RenderTargetDescriptor.hpp>
#include <Usagi/Graphics/RenderTarget/RenderTarget.hpp>
#include <Usagi/Runtime/Graphics/GpuCommandPool.hpp>
#include <Usagi/Runtime/Graphics/Framebuffer.hpp>
#include <Usagi/Asset/Converter/SpirvAssetConverter.hpp>
#include <imgui/imgui.h>
#include <Usagi/Runtime/Graphics/GraphicsPipelineCompiler.hpp>
#include <Usagi/Asset/AssetRoot.hpp>

namespace usagi
{
int FontStashSystem::dispatchRenderCreate(void *user_ptr, int width, int height)
{
    return static_cast<FontStashSystem*>(user_ptr)->renderCreate(width, height);
}

int FontStashSystem::dispatchRenderResize(void *user_ptr, int width, int height)
{
    return static_cast<FontStashSystem*>(user_ptr)->renderResize(width, height);
}

void FontStashSystem::dispatchRenderUpdate(
    void *user_ptr,
    int *rect,
    const unsigned char *data)
{
    static_cast<FontStashSystem*>(user_ptr)->renderUpdate(rect, data);
}

void FontStashSystem::dispatchRenderDraw(
    void *user_ptr,
    const float *vertices,
    const float *tex_coords,
    const unsigned *colors,
    int num_vertices)
{
    static_cast<FontStashSystem*>(user_ptr)->renderDraw(
        vertices, tex_coords, colors, num_vertices
    );
}

void FontStashSystem::dispatchRenderDelete(void *user_ptr)
{
    static_cast<FontStashSystem*>(user_ptr)->renderDelete();
}

int FontStashSystem::renderCreate(int width, int height)
{
    auto gpu = mGame->runtime()->gpu();
    {
        GpuImageCreateInfo info;
        info.format = GpuBufferFormat::R8_UNORM;
        info.size = { width, height };
        info.usage = GpuImageUsage::SAMPLED;
        mFontTexture = gpu->createImage(info);
    }
    {
        GpuImageViewCreateInfo info;
        info.components.r = GpuImageComponentSwizzle::ONE;
        info.components.g = GpuImageComponentSwizzle::ONE;
        info.components.b = GpuImageComponentSwizzle::ONE;
        info.components.a = GpuImageComponentSwizzle::R;
        mFontTextureView = mFontTexture->createView(info);
    }
    {
        GpuSamplerCreateInfo info;
        info.min_filter = GpuFilter::LINEAR;
        info.mag_filter = GpuFilter::NEAREST;
        info.addressing_mode_u = GpuSamplerAddressMode::REPEAT;
        info.addressing_mode_v = GpuSamplerAddressMode::REPEAT;
        mFontSampler = gpu->createSampler(info);
    }
    return 1;
}

int FontStashSystem::renderResize(int width, int height)
{
    return renderCreate(width, height);
}

void FontStashSystem::renderUpdate(int *rect, const unsigned char *data)
{
    // todo only update subregion
    // const Vector2i offset { rect[0], rect[1] };
    // const Vector2u32 size { rect[2] - rect[0], rect[3] - rect[1] };
    // mFontTexture->uploadRegion(
    //     data, size.x() * size.y() offset, size
    // )
    const auto size = mFontTexture->size();
    mFontTexture->upload(data, size.x() * size.y());
}

void FontStashSystem::renderDraw(
    const float *vertices,
    const float *tex_coords,
    const unsigned *colors,
    int num_vertices)
{
    mPosBuffer->allocate(num_vertices * sizeof(Vector2f));
    mTexCoordsBuffer->allocate(num_vertices * sizeof(Vector2f));
    mColorBuffer->allocate(num_vertices * sizeof(std::uint32_t));

    memcpy(mPosBuffer->mappedMemory(),
        vertices, num_vertices * sizeof(Vector2f));
    memcpy(mTexCoordsBuffer->mappedMemory(),
        tex_coords, num_vertices * sizeof(Vector2f));
    memcpy(mColorBuffer->mappedMemory(),
        colors, num_vertices * sizeof(std::uint32_t));

    mPosBuffer->flush();
    mTexCoordsBuffer->flush();
    mColorBuffer->flush();

    mCurrentCmdList->bindVertexBuffer(0, mPosBuffer);
    mCurrentCmdList->bindVertexBuffer(1, mTexCoordsBuffer);
    mCurrentCmdList->bindVertexBuffer(2, mColorBuffer);

    mCurrentCmdList->drawInstanced(num_vertices, 1, 0, 0);

    mPosBuffer->release();
    mTexCoordsBuffer->release();
    mColorBuffer->release();
}

void FontStashSystem::renderDelete()
{
    mFontTextureView.reset();
    mFontTexture.reset();
    mFontSampler.reset();
}

FontStashSystem::FontStashSystem(Game *game)
    : mGame(game)
{
    FONSparams params;

    params.width = 1024;
    params.height = 1024;
    params.flags = FONS_ZERO_TOPLEFT;
    params.renderCreate = dispatchRenderCreate;
    params.renderResize = dispatchRenderResize;
    params.renderUpdate = dispatchRenderUpdate;
    params.renderDraw = dispatchRenderDraw;
    params.renderDelete = dispatchRenderDelete;
    params.userPtr = this;

    mContext.init(params);

    auto gpu = mGame->runtime()->gpu();
    mPosBuffer = gpu->createBuffer(GpuBufferUsage::VERTEX);
    mTexCoordsBuffer = gpu->createBuffer(GpuBufferUsage::VERTEX);
    mColorBuffer = gpu->createBuffer(GpuBufferUsage::VERTEX);
    mCommandPool = gpu->createCommandPool();
}

FontStashSystem::~FontStashSystem()
{
}

void FontStashSystem::update(const Clock &clock)
{
}

void FontStashSystem::createRenderTarget(RenderTargetDescriptor &descriptor)
{
    descriptor.sharedColorTarget("fontstash");
    mRenderTarget = descriptor.finish();
}

void FontStashSystem::createPipelines()
{
    auto gpu = mGame->runtime()->gpu();
    auto assets = mGame->assets();
    auto compiler = gpu->createPipelineCompiler();

    // Shaders
    {
        compiler->setShader(ShaderStage::VERTEX,
            assets->res<SpirvAssetConverter>(
                "fontstash:shaders/shader.vert", ShaderStage::VERTEX)
        );
        compiler->setShader(ShaderStage::FRAGMENT,
            assets->res<SpirvAssetConverter>(
                "fontstash:shaders/shader.frag", ShaderStage::FRAGMENT)
        );
    }
    // Vertex Inputs
    {
        compiler->setVertexBufferBinding(0, sizeof(float) * 2);
        compiler->setVertexBufferBinding(1, sizeof(float) * 2);
        compiler->setVertexBufferBinding(2, sizeof(std::uint32_t));

        compiler->setVertexAttribute(
            "Position", 0,
            0, GpuBufferFormat::R32G32_SFLOAT
        );
        compiler->setVertexAttribute(
            "TexCoord", 1,
            0, GpuBufferFormat::R32G32_SFLOAT
        );
        compiler->setVertexAttribute(
            "Color", 2,
            0, GpuBufferFormat::R8G8B8A8_UNORM
        );
        compiler->iaSetPrimitiveTopology(PrimitiveTopology::TRIANGLE_LIST);
    }
    // Rasterization
    {
        RasterizationState state;
        state.face_culling_mode = FaceCullingMode::NONE;
        compiler->setRasterizationState(state);
    }
    // Blending
    {
        ColorBlendState state;
        state.enable = true;
        state.setBlendingFactor(
            BlendingFactor::SOURCE_ALPHA,
            BlendingFactor::ONE_MINUS_SOURCE_ALPHA);
        state.setBlendingOperation(BlendingOperation::ADD);
        compiler->setColorBlendState(state);
    }
    compiler->setRenderPass(mRenderTarget->renderPass());

    mPipeline = compiler->compile();
}

std::shared_ptr<GraphicsCommandList> FontStashSystem::render(const Clock &clock)
{
    auto framebuffer = mRenderTarget->createFramebuffer();
    const auto size = framebuffer->size();
    mCurrentCmdList = mCommandPool->allocateGraphicsCommandList();

    mCurrentCmdList->beginRecording();
    mCurrentCmdList->beginRendering(
        mRenderTarget->renderPass(),
        std::move(framebuffer)
    );

    mCurrentCmdList->bindPipeline(mPipeline);
    mCurrentCmdList->setViewport(0, { 0, 0 }, size.cast<float>());
    mCurrentCmdList->setScissor(0, { 0, 0 }, size);
    mCurrentCmdList->bindResourceSet(0, { mFontSampler, mFontTextureView });
    mCurrentCmdList->setConstant(ShaderStage::VERTEX,
        "screenDimensions", size.cast<float>().eval());
    mCurrentCmdList->setConstant(ShaderStage::VERTEX,
        "scale", Vector2f { 1, 1 });
    mCurrentCmdList->setConstant(ShaderStage::VERTEX,
        "translate", Vector2f { 0, 0 });

    for(auto &&e : mRegistry)
    {
        auto text = std::get<FontStashComponent *>(e.second);
        auto pos = std::get<Bound2DComponent *>(e.second);
        auto &state = *mContext.getState();
        state = *reinterpret_cast<FONSstate *>(& text->font);
        // todo don't hard code text shadow
        state.blur = state.size / 8;
        state.color = 0xFF000000;
        mContext.drawText(
            text->text,
            pos->bound,
            text->transition_begin, text->transition_end
        );
        state.blur = 0;
        state.color = text->color;
        mContext.drawText(
            text->text,
            pos->bound,
            text->transition_begin, text->transition_end
        );
    }
    mContext.flush();

    mCurrentCmdList->endRendering();
    mCurrentCmdList->endRecording();

    return std::move(mCurrentCmdList);
}

int FontStashSystem::addFont(std::string name, const std::filesystem::path &path)
{
    return mContext.fonsAddFont(std::move(name), path);
}
}
