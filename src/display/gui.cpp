#include "gui.h"

namespace MiniRenderer {
	bool GUI::initialize_gui(SDL_Window* window, SDL_Renderer* renderer) {
		mWindow = window;
		mRenderer = renderer;

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		mIo = &ImGui::GetIO();
		mIo->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		ImGui::StyleColorsDark();

		// Setup SDLRenderer2 binding
		ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
		ImGui_ImplSDLRenderer2_Init(renderer);

		return true;
	}

	void GUI::destroy_gui() {
		ImGui_ImplSDLRenderer2_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	GUI::~GUI() {
		GUI::destroy_gui();
	}

	void GUI::process_input(SDL_Event& event) {
		ImGui_ImplSDL2_ProcessEvent(&event);
	}


	GUIState GUI::update(Settings& s, const Model& m) {
		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		GUIState state = GUIState::None;

		// TODO: Remove this line
		ImGui::ShowDemoWindow(nullptr);

		// Main settings window
		ImGui::Begin("Window settings");
		ImGui::Checkbox("Show renderer window", &(s.open_windows.show_renderer_window));
		ImGui::Checkbox("Show camera window", &(s.open_windows.show_camera_window));
		ImGui::End();

		// Rendering settings window
		if (s.open_windows.show_renderer_window) {
			ImGui::Begin("Rendering settings");
			ImGui::Text("%.3f ms/frame (%.1f FPS) at %dx%d", 1000.0f / mIo->Framerate, mIo->Framerate, s.winWidth, s.winHeight);
			ImGui::SeparatorText("Model");
			/*
			ImGui::Checkbox("Show model", &mSettings->show_model);
			ImGui::Checkbox("Show wireframe", &mSettings->show_wireframe);
			ImGui::Checkbox("Show normals", &mSettings->show_normals);
			ImGui::Checkbox("Show bounding box", &mSettings->show_bounding_box);
			*/

			if (ImGui::Button("Open Model")) {
				IGFD::FileDialogConfig config;
				config.path = s.filepath;
				ImGuiFileDialog::Instance()->OpenDialog("ChooseObjFile", "Choose model (.obj)", ".obj", config);
			}

			if (ImGuiFileDialog::Instance()->Display("ChooseObjFile")) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					s.filename = ImGuiFileDialog::Instance()->GetFilePathName();
					s.filepath = ImGuiFileDialog::Instance()->GetCurrentPath();
					state = GUIState::LoadModel;
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), s.filename.c_str());
			ImGui::SameLine();
			ImGui::Text("loaded");
			ImGui::SameLine();

			ImGui::Text("with %d vertices, %d faces", m.nVerts(), m.nFaces());

			if (ImGui::Button("Render .tga screenshot")) {
				IGFD::FileDialogConfig config;
				config.path = s.filepath;
				ImGuiFileDialog::Instance()->OpenDialog("SaveTgaFile", "Save screenshot (.tga)", ".tga", config);
			}

			if (ImGuiFileDialog::Instance()->Display("SaveTgaFile")) {
				const auto path = ImGuiFileDialog::Instance()->GetFilePathName();
				if (ImGuiFileDialog::Instance()->IsOk()) {
					TGAImage image(s.winWidth, s.winHeight, 2 << 5);
					if (image.write_tga_file(path))
						SDL_Log("Saved screenshot to %s", path.c_str());
					else
						SDL_Log("Failed to save screenshot to %s", path.c_str());
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::Checkbox("Backface culling", &(s.backface_culling));


			// ImGui::Text(mModel->name.c_str());
			/*
			ImGui::SeparatorText("Lighting");
						ImGui::Checkbox("Enable lighting", &mSettings->enable_lighting);
			ImGui::Checkbox("Enable textures", &mSettings->enable_textures);
						ImGui::SliderFloat3("Light position", &mSettings->light.position[0], -100.0f, 100.0f);
			ImGui::SliderFloat3("Light color", &mSettings->light.color[0], 0.0f, 1.0f);
						ImGui::SliderFloat("Ambient strength", &mSettings->light.ambient_strength, 0.0f, 1.0f);
			ImGui::SliderFloat("Diffuse strength", &mSettings->light.diffuse_strength, 0.0f, 1.0f);
						ImGui::SliderFloat("Specular strength", &mSettings->light.specular_strength, 0.0f, 1.0f);
			ImGui::SliderFloat("Specular shininess", &mSettings->light.specular_shininess, 0.0f, 100.0f);
						ImGui::SeparatorText("Shading");
			ImGui::RadioButton("Flat", reinterpret_cast<int*>(&mSettings->shading), Shading::Flat);
						ImGui::SameLine();
			ImGui::RadioButton("Gouraud", reinterpret_cast<int*>(&mSettings->shading), Shading::Gouraud);
						ImGui::SameLine();
			ImGui::RadioButton("Phong", reinterpret_cast<int*>(&mSettings->shading), Shading::Phong);
						ImGui::SeparatorText("Clipping");
			ImGui::Checkbox("Enable clipping", &mSettings->enable_clipping);
						ImGui::SliderFloat("Near plane", &mSettings->near_plane, 0.0f, 100.0f);
			ImGui::SliderFloat("Far plane", &mSettings->far_plane, 0.0f, 100.0f);
						ImGui::SeparatorText("Culling");
			ImGui::Checkbox("Enable backface culling", &mSettings->enable_backface_culling);
						ImGui::SeparatorText("Rasterization");
			ImGui::RadioButton("Point", reinterpret_cast<int*>(&mSettings->rasterization), Rasterization::Point);
						ImGui::SameLine();
			ImGui::RadioButton("Wireframe", reinterpret_cast<int*>(&mSettings->rasterization), Rasterization::Wireframe);
						ImGui::SameLine();
			ImGui::RadioButton("Solid", reinterpret_cast<int*>(&mSettings->rasterization), Rasterization::Solid);
			*/


			ImGui::SeparatorText("Line algorithm");
			ImGui::RadioButton("DDA", reinterpret_cast<int*>(&s.line_algo), LineAlgorithm::DDA);
			ImGui::SameLine();
			ImGui::RadioButton("Bresenham", reinterpret_cast<int*>(&s.line_algo), LineAlgorithm::Bresenham);
			ImGui::SameLine();
			ImGui::RadioButton("Wu", reinterpret_cast<int*>(&s.line_algo), LineAlgorithm::Wu);

			ImGui::SeparatorText("Triangle algorithm");
			ImGui::RadioButton("Wireframe", reinterpret_cast<int*>(&s.triangle_algo), TriangleAlgo::Wireframe);
			ImGui::SameLine();
			ImGui::RadioButton("Flat", reinterpret_cast<int*>(&s.triangle_algo), TriangleAlgo::Flat);
			ImGui::SameLine();
			ImGui::RadioButton("Gouraud", reinterpret_cast<int*>(&s.triangle_algo), TriangleAlgo::Gouraud);
			ImGui::ColorEdit4("Line color", &s.fg_color[0]);
			ImGui::End();
		}

		// Camera settings window
		if (s.open_windows.show_camera_window) {
			ImGui::Begin("Camera settings");
			ImGui::SliderFloat3("Position", &s.camera.position[0], -100.0f, 100.0f);
			ImGui::SliderFloat3("Rotation", &s.camera.rotation[0], -180.0f, 180.0f);
			ImGui::End();
		}
		ImGui::Render();
		return state;
	}

	void GUI::render() {
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
	}
}

