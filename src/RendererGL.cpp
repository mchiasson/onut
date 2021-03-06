// Onut
#include <onut/IndexBuffer.h>
#include <onut/Renderer.h>
#include <onut/Settings.h>
#include <onut/Texture.h>
#include <onut/VertexBuffer.h>
#include <onut/Window.h>

// Private
#include "IndexBufferGL.h"
#include "RendererGL.h"
#include "ShaderGL.h"
#include "TextureGL.h"
#include "VertexBufferGL.h"
#if defined(WIN32)
    #include "WindowWIN32.h"
#else
    #include "WindowSDL2.h"
#endif

// STL
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

namespace onut
{
    ORendererRef Renderer::create(const OWindowRef& pWindow)
    {
        return OMake<RendererGL>(pWindow);
    }

    RendererGL::RendererGL(const OWindowRef& pWindow)
    {
    }

    void RendererGL::init(const OWindowRef& pWindow)
    {
        createDevice(pWindow);
        createRenderTarget();
        createRenderStates();
        createUniforms();

        Renderer::init(pWindow);
    }

    RendererGL::~RendererGL()
    {
#if defined(WIN32)
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(m_hRC);
        ReleaseDC(m_hWnd, m_hDC);
#else
        if (m_pSDLWindow)
        {
            SDL_GL_DeleteContext(m_glContext);
        }
#endif
    }

    void RendererGL::createDevice(const OWindowRef& pWindow)
    {
#if defined(WIN32)
        m_hWnd = pWindow->getHandle();

        // Initialize OpenGL
        GLuint PixelFormat;
        static PIXELFORMATDESCRIPTOR pfd =  // pfd Tells Windows How We Want Things To Be
        {
            sizeof(PIXELFORMATDESCRIPTOR),  // Size Of This Pixel Format Descriptor
            1,                              // Version Number
            PFD_DRAW_TO_WINDOW |            // Format Must Support Window
            PFD_SUPPORT_OPENGL |            // Format Must Support OpenGL
            PFD_DOUBLEBUFFER,               // Must Support Double Buffering
            PFD_TYPE_RGBA,                  // Request An RGBA Format
            32,                             // Select Our Color Depth
            0, 0, 0, 0, 0, 0,               // Color Bits Ignored
            0,                              // No Alpha Buffer
            0,                              // Shift Bit Ignored
            0,                              // No Accumulation Buffer
            0, 0, 0, 0,                     // Accumulation Bits Ignored
            16,                             // 16Bit Z-Buffer (Depth Buffer)
            0,                              // No Stencil Buffer
            0,                              // No Auxiliary Buffer
            PFD_MAIN_PLANE,                 // Main Drawing Layer
            0,                              // Reserved
            0, 0, 0                         // Layer Masks Ignored
        };

        m_hDC = GetDC(m_hWnd);
        PixelFormat = ChoosePixelFormat(m_hDC, &pfd);
        SetPixelFormat(m_hDC, PixelFormat, &pfd);
        m_hRC = wglCreateContext(m_hDC);
        wglMakeCurrent(m_hDC, m_hRC);

        m_resolution = oSettings->getResolution();
#else
        m_pSDLWindow = ODynamicCast<OWindowSDL2>(pWindow)->getSDLWindow();
        assert(m_pSDLWindow);

        SDL_GetWindowSize(m_pSDLWindow, &m_resolution.x, &m_resolution.y);

        m_glContext = SDL_GL_CreateContext(m_pSDLWindow);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        SDL_GL_SetSwapInterval(1);
#endif

#if !defined(__APPLE__)
        glewInit();
#endif
    }

    void RendererGL::createRenderTarget()
    {
    }

    void RendererGL::onResize(const Point& newSize)
    {
        m_resolution.x = newSize.x;
        m_resolution.y = newSize.y;
    }

    void RendererGL::createRenderStates()
    {
        glCullFace(GL_BACK);
        glDepthFunc(GL_LESS);
        glEnable(GL_TEXTURE_2D);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
    }

