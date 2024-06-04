/*
	imgui_impl_pge.h
	+-------------------------------------------------------------+
	|         OneLoneCoder Pixel Game Engine Extension            |
	|               Dear ImGui Integration - v3.0                 |
	+-------------------------------------------------------------+
	What is this?
	~~~~~~~~~~~~~
	This is an extension to the olcPixelGameEngine, which provides
	a method to integrate Dear ImGui in a platform agnostic way.

	Author
	~~~~~~
	Dandistine

	License (OLC-3)
	~~~~~~~~~~~~~~~
	Copyright 2018 - 2019 OneLoneCoder.com
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:
	1. Redistributions or derivations of source code must retain the above
	copyright notice, this list of conditions and the following disclaimer.
	2. Redistributions or derivative works in binary form must reproduce
	the above copyright notice. This list of conditions and the following
	disclaimer must be reproduced in the documentation and/or other
	materials provided with the distribution.
	3. Neither the name of the copyright holder nor the names of its
	contributors may be used to endorse or promote products derived
	from this software without specific prior written permission.
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/*
Versions:
1.0 - Initial release
2.0 - Addition of OpenGl 3.3 rendering option - PGE 2.11+ required
	- ImGui related OpenGL functions pulled into extension and no longer need
	  to be called by the application.
		ImGui::CreateContext
		ImGui::NewFrame
		ImGui_ImplOpenGL2_Init
		ImGui_ImplOpenGL2_NewFrame
		ImGui_ImplOpenGL2_RenderDrawData
		ImGui_ImplOpenGL3_Init
		ImGui_ImplOpenGL3_NewFrame
		ImGui_ImplOpenGL3_RenderDrawData
	- Keymap is now matches PGE and includes more symbols like []{}
	- Add support for Before/After OnUserCreate/OnUserUpdate being run automatically by PGE
	- Extension automatically registers with PGE using the new 2.10+ system
3.0 - OnBeforeUserUpdate now returns false to work with PGE 2.17
*/


