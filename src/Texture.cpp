#include "onut/ContentManager.h"
#include "onut/Texture.h"

#include "RendererD3D11.h"
#include "Utils.h"

#include "lodepng/LodePNG.h"

#include <cassert>
#include <vector>

bool oGenerateMipmaps = true;

namespace onut
{
    OTextureRef Texture::createRenderTarget(const Point& size, bool willUseFX)
    {
        auto pRet = std::make_shared<Texture>();
        pRet->m_size = size;
#if defined(WIN32)
        pRet->createRenderTargetViews(pRet->m_pTexture, pRet->m_pTextureView, pRet->m_pRenderTargetView);
        if (willUseFX)
        {
            pRet->createRenderTargetViews(pRet->m_pTextureFX, pRet->m_pTextureViewFX, pRet->m_pRenderTargetViewFX);
        }
#else
#error
#endif
        pRet->m_type = Type::RenderTarget;
        return pRet;
    }

    OTextureRef Texture::createScreenRenderTarget(bool willBeUsedInEffects)
    {
        auto pRet = createRenderTarget({OScreenW, OScreenH}, willBeUsedInEffects);
        if (pRet)
        {
            pRet->m_isScreenRenderTarget = true;
        }
        pRet->m_type = Type::ScreenRenderTarget;
        return pRet;
    }

    OTextureRef Texture::createDynamic(const Point& size)
    {
        auto pRet = std::make_shared<Texture>();

#if defined(WIN32)
        ID3D11Texture2D* pTexture = NULL;
        ID3D11ShaderResourceView* pTextureView = NULL;

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = static_cast<UINT>(size.x);
        desc.Height = static_cast<UINT>(size.y);
        desc.MipLevels = desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);
        auto pDevice = pRendererD3D11->getDevice();
        auto ret = pDevice->CreateTexture2D(&desc, NULL, &pTexture);
        assert(ret == S_OK);
        ret = pDevice->CreateShaderResourceView(pTexture, NULL, &pTextureView);
        assert(ret == S_OK);

