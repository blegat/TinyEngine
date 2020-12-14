#include "../../TinyEngine.h"
#include "../../include/helpers/image.h"
#include "../../include/helpers/color.h"

#include "effects.h"

int main( int argc, char* args[] ) {

	Tiny::window("Shader Effects Example", 1200, 800);

	Tiny::event.handler = [](){ /* ... */ };

	Tiny::view.interface = interfaceFunc;

	Texture tex(image::load("canyon.png"));		//Load Texture with Image
	Square2D flat;														//Create Primitive Model
	Shader effect({"shader/effect.vs", "shader/effect.fs"}, {"in_Quad", "in_Tex"});
	Shader blur({"shader/blur.vs", "shader/blur.fs"}, {"in_Quad", "in_Tex"});

	Billboard board(1200, 800);

	Tiny::view.pipeline = [&](){

		//Two-Pass Blur onto board FBO
		board.target(color::black);
		blur.use();
		blur.texture("imageTexture", tex);
		blur.uniform("horizontal", true);
		blur.uniform("model", flat.model);
		flat.render();
		blur.uniform("horizontal", false);
		flat.render();

		Tiny::view.target(color::black);				//Target Main Screen

		effect.use();														//Use Effect Shader
		effect.uniform("index", ind);						//Define Uniforms
		effect.uniform("res", res);
		effect.uniform("bits", bits);
		effect.texture("imageTexture", board.texture);		//Load Texture
		effect.uniform("model", flat.model);		//Add Model Matrix
		flat.render();													//Render Primitive

	};

	Tiny::loop([&](){
		/* Absolutely Nothing! */
	});

	Tiny::quit();

	return 0;
}