/*

KNOWN LIMITATIONS
PGE does not bind every key on the keyboard.
	- ALT is not bound
	- No distinction between L-CTRL and R-CTRL
	- Maybe no distinction between ENTER and NP_ENTER
		There is no NP_ENTER, but there is both an ENTER and a RETURN
	- The Super / Windows key is not bound

There is no IME support for non-English languages.

There is no support for non-US or non-QWERTY keyboards although they may work.

PGE returns mouse scrolling using an integer, and the relation of this integer
to actual scroll distance is not well understood by me.  On a Logitech G700s a
single "click" of the wheel is a wheel delta of +/-120.  The scroll sensitivity
defaults to 120.0f and can be changed with ImGui_ImplPGE_SetScrollSensitivity.
Higher values result in less scrolling per movement of the wheel.

Dear ImGui will always render 1:1 with the display's pixels.  You cannot have
Dear ImGui render its UI using pixels with size matching the application.


Dear ImGui Integration Implementation

Include this file in your application and define the implementation macro
in at least one location.  The macro will create the implementation of the
extension's functions.  It must be defined before the include in the file
where the implementation is needed.  A separate .cpp file is recommended
for this purpose.  A PGE version of at least 2.06 is required for some of
the functions used.  This can be identified in the PGE header file.  A PGE
version of at least 2.10 is required to use the OpenGL 3.3 renderer.  The
OpenGL 3.3 renderer may be specified by defining PGE_GFX_OPENGL33  in the
same location as OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION.

```
//OpenGL 2 renderer
#define OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION
#include imgui_impl_pge.h
```

```
//OpenGL 3.3 renderer
#define PGE_GFX_OPENGL33
#define OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION
#include imgui_impl_pge.h
```

To use the OpenGL 3.3 renderer, GLEW will also need to be available to
load the required OpenGL functions.  The static library will be linked
and needs to appear in the library path, and GL\glew.h will be included
and must appear on the include path.  GLEW can be found on their ssourceforge
page here: http://glew.sourceforge.net/


Dear ImGui Integration Usage

Unlike many PGE Extensions, this extension requires files from an external
library.  To integrate the Dear Imgui functionality you will need to pull
the Dear ImGui software and include a number of files in your project.

Dear ImGui Github:
https://github.com/ocornut/imgui

Files Required:
imgui.h
imgui_internal.h
imstb_rectpack.h
imstb_textedit.h
imstb_truetype.h

imgui.cpp
imgui_demo.cpp
imgui_draw.cpp
imgui_widgets.cpp

Additionally required are either the opengl2 or opengl3 files, depending
on the choice of renderer

imgui_impl_opengl2.cpp
imgui_impl_opengl2.h

imgui_impl_opengl3.cpp
imgui_impl_opengl3.h



Once all these files are included in your project you will be able to use
Dear ImGui.  Documentation on using the Dear ImGui library can be found
on their Github.

To integrate Dear ImGui with a PGE application, there are some special
things to consider.  First, Dear ImGui will render to the screen directly
using OpenGL and the GPU.  PGE has one facility to support this rendering
method, and that is the Layer's funchook which is intended to allow a game
to render a layer using its own renderer code.  This is how we will render
the Dear ImGui interface.  Second, we want the UI to appear on top of the
rest of the game.  Layer 0 is the top-most layer, so we will need to use
the Layer 0 render hook, and do all other PGE drawing to Layer 1 (or more).

Instead of writing paragraph upon paragraph on how to do this, I will just
add a basic example below:

class Example : public olc::PixelGameEngine
{
	olc::imgui::PGE_ImGUI pge_imgui;
	int m_GameLayer;

public:
	//PGE_ImGui can automatically call the SetLayerCustomRenderFunction by passing
	//true into the constructor.  false is the default value.
	Example() : pge_imgui(false)
	{
		sAppName = "Test Application";
	}

public:
	bool OnUserCreate() override
	{
		//Create a new Layer which will be used for the game
		m_GameLayer = CreateLayer();
		//The layer is not enabled by default,  so we need to enable it
		EnableLayer(m_GameLayer, true);

		//Set a custom render function on layer 0.  Since DrawUI is a member of
		//our class, we need to use std::bind
		//If the pge_imgui was constructed with _register_handler = true, this line is not needed
		SetLayerCustomRenderFunction(0, std::bind(&Example::DrawUI, this));

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		//Change the Draw Target to not be Layer 0
		SetDrawTarget((uint8_t)m_GameLayer);
		//Game Drawing code here

		//Create and react to your UI here, it will be drawn during the layer draw function
		ImGui::ShowDemoWindow();

		return true;
	}

	void DrawUI(void) {
		//This finishes the Dear ImGui and renders it to the screen
		pge_imgui.ImGui_ImplPGE_Render();
	}
};

int main() {
	Example demo;
	if (demo.Construct(640, 360, 2, 2))
		demo.Start();

	return 0;
}

*/

#pragma once
#ifndef OLC_PGEX_IMGUI_IMPL_PGE_H
#define OLC_PGEX_IMGUI_IMPL_PGE_H

#include <vector>
#include "imgui.h"
#include "olcPixelGameEngine.h"


namespace olc
{
	//Avoid polluting the olc namespace as much as we can
	namespace imgui
	{
		struct keyCharMap {
			olc::Key key;
			char lower;
			char upper;
		};

		class PGE_ImGUI : public PGEX
		{
		public:

			//_register_handler set to true will automatically register this plugin to draw on layer 0
			PGE_ImGUI(bool _register_handler = false);

			//Initialize the Dear ImGui PGE Platform implementation.
			olc::rcode ImGui_ImplPGE_Init();

			//Shutdown and perform Dear ImGui Platform cleanup
			void ImGui_ImplPGE_Shutdown(void);

			//Begin a new frame.  Must be called AFTER the renderer new frame
			void ImGui_ImplPGE_NewFrame(void);