        pRet->m_size = size;
        pRet->m_pTextureView = pTextureView;
        pRet->m_pTexture = pTexture;
#else
#error
#endif
        pRet->m_type = Type::Dynamic;
        return pRet;
    }

    OTextureRef Texture::createFromFile(const std::string& filename, const OContentManagerRef& pContentManager, bool generateMipmaps)
    {
        auto pRet = std::make_shared<Texture>();

        std::vector<uint8_t> image; //the raw pixels (holy crap that must be slow)
        unsigned int w, h;
        auto ret = lodepng::decode(image, w, h, filename);
        assert(!ret);
        Point size{static_cast<int>(w), static_cast<int>(h)};

        // Pre multiplied
        uint8_t* pImageData = &(image[0]);
        auto len = size.x * size.y;
        for (decltype(len) i = 0; i < len; ++i, pImageData += 4)
        {
            pImageData[0] = pImageData[0] * pImageData[3] / 255;
            pImageData[1] = pImageData[1] * pImageData[3] / 255;
            pImageData[2] = pImageData[2] * pImageData[3] / 255;
        }

        pRet = createFromData(image.data(), size, generateMipmaps);
        pRet->setName(onut::getFilename(filename));
        pRet->m_type = Type::Static;
        return pRet;
    }

    OTextureRef Texture::createFromFileData(const uint8_t* pData, uint32_t dataSize, bool generateMipmaps)
    {
        std::vector<uint8_t> image; //the raw pixels (holy crap that must be slow)
        unsigned int w, h;
        lodepng::State state;
        auto ret = lodepng::decode(image, w, h, state, pData, dataSize);
        assert(!ret);
        Point size{static_cast<int>(w), static_cast<int>(h)};

        // Pre multiplied
        uint8_t* pImageData = image.data();
        auto len = size.x * size.y;
        for (int i = 0; i < len; ++i, pImageData += 4)
        {
            pImageData[0] = pImageData[0] * pImageData[3] / 255;
            pImageData[1] = pImageData[1] * pImageData[3] / 255;
            pImageData[2] = pImageData[2] * pImageData[3] / 255;
        }

        return createFromData(image.data(), size, generateMipmaps);
    }

    OTextureRef Texture::createFromData(const uint8_t* pData, const Point& size, bool generateMipmaps)
    {
        auto pRet = std::make_shared<Texture>();

#if defined(WIN32)
        ID3D11Texture2D* pTexture = NULL;
        ID3D11ShaderResourceView* pTextureView = NULL;

        // Manually generate mip levels
        bool allowMipMaps = true;
        int w2 = 1;
        int h2 = 1;
        while (w2 < size.x) w2 *= 2;
        if (size.x != w2) allowMipMaps = false;
        while (h2 < size.y) h2 *= 2;
        if (size.y != h2) allowMipMaps = false;
        unsigned char* pMipMaps = NULL;
        int mipLevels = 1;
        D3D11_SUBRESOURCE_DATA* mipsData = NULL;
        allowMipMaps = allowMipMaps && generateMipmaps;
        if (allowMipMaps)
        {
            int biggest = std::max<>(w2, h2);
            int w2t = w2;
            int h2t = h2;
            int totalSize = w2t * h2t * 4;
            while (!(w2t == 1 && h2t == 1))
            {
                ++mipLevels;
                w2t /= 2;
                if (w2t < 1) w2t = 1;
                h2t /= 2;
                if (h2t < 1) h2t = 1;
                totalSize += w2t * h2t * 4;
            }
            pMipMaps = new byte[totalSize];
            memcpy(pMipMaps, pData, size.x * size.y * 4);

            mipsData = new D3D11_SUBRESOURCE_DATA[mipLevels];

            w2t = w2;
            h2t = h2;
            totalSize = 0;
            int mipTarget = mipLevels;
            mipLevels = 0;
            byte* prev;
            byte* cur;
            while (mipLevels != mipTarget)
            {
                prev = pMipMaps + totalSize;
                mipsData[mipLevels].pSysMem = prev;
                mipsData[mipLevels].SysMemPitch = w2t * 4;
                mipsData[mipLevels].SysMemSlicePitch = 0;
                totalSize += w2t * h2t * 4;
                cur = pMipMaps + totalSize;
                w2t /= 2;
                if (w2t < 1) w2t = 1;
                h2t /= 2;
                if (h2t < 1) h2t = 1;
                ++mipLevels;
                if (mipLevels == mipTarget) break;
                int accum;

                // Generate the mips
                int multX = w2 / w2t;
                int multY = h2 / h2t;
                for (int y = 0; y < h2t; ++y)
                {
                    for (int x = 0; x < w2t; ++x)
                    {
                        for (int k = 0; k < 4; ++k)
                        {
                            accum = 0;
                            accum += prev[(y * multY * w2 + x * multX) * 4 + k];
                            accum += prev[(y * multY * w2 + (x + multX / 2) * multX) * 4 + k];
                            accum += prev[((y + multY / 2) * multY * w2 + x * multX) * 4 + k];
                            accum += prev[((y + multY / 2) * multY * w2 + (x + multX / 2) * multX) * 4 + k];
                            cur[(y * w2t + x) * 4 + k] = accum / 4;
                        }
                    }
                }

                w2 = w2t;
                h2 = h2t;
            }
        }

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = static_cast<UINT>(size.x);
        desc.Height = static_cast<UINT>(size.y);
        desc.MipLevels = mipLevels;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = (pMipMaps) ? pMipMaps : pData;
        data.SysMemPitch = size.x * 4;
        data.SysMemSlicePitch = 0;

        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);
        auto pDevice = pRendererD3D11->getDevice();
        auto ret = pDevice->CreateTexture2D(&desc, (mipsData) ? mipsData : &data, &pTexture);
        assert(ret == S_OK);
        ret = pDevice->CreateShaderResourceView(pTexture, NULL, &pTextureView);
        assert(ret == S_OK);

        pTexture->Release();
        if (pMipMaps) delete[] pMipMaps;
        if (mipsData) delete[] mipsData;

        pRet->m_size = size;
        pRet->m_pTextureView = pTextureView;
