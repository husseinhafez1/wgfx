#pragma once

class ShadowMap {
public:
    ShadowMap(int width, int height);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void bindForWriting() const;
    void bind(unsigned int slot) const;
    [[nodiscard]] int getWidth() const;
    [[nodiscard]] int getHeight() const;

private:
    unsigned int framebuffer = 0;
    unsigned int depthTexture = 0;
    int width;
    int height;
};