			//Adjust the sensitivity of the mouse scroll wheel.  Default value is 120.0f
			void ImGui_ImplPGE_SetScrollSensitivity(float val);

			//Renders the UI to the screen.  Calls ImGui::Render() and ImGui_ImplOpenGL[23]_RenderDrawData() internally
			//Depending on the configured renderer.
			void ImGui_ImplPGE_Render(void);

			//A group of PGEX functions which will be automatically run by PGE at specific times
			void OnBeforeUserCreate() override;
			void OnAfterUserCreate() override;
			bool OnBeforeUserUpdate(float& fElapsedTime) override;
			void OnAfterUserUpdate(float fElapsedTime) override;


		private:
			//A list of keyboard buttons which directly input a character
			std::vector<keyCharMap> vValueInputKeys;

			//A list of keyboard buttons which have special meaning to Dear Imgui
			std::vector<olc::Key> vControlInputKeys;

			//Larger number = less scrolling.  Negative inverts scroll direction
			float fScrollSensitivity = 120.0f;

			//A flag to determine if the Extensions should automatically create a layer and add the
			//UI draw handler as the render function for that layer
			bool register_handler = false;

			//Internal function to update Dear ImGui with the current state of the mouse
			void ImGui_ImplPGE_UpdateMouse(void);

			//Internal function to update Dear ImGui with the current state of the keyboard
			void ImGui_ImplPGE_UpdateKeys(void);
		};
	}

}

#ifdef OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION
//#undef OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION

#ifdef OLC_GFX_OPENGL33
#define GLEW_STATIC
#include <GL/glew.h>
#undef GLEW_STATIC 
#include "imgui_impl_opengl3.h"
#else
#include "imgui_impl_opengl2.h"
#endif

namespace olc
{
	namespace imgui
	{
		PGE_ImGUI::PGE_ImGUI(bool _register_handler) : PGEX(true), register_handler(_register_handler) {

		}

