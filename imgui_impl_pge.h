/*
	imgui_impl_pge.h
	+-------------------------------------------------------------+
	|         OneLoneCoder Pixel Game Engine Extension            |
	|               Dear ImGui Integration - v1.0                 |
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

KNOWN LIMITATIONS
PGE does not bind every key on the keyboard.
	- ALT is not bound
	- No distinction between L-CTRl and R-CTRL
	- Maybe no distinction between ENTER and NP_ENTER
		There is no NP_ENTER, but there is both an ENTER and a RETURN
	- Many special characters which are not bound to a number or the numpad
		[]{}\|;:'"/?`~ are all unbound
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
the functions used.  This can be identified in the PGE header file.

```
#define OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION
#include imgui_impl_pge.h
```


Dear UmGui Integration Usage

Unlike many PGE Extensions, this extension requires files from an external
library.  To integrate the Dear Imgui functionality you will need to pull
the Dear ImGui software and include a number of files in your project.

Dear ImGui Github:
https://github.com/ocornut/imgui

Files Required:
imgui.h
imgui_impl_opengl2.h
imgui_internal.h
imstb_rectpack.h
imstb_textedit.h
imstb_truetype.h

imgui.cpp
imgui_demo.cpp
imgui_draw.cpp
imgui_impl_opengl2.cpp
imgui_widgets.cpp

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
	int gameLayer;

public:
	Example()
	{
		sAppName = "Test Application";
	}

public:
	bool OnUserCreate() override
	{
		//One time initialization of the Dear ImGui library
		ImGui::CreateContext();
		//Create an instance of the Dear ImGui PGE Integration
		pge_imgui = olc::imgui::PGE_ImGUI();
		
		//The vi2d for pixel size must match the values given in Construct()
		//Otherwise the mouse will not work correctly
		pge_imgui.ImGui_ImplPGE_Init(this);

		//Initialize the OpenGL2 rendering system
		ImGui_ImplOpenGL2_Init();

		//Create a new Layer which will be used for the game
		gameLayer = CreateLayer();
		//The layer is not enabled by default,  so we need to enable it
		EnableLayer(gameLayer, true);

		//Set a custom render function on layer 0.  Since DrawUI is a member of
		//our class, we need to use std::bind
		SetLayerCustomRenderFunction(0, std::bind(&Example::DrawUI, this));

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		//Change the Draw Target to not be Layer 0
		SetDrawTarget((uint8_t)gameLayer);
		//Game Drawing code here

		return true;
	}

	void DrawUI(void) {
		//These 3 lines are mandatory per-frame initialization
		ImGui_ImplOpenGL2_NewFrame();
		pge_imgui.ImGui_ImplPGE_NewFrame();
		ImGui::NewFrame();

		//Create and react to your UI here
		ImGui::ShowDemoWindow();
		//

		//This finishes the Dear ImGui and renders it to the screen
		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
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
#ifndef _OLC_PGEX_IMGUI_IMPL_PGE_H_
#define _OLC_PGEX_IMGUI_IMPL_PGE_H_

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

		class PGE_ImGUI : PGEX
		{
		public:
			//Initialize the Dear ImGui PGE Platform implementation.
			olc::rcode ImGui_ImplPGE_Init(olc::PixelGameEngine* pge);
			//Shutdown and perform Dear ImGui Platform cleanup
			void ImGui_ImplPGE_Shutdown(void);
			//Begin a new frame.  Must be called AFTER the renderer new frame
			void ImGui_ImplPGE_NewFrame(void);
			//Adjust the sensitivity of the mouse scroll wheel.  Default value is 120.0f
			void ImGui_ImplPGE_SetScrollSensitivity(float val);

		private:
			//A list of keyboard buttons which directly input a character
			std::vector<keyCharMap> vValueInputKeys;

			//A list of keyboard buttons which have special meaning to Dear Imgui
			std::vector<olc::Key> vControlInputKeys;

			//Larger number = less scrolling.  Negative inverts scroll direction
			float fScrollSensitivity = 120.0f;

			/*
			A list of keyboard buttons which directly input a character
			A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
			K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
			SPACE, TAB,
			NP0, NP1, NP2, NP3, NP4, NP5, NP6, NP7, NP8, NP9,
			NP_MUL, NP_DIV, NP_ADD, NP_SUB, NP_DECIMAL, PERIOD
			};*/


			//Internal function to update Dear ImGui with the current state of the mouse
			void ImGui_ImplPGE_UpdateMouse(void);
			//Internal function to update Dear ImGui with the current state of the keyboard
			void ImGui_ImplPGE_UpdateKeys(void);
		};
	}

}

#ifdef OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION
#undef OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION
namespace olc
{
	namespace imgui
	{
		//PGE does not expose the actual window size, so we have to initialize with the pixel size and calulate the actual size on the fly
		olc::rcode PGE_ImGUI::ImGui_ImplPGE_Init(olc::PixelGameEngine* pge) {
			ImGuiIO& io = ImGui::GetIO();

			//Fail if the pge pointer is a nullptr
			if (!pge) {
				return olc::FAIL;
			}

			this->pge = pge;

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
			io.KeyMap[ImGuiKey_KeyPadEnter] = olc::RETURN;
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
				{olc::NP_MUL, '*', '*'},
				{olc::NP_DIV, '/', '/'},
				{olc::NP_ADD, '+', '+'},
				{olc::NP_SUB, '-', '-'},
				{olc::NP_DECIMAL, '.', '.'},
				{olc::PERIOD, '.', '>'},
				{olc::SPACE, ' ', ' '}
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
	}
}
#endif //OLC_PGEX_DEAR_IMGUI_IMPLEMENTATION
#endif //_OLC_PGE_IMGUI_IMPL_PGE_H_