#else
#error
#endif

        pRet->m_type = Type::Static;
        return pRet;
    }

    void Texture::setData(const uint8_t* pData)
    {
        assert(isDynamic()); // Only dynamic texture can be set data

        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);
        auto pDeviceContext = pRendererD3D11->getDeviceContext();

        D3D11_MAPPED_SUBRESOURCE data;
        pDeviceContext->Map(m_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &data);
        memcpy(data.pData, pData, m_size.x * m_size.y * 4);
        pDeviceContext->Unmap(m_pTexture, 0);
    }

    Texture::~Texture()
    {
        if (m_pTextureView) m_pTextureView->Release();
        if (m_pTexture) m_pTexture->Release();
        if (m_pRenderTargetView) m_pRenderTargetView->Release();
        if (m_pTextureViewFX) m_pTextureViewFX->Release();
        if (m_pTextureFX) m_pTextureFX->Release();
        if (m_pRenderTargetViewFX) m_pRenderTargetViewFX->Release();
    }

    const Point& Texture::getSize() const
    {
        return m_size;
    }

    Vector2 Texture::getSizef() const
    {
        return Vector2(static_cast<float>(m_size.x), static_cast<float>(m_size.y));
    }

    bool Texture::isRenderTarget() const
    {
        return m_type == Type::RenderTarget || m_type == Type::ScreenRenderTarget;
    }

    bool Texture::isDynamic() const
    {
        return m_type == Type::Dynamic;
    }

    void Texture::bind(int slot)
    {
        assert(slot >= 0 && slot < RenderStates::MAX_TEXTURES);
        oRenderer->renderStates.textures[slot] = shared_from_this();
    }

    void Texture::resizeTarget(const Point& size)
    {
        m_size = size;

        // Release
        if (m_pTexture) m_pTexture->Release();
        if (m_pTextureView) m_pTextureView->Release();
        if (m_pRenderTargetView)
        {
            m_pRenderTargetView->Release();
            m_pTexture = nullptr;
            m_pTextureView = nullptr;
            m_pRenderTargetView = nullptr;
            createRenderTargetViews(m_pTexture, m_pTextureView, m_pRenderTargetView);
        }

        if (m_pTextureFX) m_pTextureFX->Release();
        if (m_pTextureViewFX) m_pTextureViewFX->Release();
        if (m_pRenderTargetViewFX)
        {
            m_pRenderTargetViewFX->Release();
            m_pTextureFX = nullptr;
            m_pTextureViewFX = nullptr;
            m_pRenderTargetViewFX = nullptr;
            createRenderTargetViews(m_pTextureFX, m_pTextureViewFX, m_pRenderTargetViewFX);
        }
    }

    void Texture::bindRenderTarget()
    {
        if (m_pRenderTargetView)
        {
            if (m_isScreenRenderTarget)
            {
                if (m_size.x != OScreenW ||
                    m_size.y != OScreenH)
                {
                    m_size = {OScreenW, OScreenH};

                    // Release
                    if (m_pTexture) m_pTexture->Release();
                    m_pTexture = nullptr;
                    if (m_pTextureView) m_pTextureView->Release();
                    m_pTextureView = nullptr;
                    if (m_pRenderTargetView)
                    {
                        m_pRenderTargetView->Release();
                        m_pRenderTargetView = nullptr;
                        createRenderTargetViews(m_pTexture, m_pTextureView, m_pRenderTargetView);
                    }

                    if (m_pTextureFX) m_pTextureFX->Release();
                    m_pTextureFX = nullptr;
                    if (m_pTextureViewFX) m_pTextureViewFX->Release();
                    m_pTextureViewFX = nullptr;
                    if (m_pRenderTargetViewFX)
                    {
                        m_pRenderTargetViewFX->Release();
                        createRenderTargetViews(m_pTextureFX, m_pTextureViewFX, m_pRenderTargetViewFX);
                    }
                    m_pRenderTargetViewFX = nullptr;
                }
            }
            oRenderer->renderStates.renderTarget = shared_from_this();
        }
    }

    void Texture::unbindRenderTarget()
    {
        oRenderer->renderStates.renderTarget = nullptr;
    }

    void Texture::clearRenderTarget(const Color& color)
    {
        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);
        pRendererD3D11->getDeviceContext()->ClearRenderTargetView(m_pRenderTargetView, &color.x);
    }

    void Texture::blur(float amount)
    {
        if (!m_pRenderTargetView) return; // Not a render target
        if (!m_pRenderTargetViewFX)
        {
            createRenderTargetViews(m_pTextureFX, m_pTextureViewFX, m_pRenderTargetViewFX);
        }

        ID3D11RenderTargetView* pPrevRT = nullptr;
        const FLOAT clearColor[] = {0, 0, 0, 0};

        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);

        pRendererD3D11->getDeviceContext()->OMGetRenderTargets(1, &pPrevRT, nullptr);
        UINT prevViewportCount = 1;
        D3D11_VIEWPORT pPrevViewports[8];
        pRendererD3D11->getDeviceContext()->RSGetViewports(&prevViewportCount, pPrevViewports);

        D3D11_VIEWPORT viewport = {0, 0, (FLOAT)m_size.x, (FLOAT)m_size.y, 0, 1};
        pRendererD3D11->getDeviceContext()->RSSetViewports(1, &viewport);

        int i = 0;
        while (amount > 0.f)
        {
            pRendererD3D11->setKernelSize({
                1.f / static_cast<float>(m_size.x) * ((float)i + amount) / 6,
                1.f / static_cast<float>(m_size.y) * ((float)i + amount) / 6
            });
            amount -= 6.f;

            pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &m_pRenderTargetViewFX, nullptr);
            pRendererD3D11->getDeviceContext()->ClearRenderTargetView(m_pRenderTargetViewFX, clearColor);
            pRendererD3D11->getDeviceContext()->PSSetShaderResources(0, 1, &m_pTextureView);
            pRendererD3D11->drawBlurH();

            pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);
            pRendererD3D11->getDeviceContext()->ClearRenderTargetView(m_pRenderTargetView, clearColor);
            pRendererD3D11->getDeviceContext()->PSSetShaderResources(0, 1, &m_pTextureViewFX);
            pRendererD3D11->drawBlurV();
            i += 1;
        }

        pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &pPrevRT, nullptr);
        pRendererD3D11->getDeviceContext()->RSSetViewports(prevViewportCount, pPrevViewports);
    }

    void Texture::sepia(const Vector3& tone, float saturation, float sepiaAmount)
    {
        if (!m_pRenderTargetView) return; // Not a render target
        if (!m_pRenderTargetViewFX)
        {
            createRenderTargetViews(m_pTextureFX, m_pTextureViewFX, m_pRenderTargetViewFX);
        }

        ID3D11RenderTargetView* pPrevRT = nullptr;
        const FLOAT clearColor[] = {0, 0, 0, 0};

        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);

        pRendererD3D11->getDeviceContext()->OMGetRenderTargets(1, &pPrevRT, nullptr);
        UINT prevViewportCount = 1;
        D3D11_VIEWPORT pPrevViewports[8];
        pRendererD3D11->getDeviceContext()->RSGetViewports(&prevViewportCount, pPrevViewports);

        D3D11_VIEWPORT viewport = {0, 0, (FLOAT)m_size.x, (FLOAT)m_size.y, 0, 1};
        pRendererD3D11->getDeviceContext()->RSSetViewports(1, &viewport);

        pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &m_pRenderTargetViewFX, nullptr);
        pRendererD3D11->getDeviceContext()->ClearRenderTargetView(m_pRenderTargetViewFX, clearColor);
        pRendererD3D11->getDeviceContext()->PSSetShaderResources(0, 1, &m_pTextureView);
        pRendererD3D11->setSepia(tone, saturation, sepiaAmount);
        pRendererD3D11->drawSepia();

        std::swap(m_pTexture, m_pTextureFX);
        std::swap(m_pTextureView, m_pTextureViewFX);
        std::swap(m_pRenderTargetView, m_pRenderTargetViewFX);

        pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &pPrevRT, nullptr);
        pRendererD3D11->getDeviceContext()->RSSetViewports(prevViewportCount, pPrevViewports);
    }

    void Texture::crt()
    {
        if (!m_pRenderTargetView) return; // Not a render target
        if (!m_pRenderTargetViewFX)
        {
            createRenderTargetViews(m_pTextureFX, m_pTextureViewFX, m_pRenderTargetViewFX);
        }

        ID3D11RenderTargetView* pPrevRT = nullptr;
        const FLOAT clearColor[] = {0, 0, 0, 0};

        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);

        pRendererD3D11->getDeviceContext()->OMGetRenderTargets(1, &pPrevRT, nullptr);
        UINT prevViewportCount = 1;
        D3D11_VIEWPORT pPrevViewports[8];
        pRendererD3D11->getDeviceContext()->RSGetViewports(&prevViewportCount, pPrevViewports);

        D3D11_VIEWPORT viewport = {0, 0, (FLOAT)m_size.x, (FLOAT)m_size.y, 0, 1};
        pRendererD3D11->getDeviceContext()->RSSetViewports(1, &viewport);

        pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &m_pRenderTargetViewFX, nullptr);
        pRendererD3D11->getDeviceContext()->ClearRenderTargetView(m_pRenderTargetViewFX, clearColor);
        pRendererD3D11->getDeviceContext()->PSSetShaderResources(0, 1, &m_pTextureView);
        pRendererD3D11->setCRT(getSizef());
        pRendererD3D11->drawCRT();

        std::swap(m_pTexture, m_pTextureFX);
        std::swap(m_pTextureView, m_pTextureViewFX);
        std::swap(m_pRenderTargetView, m_pRenderTargetViewFX);

        pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &pPrevRT, nullptr);
        pRendererD3D11->getDeviceContext()->RSSetViewports(prevViewportCount, pPrevViewports);
    }

    void Texture::cartoon(const Vector3& tone)
    {
        if (!m_pRenderTargetView) return; // Not a render target
        if (!m_pRenderTargetViewFX)
        {
            createRenderTargetViews(m_pTextureFX, m_pTextureViewFX, m_pRenderTargetViewFX);
        }

        ID3D11RenderTargetView* pPrevRT = nullptr;
        const FLOAT clearColor[] = {0, 0, 0, 0};

        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);

        pRendererD3D11->getDeviceContext()->OMGetRenderTargets(1, &pPrevRT, nullptr);
        UINT prevViewportCount = 1;
        D3D11_VIEWPORT pPrevViewports[8];
        pRendererD3D11->getDeviceContext()->RSGetViewports(&prevViewportCount, pPrevViewports);

        D3D11_VIEWPORT viewport = {0, 0, (FLOAT)m_size.x, (FLOAT)m_size.y, 0, 1};
        pRendererD3D11->getDeviceContext()->RSSetViewports(1, &viewport);

        pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &m_pRenderTargetViewFX, nullptr);
        pRendererD3D11->getDeviceContext()->ClearRenderTargetView(m_pRenderTargetViewFX, clearColor);
        pRendererD3D11->getDeviceContext()->PSSetShaderResources(0, 1, &m_pTextureView);
        pRendererD3D11->setCartoon(tone);
        pRendererD3D11->drawCartoon();

        std::swap(m_pTexture, m_pTextureFX);
        std::swap(m_pTextureView, m_pTextureViewFX);
        std::swap(m_pRenderTargetView, m_pRenderTargetViewFX);

        pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &pPrevRT, nullptr);
        pRendererD3D11->getDeviceContext()->RSSetViewports(prevViewportCount, pPrevViewports);
    }
        
    void Texture::vignette(float amount)
    {
        if (!m_pRenderTargetView) return; // Not a render target
        if (!m_pRenderTargetViewFX)
        {
            createRenderTargetViews(m_pTextureFX, m_pTextureViewFX, m_pRenderTargetViewFX);
        }

        ID3D11RenderTargetView* pPrevRT = nullptr;
        const FLOAT clearColor[] = {0, 0, 0, 0};

        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);

        pRendererD3D11->getDeviceContext()->OMGetRenderTargets(1, &pPrevRT, nullptr);
        UINT prevViewportCount = 1;
        D3D11_VIEWPORT pPrevViewports[8];
        pRendererD3D11->getDeviceContext()->RSGetViewports(&prevViewportCount, pPrevViewports);

        D3D11_VIEWPORT viewport = {0, 0, (FLOAT)m_size.x, (FLOAT)m_size.y, 0, 1};
        pRendererD3D11->getDeviceContext()->RSSetViewports(1, &viewport);

        pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &m_pRenderTargetViewFX, nullptr);
        pRendererD3D11->getDeviceContext()->ClearRenderTargetView(m_pRenderTargetViewFX, clearColor);
        pRendererD3D11->getDeviceContext()->PSSetShaderResources(0, 1, &m_pTextureView);
        pRendererD3D11->setVignette({
            1.f / static_cast<float>(m_size.x),
            1.f / static_cast<float>(m_size.y)
        }, amount);
        pRendererD3D11->drawVignette();

        std::swap(m_pTexture, m_pTextureFX);
        std::swap(m_pTextureView, m_pTextureViewFX);
        std::swap(m_pRenderTargetView, m_pRenderTargetViewFX);

        pRendererD3D11->getDeviceContext()->OMSetRenderTargets(1, &pPrevRT, nullptr);
        pRendererD3D11->getDeviceContext()->RSSetViewports(prevViewportCount, pPrevViewports);
    }

    void Texture::createRenderTargetViews(ID3D11Texture2D*& pTexture, ID3D11ShaderResourceView*& pTextureView, ID3D11RenderTargetView*& pRenderTargetView)
    {
        auto pRendererD3D11 = std::dynamic_pointer_cast<ORendererD3D11>(oRenderer);
        auto pDevice = pRendererD3D11->getDevice();

        D3D11_TEXTURE2D_DESC textureDesc = {0};
        HRESULT result;
        D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
        memset(&renderTargetViewDesc, 0, sizeof(renderTargetViewDesc));
        memset(&shaderResourceViewDesc, 0, sizeof(shaderResourceViewDesc));

        // Setup the render target texture description.
        textureDesc.Width = m_size.x;
        textureDesc.Height = m_size.y;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        // Create the render target texture.
        result = pDevice->CreateTexture2D(&textureDesc, NULL, &pTexture);
        if (result != S_OK)
        {
            assert(false && "Failed CreateTexture2D");
            return;
        }

        // Setup the description of the render target view.
        renderTargetViewDesc.Format = textureDesc.Format;
        renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        renderTargetViewDesc.Texture2D.MipSlice = 0;

        // Create the render target view.
        result = pDevice->CreateRenderTargetView(pTexture, &renderTargetViewDesc, &pRenderTargetView);
        if (result != S_OK)
        {
            assert(false && "Failed CreateRenderTargetView");
            return;
        }

        // Setup the description of the shader resource view.
        shaderResourceViewDesc.Format = textureDesc.Format;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;

        // Create the shader resource view.
        result = pDevice->CreateShaderResourceView(pTexture, &shaderResourceViewDesc, &pTextureView);
        if (result != S_OK)
        {
            assert(false && "Failed CreateShaderResourceView");
            return;
        }
    }
}

OTextureRef OGetTexture(const std::string& name)
{
    return oContentManager->getResourceAs<OTexture>(name);
}
