#ifndef CH_OPENGL_FRAMEBUFFER_H
#define CH_OPENGL_FRAMEBUFFER_H

#include "engine/graphics/api/framebuffer.h"

namespace Chained
{
	class GLFramebuffer : public Framebuffer
	{
	public:
		GLFramebuffer(const FramebufferSpecification& spec);
		virtual ~GLFramebuffer();

		void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual void Resolve() override;

		virtual std::shared_ptr<Texture> GetColorAttachment() const override
		{
			return m_ColorTexture;
		}
		virtual uint32_t GetColorAttachmentRendererID() const override
		{
			return m_Specification.Samples > 1 ? m_ResolveColorAttachment : m_ColorAttachment;
		}

		virtual std::shared_ptr<Texture> GetDepthAttachment() const override
		{
			return m_DepthTexture;
		}
		virtual uint32_t GetDepthAttachmentRendererID() const override
		{
			return m_Specification.Samples > 1 ? m_ResolveDepthAttachment : m_DepthAttachment;
		}

		virtual const FramebufferSpecification& GetSpecification() const override
		{
			return m_Specification;
		}

		virtual void* GetNativeFramebuffer() override
		{
			return (void*)(uintptr_t)m_RendererID;
		}

		virtual bool IsValid() const override
		{
			return m_RendererID != 0 && m_IsComplete;
		}

	private:
		uint32_t m_RendererID = 0;
		uint32_t m_ColorAttachment = 0;
		uint32_t m_DepthAttachment = 0;

		// Only used when m_Specification.Samples > 1: a same-size single-sample FBO that
		// Resolve() blits the multisample attachments into, so downstream code can keep
		// sampling a plain sampler2D regardless of whether MSAA is on.
		uint32_t m_ResolveFBO = 0;
		uint32_t m_ResolveColorAttachment = 0;
		uint32_t m_ResolveDepthAttachment = 0;

		std::shared_ptr<Texture> m_ColorTexture;
		std::shared_ptr<Texture> m_DepthTexture;

		FramebufferSpecification m_Specification;
		bool m_IsComplete = false;
	};
} // namespace Chained

#endif // CH_OPENGL_FRAMEBUFFER_H