		olc::rcode PGE_ImGUI::ImGui_ImplPGE_Init() {
			ImGui::CreateContext();

#ifdef OLC_GFX_OPENGL33
			GLenum err = glewInit();
			ImGui_ImplOpenGL3_Init();
#else
			ImGui_ImplOpenGL2_Init();
#endif
			ImGuiIO& io = ImGui::GetIO();

			io.BackendPlatformName = "imgui_impl_pge";

			io.KeyMap[ImGuiKey_Tab] = olc::TAB;
			io.KeyMap[ImGuiKey_LeftArrow] = olc::LEFT;
			io.KeyMap[ImGuiKey_RightArrow] = olc::RIGHT;
			io.KeyMap[ImGuiKey_UpArrow] = olc::UP;
			io.KeyMap[ImGuiKey_DownArrow] = olc::DOWN;
			io.KeyMap[ImGuiKey_PageUp] = olc::PGUP;
			io.KeyMap[ImGuiKey_PageDown] = olc::PGDN;
			io.KeyMap[ImGuiKey_Home] = olc::HOME;
			io.KeyMap[ImGuiKey_End] = olc::END;
			io.KeyMap[ImGuiKey_Insert] = olc::INS;
			io.KeyMap[ImGuiKey_Delete] = olc::DEL;
			io.KeyMap[ImGuiKey_Backspace] = olc::BACK;
			io.KeyMap[ImGuiKey_Space] = olc::SPACE;
			io.KeyMap[ImGuiKey_Enter] = olc::ENTER;
			io.KeyMap[ImGuiKey_Escape] = olc::ESCAPE;
			io.KeyMap[ImGuiKey_KeypadEnter] = olc::RETURN;
			io.KeyMap[ImGuiKey_A] = olc::A;
			io.KeyMap[ImGuiKey_C] = olc::C;
			io.KeyMap[ImGuiKey_V] = olc::V;
			io.KeyMap[ImGuiKey_X] = olc::X;
			io.KeyMap[ImGuiKey_Y] = olc::Y;
			io.KeyMap[ImGuiKey_Z] = olc::Z;

			//Create a listing of all the control keys so we can iterate them later
			vControlInputKeys = {
				olc::TAB, olc::LEFT, olc::RIGHT, olc::UP, olc::DOWN,
				olc::PGUP, olc::PGDN, olc::HOME, olc::END, olc::INS,
				olc::DEL, olc::BACK, olc::SPACE, olc::ENTER, olc::ESCAPE,
				olc::RETURN, olc::A, olc::C, olc::V, olc::X, olc::Y, olc::Z
			};

			//Map keys which input values to input boxes.
			//This is not the ideal way to do this, but PGE does not expose
			//much in the way of keyboard or IME input.  Many special characters
			//are absent as well.
			vValueInputKeys = {
				{olc::A, 'a', 'A'},
				{olc::B, 'b', 'B'},
				{olc::C, 'c', 'C'},
				{olc::D, 'd', 'D'},
				{olc::E, 'e', 'E'},
				{olc::F, 'f', 'F'},
				{olc::G, 'g', 'G'},
				{olc::H, 'h', 'H'},
				{olc::I, 'i', 'I'},
				{olc::J, 'j', 'J'},
				{olc::K, 'k', 'K'},
				{olc::L, 'l', 'L'},
				{olc::M, 'm', 'M'},
				{olc::N, 'n', 'N'},
				{olc::O, 'o', 'O'},
				{olc::P, 'p', 'P'},
				{olc::Q, 'q', 'Q'},
				{olc::R, 'r', 'R'},
				{olc::S, 's', 'S'},
				{olc::T, 't', 'T'},
				{olc::U, 'u', 'U'},
				{olc::V, 'v', 'V'},
				{olc::W, 'w', 'W'},
				{olc::X, 'x', 'X'},
				{olc::Y, 'y', 'Y'},
				{olc::Z, 'z', 'Z'},
				{olc::K0, '0', ')'},
				{olc::K1, '1', '!'},
				{olc::K2, '2', '@'},
				{olc::K3, '3', '#'},
				{olc::K4, '4', '$'},
				{olc::K5, '5', '%'},
				{olc::K6, '6', '^'},
				{olc::K7, '7', '&'},
				{olc::K8, '8', '*'},
				{olc::K9, '9', '('},
				{olc::NP0, '0', '0'},
				{olc::NP1, '1', '1'},
				{olc::NP2, '2', '2'},
				{olc::NP3, '3', '3'},
				{olc::NP4, '4', '4'},
				{olc::NP5, '5', '5'},
				{olc::NP6, '6', '6'},
				{olc::NP7, '7', '7'},
				{olc::NP8, '8', '8'},
				{olc::NP9, '9', '9'},
				{olc::NP_MUL, '*', '*'},
				{olc::NP_DIV, '/', '/'},
				{olc::NP_ADD, '+', '+'},
				{olc::NP_SUB, '-', '-'},
				{olc::NP_DECIMAL, '.', '.'},
				{olc::PERIOD, '.', '>'},
				{olc::SPACE, ' ', ' '},
				{olc::OEM_1, ';', ':'},
				{olc::OEM_2, '/', '?'},
				{olc::OEM_3, '`', '~'},
				{olc::OEM_4, '[', '{'},
				{olc::OEM_5, '\\', '|'},
				{olc::OEM_6, ']', '}'},
				{olc::OEM_7, '\'', '"'},
				{olc::OEM_8, '-', '-'},
				{olc::EQUALS, '=', '+'},
				{olc::COMMA, ',', '<'},
				{olc::MINUS, '-', '_'}
			};

			return olc::OK;
		}

		//This currently does nothing, but is defined in the event that it needs to eventually do something
		void PGE_ImGUI::ImGui_ImplPGE_Shutdown(void) {
			return;
		}