    void RendererGL::createUniforms()
    {
    }

    void RendererGL::beginFrame()
    {
        // Bind render target
        renderStates.reset();

        // Set viewport/scissor
        const auto& res = getResolution();
        renderStates.viewport = iRect{0, 0, res.x, res.y};
        renderStates.scissorEnabled = false;
        renderStates.scissor = renderStates.viewport.get();

        // Reset 2d view
        set2DCamera(Vector2::Zero);
    }

    void RendererGL::endFrame()
    {
#if defined(WIN32)
        SwapBuffers(m_hDC);
#else
        SDL_GL_SwapWindow(m_pSDLWindow);
#endif
    }

    Point RendererGL::getTrueResolution() const
    {
        return m_resolution;
    }

    void RendererGL::clear(const Color& color)
    {
        renderStates.clearColor = color;
        applyRenderStates();
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void RendererGL::clearDepth()
    {
        applyRenderStates();
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void RendererGL::draw(uint32_t vertexCount)
    {
        applyRenderStates();
        
        GLenum mode = GL_POINTS;
        switch (renderStates.primitiveMode.get())
        {
            case PrimitiveMode::PointList:
                mode = GL_POINTS;
                break;
            case PrimitiveMode::LineList:
                mode = GL_LINES;
                break;
            case PrimitiveMode::LineStrip:
                mode = GL_LINE_STRIP;
                break;
            case PrimitiveMode::TriangleList:
                mode = GL_TRIANGLES;
                break;
            case PrimitiveMode::TriangleStrip:
                mode = GL_TRIANGLE_STRIP;
                break;
            default:
                break;
        }

        glDrawArrays(mode, 0, vertexCount);
    }

    void RendererGL::drawIndexed(uint32_t indexCount)
    {
        applyRenderStates();
        
        GLenum mode = GL_POINTS;
        switch (renderStates.primitiveMode.get())
        {
            case PrimitiveMode::PointList:
                mode = GL_POINTS;
                break;
            case PrimitiveMode::LineList:
                mode = GL_LINES;
                break;
            case PrimitiveMode::LineStrip:
                mode = GL_LINE_STRIP;
                break;
            case PrimitiveMode::TriangleList:
                mode = GL_TRIANGLES;
                break;
            case PrimitiveMode::TriangleStrip:
                mode = GL_TRIANGLE_STRIP;
                break;
            default:
                break;
        }

        glDrawElements(mode, indexCount, GL_UNSIGNED_SHORT, NULL);
    }

    void RendererGL::applyRenderStates()
    {
        // Clear color
        if (renderStates.clearColor.isDirty())
        {
            const auto& clearColor = renderStates.clearColor.get();
            glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
            renderStates.clearColor.resetDirty();
        }
        
        // Render target
        if (renderStates.renderTarget.isDirty())
        {
            auto& pRenderTarget = renderStates.renderTarget.get();
            if (pRenderTarget)
            {
                auto pRenderTargetGL = ODynamicCast<OTextureGL>(pRenderTarget);
                auto frameBuffer = pRenderTargetGL->getFramebuffer();
                glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
            }
            else
            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }

        // Textures
        bool isSampleDirty = 
            renderStates.sampleFiltering.isDirty() ||
            renderStates.sampleAddressMode.isDirty();
        for (int i = 0; i < RenderStates::MAX_TEXTURES; ++i)
        {
            auto& pTextureState = renderStates.textures[i];
            if (pTextureState.isDirty() || isSampleDirty)
            {
                auto pTexture = pTextureState.get().get();
                if (pTexture != nullptr)
                {
                    auto pTextureEGLS2 = static_cast<TextureGL*>(pTexture);
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, pTextureEGLS2->getHandle());
                    
                    if (renderStates.sampleFiltering.get() != pTextureEGLS2->filtering)
                    {
                        pTextureEGLS2->filtering = renderStates.sampleFiltering.get();
                        if (pTextureEGLS2->filtering == sample::Filtering::Nearest)
                        {
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        }
                        else if (pTextureEGLS2->filtering == sample::Filtering::Linear)
                        {
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        }
                    }
                    if (renderStates.sampleAddressMode.get() != pTextureEGLS2->addressMode)
                    {
                        pTextureEGLS2->addressMode = renderStates.sampleAddressMode.get();
                        if (pTextureEGLS2->addressMode == sample::AddressMode::Wrap)
                        {
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        }
                        else if (pTextureEGLS2->addressMode == sample::AddressMode::Clamp)
                        {
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        }
                    }
                }
                pTextureState.resetDirty();
            }
        }
        if (isSampleDirty)
        {
            renderStates.sampleFiltering.resetDirty();
            renderStates.sampleAddressMode.resetDirty();
        }
        
        // Blend
        if (renderStates.blendMode.isDirty())
        {
            switch (renderStates.blendMode.get())
            {
                case onut::BlendMode::Opaque:
                    glDisable(GL_BLEND);
                    break;
                case onut::BlendMode::Alpha:
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case onut::BlendMode::Add:
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    break;
                case onut::BlendMode::PreMultiplied:
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case onut::BlendMode::Multiply:
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case onut::BlendMode::ForceWrite:
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ZERO);
                    break;
                default: 
                    break;
            }
            renderStates.blendMode.resetDirty();
        }
        
        // Viewport
        if (renderStates.viewport.isDirty())
        {
            auto& rect = renderStates.viewport.get();
            glViewport(
                static_cast<GLint>(rect.left), 
                static_cast<GLint>(getResolution().y) - static_cast<GLint>(rect.top) - static_cast<GLint>(rect.bottom - rect.top), 
                static_cast<GLsizei>(rect.right - rect.left),
                static_cast<GLsizei>(rect.bottom - rect.top));
            renderStates.viewport.resetDirty();
        }
        
        // Scissor enabled
        if (renderStates.scissorEnabled.isDirty())
        {
            if (renderStates.scissorEnabled.get())
            {
                glEnable(GL_SCISSOR_TEST);
            }
            else
            {
                glDisable(GL_SCISSOR_TEST);
            }
            renderStates.scissorEnabled.resetDirty();
        }
        
        // Culling
        if (renderStates.backFaceCull.isDirty())
        {
            if (renderStates.backFaceCull.get())
            {
                glEnable(GL_CULL_FACE);
            }
            else
            {
                glDisable(GL_CULL_FACE);
            }
            renderStates.backFaceCull.resetDirty();
        }
        
        // Scissor
        if (renderStates.scissorEnabled.get() &&
            renderStates.scissor.isDirty())
        {
            auto& rect = renderStates.scissor.get();
            glScissor(
                static_cast<GLint>(rect.left), 
                static_cast<GLint>(getResolution().y) - static_cast<GLint>(rect.top) - static_cast<GLint>(rect.bottom - rect.top), 
                static_cast<GLsizei>(rect.right - rect.left),
                static_cast<GLsizei>(rect.bottom - rect.top));
            renderStates.scissor.resetDirty();
        }
        
        // Projection
        if (renderStates.projection.isDirty())
        {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glMultMatrixf(renderStates.projection.get().v);
            renderStates.projection.resetDirty();
        }
        
        // View * World
        if (renderStates.view.isDirty() ||
            renderStates.world.isDirty())
        {
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            auto finalTransform = renderStates.world.get() * renderStates.view.get();
            glMultMatrixf(finalTransform.v);
            renderStates.view.resetDirty();
            renderStates.world.resetDirty();
        }
        
        // Depth write
        if (renderStates.depthWrite.isDirty())
        {
            if (renderStates.depthWrite.get())
            {
                glDepthMask(GL_TRUE);
            }
            else
            {
                glDepthMask(GL_FALSE);
            }
            renderStates.depthWrite.resetDirty();
        }
        
        // Depth test
        if (renderStates.depthEnabled.isDirty())
        {
            if (renderStates.depthEnabled.get())
            {
                glEnable(GL_DEPTH_TEST);
            }
            else
            {
                glDisable(GL_DEPTH_TEST);
            }
            renderStates.depthEnabled.resetDirty();
        }

        // Shaders
        if (renderStates.vertexShader.isDirty() ||
            renderStates.pixelShader.isDirty())
        {
        }
        /*
        auto pShaderGL = ODynamicCast<OShaderGL>(renderStates.vertexShader.get());
        if (pShaderGL)
        {
            if (renderStates.vertexShader.isDirty())
            {
                m_pDeviceContext->VSSetShader(pShaderD3D11->getVertexShader(), nullptr, 0);
                m_pDeviceContext->IASetInputLayout(pShaderD3D11->getInputLayout());
                renderStates.vertexShader.resetDirty();

                auto& uniforms = pShaderD3D11->getUniforms();
                for (UINT i = 0; i < (UINT)uniforms.size(); ++i)
                {
                    m_pDeviceContext->VSSetConstantBuffers(4 + i, 1, &(uniforms[i].pBuffer));
                    uniforms[i].dirty = false;
                }
            }
            else
            {
                auto& uniforms = pShaderD3D11->getUniforms();
                for (UINT i = 0; i < (UINT)uniforms.size(); ++i)*
                {
                    auto& uniform = uniforms[i];
                    if (uniform.dirty)
                    {
                        m_pDeviceContext->VSSetConstantBuffers(4 + i, 1, &(uniform.pBuffer));
                        uniform.dirty = false;
                    }
                }
            }
        }

        pShaderD3D11 = std::dynamic_pointer_cast<OShaderD3D11>(renderStates.pixelShader.get());
        if (pShaderD3D11)
        {
            if (renderStates.pixelShader.isDirty())
            {
                m_pDeviceContext->PSSetShader(pShaderD3D11->getPixelShader(), nullptr, 0);
                renderStates.pixelShader.resetDirty();

                auto& uniforms = pShaderD3D11->getUniforms();
                for (UINT i = 0; i < (UINT)uniforms.size(); ++i)
                {
                    m_pDeviceContext->PSSetConstantBuffers(4 + i, 1, &(uniforms[i].pBuffer));
                    uniforms[i].dirty = false;
                }
            }
            else
            {
                auto& uniforms = pShaderD3D11->getUniforms();
                for (UINT i = 0; i < (UINT)uniforms.size(); ++i)
                {
                    auto& uniform = uniforms[i];
                    if (uniform.dirty)
                    {
                        m_pDeviceContext->PSSetConstantBuffers(4 + i, 1, &(uniform.pBuffer));
                        uniform.dirty = false;
                    }
                }
            }
        }
        */
        // Vertex/Index buffers
        if (renderStates.vertexBuffer.isDirty())
        {
            if (renderStates.vertexBuffer.get())
            {
                auto handle = static_cast<OVertexBufferGL*>(renderStates.vertexBuffer.get().get())->getHandle();
                glBindBuffer(GL_ARRAY_BUFFER, handle);
                
                glVertexPointer(2, GL_FLOAT, 32, NULL);
                glTexCoordPointer(2, GL_FLOAT, 32, (float*)(sizeof(GL_FLOAT) * 2));
                glColorPointer(4, GL_FLOAT, 32, (float*)(sizeof(GL_FLOAT) * 4));
            }
            renderStates.vertexBuffer.resetDirty();
        }
        if (renderStates.indexBuffer.isDirty())
        {
            if (renderStates.indexBuffer.get())
            {
                auto handle = static_cast<OIndexBufferGL*>(renderStates.indexBuffer.get().get())->getHandle();
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
            }
            renderStates.indexBuffer.resetDirty();
        }
    }
}
