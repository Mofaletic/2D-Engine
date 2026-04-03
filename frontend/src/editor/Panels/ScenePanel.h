#pragma once
#include <SDL3/SDL.h>
#include "../../../backend/core/SceneState.h"
#include "../../EditorState.h"

void DrawScenePanel(SceneState& sceneState, EditorState& editorState, SDL_Texture* sceneTexture);
