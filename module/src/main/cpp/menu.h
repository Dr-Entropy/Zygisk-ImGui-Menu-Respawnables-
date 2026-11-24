// menu.h - Bounty Rush 60 FPS menu (ImGui)
#ifndef ZYGISK_MENU_TEMPLATE_MENU_H
#define ZYGISK_MENU_TEMPLATE_MENU_H

#include "gg_engine.h"
#include "fps60.h"

using namespace ImGui;

static gg::Engine g_engine;
static gg::Fps60 g_fps(g_engine);

void DrawMenu()
{
    Begin(OBFUSCATE("Bounty Rush 60 FPS"));

    TextUnformatted("FPS 60");
    TextDisabled("Status: %s", g_fps.enabled() ? "ON" : "OFF");
    if (g_engine.busy) {
        TextUnformatted("Working...");
    } else {
        if (!g_fps.enabled()) {
            if (Button("Enable")) g_fps.enableAsync();
        } else {
            if (Button("Disable")) g_fps.disableAsync();
        }
    }
    Separator();
    TextUnformatted("Log");
    for (auto &l : g_engine.logLines()) TextWrapped("%s", l.c_str());

    End();
}

void SetupImgui() {
    IMGUI_CHECKVERSION();
    CreateContext();
    ImGuiIO &io = GetIO();
    io.DisplaySize = ImVec2((float) glWidth, (float) glHeight);
    ImGui_ImplOpenGL3_Init("#version 100");
    StyleColorsDark();
    GetStyle().ScaleAllSizes(7.0f);
    io.Fonts->AddFontFromMemoryTTF(Roboto_Regular, 30, 30.0f);
}

EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);

    if (glWidth <= 0 || glHeight <= 0)
        return old_eglSwapBuffers(dpy, surface);

    if (!setupimg) {
        SetupImgui();
        setupimg = true;
    }

    ImGuiIO &io = GetIO();
    // keep the size in sync every frame (match start, rotation, multi-window / DeX)
    io.DisplaySize = ImVec2((float) glWidth, (float) glHeight);

    // save the game's GL state so the menu never leaks into the game's rendering
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    GLboolean prevScissor = glIsEnabled(GL_SCISSOR_TEST);
    GLint prevScissorBox[4];
    glGetIntegerv(GL_SCISSOR_BOX, prevScissorBox);

    ImGui_ImplOpenGL3_NewFrame();
    NewFrame();

    DrawMenu();

    EndFrame();
    Render();
    glViewport(0, 0, glWidth, glHeight);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // restore
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glScissor(prevScissorBox[0], prevScissorBox[1], prevScissorBox[2], prevScissorBox[3]);
    if (prevScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

#endif //ZYGISK_MENU_TEMPLATE_MENU_H
