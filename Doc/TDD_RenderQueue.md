# 概述
为了让游戏中画面渲染物体的顺序变得可控，就需要制作一个RenderQueue，来对游戏中的每一帧画面的渲染的顺序进行排序。

# 核心思路
1. StaticMeshComponent不再直接渲染物体。而是创建一个RenderItem，然后放入RenderManager中。
2. RenderManager中有一个std::vector<RenderItem> 保存每一帧要渲染的内容，然后Sort。最后按照Sort的顺序进行渲染。
------
那么对于这个问题只有一个问题需要解决了如何进行排序？
对于这问题我们可以定义RenderPass，来将游戏中渲染的物体进行区分：
```C++
enum class RenderPass : uint8_t
{
    OPAQUE = 0,
    SKYBOX = 1,
    TRANSPARENT = 2,
    UI = 3,
};
```
- **OQAQUE**。最先渲染，进行深度测试和深度写入。
- **SKYBOX**。在普通物体之后进行渲染，进行深度测试，但是不进行深度写入。防止SKYBOX遮挡其他物体。
- **TRANSPARENT**。使用alpha blending来实现透明度渲染，因为要包装在后面的物体也能够渲染出来同样的需要深度测试，但是不进行深度写入。并且透明物体是从远到近开始画，这和普通物体相反。
- **UI**。则单独一个PASS，其层级关系由后续的UIManager来进行管理。
------------
当我们定义好了各种渲染Pass之后，下面就是对同一个Pass中的物体进行排序了，对于普通物体使用物体到摄像机的距离作为渲染的深度计算标准。而透明物体为了要实现**从远到近渲染**则是要用最大值减去天空盒到摄像机的距离来进行渲染
```C++
struct RenderItem
{
    SortKey sortKey;
    RenderResource* renderRes;
};

struct SortKey
{
    uint64_t value;
    static SortKey sMake(const RenderPass pass, float cameraDistance)
    {
        uint32_t depthBits = 0;
        switch (pass)
        {
        case RenderPass::OPAQUE:
            depthBits = static_cast<uint32_t>(cameraDistance * 1000.0f);
            break;
        case RenderPass::TRANSPARENT:
            depthBits = 0x00FFFFFF - static_cast<uint32_t>(cameraDistance * 1000.0f);
            break;
        case RenderPass::SKYBOX:
            depthBits = 0;
            break;
        case RenderPass::UI:
            depthBits = 0;
            break;
        }

        return SortKey{(static_cast<uint64_t>(pass) << 56) | (static_cast<uint64_t>(depthBits) << 24)};
    }
};
```