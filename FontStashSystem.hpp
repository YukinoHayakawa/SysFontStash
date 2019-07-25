#pragma once

#include <Usagi/Graphics/Game/OverlayRenderingSystem.hpp>
#include <Usagi/Game/CollectionSystem.hpp>

#include "FontStash.hpp"
#include "FontStashComponent.hpp"
#include "Position2DComponent.hpp"

namespace usagi
{
class Game;
class GpuSampler;
class GpuImage;
class GpuImageView;
class GpuBuffer;
class RenderPass;
class GpuCommandPool;
class GraphicsPipeline;

class FontStashSystem final
    : public OverlayRenderingSystem
    , public CollectionSystem<
        FontStashComponent,
        Position2DComponent
    >
{
    Game *mGame = nullptr;

    std::shared_ptr<GraphicsPipeline> mPipeline;
    std::shared_ptr<GpuCommandPool> mCommandPool;
    std::shared_ptr<GpuBuffer> mPosBuffer;
    std::shared_ptr<GpuBuffer> mTexCoordsBuffer;
    std::shared_ptr<GpuBuffer> mColorBuffer;
    std::shared_ptr<GpuImage> mFontTexture;
    std::shared_ptr<GpuImageView> mFontTextureView;
    std::shared_ptr<GpuSampler> mFontSampler;
    mutable std::shared_ptr<GraphicsCommandList> mCurrentCmdList;

    FONScontext *mContext = nullptr;

    // the following dispatching functions all returns non-zeros when succeed

    // create texture, only called once during init
    static int dispatchRenderCreate(void *user_ptr, int width, int height);
    // resize texture, called when expanding texture atlas
    static int dispatchRenderResize(void *user_ptr, int width, int height);
    // update texture subregion, called during command flushing
    static void dispatchRenderUpdate(
        void *user_ptr,
        int *rect,
        const unsigned char *data);
    // flush drawing commands
    static void dispatchRenderDraw(
        void *user_ptr,
        const float *vertices,
        const float *tex_coords,
        const unsigned int *colors,
        int num_vertices);
    // destroy texture atlas, called during system destruction
    static void dispatchRenderDelete(void *user_ptr);

    int renderCreate(int width, int height);
    int renderResize(int width, int height);
    void renderUpdate(int *rect, const unsigned char *data);
    void renderDraw(
        const float *vertices,
        const float *tex_coords,
        const unsigned int *colors,
        int num_vertices);
    void renderDelete();

public:
    explicit FontStashSystem(Game *game);
    ~FontStashSystem();

    const std::type_info & type() override
    {
        return typeid(decltype(*this));
    }

    void update(const Clock &clock) override;
    void createRenderTarget(RenderTargetDescriptor &descriptor) override;
    void createPipelines() override;
    std::shared_ptr<GraphicsCommandList> render(const Clock &clock) override;

    int addFont(std::string_view name, std::string_view path);
};
}
