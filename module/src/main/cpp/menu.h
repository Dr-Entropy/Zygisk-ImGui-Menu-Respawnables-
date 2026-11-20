// menu.h - Respawnables menu (ImGui) driving the GG-style memory engine
#ifndef ZYGISK_MENU_TEMPLATE_MENU_H
#define ZYGISK_MENU_TEMPLATE_MENU_H

#include "gg_engine.h"
#include "gg_features.h"

using namespace ImGui;

static gg::Engine g_engine;
static std::vector<gg::Feature> g_features = BuildFeatures();

static const char *kTabs[] = {"Combat", "Spawns", "Time", "Shop", "XP"};

void DrawMenu()
{
    Begin(OBFUSCATE("Respawnables"));
    if (g_engine.busy) {
        ProgressBar(g_engine.progress, ImVec2(-1, 0), "Working...");
        if (Button("Cancel")) g_engine.cancel = true;
    }
    if (BeginTabBar("Tabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
        for (const char *tab : kTabs) {
            if (!BeginTabItem(tab)) continue;
            for (auto &f : g_features) {
                if (strcmp(f.category, tab) != 0) continue;
                PushID(f.id);
                TextUnformatted(f.name);
                if (f.note[0]) TextDisabled("%s", f.note);
                if (Button("Apply")) g_engine.runAsync(f);
                if (g_engine.hasUndo(f.id)) {
                    SameLine();
                    if (Button("Undo")) g_engine.revertAsync(f.id);
                }
                Separator();
                PopID();
            }
            EndTabItem();
        }
        if (BeginTabItem("Log")) {
            for (auto &l : g_engine.logLines()) TextWrapped("%s", l.c_str());
            EndTabItem();
        }
        EndTabBar();
    }
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