		void PGE_ImGUI::ImGui_ImplPGE_UpdateMouse(void) {
			ImGuiIO& io = ImGui::GetIO();
			olc::vi2d windowMouse = pge->GetWindowMouse();

			//Update Mouse Buttons
			io.MouseDown[0] = pge->GetMouse(0).bHeld;
			io.MouseDown[1] = pge->GetMouse(1).bHeld;
			io.MouseDown[2] = pge->GetMouse(2).bHeld;
			io.MouseDown[3] = pge->GetMouse(3).bHeld;
			io.MouseDown[4] = pge->GetMouse(4).bHeld;

			//Update the Mouse Position
			io.MousePos = ImVec2((float)windowMouse.x, (float)windowMouse.y);

			//Update Mouse Wheel
			io.MouseWheel = static_cast<float>(pge->GetMouseWheel()) / fScrollSensitivity;
		}

		void PGE_ImGUI::ImGui_ImplPGE_UpdateKeys(void) {
			ImGuiIO& io = ImGui::GetIO();

			io.KeyCtrl = pge->GetKey(olc::CTRL).bHeld;
			io.KeyAlt = false; // pge->GetKey(olc::ALT).bHeld;
			io.KeyShift = pge->GetKey(olc::SHIFT).bHeld;
			io.KeySuper = false;

			//Update the status of the control keys.
			for (auto const& x : vControlInputKeys) {
				io.KeysDown[x] = pge->GetKey(x).bHeld;
			}

			//Get the status of the SHIFT key to so we can change the inserted character
			bool isShift = pge->GetKey(SHIFT).bHeld;

			//Get characters that can be entered into inputs
			for (auto& m : vValueInputKeys) {
				if (pge->GetKey(m.key).bPressed) {
					io.AddInputCharacter(isShift ? m.upper : m.lower);
				}
			}
		}

		void PGE_ImGUI::ImGui_ImplPGE_NewFrame(void) {
#ifdef OLC_GFX_OPENGL33
			ImGui_ImplOpenGL3_NewFrame();
#else
			ImGui_ImplOpenGL2_NewFrame();
#endif
			ImGuiIO& io = ImGui::GetIO();
			olc::vi2d windowSize = pge->GetWindowSize();
			IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL2_NewFrame().");

			io.DisplaySize = ImVec2((float)windowSize.x, (float)windowSize.y);

			io.DeltaTime = pge->GetElapsedTime();

			ImGui_ImplPGE_UpdateKeys();

			ImGui_ImplPGE_UpdateMouse();
		}

		void PGE_ImGUI::ImGui_ImplPGE_SetScrollSensitivity(float val) {
			this->fScrollSensitivity = val;
		}

		void PGE_ImGUI::ImGui_ImplPGE_Render(void) {
			//This finishes the Dear ImGui and renders it to the screen
			ImGui::Render();
#ifdef OLC_GFX_OPENGL33
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
			ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#endif
		}

		//Before the OnUserCreate function runs, we will register the UI drawing function
		//to the layer 0, if we were told to do so during construction
		//We can't register this during construction because the pge-vLayers is not configured until after construction
		void PGE_ImGUI::OnBeforeUserCreate() {
			if (register_handler) {
				pge->SetLayerCustomRenderFunction(0, std::bind(&PGE_ImGUI::ImGui_ImplPGE_Render, this));
			}
		}

		//After the OnUserCreate function runs, we will run all of the Init code to setup
		//the ImGui.  This will happen automatically in PGE 2.10+
		void PGE_ImGUI::OnAfterUserCreate() {
			ImGui_ImplPGE_Init();
		}

		//Before the OnUserUpdate runs, do the pre-frame ImGui intialization
		bool PGE_ImGUI::OnBeforeUserUpdate(float& fElapsedTime) {
			ImGui_ImplPGE_NewFrame();
			ImGui::NewFrame();
			return false;
		}

		//There is currently no "after update" logic to run for ImGui
		void PGE_ImGUI::OnAfterUserUpdate(float fElapsedTime) {}
	}
}
#endif //OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION
#endif //OLC_PGE_IMGUI_IMPL_PGE_H
